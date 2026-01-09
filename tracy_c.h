#pragma once

#include <tracy/tracy/TracyC.h>

#define TRACY_ZONE_START(name) TracyCZoneN(tracy_ctx, name, 1);
#define TRACY_ZONE_END {TracyCZoneEnd(tracy_ctx);}
#define TRACY_ZONE_END_ERROR(name) {TracyCMessage(name, strlen(name));TracyCZoneEnd(tracy_ctx);}
