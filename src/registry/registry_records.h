#pragma once

#include <stdint.h>

#include "../core/types/payload_types.h"
#include "../core/types/status_types.h"

namespace Cyber32 {

struct ModuleRecord {
    const char* module_id;
    const char* module_type;
    RecordStatus status;
    uint8_t device_count;
    uint8_t capability_count;
};

struct DeviceRecord {
    const char* device_id;
    const char* device_type;
    RecordStatus status;
    uint8_t module_index;
    uint8_t capability_count;
};

struct CapabilityPayload {
    const char* capability_id;
    uint8_t schema_version;
    uint32_t timestamp_ms;
    Availability available;
    StaleState stale;
    PayloadValueType value_type;
    float value_float;
    int32_t value_int;
    const char* unit;
    const char* quality;
    const char* error_code;
};

struct CapabilityRecord {
    const char* capability_id;
    const char* category;
    const char* kind;
    PayloadValueType data_type;
    const char* access;
    RecordStatus status;
    uint8_t owner_device_index;
    CapabilityPayload latest_payload;
};

}  // namespace Cyber32
