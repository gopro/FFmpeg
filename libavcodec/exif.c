/*
 * EXIF metadata parser
 * Copyright (c) 2013 Thilo Borgmann <thilo.borgmann _at_ mail.de>
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

/**
 * @file
 * EXIF metadata parser
 * @author Thilo Borgmann <thilo.borgmann _at_ mail.de>
 */

#include "libavutil/avstring.h"

#include "exif.h"
#include "tiff_common.h"
#include "exif_utils.h"

#define EXIF_TAG_NAME_LENGTH   32

struct exif_tag {
    char      name[EXIF_TAG_NAME_LENGTH];
    uint16_t  id;
};

static const struct exif_tag tag_list[] = { // JEITA CP-3451 EXIF specification:
    {"GPSVersionID",               0x00}, // <- Table 12 GPS Attribute Information
    {"GPSLatitudeRef",             0x01},
    {"GPSLatitude",                0x02},
    {"GPSLongitudeRef",            0x03},
    {"GPSLongitude",               0x04},
    {"GPSAltitudeRef",             0x05},
    {"GPSAltitude",                0x06},
    {"GPSTimeStamp",               0x07},
    {"GPSSatellites",              0x08},
    {"GPSStatus",                  0x09},
    {"GPSMeasureMode",             0x0A},
    {"GPSDOP",                     0x0B},
    {"GPSSpeedRef",                0x0C},
    {"GPSSpeed",                   0x0D},
    {"GPSTrackRef",                0x0E},
    {"GPSTrack",                   0x0F},
    {"GPSImgDirectionRef",         0x10},
    {"GPSImgDirection",            0x11},
    {"GPSMapDatum",                0x12},
    {"GPSDestLatitudeRef",         0x13},
    {"GPSDestLatitude",            0x14},
    {"GPSDestLongitudeRef",        0x15},
    {"GPSDestLongitude",           0x16},
    {"GPSDestBearingRef",          0x17},
    {"GPSDestBearing",             0x18},
    {"GPSDestDistanceRef",         0x19},
    {"GPSDestDistance",            0x1A},
    {"GPSProcessingMethod",        0x1B},
    {"GPSAreaInformation",         0x1C},
    {"GPSDateStamp",               0x1D},
    {"GPSDifferential",            0x1E},
    {"ImageWidth",                 0x100}, // <- Table 3 TIFF Rev. 6.0 Attribute Information Used in Exif
    {"ImageLength",                0x101},
    {"BitsPerSample",              0x102},
    {"Compression",                0x103},
    {"PhotometricInterpretation",  0x106},
    {"Orientation",                0x112},
    {"SamplesPerPixel",            0x115},
    {"PlanarConfiguration",        0x11C},
    {"YCbCrSubSampling",           0x212},
    {"YCbCrPositioning",           0x213},
    {"XResolution",                0x11A},
    {"YResolution",                0x11B},
    {"ResolutionUnit",             0x128},
    {"StripOffsets",               0x111},
    {"RowsPerStrip",               0x116},
    {"StripByteCounts",            0x117},
    {"JPEGInterchangeFormat",      0x201},
    {"JPEGInterchangeFormatLength",0x202},
    {"TransferFunction",           0x12D},
    {"WhitePoint",                 0x13E},
    {"PrimaryChromaticities",      0x13F},
    {"YCbCrCoefficients",          0x211},
    {"ReferenceBlackWhite",        0x214},
    {"DateTime",                   0x132},
    {"ImageDescription",           0x10E},
    {"Make",                       0x10F},
    {"Model",                      0x110},
    {"Software",                   0x131},
    {"Artist",                     0x13B},
    {"Copyright",                  0x8298},
    {"ExifVersion",                0x9000}, // <- Table 4 Exif IFD Attribute Information (1)
    {"FlashpixVersion",            0xA000},
    {"ColorSpace",                 0xA001},
    {"ComponentsConfiguration",    0x9101},
    {"CompressedBitsPerPixel",     0x9102},
    {"PixelXDimension",            0xA002},
    {"PixelYDimension",            0xA003},
    {"MakerNote",                  0x927C},
    {"UserComment",                0x9286},
    {"RelatedSoundFile",           0xA004},
    {"DateTimeOriginal",           0x9003},
    {"DateTimeDigitized",          0x9004},
    {"SubSecTime",                 0x9290},
    {"SubSecTimeOriginal",         0x9291},
    {"SubSecTimeDigitized",        0x9292},
    {"ImageUniqueID",              0xA420},
    {"ExposureTime",               0x829A}, // <- Table 5 Exif IFD Attribute Information (2)
    {"FNumber",                    0x829D},
    {"ExposureProgram",            0x8822},
    {"SpectralSensitivity",        0x8824},
    {"ISOSpeedRatings",            0x8827},
    {"OECF",                       0x8828},
    {"ShutterSpeedValue",          0x9201},
    {"ApertureValue",              0x9202},
    {"BrightnessValue",            0x9203},
    {"ExposureBiasValue",          0x9204},
    {"MaxApertureValue",           0x9205},
    {"SubjectDistance",            0x9206},
    {"MeteringMode",               0x9207},
    {"LightSource",                0x9208},
    {"Flash",                      0x9209},
    {"FocalLength",                0x920A},
    {"SubjectArea",                0x9214},
    {"FlashEnergy",                0xA20B},
    {"SpatialFrequencyResponse",   0xA20C},
    {"FocalPlaneXResolution",      0xA20E},
    {"FocalPlaneYResolution",      0xA20F},
    {"FocalPlaneResolutionUnit",   0xA210},
    {"SubjectLocation",            0xA214},
    {"ExposureIndex",              0xA215},
    {"SensingMethod",              0xA217},
    {"FileSource",                 0xA300},
    {"SceneType",                  0xA301},
    {"CFAPattern",                 0xA302},
    {"CustomRendered",             0xA401},
    {"ExposureMode",               0xA402},
    {"WhiteBalance",               0xA403},
    {"DigitalZoomRatio",           0xA404},
    {"FocalLengthIn35mmFilm",      0xA405},
    {"SceneCaptureType",           0xA406},
    {"GainControl",                0xA407},
    {"Contrast",                   0xA408},
    {"Saturation",                 0xA409},
    {"Sharpness",                  0xA40A},
    {"DeviceSettingDescription",   0xA40B},
    {"SubjectDistanceRange",       0xA40C},
    {"MPFormatVersionNumber",      0xB000}, // <- MPF
    {"MPNumberOfImages",           0xB001},
    {"MPEntry",                    0xB002},
    {"MPImageUIDList",             0xB003},
    {"MPTotalFrames",              0xB004}
//    {"InteroperabilityIndex",      0x1}, // <- Table 13 Interoperability IFD Attribute Information
};

static int mpf_add_mpentry_metadata(int count, const char *name, const char *sep,
                                    GetByteContext *gb, int le, AVDictionary **metadata)
{
    int i;
    unsigned value;
    unsigned flag;
    const char *strflag;
    int nb_pictures = count / 16;

    if (!count)
        return 0;

    for (i = 0; i < nb_pictures; i++) {
        char buf[64];
        value = le ? bytestream2_get_le32(gb) : bytestream2_get_be32(gb);

        /* MPDependantParentImageFlag */
        flag = (value >> 31) & 0x1;
        snprintf(buf, sizeof(buf), "MPDependantParentImageFlag-%d", i);
        av_dict_set(metadata, buf, flag ? "1" : "0", 0);

        /* MPDependantParentImageFlag */
        flag = (value >> 30) & 0x1;
        snprintf(buf, sizeof(buf), "MPDependantChildImageFlag-%d", i);
        av_dict_set(metadata, buf, flag ? "1" : "0", 0);

        /* MPRepresentativeImageFlag */
        flag = (value >> 29) & 0x1;
        snprintf(buf, sizeof(buf), "MPRepresentativeImageFlag-%d", i);
        av_dict_set(metadata, buf, flag ? "1" : "0", 0);

        /* MPImageDataFormat */
        flag = (value >> 24) & 0x7;
        snprintf(buf, sizeof(buf), "MPImageDataFormat-%d", i);
        av_dict_set(metadata, buf, flag ? "other" : "jpeg", 0);

        /* MPTypeCode  */
        flag = value & ((1 << 20) - 1);
        switch (flag) {
            case 0x30000:
                strflag = "primary-image";
                break;
            case 0x10001:
                strflag = "large-thumbnail-vga";
                break;
            case 0x10002:
                strflag = "large-thumbnail-1080p";
                break;
            case 0x20001:
                strflag = "panorama";
                break;
            case 0x20002:
                strflag = "disparity";
                break;
            case 0x20003:
                strflag = "multi-angle";
                break;
            case 0x00000:
                strflag = "undefined";
                break;
            default:
                av_log(0, 0, "%x\n", flag);
                strflag = "unknown";
        }
        snprintf(buf, sizeof(buf), "MPTypeCode-%d", i);
        av_dict_set(metadata, buf, strflag, 0);

        /* MPIndividualImageSize */
        value = le ? bytestream2_get_le32(gb) : bytestream2_get_be32(gb);
        snprintf(buf, sizeof(buf), "MPIndividualImageSize-%d", i);
        av_dict_set_int(metadata, buf, value, 0);

        /* MPIndividualImageDataOffset */
        value = le ? bytestream2_get_le32(gb) : bytestream2_get_be32(gb);
        snprintf(buf, sizeof(buf), "MPIndividualImageDataOffset-%d", i);
        av_dict_set_int(metadata, buf, value, 0);

        /* MPDependentImage1EntryNumber */
        value = le ? bytestream2_get_le16(gb) : bytestream2_get_be16(gb);
        snprintf(buf, sizeof(buf), "MPDependentImage1EntryNumber-%d", i);
        av_dict_set_int(metadata, buf, value, 0);
        /* MPDependentImage2EntryNumber */
        value = le ? bytestream2_get_le16(gb) : bytestream2_get_be16(gb);
        snprintf(buf, sizeof(buf), "MPDependentImage2EntryNumber-%d", i);
        av_dict_set_int(metadata, buf, value, 0);
    }

    return 0;
}


static const char *exif_get_tag_name(uint16_t id)
{
    int i;

    for (i = 0; i < FF_ARRAY_ELEMS(tag_list); i++) {
        if (tag_list[i].id == id)
            return tag_list[i].name;
    }

    return NULL;
}


static int exif_add_metadata(void *logctx, int count, int type,
                             const char *name, const char *sep,
                             GetByteContext *gb, int le,
                             AVDictionary **metadata)
{
    if (type == TIFF_UNDEFINED && !av_strcasecmp(name, "MPEntry")) {
        return mpf_add_mpentry_metadata(count, name, sep, gb, le, metadata);
    }

    switch(type) {
    case 0:
        av_log(logctx, AV_LOG_WARNING,
               "Invalid TIFF tag type 0 found for %s with size %d\n",
               name, count);
        return 0;
    case TIFF_DOUBLE   : return ff_tadd_doubles_metadata(count, name, sep, gb, le, metadata);
    case TIFF_SSHORT   : return ff_tadd_shorts_metadata(count, name, sep, gb, le, 1, metadata);
    case TIFF_SHORT    : return ff_tadd_shorts_metadata(count, name, sep, gb, le, 0, metadata);
    case TIFF_SBYTE    : return ff_tadd_bytes_metadata(count, name, sep, gb, le, 1, metadata);
    case TIFF_BYTE     :
    case TIFF_UNDEFINED: return ff_tadd_bytes_metadata(count, name, sep, gb, le, 0, metadata);
    case TIFF_STRING   : return ff_tadd_string_metadata(count, name, gb, le, metadata);
    case TIFF_SRATIONAL:
    case TIFF_RATIONAL : return ff_tadd_rational_metadata(count, name, sep, gb, le, metadata);
    case TIFF_SLONG    :
    case TIFF_LONG     : return ff_tadd_long_metadata(count, name, sep, gb, le, metadata);
    default:
        avpriv_request_sample(logctx, "TIFF tag type (%u)", type);
        return 0;
    };
}


static int exif_decode_tag(void *logctx, GetByteContext *gbytes, int le,
                           int depth, AVDictionary **metadata)
{
    int ret, cur_pos;
    unsigned id, count;
    enum TiffTypes type;

    if (depth > 2) {
        return 0;
    }

    ff_tread_tag(gbytes, le, &id, &type, &count, &cur_pos);

    if (!bytestream2_tell(gbytes)) {
        bytestream2_seek(gbytes, cur_pos, SEEK_SET);
        return 0;
    }

    // read count values and add it metadata
    // store metadata or proceed with next IFD
    ret = ff_tis_ifd(id);
    if (ret) {
        ret = ff_exif_decode_ifd(logctx, gbytes, le, depth + 1, metadata);
    } else {
        const char *name = exif_get_tag_name(id);
        char buf[7];

        if (!name) {
            name = buf;
            snprintf(buf, sizeof(buf), "0x%04X", id);
        }

        ret = exif_add_metadata(logctx, count, type, name, NULL,
                                gbytes, le, metadata);
    }

    bytestream2_seek(gbytes, cur_pos, SEEK_SET);

    return ret;
}


int ff_exif_decode_ifd(void *logctx, GetByteContext *gbytes,
                       int le, int depth, AVDictionary **metadata)
{
    int i, ret;
    int entries;

    entries = ff_tget_short(gbytes, le);

    if (bytestream2_get_bytes_left(gbytes) < entries * 12) {
        return AVERROR_INVALIDDATA;
    }

    for (i = 0; i < entries; i++) {
        if ((ret = exif_decode_tag(logctx, gbytes, le, depth, metadata)) < 0) {
            return ret;
        }
    }

    // return next IDF offset or 0x000000000 or a value < 0 for failure
    return ff_tget_long(gbytes, le);
}

int avpriv_exif_decode_ifd(void *logctx, const uint8_t *buf, int size,
                           int le, int depth, AVDictionary **metadata)
{
    GetByteContext gb;

    bytestream2_init(&gb, buf, size);

    return ff_exif_decode_ifd(logctx, &gb, le, depth, metadata);
}

int av_exif_parse(const uint8_t *data, int size, AVDictionary **metadata, void *log_ctx)
{
    int ret, le, ifd_offset;
    GetByteContext gbytes;

    bytestream2_init(&gbytes, data, size);

    ret = ff_tdecode_header(&gbytes, &le, &ifd_offset);
    if (ret < 0) {
        av_log(log_ctx, AV_LOG_ERROR, "Invalid TIFF header in Exif data\n");
        return ret;
    }

    bytestream2_seek(&gbytes, ifd_offset, SEEK_SET);

    // read 0th IFD and store the metadata
    // (return values > 0 indicate the presence of subimage metadata)
    ret = ff_exif_decode_ifd(log_ctx, &gbytes, le, 0, metadata);
    if (ret < 0) {
        av_log(log_ctx, AV_LOG_ERROR, "Error decoding Exif data\n");
        return ret;
    }

    return bytestream2_tell(&gbytes);
}

int av_exif_parse2(const uint8_t *data, int size, AVDictionary **metadata, AVDictionary **thumb_metadata, void *log_ctx)
{
    int ret, le, ifd_offset;
    GetByteContext gbytes;

    bytestream2_init(&gbytes, data, size);

    ret = ff_tdecode_header(&gbytes, &le, &ifd_offset);
    if (ret < 0) {
        av_log(log_ctx, AV_LOG_ERROR, "Invalid TIFF header in Exif data\n");
        return ret;
    }

    bytestream2_seek(&gbytes, ifd_offset, SEEK_SET);

    // read 0th IFD and store the metadata
    // (return values > 0 indicate the presence of subimage metadata)
    ret = ff_exif_decode_ifd(log_ctx, &gbytes, le, 0, metadata);
    if (ret < 0) {
        av_log(log_ctx, AV_LOG_ERROR, "Error decoding Exif data\n");
        return ret;
    }

    if (ret > 0) {
        bytestream2_seek(&gbytes, ret, SEEK_SET);

        // read 1th IFD and store the metadata
        // (return values > 0 indicate the presence of subimage metadata)
        ret = ff_exif_decode_ifd(log_ctx, &gbytes, le, 0, thumb_metadata);
        if (ret < 0) {
            av_log(log_ctx, AV_LOG_ERROR, "Error decoding Exif data\n");
            return ret;
        }
    }

    return bytestream2_tell(&gbytes);
}
