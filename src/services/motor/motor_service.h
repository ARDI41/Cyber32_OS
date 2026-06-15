#pragma once

#include <stdint.h>

#include "../../core/types/command_types.h"
#include "../../core/types/motor_types.h"
#include "../../devices/actuators/sim_motor_device.h"
#include "../../registry/registry.h"
#include "../../runtime/runtime.h"

namespace Cyber32 {

struct MotorCommandRequest {
    MotorDirection direction;
    float speed_percent;
    uint32_t timeout_ms;
};

struct MotorCommandResult {
    CommandState state;
    RegistryResult registry_result;
    bool accepted;
    bool executed;
    const char* error_code;
};

struct MotorPendingCommand {
    bool occupied;
    MotorDirection direction;
    float speed_percent;
    uint32_t requested_at_ms;
    uint32_t timeout_ms;
    CommandState state;
    const char* error_code;
};

class MotorService {
public:
    MotorService();

    bool begin(Registry* registry, SimMotorDevice* device);
    void attachRuntime(Runtime* runtime);
    const char* id() const;
    bool updateState(uint32_t now_ms);
    bool setMotor(uint32_t now_ms, const MotorCommandRequest& request, MotorCommandResult& out_result);
    bool executePendingCommand(uint32_t now_ms);
    bool failPendingCommand(uint32_t now_ms, const char* error_code);
    bool stop(uint32_t now_ms, MotorCommandResult& out_result);
    RegistryResult lastRegistryResult() const;
    CommandState lastCommandState() const;
    bool hasPendingCommand() const;
    void clearPendingCommand();

private:
    Registry* registry_;
    SimMotorDevice* device_;
    Runtime* runtime_;
    RegistryResult last_registry_result_;
    CommandState last_command_state_;
    MotorPendingCommand pending_command_;

    bool isValidMotorRequest(MotorDirection direction, float speed_percent) const;
    bool isValidCommandTimeout(uint32_t timeout_ms) const;
    bool isTimedOut(uint32_t now_ms, uint32_t requested_at_ms, uint32_t timeout_ms) const;
    bool isStopCommand(MotorDirection direction, float speed_percent) const;
    bool runtimeAllowsMotorCommand(MotorDirection direction, float speed_percent) const;
    bool pendingCommandAllowedInCurrentRuntime() const;
    const char* motorRequestValidationError(MotorDirection direction, float speed_percent) const;
    const char* runtimeCommandError(MotorDirection direction, float speed_percent) const;
    void fillFailedResult(
        uint32_t now_ms,
        MotorDirection direction,
        float speed_percent,
        MotorCommandResult& out_result,
        const char* error_code);
    void writeCommandState(
        uint32_t now_ms,
        CommandState command_state,
        RegistryResult registry_result,
        const char* error_code,
        MotorDirection direction,
        float speed_percent);
};

}  // namespace Cyber32
