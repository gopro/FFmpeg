#pragma once

#include <tracy/tracy/TracyC.h>

#define TRACY_DISP_ERROR_TEXT_INV(d, c, b, a) -(int)((a) | ((b) << 8) | ((c) << 16) | ((unsigned)(d) << 24))
#define TRACY_DISP_ERROR_TEXT(ret) TRACY_DISP_ERROR_TEXT_INV(ret&0xFF, (ret>>8)&0xFF, (ret>>16)&0xFF, (ret>>24)&0xFF)

#define TRACY_ZONE_START_CTX(tracy_ctx, name) TracyCZoneN(tracy_ctx, name, 1);
#define TRACY_ZONE_END_CTX(tracy_ctx) {TracyCZoneEnd(tracy_ctx);}

#define TRACY_ZONE_START(name) TRACY_ZONE_START_CTX(tracy_ctx, name);
#define TRACY_ZONE_END TRACY_ZONE_END_CTX(tracy_ctx);
#define TRACY_MESSAGE(msg) {TracyCMessage(msg, strlen(msg));}
#define TRACY_ZONE_END_ERROR(msg) {TRACY_MESSAGE(msg);TRACY_ZONE_END(tracy_ctx);}
#define TRACY_ZONE_END_ERROR_CODE(msg, ret) {TRACY_MESSAGE(msg); if(ret != 0){TracyCZoneValue(tracy_ctx, ret);} TRACY_ZONE_END(tracy_ctx);}
#define TRACY_ZONE_END_ERROR_CODE_TEXT(msg, ret) {if(ret != 0){int code = TRACY_DISP_ERROR_TEXT(ret); TracyCZoneText(tracy_ctx, &code, 4);} TRACY_ZONE_END_ERROR_CODE(msg, ret);}
