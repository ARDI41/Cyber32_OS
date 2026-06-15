#include "relay_service.h"

#include "../../core/ids/capability_ids.h"
#include "../../core/ids/error_ids.h"
#include "../../core/ids/service_ids.h"

namespace Cyber32 {

RelayService::RelayService()
    : registry_(0),
      device_(0),
      runtime_(0),
      last_registry_result_(RegistryResult::NOT_ATTACHED),
      last_command_state_(CommandState::REQUEST),
      pending_command_() {
    clearPendingCommand();
}

bool RelayService::begin(Registry* registry, SimRelayDevice* device) {
    if (registry == 0 || device == 0) {
        return false;
    }

    registry_ = registry;
    device_ = device;
    last_registry_result_ = RegistryResult::OK;
    last_command_state_ = CommandState::REQUEST;
    clearPendingCommand();
    return true;
}

void RelayService::attachRuntime(Runtime* runtime) {
    runtime_ = runtime;
}

const char* RelayService::id() const {
    return SERVICE_RELAY;
}

bool RelayService::updateState(uint32_t now_ms) {
    if (registry_ == 0 || device_ == 0) {
        last_registry_result_ = RegistryResult::NOT_ATTACHED;
        return false;
    }

    CapabilityPayload payload;
    const bool read_success = device_->readPayload(now_ms, payload);
    last_registry_result_ = registry_->updateCapabilityPayloadWithResult(CAP_RELAY_CONTROL, payload);

    return read_success && last_registry_result_ == RegistryResult::OK;
}

bool RelayService::setEnabled(
    uint32_t now_ms,
    const RelayCommandRequest& request,
    RelayCommandResult& out_result) {
    out_result.registry_result = RegistryResult::OK;

    if (registry_ == 0 || device_ == 0) {
        last_registry_result_ = RegistryResult::NOT_ATTACHED;
        fillFailedResult(now_ms, request.enabled, out_result, ERR_ACTUATOR_UNAVAILABLE);
        return false;
    }

    if (!runtimeAllowsRelayCommand(request.enabled)) {
        last_registry_result_ = RegistryResult::OK;
        fillFailedResult(now_ms, request.enabled, out_result, runtimeCommandError(request.enabled));
        return false;
    }

    if (!isValidCommandTimeout(request.timeout_ms)) {
        last_registry_result_ = RegistryResult::INVALID_RECORD;
        fillFailedResult(now_ms, request.enabled, out_result, ERR_COMMAND_INVALID_TIMEOUT);
        return false;
    }

    if (hasPendingCommand() && request.enabled) {
        last_registry_result_ = RegistryResult::UNAVAILABLE;
        fillFailedResult(now_ms, request.enabled, out_result, ERR_PENDING_COMMAND_EXISTS);
        return false;
    }

    pending_command_.occupied = true;
    pending_command_.enabled = request.enabled;
    pending_command_.requested_at_ms = now_ms;
    pending_command_.timeout_ms = request.timeout_ms;
    pending_command_.state = CommandState::ACCEPTED;
    pending_command_.error_code = "none";

    out_result.accepted = true;
    out_result.executed = false;
    out_result.state = CommandState::ACCEPTED;
    out_result.error_code = "none";
    out_result.registry_result = RegistryResult::OK;
    last_registry_result_ = RegistryResult::OK;
    last_command_state_ = CommandState::ACCEPTED;
    writeCommandState(
        now_ms,
        CommandState::ACCEPTED,
        last_registry_result_,
        "none",
        request.enabled);
    return true;
}

bool RelayService::disable(uint32_t now_ms, RelayCommandResult& out_result) {
    RelayCommandRequest request;
    request.enabled = false;
    request.timeout_ms = 1000;
    return setEnabled(now_ms, request, out_result);
}

bool RelayService::executePendingCommand(uint32_t now_ms) {
    if (!hasPendingCommand()) {
        return false;
    }

    const bool enabled = pending_command_.enabled;
    const uint32_t requested_at_ms = pending_command_.requested_at_ms;
    const uint32_t timeout_ms = pending_command_.timeout_ms;

    if (!pendingCommandAllowedInCurrentRuntime()) {
        last_registry_result_ = RegistryResult::OK;
        last_command_state_ = CommandState::FAILED;
        writeCommandState(
            now_ms,
            CommandState::FAILED,
            last_registry_result_,
            runtimeCommandError(enabled),
            enabled);
        clearPendingCommand();
        return false;
    }

    if (isTimedOut(now_ms, requested_at_ms, timeout_ms)) {
        last_registry_result_ = RegistryResult::OK;
        last_command_state_ = CommandState::TIMED_OUT;
        writeCommandState(
            now_ms,
            CommandState::TIMED_OUT,
            last_registry_result_,
            ERR_COMMAND_TIMEOUT,
            enabled);
        clearPendingCommand();
        return false;
    }

    CapabilityPayload payload;
    const bool command_success = device_ != 0 &&
        (enabled
            ? device_->setEnabled(now_ms, true, payload)
            : device_->disable(now_ms, payload));
    last_registry_result_ = registry_ == 0
        ? RegistryResult::NOT_ATTACHED
        : registry_->updateCapabilityPayloadWithResult(CAP_RELAY_CONTROL, payload);

    if (command_success && last_registry_result_ == RegistryResult::OK) {
        last_command_state_ = CommandState::COMPLETED;
        writeCommandState(
            now_ms,
            CommandState::COMPLETED,
            last_registry_result_,
            "none",
            enabled);
        clearPendingCommand();
        return true;
    }

    last_command_state_ = CommandState::FAILED;
    writeCommandState(
        now_ms,
        CommandState::FAILED,
        last_registry_result_,
        command_success
            ? ERR_ACTUATOR_UNAVAILABLE
            : (device_ == 0 ? ERR_ACTUATOR_UNAVAILABLE : ERR_ACTUATOR_EXECUTION_FAILED),
        enabled);
    clearPendingCommand();
    return false;
}

bool RelayService::hasPendingCommand() const {
    return pending_command_.occupied;
}

void RelayService::clearPendingCommand() {
    pending_command_.occupied = false;
    pending_command_.enabled = false;
    pending_command_.requested_at_ms = 0;
    pending_command_.timeout_ms = 0;
    pending_command_.state = CommandState::REQUEST;
    pending_command_.error_code = "none";
}

RegistryResult RelayService::lastRegistryResult() const {
    return last_registry_result_;
}

CommandState RelayService::lastCommandState() const {
    return last_command_state_;
}

bool RelayService::isValidCommandTimeout(uint32_t timeout_ms) const {
    return timeout_ms >= 1 && timeout_ms <= 10000;
}

bool RelayService::isTimedOut(
    uint32_t now_ms,
    uint32_t requested_at_ms,
    uint32_t timeout_ms) const {
    return (now_ms - requested_at_ms) >= timeout_ms;
}

bool RelayService::runtimeAllowsRelayCommand(bool enabled) const {
    if (runtime_ == 0) {
        return false;
    }

    const RuntimeState state = runtime_->state();
    switch (state) {
        case RuntimeState::READY:
        case RuntimeState::RUNNING:
            return true;
        case RuntimeState::SAFE_MODE:
            return !enabled;
        case RuntimeState::BOOTING:
        case RuntimeState::INITIALIZING:
        case RuntimeState::DISCOVERING:
        case RuntimeState::REGISTERING:
        case RuntimeState::STARTING:
        case RuntimeState::ERROR_STATE:
        default:
            return false;
    }
}

bool RelayService::pendingCommandAllowedInCurrentRuntime() const {
    if (!hasPendingCommand()) {
        return false;
    }

    return runtimeAllowsRelayCommand(pending_command_.enabled);
}

const char* RelayService::runtimeCommandError(bool enabled) const {
    if (runtime_ == 0) {
        return ERR_RUNTIME_NOT_READY;
    }

    if (runtime_->state() == RuntimeState::SAFE_MODE && enabled) {
        return ERR_SAFE_MODE_BLOCKED;
    }

    return ERR_RUNTIME_NOT_READY;
}

void RelayService::fillFailedResult(
    uint32_t now_ms,
    bool enabled,
    RelayCommandResult& out_result,
    const char* error_code) {
    out_result.state = CommandState::FAILED;
    out_result.registry_result = last_registry_result_;
    out_result.accepted = false;
    out_result.executed = false;
    out_result.error_code = error_code == 0 ? ERR_COMMAND_INVALID : error_code;
    last_command_state_ = CommandState::FAILED;
    writeCommandState(
        now_ms,
        CommandState::FAILED,
        last_registry_result_,
        out_result.error_code,
        enabled);
}

void RelayService::writeCommandState(
    uint32_t now_ms,
    CommandState command_state,
    RegistryResult registry_result,
    const char* error_code,
    bool enabled) {
    if (registry_ == 0) {
        return;
    }

    CommandStateRecord record;
    record.capability_id = CAP_RELAY_CONTROL;
    record.command_state = command_state;
    record.timestamp_ms = now_ms;
    record.registry_result = registry_result;
    record.error_code = error_code;
    record.value_float = 0.0F;
    record.value_int = enabled ? 1 : 0;

    registry_->updateCommandState(record);
}

}  // namespace Cyber32
