#pragma once

#include <tracy/tracy/TracyC.h>

#define TRACY_ZONE_START(name) TracyCZoneN(tracy_ctx, name, 1);
#define TRACY_ZONE_END {TracyCZoneEnd(tracy_ctx);}
#define TRACY_MESSAGE(msg) {TracyCMessage(msg, strlen(msg));}
#define TRACY_ZONE_END_ERROR(msg) {TRACY_MESSAGE(msg);TracyCZoneEnd(tracy_ctx);}
#define TRACY_ZONE_END_ERROR_CODE(msg, code) {TRACY_MESSAGE(msg);TracyCZoneValue(tracy_ctx, code);TracyCZoneEnd(tracy_ctx);}
