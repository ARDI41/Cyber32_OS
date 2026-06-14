#pragma once

#include <stdint.h>

#include "../../core/types/command_types.h"
#include "../../devices/actuators/sim_servo_device.h"
#include "../../registry/registry.h"
#include "../../runtime/runtime.h"

namespace Cyber32 {

struct ServoCommandRequest {
    float position_degrees;
    uint32_t timeout_ms;
};

struct ServoCommandResult {
    CommandState state;
    RegistryResult registry_result;
    bool accepted;
    bool executed;
    const char* error_code;
};

class ServoService {
public:
    ServoService();

    bool begin(Registry* registry, SimServoDevice* device);
    void attachRuntime(Runtime* runtime);
    const char* id() const;
    bool updateState(uint32_t now_ms);
    bool setPosition(uint32_t now_ms, const ServoCommandRequest& request, ServoCommandResult& out_result);
    RegistryResult lastRegistryResult() const;
    CommandState lastCommandState() const;

private:
    Registry* registry_;
    SimServoDevice* device_;
    Runtime* runtime_;
    RegistryResult last_registry_result_;
    CommandState last_command_state_;

    bool isValidPosition(float position_degrees) const;
    bool runtimeAllowsActuatorCommands() const;
    void fillFailedResult(
        uint32_t now_ms,
        float position_degrees,
        ServoCommandResult& out_result,
        const char* error_code);
    void writeCommandState(
        uint32_t now_ms,
        CommandState command_state,
        RegistryResult registry_result,
        const char* error_code,
        float value_float);
};

}  // namespace Cyber32
