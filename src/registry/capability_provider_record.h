#pragma once

#include <stdint.h>

#include "registry_records.h"

namespace Cyber32 {

enum class CapabilityProviderType : unsigned char {
    UNKNOWN,
    WIRED,
    SIMULATED,
    WIRELESS
};

enum class CapabilityProviderStatus : unsigned char {
    UNKNOWN,
    AVAILABLE,
    STALE,
    UNAVAILABLE,
    LOST,
    DISABLED
};

struct CapabilityProviderRecord {
    const char* provider_id;
    const char* capability_id;
    uint8_t owner_module_index;
    uint8_t owner_device_index;
    CapabilityProviderType provider_type;
    CapabilityProviderStatus status;
    uint8_t priority;
    uint32_t last_update_ms;
    CapabilityPayload latest_payload;
};

}  // namespace Cyber32
