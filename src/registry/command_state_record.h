#pragma once

#include <stdint.h>

#include "../core/types/command_types.h"
#include "registry_result.h"

namespace Cyber32 {

struct CommandStateRecord {
    const char* capability_id;
    CommandState command_state;
    uint32_t timestamp_ms;
    RegistryResult registry_result;
    const char* error_code;
    float value_float;
    int32_t value_int;
};

}  // namespace Cyber32
