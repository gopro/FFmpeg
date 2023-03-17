
/*
 * AVCodec exported shim functions for libavcodec
 *
 * This file is part of GoPro forled FFmpeg.
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

/**
 * @file
 * AVCodec exported shim functions for libavcodec
 */

#include "config.h"
#include "libavutil/avassert.h"
#include "libavutil/avstring.h"
#include "libavutil/bprint.h"
#include "libavutil/channel_layout.h"
#include "libavutil/imgutils.h"
#include "libavutil/mem.h"
#include "libavutil/opt.h"
#include "libavutil/thread.h"
#include "avcodec.h"
#include "bsf.h"
#include "decode.h"
#include "encode.h"
#include "frame_thread_encoder.h"
#include "internal.h"
#include "thread.h"
#include "mf_utils.h"

extern IMFSample *av_ff_create_memory_sample(void *fill_data, size_t size, size_t align) 
{
    return ff_create_memory_sample(fill_data, size, align);
}

int av_ff_set_mf_attributes(IMFAttributes *pattrs, const GUID *attrid, UINT32 inhi, UINT32 inlo) 
{
    return ff_MFSetAttributeSize(pattrs, attrid, inhi, inlo);
}

int av_ff_get_mf_attributes(IMFAttributes *pattrs, const GUID *attrid, UINT32 *outhi, UINT32 *outlo)
{
    return ff_MFGetAttributeSize(pattrs, attrid, outhi, outlo);
}

int av_ff_set_dimensions(AVCodecContext *s, int width, int height) 
{
    return ff_set_dimensions(s, width, height);
}

char *av_ff_hr_str_buf(char *buf, size_t size, HRESULT hr)
{
    return ff_hr_str_buf(buf, size, hr);
}

char *av_ff_guid_str_buf(char *buf, size_t buf_size, const GUID *guid)
{
    return ff_guid_str_buf(buf, buf_size, guid);
}

int av_ff_decode_get_packet(AVCodecContext *avctx, AVPacket *pkt)
{
    return ff_decode_get_packet(avctx, pkt);
}

const CLSID *av_ff_codec_to_mf_subtype(enum AVCodecID codec)
{
    return ff_codec_to_mf_subtype(codec);
}

enum AVSampleFormat av_ff_media_type_to_sample_fmt(IMFAttributes *type)
{
    return ff_media_type_to_sample_fmt(type);
}

enum AVPixelFormat av_ff_media_type_to_pix_fmt(IMFAttributes *type)
{
    return ff_media_type_to_pix_fmt(type);
}

int av_ff_fourcc_from_guid(const GUID *guid, uint32_t *out_fourcc)
{
    return ff_fourcc_from_guid(guid, out_fourcc);
}

void av_ff_attributes_dump(void *log, IMFAttributes *attrs)
{
    ff_attributes_dump(log, attrs);
}

int av_ff_decode_frame_props(AVCodecContext *avctx, AVFrame *frame)
{
    return ff_decode_frame_props(avctx, frame);
}

int av_ff_get_format(AVCodecContext *avctx, const enum AVPixelFormat *fmt)
{
    return ff_get_format(avctx, fmt);
}

int av_ff_attach_decode_data(AVFrame *frame)
{
    return ff_attach_decode_data(frame);
}

int av_ff_get_buffer(AVCodecContext *avctx, AVFrame *frame, int flags)
{
    return ff_get_buffer(avctx, frame, flags);
}

int av_ff_instantiate_mf(void* log, GUID category, MFT_REGISTER_TYPE_INFO* in_type,
    MFT_REGISTER_TYPE_INFO* out_type, int use_hw, IMFTransform** res)
{
    return ff_instantiate_mf(log, category, in_type, out_type, use_hw, res);
}

void av_ff_free_mf(IMFTransform** mft)
{
    ff_free_mf(mft);
}
