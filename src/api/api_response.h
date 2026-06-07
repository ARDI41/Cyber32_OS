#pragma once

#include <stdint.h>

#include "../core/types/runtime_state.h"
#include "../registry/registry_records.h"

namespace Cyber32 {

struct ApiSystemStatus {
    bool ok;
    RuntimeState runtime_state;
    uint8_t module_count;
    uint8_t device_count;
    uint8_t capability_count;
    const char* latest_error_code;
};

struct ApiCapabilityState {
    bool ok;
    CapabilityPayload payload;
    const char* error_code;
};

}  // namespace Cyber32
