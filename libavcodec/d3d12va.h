/*
 * Direct3D12 HW acceleration
 *
 * copyright (c) 2009 Laurent Aimar
 * copyright (c) 2015 Steve Lhomme
 *
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

#ifndef AVCODEC_D3D12VA_H
#define AVCODEC_D3D12VA_H

/**
 * @file
 * @ingroup lavc_codec_hwaccel_d3d12va
 * Public libavcodec D3D12VA header.
 */

#if !defined(_WIN32_WINNT) || _WIN32_WINNT < 0x0602
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0602
#endif

#include <stdint.h>
#include <d3d12.h>

/**
 * @defgroup lavc_codec_hwaccel_d3d12va Direct3D12
 * @ingroup lavc_codec_hwaccel
 *
 * @{
 */

#define FF_DXVA2_WORKAROUND_SCALING_LIST_ZIGZAG 1 ///< Work around for Direct3D11 and old UVD/UVD+ ATI video cards
#define FF_DXVA2_WORKAROUND_INTEL_CLEARVIDEO    2 ///< Work around for Direct3D11 and old Intel GPUs with ClearVideo interface

/**
 * This structure is used to provides the necessary configurations and data
 * to the Direct3D11 FFmpeg HWAccel implementation.
 *
 * The application must make it available as AVCodecContext.hwaccel_context.
 *
 * Use av_d3d12va_alloc_context() exclusively to allocate an AVD3D12VAContext.
 */
typedef struct AVD3D12VAContext {
    /**
     * D3D12 decoder object
     */
    ID3D12VideoDecoder *decoder;

    /**
     * The number of surface in the surface array
     */
    unsigned surface_count;

    /**
     * A bit field configuring the workarounds needed for using the decoder
     */
    uint64_t workaround;

    /**
     * Private to the FFmpeg AVHWAccel implementation
     */
    unsigned report_id;

} AVD3D12VAContext;

/**
 * Allocate an AVD3D12VAContext.
 *
 * @return Newly-allocated AVD3D12VAContext or NULL on failure.
 */
AVD3D12VAContext *av_d3d12va_alloc_context(void);

/**
 * @}
 */

#endif /* AVCODEC_D3D12VA_H */
