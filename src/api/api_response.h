#pragma once

#include <stdint.h>

#include "../core/types/command_types.h"
#include "../core/types/motor_types.h"
#include "../core/types/runtime_state.h"
#include "../registry/registry_records.h"
#include "../registry/registry_result.h"

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

struct ApiServoCommandRequest {
    float position_degrees;
    uint32_t timeout_ms;
};

struct ApiServoCommandResponse {
    bool ok;
    CommandState command_state;
    bool accepted;
    bool executed;
    const char* error_code;
};

struct ApiMotorCommandRequest {
    MotorDirection direction;
    float speed_percent;
    uint32_t timeout_ms;
};

struct ApiMotorCommandResponse {
    bool ok;
    CommandState command_state;
    bool accepted;
    bool executed;
    const char* error_code;
};

struct ApiRelayCommandRequest {
    bool enabled;
    uint32_t timeout_ms;
};

struct ApiRelayCommandResponse {
    bool ok;
    CommandState command_state;
    bool accepted;
    bool executed;
    const char* error_code;
};

struct ApiCommandStateResponse {
    bool ok;
    CommandState command_state;
    RegistryResult registry_result;
    const char* capability_id;
    const char* error_code;
    float value_float;
    int32_t value_int;
    uint32_t timestamp_ms;
};

}  // namespace Cyber32
