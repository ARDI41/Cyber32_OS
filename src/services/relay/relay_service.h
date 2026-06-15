#pragma once

#include <stdint.h>

#include "../../core/types/command_types.h"
#include "../../devices/actuators/sim_relay_device.h"
#include "../../registry/registry.h"
#include "../../runtime/runtime.h"

namespace Cyber32 {

struct RelayCommandRequest {
    bool enabled;
    uint32_t timeout_ms;
};

struct RelayCommandResult {
    CommandState state;
    RegistryResult registry_result;
    bool accepted;
    bool executed;
    const char* error_code;
};

struct RelayPendingCommand {
    bool occupied;
    bool enabled;
    uint32_t requested_at_ms;
    uint32_t timeout_ms;
    CommandState state;
    const char* error_code;
};

class RelayService {
public:
    RelayService();

    bool begin(Registry* registry, SimRelayDevice* device);
    void attachRuntime(Runtime* runtime);
    const char* id() const;
    bool updateState(uint32_t now_ms);
    bool setEnabled(uint32_t now_ms, const RelayCommandRequest& request, RelayCommandResult& out_result);
    bool disable(uint32_t now_ms, RelayCommandResult& out_result);
    bool executePendingCommand(uint32_t now_ms);
    bool hasPendingCommand() const;
    void clearPendingCommand();
    RegistryResult lastRegistryResult() const;
    CommandState lastCommandState() const;

private:
    Registry* registry_;
    SimRelayDevice* device_;
    Runtime* runtime_;
    RegistryResult last_registry_result_;
    CommandState last_command_state_;
    RelayPendingCommand pending_command_;

    bool isValidCommandTimeout(uint32_t timeout_ms) const;
    bool isTimedOut(uint32_t now_ms, uint32_t requested_at_ms, uint32_t timeout_ms) const;
    bool runtimeAllowsRelayCommand(bool enabled) const;
    bool pendingCommandAllowedInCurrentRuntime() const;
    const char* runtimeCommandError(bool enabled) const;
    void fillFailedResult(
        uint32_t now_ms,
        bool enabled,
        RelayCommandResult& out_result,
        const char* error_code);
    void writeCommandState(
        uint32_t now_ms,
        CommandState command_state,
        RegistryResult registry_result,
        const char* error_code,
        bool enabled);
};

}  // namespace Cyber32
