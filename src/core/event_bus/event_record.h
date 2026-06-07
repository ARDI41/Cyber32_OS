#pragma once

#include <stdint.h>

#include "../types/event_types.h"

namespace Cyber32 {

struct EventRecord {
    const char* event_id;
    EventPriority priority;
    uint32_t timestamp_ms;
    const char* source_layer;
    const char* source_id;
    const char* target_id;
    uint8_t value_type;
    float value_float;
    int32_t value_int;
};

}  // namespace Cyber32
