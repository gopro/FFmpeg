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

#ifndef AVCODEC_MF_UTILS_H
#define AVCODEC_MF_UTILS_H

#include <windows.h>
#include <initguid.h>
#ifdef _MSC_VER
// The official way of including codecapi (via dshow.h) makes the ICodecAPI
// interface unavailable in UWP mode, but including icodecapi.h + codecapi.h
// seems to be equivalent. (These headers conflict with the official way
// of including it though, through strmif.h via dshow.h. And on mingw, the
// mf*.h headers below indirectly include strmif.h.)
#include <icodecapi.h>
#else
#define NO_DSHOW_STRSAFE
#include <dshow.h>
// Older versions of mingw-w64 need codecapi.h explicitly included, while newer
// ones include it implicitly from dshow.h (via uuids.h).
#include <codecapi.h>
#endif
#include <mfapi.h>
#include <mferror.h>
#include <mfobjects.h>
#include <mftransform.h>

#include "avcodec.h"
#include "bsf.h"

// Windows N editions does not provide MediaFoundation by default.
// So to avoid DLL loading error, MediaFoundation will be dynamically loaded
// except on UWP build since LoadLibrary is not available on it.
typedef struct MFFunctions {
    HMODULE library;
    HRESULT (WINAPI *MFStartup) (ULONG Version, DWORD dwFlags);
    HRESULT (WINAPI *MFShutdown) (void);
    HRESULT (WINAPI *MFCreateAlignedMemoryBuffer) (DWORD cbMaxLength,
                                                   DWORD cbAligment,
                                                   IMFMediaBuffer **ppBuffer);
    HRESULT (WINAPI *MFCreateSample) (IMFSample **ppIMFSample);
    HRESULT (WINAPI *MFCreateMediaType) (IMFMediaType **ppMFType);
    // MFTEnumEx is missing in Windows Vista's mfplat.dll.
    HRESULT (WINAPI *MFTEnumEx)(GUID guidCategory, UINT32 Flags,
                                const MFT_REGISTER_TYPE_INFO *pInputType,
                                const MFT_REGISTER_TYPE_INFO *pOutputType,
                                IMFActivate ***pppMFTActivate,
                                UINT32 *pnumMFTActivate);
} MFFunctions;

#define BASE_CODEC_CONTEXT                      \
    AVClass *av_class;                          \
    AVFrame *frame;                             \
    GUID main_subtype;                          \
    MFFunctions mf_api;                         \
    IMFTransform *mft;                          \
    ICodecAPI *codec_api;                       \
    IMFMediaEventGenerator *async_events;       \
    int async_need_input;                       \
    int async_have_output;                      \
    int async_marker;                           \
    DWORD in_stream_id, out_stream_id;          \
    MFT_INPUT_STREAM_INFO in_info;              \
    MFT_OUTPUT_STREAM_INFO out_info;            \
    int is_video, is_audio;                     \
    int out_stream_provides_samples;            \
    int draining, draining_done;                \
    int sample_sent;

// These functions do exist in mfapi.h, but are only available within
// __cplusplus ifdefs.
HRESULT ff_MFGetAttributeSize(IMFAttributes *pattr, REFGUID guid,
                              UINT32 *pw, UINT32 *ph);
HRESULT ff_MFSetAttributeSize(IMFAttributes *pattr, REFGUID guid,
                              UINT32 uw, UINT32 uh);
#define ff_MFSetAttributeRatio ff_MFSetAttributeSize
#define ff_MFGetAttributeRatio ff_MFGetAttributeSize

// Consider cleaning up ff_CODECAPI_xxx defines (these exist because the official MS GUIDS were NOT defined in earlier MINGW headers)
// for FFmpeg main branch pull request, given MINGW version is up to 11 now:
// See:  https://www.mingw-w64.org/changelog

DEFINE_GUID(ff_MF_MT_VIDEO_ROTATION, 0xc380465d, 0x2271, 0x428c, 0x9b, 0x83, 0xec, 0xea, 0x3b, 0x4a, 0x85, 0xc1);
DEFINE_GUID(ff_CODECAPI_AVDecVideoAcceleration_H264, 0xf7db8a2f, 0x4f48, 0x4ee8, 0xae, 0x31, 0x8b, 0x6e, 0xbe, 0x55, 0x8a, 0xe2);

// WMA1. Apparently there is no official GUID symbol for this.
DEFINE_GUID(ff_MFAudioFormat_MSAUDIO1, 0x00000160, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);

// MP42 FourCC. WM4/Pawel Wegner made up the symbol name.
DEFINE_GUID(ff_MFVideoFormat_MP42, 0x3234504D, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);

// These do exist in mingw-w64's codecapi.h, but they aren't properly defined
// by the header until after mingw-w64 v7.0.0.
DEFINE_GUID(ff_CODECAPI_AVDecVideoThumbnailGenerationMode, 0x2efd8eee,0x1150,0x4328,0x9c,0xf5,0x66,0xdc,0xe9,0x33,0xfc,0xf4);
DEFINE_GUID(ff_CODECAPI_AVDecVideoDropPicWithMissingRef, 0xf8226383,0x14c2,0x4567,0x97,0x34,0x50,0x04,0xe9,0x6f,0xf8,0x87);
DEFINE_GUID(ff_CODECAPI_AVDecVideoSoftwareDeinterlaceMode, 0x0c08d1ce,0x9ced,0x4540,0xba,0xe3,0xce,0xb3,0x80,0x14,0x11,0x09);
DEFINE_GUID(ff_CODECAPI_AVDecVideoFastDecodeMode, 0x6b529f7d,0xd3b1,0x49c6,0xa9,0x99,0x9e,0xc6,0x91,0x1b,0xed,0xbf);
DEFINE_GUID(ff_CODECAPI_AVLowLatencyMode, 0x9c27891a,0xed7a,0x40e1,0x88,0xe8,0xb2,0x27,0x27,0xa0,0x24,0xee);
DEFINE_GUID(ff_CODECAPI_AVDecVideoH264ErrorConcealment, 0xececace8,0x3436,0x462c,0x92,0x94,0xcd,0x7b,0xac,0xd7,0x58,0xa9);
DEFINE_GUID(ff_CODECAPI_AVDecVideoMPEG2ErrorConcealment, 0x9d2bfe18,0x728d,0x48d2,0xb3,0x58,0xbc,0x7e,0x43,0x6c,0x66,0x74);
DEFINE_GUID(ff_CODECAPI_AVDecVideoCodecType, 0x434528e5,0x21f0,0x46b6,0xb6,0x2c,0x9b,0x1b,0x6b,0x65,0x8c,0xd1);
DEFINE_GUID(ff_CODECAPI_AVDecVideoDXVAMode, 0xf758f09e,0x7337,0x4ae7,0x83,0x87,0x73,0xdc,0x2d,0x54,0xe6,0x7d);
DEFINE_GUID(ff_CODECAPI_AVDecVideoDXVABusEncryption, 0x42153c8b,0xfd0b,0x4765,0xa4,0x62,0xdd,0xd9,0xe8,0xbc,0xc3,0x88);
DEFINE_GUID(ff_CODECAPI_AVDecVideoSWPowerLevel, 0xfb5d2347,0x4dd8,0x4509,0xae,0xd0,0xdb,0x5f,0xa9,0xaa,0x93,0xf4);
DEFINE_GUID(ff_CODECAPI_AVDecVideoMaxCodedWidth, 0x5ae557b8,0x77af,0x41f5,0x9f,0xa6,0x4d,0xb2,0xfe,0x1d,0x4b,0xca);
DEFINE_GUID(ff_CODECAPI_AVDecVideoMaxCodedHeight, 0x7262a16a,0xd2dc,0x4e75,0x9b,0xa8,0x65,0xc0,0xc6,0xd3,0x2b,0x13);
DEFINE_GUID(ff_CODECAPI_AVDecNumWorkerThreads, 0x9561c3e8,0xea9e,0x4435,0x9b,0x1e,0xa9,0x3e,0x69,0x18,0x94,0xd8);
DEFINE_GUID(ff_CODECAPI_AVDecSoftwareDynamicFormatChange, 0x862e2f0a,0x507b,0x47ff,0xaf,0x47,0x01,0xe2,0x62,0x42,0x98,0xb7);
DEFINE_GUID(ff_CODECAPI_AVDecDisableVideoPostProcessing, 0xf8749193,0x667a,0x4f2c,0xa9,0xe8,0x5d,0x4a,0xf9,0x24,0xf0,0x8f);

// These are missing from mingw-w64's headers until after mingw-w64 v7.0.0.
DEFINE_GUID(ff_CODECAPI_AVEncCommonRateControlMode,      0x1c0608e9, 0x370c, 0x4710, 0x8a, 0x58, 0xcb, 0x61, 0x81, 0xc4, 0x24, 0x23);
DEFINE_GUID(ff_CODECAPI_AVEncCommonQuality,       0xfcbf57a3, 0x7ea5, 0x4b0c, 0x96, 0x44, 0x69, 0xb4, 0x0c, 0x39, 0xc3, 0x91);
DEFINE_GUID(ff_CODECAPI_AVEncCommonMeanBitRate,   0xf7222374, 0x2144, 0x4815, 0xb5, 0x50, 0xa3, 0x7f, 0x8e, 0x12, 0xee, 0x52);
DEFINE_GUID(ff_CODECAPI_AVEncH264CABACEnable,    0xee6cad62, 0xd305, 0x4248, 0xa5, 0xe, 0xe1, 0xb2, 0x55, 0xf7, 0xca, 0xf8);
DEFINE_GUID(ff_CODECAPI_AVEncVideoForceKeyFrame, 0x398c1b98, 0x8353, 0x475a, 0x9e, 0xf2, 0x8f, 0x26, 0x5d, 0x26, 0x3, 0x45);
DEFINE_GUID(ff_CODECAPI_AVEncMPVDefaultBPictureCount, 0x8d390aac, 0xdc5c, 0x4200, 0xb5, 0x7f, 0x81, 0x4d, 0x04, 0xba, 0xba, 0xb2);
DEFINE_GUID(ff_CODECAPI_AVScenarioInfo, 0xb28a6e64,0x3ff9,0x446a,0x8a,0x4b,0x0d,0x7a,0x53,0x41,0x32,0x36);

DEFINE_GUID(ff_CODECAPI_AVEncCommonLowLatency,    0x9d3ecd55, 0x89e8, 0x490a, 0x97, 0x0a, 0x0c, 0x95, 0x48, 0xd5, 0xa5, 0x6e);
DEFINE_GUID(ff_CODECAPI_AVEncCommonRealTime,      0x143a0ff6, 0xa131, 0x43da, 0xb8, 0x1e, 0x98, 0xfb, 0xb8, 0xec, 0x37, 0x8e);
DEFINE_GUID(ff_CODECAPI_AVEncCommonQualityVsSpeed, 0x98332df8, 0x03cd, 0x476b, 0x89, 0xfa, 0x3f, 0x9e, 0x44, 0x2d, 0xec, 0x9f);
DEFINE_GUID(ff_CODECAPI_AVEncCommonTranscodeEncodingProfile, 0x6947787C, 0xF508, 0x4EA9, 0xB1, 0xE9, 0xA1, 0xFE, 0x3A, 0x49, 0xFB, 0xC9);
DEFINE_GUID(ff_CODECAPI_AVEncCommonMeanBitRateInterval, 0xbfaa2f0c, 0xcb82, 0x4bc0, 0x84, 0x74, 0xf0, 0x6a, 0x8a, 0x0d, 0x02, 0x58);
DEFINE_GUID(ff_CODECAPI_AVEncCommonMaxBitRate,    0x9651eae4, 0x39b9, 0x4ebf, 0x85, 0xef, 0xd7, 0xf4, 0x44, 0xec, 0x74, 0x65);
DEFINE_GUID(ff_CODECAPI_AVEncCommonMinBitRate,    0x101405b2, 0x2083, 0x4034, 0xa8, 0x06, 0xef, 0xbe, 0xdd, 0xd7, 0xc9, 0xff);
DEFINE_GUID(ff_CODECAPI_AVEncVideoCBRMotionTradeoff, 0x0d49451e, 0x18d5, 0x4367, 0xa4, 0xef, 0x32, 0x40, 0xdf, 0x16, 0x93, 0xc4);
DEFINE_GUID(ff_CODECAPI_AVEncVideoCodedVideoAccessUnitSize, 0xb4b10c15, 0x14a7, 0x4ce8, 0xb1, 0x73, 0xdc, 0x90, 0xa0, 0xb4, 0xfc, 0xdb);
DEFINE_GUID(ff_CODECAPI_AVEncVideoMaxKeyframeDistance,   0x2987123a, 0xba93, 0x4704, 0xb4, 0x89, 0xec, 0x1e, 0x5f, 0x25, 0x29, 0x2c);
DEFINE_GUID(ff_CODECAPI_AVEncVideoContentType,   0x66117aca, 0xeb77, 0x459d, 0x93, 0xc, 0xa4, 0x8d, 0x9d, 0x6, 0x83, 0xfc);
DEFINE_GUID(ff_CODECAPI_AVEncNumWorkerThreads,   0xb0c8bf60, 0x16f7, 0x4951, 0xa3, 0xb, 0x1d, 0xb1, 0x60, 0x92, 0x93, 0xd6);
DEFINE_GUID(ff_CODECAPI_AVEncVideoEncodeQP,      0x2cb5696b, 0x23fb, 0x4ce1, 0xa0, 0xf9, 0xef, 0x5b, 0x90, 0xfd, 0x55, 0xca);
DEFINE_GUID(ff_CODECAPI_AVEncVideoMinQP,         0x0ee22c6a, 0xa37c, 0x4568, 0xb5, 0xf1, 0x9d, 0x4c, 0x2b, 0x3a, 0xb8, 0x86);
DEFINE_GUID(ff_CODECAPI_AVEncAdaptiveMode,       0x4419b185, 0xda1f, 0x4f53, 0xbc, 0x76, 0x9, 0x7d, 0xc, 0x1e, 0xfb, 0x1e);
DEFINE_GUID(ff_CODECAPI_AVEncVideoTemporalLayerCount,    0x19caebff, 0xb74d, 0x4cfd, 0x8c, 0x27, 0xc2, 0xf9, 0xd9, 0x7d, 0x5f, 0x52);
DEFINE_GUID(ff_CODECAPI_AVEncVideoUsage,         0x1f636849, 0x5dc1, 0x49f1, 0xb1, 0xd8, 0xce, 0x3c, 0xf6, 0x2e, 0xa3, 0x85);
DEFINE_GUID(ff_CODECAPI_AVEncVideoSelectLayer,        0xeb1084f5, 0x6aaa, 0x4914, 0xbb, 0x2f, 0x61, 0x47, 0x22, 0x7f, 0x12, 0xe7);
DEFINE_GUID(ff_CODECAPI_AVEncVideoRateControlParams,  0x87d43767, 0x7645, 0x44ec, 0xb4, 0x38, 0xd3, 0x32, 0x2f, 0xbc, 0xa2, 0x9f);
DEFINE_GUID(ff_CODECAPI_AVEncVideoSupportedControls,  0xd3f40fdd, 0x77b9, 0x473d, 0x81, 0x96, 0x6, 0x12, 0x59, 0xe6, 0x9c, 0xff);
DEFINE_GUID(ff_CODECAPI_AVEncVideoEncodeFrameTypeQP, 0xaa70b610, 0xe03f, 0x450c, 0xad, 0x07, 0x07, 0x31, 0x4e, 0x63, 0x9c, 0xe7);
DEFINE_GUID(ff_CODECAPI_AVEncSliceControlMode,       0xe9e782ef, 0x5f18, 0x44c9, 0xa9, 0x0b, 0xe9, 0xc3, 0xc2, 0xc1, 0x7b, 0x0b);
DEFINE_GUID(ff_CODECAPI_AVEncSliceControlSize,       0x92f51df3, 0x07a5, 0x4172, 0xae, 0xfe, 0xc6, 0x9c, 0xa3, 0xb6, 0x0e, 0x35);
DEFINE_GUID(ff_CODECAPI_AVEncVideoMaxNumRefFrame,    0x964829ed, 0x94f9, 0x43b4, 0xb7, 0x4d, 0xef, 0x40, 0x94, 0x4b, 0x69, 0xa0);
DEFINE_GUID(ff_CODECAPI_AVEncVideoMeanAbsoluteDifference,    0xe5c0c10f, 0x81a4, 0x422d, 0x8c, 0x3f, 0xb4, 0x74, 0xa4, 0x58, 0x13, 0x36);
DEFINE_GUID(ff_CODECAPI_AVEncVideoMaxQP,             0x3daf6f66, 0xa6a7, 0x45e0, 0xa8, 0xe5, 0xf2, 0x74, 0x3f, 0x46, 0xa3, 0xa2);
DEFINE_GUID(ff_CODECAPI_AVEncMPVGOPSize,          0x95f31b26, 0x95a4, 0x41aa, 0x93, 0x03, 0x24, 0x6a, 0x7f, 0xc6, 0xee, 0xf1);
DEFINE_GUID(ff_CODECAPI_AVEncMPVGOPOpen,          0xb1d5d4a6, 0x3300, 0x49b1, 0xae, 0x61, 0xa0, 0x99, 0x37, 0xab, 0x0e, 0x49);
DEFINE_GUID(ff_CODECAPI_AVEncMPVProfile,          0xdabb534a, 0x1d99, 0x4284, 0x97, 0x5a, 0xd9, 0x0e, 0x22, 0x39, 0xba, 0xa1);
DEFINE_GUID(ff_CODECAPI_AVEncMPVLevel,            0x6ee40c40, 0xa60c, 0x41ef, 0x8f, 0x50, 0x37, 0xc2, 0x24, 0x9e, 0x2c, 0xb3);
DEFINE_GUID(ff_CODECAPI_AVEncMPVFrameFieldMode,   0xacb5de96, 0x7b93, 0x4c2f, 0x88, 0x25, 0xb0, 0x29, 0x5f, 0xa9, 0x3b, 0xf4);
DEFINE_GUID(ff_CODECAPI_AVEncMPVAddSeqEndCode,    0xa823178f, 0x57df, 0x4c7a, 0xb8, 0xfd, 0xe5, 0xec, 0x88, 0x87, 0x70, 0x8d);
DEFINE_GUID(ff_CODECAPI_AVEncMPVGOPSInSeq,        0x993410d4, 0x2691, 0x4192, 0x99, 0x78, 0x98, 0xdc, 0x26, 0x03, 0x66, 0x9f);
DEFINE_GUID(ff_CODECAPI_AVEncMPVUseConcealmentMotionVectors,  0xec770cf3, 0x6908, 0x4b4b, 0xaa, 0x30, 0x7f, 0xb9, 0x86, 0x21, 0x4f, 0xea);

DEFINE_GUID(ff_MF_SA_D3D11_BINDFLAGS, 0xeacf97ad, 0x065c, 0x4408, 0xbe, 0xe3, 0xfd, 0xcb, 0xfd, 0x12, 0x8b, 0xe2);
DEFINE_GUID(ff_MF_SA_D3D11_USAGE, 0xe85fe442, 0x2ca3, 0x486e, 0xa9, 0xc7, 0x10, 0x9d, 0xda, 0x60, 0x98, 0x80);
DEFINE_GUID(ff_MF_SA_D3D11_AWARE, 0x206b4fc8, 0xfcf9, 0x4c51, 0xaf, 0xe3, 0x97, 0x64, 0x36, 0x9e, 0x33, 0xa0);
DEFINE_GUID(ff_MF_SA_D3D11_SHARED, 0x7b8f32c3, 0x6d96, 0x4b89, 0x92, 0x3, 0xdd, 0x38, 0xb6, 0x14, 0x14, 0xf3);
DEFINE_GUID(ff_MF_SA_D3D11_SHARED_WITHOUT_MUTEX, 0x39dbd44d, 0x2e44, 0x4931, 0xa4, 0xc8, 0x35, 0x2d, 0x3d, 0xc4, 0x21, 0x15);
DEFINE_GUID(ff_MF_SA_MINIMUM_OUTPUT_SAMPLE_COUNT, 0x851745d5, 0xc3d6, 0x476d, 0x95, 0x27, 0x49, 0x8e, 0xf2, 0xd1, 0xd, 0x18);
DEFINE_GUID(ff_MF_SA_MINIMUM_OUTPUT_SAMPLE_COUNT_PROGRESSIVE, 0xf5523a5, 0x1cb2, 0x47c5, 0xa5, 0x50, 0x2e, 0xeb, 0x84, 0xb4, 0xd1, 0x4a);

// This enum is missing from mingw-w64's codecapi.h by v7.0.0.
enum ff_eAVEncCommonRateControlMode {
    ff_eAVEncCommonRateControlMode_CBR                 = 0,
    ff_eAVEncCommonRateControlMode_PeakConstrainedVBR  = 1,
    ff_eAVEncCommonRateControlMode_UnconstrainedVBR    = 2,
    ff_eAVEncCommonRateControlMode_Quality             = 3,
    ff_eAVEncCommonRateControlMode_LowDelayVBR         = 4,
    ff_eAVEncCommonRateControlMode_GlobalVBR           = 5,
    ff_eAVEncCommonRateControlMode_GlobalLowDelayVBR   = 6
};

enum ff_eAVScenarioInfo {
    ff_eAVScenarioInfo_Unknown                         = 0,
    ff_eAVScenarioInfo_DisplayRemoting                 = 1,
    ff_eAVScenarioInfo_VideoConference                 = 2,
    ff_eAVScenarioInfo_Archive                         = 3,
    ff_eAVScenarioInfo_LiveStreaming                   = 4,
    ff_eAVScenarioInfo_CameraRecord                    = 5,
    ff_eAVScenarioInfo_DisplayRemotingWithFeatureMap   = 6
};

// These do exist in mingw-w64's mfobjects.idl, but are missing from
// mfobjects.h that is generated from the former, due to incorrect use of
// ifdefs in the IDL file.
enum {
    ff_METransformUnknown = 600,
    ff_METransformNeedInput,
    ff_METransformHaveOutput,
    ff_METransformDrainComplete,
    ff_METransformMarker,
};

// These do exist in all supported headers, but are manually defined here
// to avoid having to include codecapi.h, as there's problems including that
// header when targeting UWP (where including it with MSVC seems to work,
// but fails when built with clang in MSVC mode).
enum ff_eAVEncH264VProfile {
   ff_eAVEncH264VProfile_Base = 66,
   ff_eAVEncH264VProfile_Main = 77,
   ff_eAVEncH264VProfile_High = 100,
};

char *ff_hr_str_buf(char *buf, size_t size, HRESULT hr);
#define ff_hr_str(hr) ff_hr_str_buf((char[80]){0}, 80, hr)

// Possibly compiler-dependent; the MS/MinGW definition for this is just crazy.
#define FF_VARIANT_VALUE(type, contents) &(VARIANT){ .vt = (type), contents }

#define FF_VAL_VT_UI4(v) FF_VARIANT_VALUE(VT_UI4, .ulVal = (v))
#define FF_VAL_VT_BOOL(v) FF_VARIANT_VALUE(VT_BOOL, .boolVal = (v))

int ff_mf_load_library(AVCodecContext* avctx, MFFunctions* functions);
IMFSample *ff_create_memory_sample(MFFunctions *f, void *fill_data,
                                   size_t size, size_t align);
enum AVSampleFormat ff_media_type_to_sample_fmt(IMFAttributes *type);
enum AVPixelFormat ff_media_type_to_pix_fmt(IMFAttributes *type);
const GUID *ff_pix_fmt_to_guid(enum AVPixelFormat pix_fmt);
int ff_fourcc_from_guid(const GUID *guid, uint32_t *out_fourcc);
char *ff_guid_str_buf(char *buf, size_t buf_size, const GUID *guid);
#define ff_guid_str(guid) ff_guid_str_buf((char[80]){0}, 80, guid)
void ff_attributes_dump(void *log, IMFAttributes *attrs);
void ff_media_type_dump(void *log, IMFMediaType *type);
const CLSID *ff_codec_to_mf_subtype(enum AVCodecID codec);
int ff_instantiate_mf(void *log, MFFunctions *f, GUID category,
                      MFT_REGISTER_TYPE_INFO *in_type,
                      MFT_REGISTER_TYPE_INFO *out_type,
                      int use_hw, IMFTransform **res);
void ff_free_mf(MFFunctions *f);
int mf_create(void* log, MFFunctions* f, IMFTransform** mft, const AVCodec* codec, int use_hw);

#endif
