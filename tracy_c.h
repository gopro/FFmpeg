#pragma once

#include <tracy/tracy/TracyC.h>

#define TRACY_ZONE_START_CTX(tracy_ctx, name) TracyCZoneN(tracy_ctx, name, 1);
#define TRACY_ZONE_END_CTX(tracy_ctx) {TracyCZoneEnd(tracy_ctx);}

#define TRACY_ZONE_START(name) TRACY_ZONE_START_CTX(tracy_ctx, name);
#define TRACY_ZONE_END TRACY_ZONE_END_CTX(tracy_ctx);
#define TRACY_MESSAGE(msg) {TracyCMessage(msg, strlen(msg));}
#define TRACY_ZONE_END_ERROR(msg) {TRACY_MESSAGE(msg);TRACY_ZONE_END(tracy_ctx);}
#define TRACY_ZONE_END_ERROR_CODE(msg, code) {TRACY_MESSAGE(msg);if(code != 0) {TracyCZoneValue(tracy_ctx, code);} else {TRACY_ZONE_END;} TracyCZoneEnd(tracy_ctx);}
#define TRACY_ZONE_END_ERROR_CODE_TEXT(msg, code) {TRACY_MESSAGE(msg);if(code != 0) {TracyCZoneText(tracy_ctx, &code, 4);} else {TRACY_ZONE_END;} TracyCZoneEnd(tracy_ctx);}
