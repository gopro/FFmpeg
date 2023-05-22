/*
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#define COBJMACROS
#if !defined(_WIN32_WINNT) || _WIN32_WINNT < 0x0602
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0602
#endif

#include <windows.h>
#include <initguid.h>
#include <wmcodecdsp.h>
#include <mftransform.h>
#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include <uuids.h>
#include <codecapi.h>
#include <d3d12.h>
#include <d3d11on12.h>
#include <d3d11.h>
#include <d3d11_4.h>
#include <d3d9.h>
#include <d2d1.h>
#include <dxva2api.h>
#include <dxgi1_6.h>
#include <combaseapi.h>

#include <stdint.h>
#include <string.h>

#include "avassert.h"
#include "buffer.h"
#include "common.h"
#include "imgutils.h"
#include "hwcontext.h"
#include "hwcontext_mediafoundation.h"
#include "hwcontext_internal.h"
#include "mem.h"
#include "pixfmt.h"
#include "pixdesc.h"
#include "compat/w32dlfcn.h"

static const UINT FrameCount = 3;

typedef struct MFDeviceContext {
    HANDLE d3d12_dll;
    HANDLE d3d11on12_dll;
    HANDLE dxgi_dll;
    HANDLE d3d11_dll;
    HANDLE d3d9_dll;
    HANDLE dxva2_dll;
    IDirect3D9 *d3d9;
} MFDeviceContext;

typedef IDirect3D9* WINAPI pDirect3DCreate9(UINT);
typedef HRESULT WINAPI pDirect3DCreate9Ex(UINT, IDirect3D9Ex**);
typedef HRESULT WINAPI pCreateDeviceManager9(UINT *, IDirect3DDeviceManager9 **);

#define FF_D3DCREATE_FLAGS (D3DCREATE_SOFTWARE_VERTEXPROCESSING | \
                            D3DCREATE_MULTITHREADED | \
                            D3DCREATE_FPU_PRESERVE)

static const D3DPRESENT_PARAMETERS d3d_present_params = {
    .Windowed = TRUE,
    .BackBufferWidth = 640,
    .BackBufferHeight = 480,
    .BackBufferCount = 0,
    .SwapEffect = D3DSWAPEFFECT_DISCARD,
    .Flags = D3DPRESENTFLAG_VIDEO,
};

static void mf_uninit_d3d(AVHWDeviceContext *ctx)
{
    AVMFDeviceContext *hwctx = ctx->hwctx;
    MFDeviceContext *priv = ctx->internal->priv;

    if (hwctx->d3d11_manager) {
        IMFDXGIDeviceManager_Release(hwctx->d3d11_manager);
        hwctx->d3d11_manager = NULL;
    }

    if (priv->d3d12_dll)
        FreeLibrary(priv->d3d12_dll);
    priv->d3d12_dll = NULL;

    if (priv->d3d11on12_dll)
        FreeLibrary(priv->d3d11on12_dll);
    priv->d3d11on12_dll = NULL;

    if (priv->dxgi_dll)
        FreeLibrary(priv->dxgi_dll);
    priv->dxgi_dll = NULL;

    if (priv->d3d11_dll)
        FreeLibrary(priv->d3d11_dll);
    priv->d3d11_dll = NULL;

    if (hwctx->d3d9_manager) {
        IDirect3DDeviceManager9_Release(hwctx->d3d9_manager);
        hwctx->d3d9_manager = NULL;
    }

    if (hwctx->init_d3d9_device) {
        IDirect3DDevice9_Release(hwctx->init_d3d9_device);
        hwctx->init_d3d9_device = NULL;
    }

    if (hwctx->init_d3d11_device) {
        ID3D11Device_Release(hwctx->init_d3d11_device);
        hwctx->init_d3d11_device = NULL;
    }

    if (priv->d3d9) {
        IDirect3D9_Release(priv->d3d9);
        priv->d3d9 = NULL;
    }

    if (priv->d3d9_dll) {
        dlclose(priv->d3d9_dll);
        priv->d3d9_dll = NULL;
    }

    if (priv->d3d11_dll) {
        dlclose(priv->d3d11_dll);
        priv->d3d11_dll = NULL;
    }

    if (priv->dxva2_dll) {
        dlclose(priv->dxva2_dll);
        priv->dxva2_dll = NULL;
    }
}

static int mf_create_d3d11on12_device(AVHWDeviceContext * ctx, int loglevel)
{
    AVMFDeviceContext * hwctx = ctx->hwctx;
    MFDeviceContext * priv = ctx->internal->priv;
    HRESULT hr;
    UINT token;
    UINT dxgiFactoryFlags = 0;
    D2D1_FACTORY_OPTIONS d2dFactoryOptions; //  = 0;
    IDXGIFactory4 * dxgi_factory = NULL;
    ID3D11Device * d3d11_device = NULL;
    ID3D12Device * d3d12_device = NULL;
    ID3D11On12Device* d3d11on12_device = NULL;
    ID3D11On12Device2* d3d11On12Device2 = NULL;
    ID3D11DeviceContext * d3d11_deviceContext = NULL;
    
    // SAVE On device context!!
    IDXGISwapChain3 * dxgi_swapchain = NULL;

    ID3D10Multithread * multithread;
    HRESULT(WINAPI * pD3D12CreateDevice)(
        _In_opt_ IDXGIAdapter * pAdapter,
        _In_opt_ const D3D_FEATURE_LEVEL * pFeatureLevels,
        _In_ REFIID riid, // Expected: ID3D12Device
        _COM_Outptr_opt_ void ** ppDevice
        );
    HRESULT(WINAPI * pD3D11On12CreateDevice)(
        _In_ IUnknown * pDevice,
        UINT Flags,
        _In_reads_opt_(FeatureLevels) CONST D3D_FEATURE_LEVEL * pFeatureLevels,
        UINT FeatureLevels,
        _In_reads_opt_(NumQueues) IUnknown * CONST * ppCommandQueues,
        UINT NumQueues,
        UINT NodeMask,
        _COM_Outptr_opt_ ID3D11Device ** ppDevice,
        _COM_Outptr_opt_ ID3D11DeviceContext ** ppImmediateContext,
        _Out_opt_ D3D_FEATURE_LEVEL * pChosenFeatureLevel
        );

    HRESULT(WINAPI * pCreateDXGIFactory2)(
        UINT Flags, 
        REFIID riid, 
        _COM_Outptr_ void** ppFactory
        );

    HRESULT(WINAPI * pMFCreateDXGIDeviceManager)(
        _Out_ UINT * pResetToken,
        _Out_ IMFDXGIDeviceManager ** ppDXVAManager
        );
    HANDLE mfplat_dll;

    if (hwctx->init_d3d12_device) {
        d3d12_device = hwctx->init_d3d12_device;
        ID3D12Device_AddRef(d3d12_device);
    }
    else {
        priv->d3d12_dll = LoadLibraryW(L"D3D12.dll");
        if (!priv->d3d12_dll)
            return AVERROR_EXTERNAL;

        priv->dxgi_dll = LoadLibraryW(L"DXGI.dll");
        if (!priv->dxgi_dll)
            return AVERROR_EXTERNAL;

        pD3D12CreateDevice = (void*)GetProcAddress(priv->d3d12_dll, "D3D12CreateDevice");
        if (!pD3D12CreateDevice)
            return AVERROR_EXTERNAL;

        pD3D11On12CreateDevice = (void*)GetProcAddress(priv->d3d12_dll, "D3D11On12CreateDevice");
        if (!pD3D11On12CreateDevice)
            return AVERROR_EXTERNAL;

        pCreateDXGIFactory2 = (void*)GetProcAddress(priv->dxgi_dll, "CreateDXGIFactory2");
        if (!pCreateDXGIFactory2)
            return AVERROR_EXTERNAL;

        D3D_FEATURE_LEVEL level = D3D_FEATURE_LEVEL_11_0;

        hr = pD3D12CreateDevice(NULL,
            &level,
            &IID_ID3D12Device,
            &d3d12_device);
        if (FAILED(hr)) {
            av_log(ctx, loglevel, "failed to create D3D12 device\n");
            goto error;
        }

        hr = pCreateDXGIFactory2(dxgiFactoryFlags, &IID_IDXGIFactory2, &dxgi_factory);
        if (FAILED(hr)) {
            av_log(ctx, loglevel, "failed to create DXGI factory object\n");
            goto error;
        }

        // TODO: Read up on D3D11on12 for this part ...
        // NOT Sure if I need to create the command queue and swapchains for this 11on12 device

        // Describe and create the command queue.
        D3D12_COMMAND_QUEUE_DESC queueDesc = {
            .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
            .Type = D3D12_COMMAND_LIST_TYPE_DIRECT
        };

        hr = ID3D12Device_CreateCommandQueue(d3d12_device, &queueDesc, &IID_ID3D12CommandQueue, &hwctx->d3d12_command_queue);
        if (FAILED(hr)) {
            av_log(ctx, loglevel, "failed to create D3D12 device command queue\n");
            goto error;
        }
        ID3D12Object_SetName(hwctx->d3d12_command_queue, L"CommandQueue");

        // Describe the swap chain.
        DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {
            .BufferCount = hwctx->d3d12_frame_cnt,
            .Width = hwctx->d3d12_window_width,
            .Height = hwctx->d3d12_window_height,
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
            .SampleDesc.Count = 1
        };
        
        hr = IDXGIFactory4_CreateSwapChain(dxgi_factory,
            (IUnknown *)(hwctx->d3d12_command_queue),
            (DXGI_SWAP_CHAIN_DESC *)&swapchain_desc,
            (IDXGISwapChain **)&dxgi_swapchain);
        if (FAILED(hr)) {
            av_log(ctx, loglevel, "failed to create DXGI swapchain\n");
            goto error;
        }

        // Might need to query interface fpr IDXGISwapShain3
        // ... ThrowIfFailed(swapChain.As(&m_swapChain));

        UINT frameIndex = IDXGISwapChain3_GetCurrentBackBufferIndex(dxgi_swapchain);
        
        // end of this TODO section

        // Create an 11 device wrapped around the 12 device and share
        // 12's command queue.
        hr = pD3D11On12CreateDevice(
            (IUnknown *)d3d12_device,
            D3D11_CREATE_DEVICE_VIDEO_SUPPORT,
            NULL,
            0,
            NULL, // ptr to command queue obj
            1,
            0,
            (ID3D11Device **)&d3d11_device,
            (ID3D11DeviceContext **)&d3d11_deviceContext,
            NULL
        );
        if (FAILED(hr)) {
            av_log(ctx, loglevel, "failed to create D3D11on12 device\n");
            goto error;
        }

        hr = ID3D11Device_QueryInterface(d3d11_device, &IID_ID3D11On12Device, (void **) & d3d11on12_device);
        if (FAILED(hr)) {
            av_log(ctx, loglevel, "failed to query interface for D3D11on12 device\n");
            goto error;
        }

        hr = IMFMediaBuffer_QueryInterface(d3d11on12_device, &IID_ID3D10Multithread, (void **)&multithread);
        if (FAILED(hr)) {
            av_log(ctx, loglevel, "could not get ID3D10Multithread\n");
            goto error;
        }

        hr = ID3D10Multithread_SetMultithreadProtected(multithread, TRUE);
        if (FAILED(hr)) {
            av_log(ctx, loglevel, "failed to call ID3D10Multithread::SetMultithreadProtected\n");
            ID3D10Multithread_Release(multithread);
            goto error;
        }

        ID3D10Multithread_Release(multithread);
    }

    // Add the D3DDeviceManager block from end of function: mf_create_d3d11_device
    ID3D12Device_Release(d3d12_device);
    return 0;

error:
    if (d3d12_device)
        ID3D12Device_Release(d3d12_device);
    mf_uninit_d3d(ctx);
    return AVERROR_EXTERNAL;
}

/*
HAL::Surface> GraphicsDevice::CreateSurface2D(std::string name, ResourceUse use, void* handle, uint32_t sub_resource_index) {
    LockGuard lock(contextLock, contextLockThreadId);
    HRESULT hResult;
    ID3D12Resource* d3d12Resource = nullptr;
    if (CheckFlag(use, ResourceUse::SHARED_OBJECT)) {
        SSDK_ERR("SHARED_OBJECT not supported");
    }
    auto d3dCtx = d3d(graphicsContext.get());
    V(d3d11On12Device2->UnwrapUnderlyingResource((ID3D11Resource*)handle, d3dCtx->d3dCommandQueue.v.Get(), IID_PPV_ARGS(&d3d12Resource)));
    ID3D11Texture2D* d3d11Texture = (ID3D11Texture2D*)handle;
    D3D11_TEXTURE2D_DESC d3d11TextureDesc;
    d3d11Texture->GetDesc(&d3d11TextureDesc);
    BitstreamFormat format = TypeUtil::ToBitstreamFormat(PixelFormat(d3d11TextureDesc.Format));
    uint32_t width = d3d11TextureDesc.Width, height = d3d11TextureDesc.Height;
    auto d3dTextureHandle = new TextureHandle(nullptr);
    auto createHandleFn = [=]() {
        Texture* texture = Texture::createFromHandle(graphicsContext.get(), graphics.get(), d3d12Resource,
            TypeUtil::ToPixelFormat(format), width, height, 1, 1, TypeUtil::ToImageUsageFlags(use), TEXTURE_TYPE_2D, 1, nullptr);
        texture->setName(name);
        d3dTextureHandle->v = texture;
    };
    deferredFunctions.push_back({ -1, createHandleFn });
    auto fence = Fence::create(graphicsContext->device);
    d3dCtx->d3dCommandQueue.signal(d3d(fence), D3DFence::SIGNALED);
    TextureInteropSupport s;
    s.callback = [=]() {
        HRESULT hResult;
        V(d3d11On12Device2->ReturnUnderlyingResource((ID3D11Resource*)handle, 0, nullptr, nullptr));
        ((ID3D11Texture2D*)handle)->Release();
    };
    s.fence = fence;
    s.handle = handle;
    textureInteropSupportVector.emplace_back(std::move(s));
    //TODO: call this API after DirectX12 finishes using it

    auto surface = make_shared<NGFXSurface>(this, d3dTextureHandle, name.c_str(), SurfaceType::AREA, use,
        format, 0, 1, width, height, 1, 0, 1);
    return surface;
}
*/

static int mf_create_d3d11_device(AVHWDeviceContext *ctx, int loglevel)
{
    AVMFDeviceContext *hwctx = ctx->hwctx;
    MFDeviceContext *priv = ctx->internal->priv;
    HRESULT hr;
    UINT token;
    ID3D11Device *d3d11_device = NULL;
    ID3D11Multithread *multithread;
    HRESULT (WINAPI *pD3D11CreateDevice)(
    _In_opt_        IDXGIAdapter        *pAdapter,
                    D3D_DRIVER_TYPE     DriverType,
                    HMODULE             Software,
                    UINT                Flags,
    _In_opt_  const D3D_FEATURE_LEVEL   *pFeatureLevels,
                    UINT                FeatureLevels,
                    UINT                SDKVersion,
    _Out_opt_       ID3D11Device        **ppDevice,
    _Out_opt_       D3D_FEATURE_LEVEL   *pFeatureLevel,
    _Out_opt_       ID3D11DeviceContext **ppImmediateContext
    );
    HRESULT (WINAPI *pMFCreateDXGIDeviceManager)(
    _Out_ UINT                 *pResetToken,
    _Out_ IMFDXGIDeviceManager **ppDXVAManager
    );
    HANDLE mfplat_dll;

    if (hwctx->init_d3d11_device) {
        d3d11_device = hwctx->init_d3d11_device;
        ID3D11Device_AddRef(d3d11_device);
    } else {
        priv->d3d11_dll = dlopen("D3D11.dll", 0);
        if (!priv->d3d11_dll)
            return AVERROR_EXTERNAL;

        pD3D11CreateDevice = (void *)dlsym(priv->d3d11_dll, "D3D11CreateDevice");
        if (!pD3D11CreateDevice)
            return AVERROR_EXTERNAL;

        hr = pD3D11CreateDevice(0,
                                D3D_DRIVER_TYPE_HARDWARE,
                                NULL,
                                D3D11_CREATE_DEVICE_VIDEO_SUPPORT,
                                NULL,
                                0,
                                D3D11_SDK_VERSION,
                                &d3d11_device,
                                NULL,
                                NULL);
        if (FAILED(hr)) {
            av_log(ctx, loglevel, "failed to create D3D device\n");
            goto error;
        }

        hr = IMFMediaBuffer_QueryInterface(d3d11_device, &IID_ID3D11Multithread, (void **)&multithread);
        if (FAILED(hr)) {
            av_log(ctx, loglevel, "could not get ID3D11Multithread\n");
            goto error;
        }

        hr = ID3D11Multithread_SetMultithreadProtected(multithread, TRUE);
        if (FAILED(hr)) {
            av_log(ctx, loglevel, "failed to call ID3D11Multithread::SetMultithreadProtected\n");
            ID3D11Multithread_Release(multithread);
            goto error;
        }

        ID3D11Multithread_Release(multithread);
    }

    // If this code is enabled, we already link against this DLL.
    // MFCreateDXGIDeviceManager is just not available on Windows 7.
    mfplat_dll = GetModuleHandleW(L"mfplat.dll");
    if (!mfplat_dll) {
        av_log(ctx, loglevel, "mfplat.dll not present\n");
        goto error;
    }
    pMFCreateDXGIDeviceManager = (void *)dlsym(mfplat_dll, "MFCreateDXGIDeviceManager");
    if (!pMFCreateDXGIDeviceManager) {
        av_log(ctx, loglevel, "MFCreateDXGIDeviceManager not found\n");
        goto error;
    }

    hr = pMFCreateDXGIDeviceManager(&token, &hwctx->d3d11_manager);
    if (FAILED(hr)) {
        av_log(ctx, loglevel, "failed to create IMFDXGIDeviceManager\n");
        goto error;
    }

    hr = IMFDXGIDeviceManager_ResetDevice(hwctx->d3d11_manager, (IUnknown *)d3d11_device, token);
    if (FAILED(hr)) {
        av_log(ctx, loglevel, "failed to init IMFDXGIDeviceManager\n");
        goto error;
    }

    ID3D11Device_Release(d3d11_device);
    return 0;

error:
    if (d3d11_device)
        ID3D11Device_Release(d3d11_device);
    mf_uninit_d3d(ctx);
    return AVERROR_EXTERNAL;
}

static int mf_create_d3d9_device(AVHWDeviceContext *ctx, int loglevel)
{
    AVMFDeviceContext *hwctx = ctx->hwctx;
    MFDeviceContext *priv = ctx->internal->priv;
    pDirect3DCreate9      *createD3D = NULL;
    pDirect3DCreate9Ex    *createD3DEx = NULL;;
    pCreateDeviceManager9 *createDeviceManager = NULL;
    IDirect3D9Ex          *d3d9ex = NULL;
    IDirect3DDevice9Ex    *d3d9deviceEx = NULL;
    IDirect3DDevice9      *d3d9device = NULL;
    HRESULT hr;
    D3DPRESENT_PARAMETERS d3dpp = d3d_present_params;
    D3DDISPLAYMODE        d3ddm;
    BOOL                  fallbackto_d3d9 = TRUE;
    unsigned resetToken = 0;

    if (hwctx->init_d3d9_device) {
        d3d9device = hwctx->init_d3d9_device;
        IDirect3DDevice9_AddRef(d3d9device);
    } else {
        priv->d3d9_dll = dlopen("d3d9.dll", 0);
        if (!priv->d3d9_dll) {
            av_log(ctx, loglevel, "Failed to load D3D9 library\n");
            goto fail;
        }

        // try using Direct3DCreate9Ex first
        createD3DEx = (pDirect3DCreate9Ex*)dlsym(priv->d3d9_dll, "Direct3DCreate9Ex");
        if (createD3DEx) {
            hr = createD3DEx(D3D_SDK_VERSION, &d3d9ex);
            if (FAILED(hr) || d3d9ex == NULL) {
                av_log(ctx, loglevel, "Failed to locate Direct3DCreate9Ex\n");
            } else {
                D3DDISPLAYMODEEX modeex = { 0 };
                modeex.Size = sizeof(D3DDISPLAYMODEEX);

                hr = IDirect3D9Ex_GetAdapterDisplayModeEx(d3d9ex, hwctx->init_d3d9_adapter, &modeex, NULL);
                if (FAILED(hr)) {
                    av_log(ctx, loglevel, "Failed to get adapter display mode ex\n");
                } else {
                    d3dpp.BackBufferFormat = modeex.Format;

                    hr = IDirect3D9Ex_CreateDeviceEx(d3d9ex, hwctx->init_d3d9_adapter, D3DDEVTYPE_HAL, GetDesktopWindow(),
                        FF_D3DCREATE_FLAGS,
                        &d3dpp, NULL, &d3d9deviceEx);

                    if (FAILED(hr)) {
                        av_log(ctx, loglevel, "Failed to create D3D9Ex device\n");
                    } else {
                        av_log(ctx, AV_LOG_VERBOSE, "Using D3D9Ex device.\n");
                        priv->d3d9 = (IDirect3D9*)d3d9ex;
                        hwctx->init_d3d9_device = (IDirect3DDevice9 *)d3d9deviceEx;
                        fallbackto_d3d9 = FALSE;
                    }
                }
            }
        }

        // fallback to using Direct2DCreate9 
        if (fallbackto_d3d9) {
            createD3D = (pDirect3DCreate9*)dlsym(priv->d3d9_dll, "Direct3DCreate9");
            if (!createD3D) {
                av_log(ctx, loglevel, "Failed to locate Direct3DCreate9\n");
                goto fail;
            }

            priv->d3d9 = createD3D(D3D_SDK_VERSION);
            if (!priv->d3d9) {
                av_log(ctx, loglevel, "Failed to create IDirect3D object\n");
                goto fail;
            }

            IDirect3D9_GetAdapterDisplayMode(priv->d3d9, hwctx->init_d3d9_adapter, &d3ddm);

            d3dpp.BackBufferFormat = d3ddm.Format;

            hr = IDirect3D9_CreateDevice(priv->d3d9, hwctx->init_d3d9_adapter, D3DDEVTYPE_HAL, GetDesktopWindow(),
                FF_D3DCREATE_FLAGS,
                &d3dpp, &d3d9device);
            if (FAILED(hr)) {
                av_log(ctx, loglevel, "Failed to create Direct3D device\n");
                goto fail;
            }

            hwctx->init_d3d9_device = d3d9device;
        }
    }

    priv->dxva2_dll = dlopen("dxva2.dll", 0);
    if (!priv->dxva2_dll) {
        av_log(ctx, loglevel, "Failed to load DXVA2 library\n");
        goto fail;
    }

    createDeviceManager = (pCreateDeviceManager9 *)dlsym(priv->dxva2_dll, "DXVA2CreateDirect3DDeviceManager9");
    if (!createDeviceManager) {
        av_log(ctx, loglevel, "Failed to locate DXVA2CreateDirect3DDeviceManager9\n");
        goto fail;
    }

    hr = createDeviceManager(&resetToken, &hwctx->d3d9_manager);
    if (FAILED(hr)) {
        av_log(ctx, loglevel, "Failed to create Direct3D device manager\n");
        goto fail;
    }

    hr = IDirect3DDeviceManager9_ResetDevice(hwctx->d3d9_manager, d3d9device, resetToken);
    if (FAILED(hr)) {
        av_log(ctx, loglevel, "Failed to bind Direct3D device to device manager\n");
        goto fail;
    }

    return 0;

fail:
    if (d3d9device)
        IDirect3DDevice9_Release(d3d9device);
    if (d3d9deviceEx)
        IDirect3DDevice9Ex_Release(d3d9deviceEx);
    if (d3d9ex)
        IDirect3D9Ex_Release(d3d9ex);
     
     mf_uninit_d3d(ctx);
    return AVERROR_EXTERNAL;
}

static int mf_device_init(AVHWDeviceContext *ctx)
{
    AVMFDeviceContext *hwctx = ctx->hwctx;
    int ret;

    switch (hwctx->device_type) {
    case AV_MF_NONE:
        if (hwctx->d3d11_manager || hwctx->d3d9_manager || hwctx->d3d12_command_queue)
            return AVERROR(EINVAL);
        break;

    case AV_MF_D3D11on12:
        if (hwctx->d3d9_manager || hwctx->d3d11_manager)
            return AVERROR(EINVAL);
        if (!hwctx->d3d12_command_queue && ((ret = mf_create_d3d11on12_device(ctx, AV_LOG_ERROR)) < 0))
            return ret;
        break;

    case AV_MF_D3D11:
        if (hwctx->d3d9_manager || hwctx->d3d12_command_queue)
            return AVERROR(EINVAL);
        if (!hwctx->d3d11_manager && ((ret = mf_create_d3d11_device(ctx, AV_LOG_ERROR)) < 0))
            return ret;
        break;

    case AV_MF_D3D9:
        if (hwctx->d3d11_manager || hwctx->d3d12_command_queue)
            return AVERROR(EINVAL);
        if (!hwctx->d3d9_manager && ((ret = mf_create_d3d9_device(ctx, AV_LOG_ERROR)) < 0))
            return ret;
        break;

    case AV_MF_AUTO:
        if (mf_create_d3d11on12_device(ctx, AV_LOG_VERBOSE) >= 0) {
            hwctx->device_type = AV_MF_D3D11on12;
        }
        else if (mf_create_d3d11_device(ctx, AV_LOG_VERBOSE) >= 0) {
            hwctx->device_type = AV_MF_D3D11;
        }
        else if (mf_create_d3d9_device(ctx, AV_LOG_VERBOSE) >= 0) {
            hwctx->device_type = AV_MF_D3D9;
        }
        else {
            hwctx->device_type = AV_MF_NONE;
        }
        break;

    default:
        return AVERROR(EINVAL);
        break;
    }

    return 0;
}

static void mf_device_uninit(AVHWDeviceContext *ctx)
{
    AVMFDeviceContext* hwctx = ctx->hwctx;

    mf_uninit_d3d(ctx);

    if (hwctx->init_d3d9_device)
        IDirect3DDevice9_Release(hwctx->init_d3d9_device);

    if (hwctx->init_d3d11_device)
        ID3D11Device_Release(hwctx->init_d3d11_device);

    if (hwctx->init_d3d12_device)
        ID3D12Device_Release(hwctx->init_d3d12_device);
}

static int mf_transfer_get_formats(AVHWFramesContext *ctx,
                                   enum AVHWFrameTransferDirection dir,
                                   enum AVPixelFormat **formats)
{
    *formats = av_malloc_array(2, sizeof(*formats));
    if (!*formats)
        return AVERROR(ENOMEM);

    (*formats)[0] = ctx->sw_format;
    (*formats)[1] = AV_PIX_FMT_NONE;

    return 0;
}

static int mf_transfer_data_from(AVHWFramesContext *ctx, AVFrame *dst,
                                    const AVFrame *src)
{
    IMFSample *sample = (void *)src->data[3];
    HRESULT hr;
    DWORD num_buffers;
    IMFMediaBuffer *buffer;
    IMF2DBuffer *buffer_2d = NULL;
    IMF2DBuffer2 *buffer_2d2 = NULL;
    uint8_t *src_data[4] = {0};
    int src_linesizes[4] = {0};
    int locked_1d = 0;
    int locked_2d = 0;
    int copy_w = FFMIN(dst->width, ctx->width);
    int copy_h = FFMIN(dst->height, ctx->height);
    int ret = 0;

    av_assert0(dst->format == ctx->sw_format);

    hr = IMFSample_GetBufferCount(sample, &num_buffers);
    if (FAILED(hr) || num_buffers != 1)
        return AVERROR_EXTERNAL;

    hr = IMFSample_GetBufferByIndex(sample, 0, &buffer);
    if (FAILED(hr))
        return AVERROR_EXTERNAL;

    // Prefer IMF2DBuffer(2) if supported - it's faster, but usually only
    // present if hwaccel is used.
    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer, (void **)&buffer_2d);
    if (!FAILED(hr) && (ctx->sw_format == AV_PIX_FMT_NV12 ||
                        ctx->sw_format == AV_PIX_FMT_P010)) {
        BYTE *sc = NULL;
        LONG pitch = 0;

        // Prefer IMF2DBuffer2 if supported.
        IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer2, (void **)&buffer_2d2);
        if (buffer_2d2) {
            BYTE *start = NULL;
            DWORD length = 0;
            hr = IMF2DBuffer2_Lock2DSize(buffer_2d2, MF2DBuffer_LockFlags_Read, &sc, &pitch, &start, &length);
        } else {
            hr = IMF2DBuffer_Lock2D(buffer_2d, &sc, &pitch);
        }
        if (FAILED(hr)) {
            ret = AVERROR_EXTERNAL;
            goto done;
        }
        locked_2d = 1; // (always uses IMF2DBuffer_Unlock2D)

        src_data[0] = (uint8_t *)sc;
        src_linesizes[0] = pitch;
        src_data[1] = (uint8_t *)sc + pitch * ctx->height;
        src_linesizes[1] = pitch;
    } else {
        BYTE *data;
        DWORD length;

        hr = IMFMediaBuffer_Lock(buffer, &data, NULL, &length);
        if (FAILED(hr)) {
            ret = AVERROR_EXTERNAL;
            goto done;
        }
        locked_1d = 1;

        av_image_fill_arrays(src_data, src_linesizes, data, dst->format,
                             ctx->width, ctx->height, 1);
    }

    av_image_copy(dst->data, dst->linesize, (void *)src_data, src_linesizes,
                  ctx->sw_format, copy_w, copy_h);

done:

    if (locked_1d)
        IMFMediaBuffer_Unlock(buffer);
    if (locked_2d)
        IMF2DBuffer_Unlock2D(buffer_2d);
    if (buffer_2d)
        IMF2DBuffer_Release(buffer_2d);
    if (buffer_2d2)
        IMF2DBuffer2_Release(buffer_2d2);

    IMFMediaBuffer_Release(buffer);
    return ret;
}

const HWContextType ff_hwcontext_type_mediafoundation = {
    .type                 = AV_HWDEVICE_TYPE_MEDIAFOUNDATION,
    .name                 = "mediafoundation",

    .device_hwctx_size    = sizeof(AVMFDeviceContext),
    .device_priv_size     = sizeof(MFDeviceContext),
    .frames_priv_size     = 0,

    .device_init          = mf_device_init,
    .device_uninit        = mf_device_uninit,
    .frames_init          = NULL,
    .frames_get_buffer    = NULL,
    .transfer_get_formats = mf_transfer_get_formats,
    .transfer_data_to     = NULL,
    .transfer_data_from   = mf_transfer_data_from,

    .pix_fmts = (const enum AVPixelFormat[]){ AV_PIX_FMT_MEDIAFOUNDATION, AV_PIX_FMT_NONE },
};