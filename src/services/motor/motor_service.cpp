#include "motor_service.h"

#include "../../core/ids/capability_ids.h"
#include "../../core/ids/error_ids.h"
#include "../../core/ids/service_ids.h"

namespace Cyber32 {

MotorService::MotorService()
    : registry_(0),
      device_(0),
      runtime_(0),
      last_registry_result_(RegistryResult::NOT_ATTACHED),
      last_command_state_(CommandState::REQUEST) {
}

bool MotorService::begin(Registry* registry, SimMotorDevice* device) {
    if (registry == 0 || device == 0) {
        return false;
    }

    registry_ = registry;
    device_ = device;
    last_registry_result_ = RegistryResult::OK;
    last_command_state_ = CommandState::REQUEST;
    return true;
}

void MotorService::attachRuntime(Runtime* runtime) {
    runtime_ = runtime;
}

const char* MotorService::id() const {
    return SERVICE_MOTOR;
}

bool MotorService::updateState(uint32_t now_ms) {
    if (registry_ == 0 || device_ == 0) {
        last_registry_result_ = RegistryResult::NOT_ATTACHED;
        return false;
    }

    CapabilityPayload payload;
    const bool read_success = device_->readPayload(now_ms, payload);
    last_registry_result_ = registry_->updateCapabilityPayloadWithResult(CAP_MOTOR_CONTROL, payload);

    return read_success && last_registry_result_ == RegistryResult::OK;
}

bool MotorService::setMotor(
    uint32_t now_ms,
    const MotorCommandRequest& request,
    MotorCommandResult& out_result) {
    out_result.registry_result = RegistryResult::OK;

    if (registry_ == 0 || device_ == 0) {
        last_registry_result_ = RegistryResult::NOT_ATTACHED;
        fillFailedResult(
            now_ms,
            request.direction,
            request.speed_percent,
            out_result,
            ERR_CAPABILITY_UNAVAILABLE);
        return false;
    }

    if (!runtimeAllowsActuatorCommands()) {
        last_registry_result_ = RegistryResult::OK;
        fillFailedResult(
            now_ms,
            request.direction,
            request.speed_percent,
            out_result,
            ERR_CAPABILITY_UNAVAILABLE);
        return false;
    }

    if (!isValidMotorRequest(request.direction, request.speed_percent)) {
        last_registry_result_ = RegistryResult::INVALID_RECORD;
        fillFailedResult(
            now_ms,
            request.direction,
            request.speed_percent,
            out_result,
            ERR_CAPABILITY_UNAVAILABLE);
        return false;
    }

    out_result.accepted = true;
    out_result.executed = false;
    out_result.state = CommandState::EXECUTING;
    out_result.error_code = "none";
    out_result.registry_result = RegistryResult::OK;
    last_command_state_ = CommandState::EXECUTING;

    CapabilityPayload payload;
    const bool command_success = device_->setMotor(
        now_ms,
        request.direction,
        request.speed_percent,
        payload);
    last_registry_result_ = registry_->updateCapabilityPayloadWithResult(CAP_MOTOR_CONTROL, payload);
    out_result.registry_result = last_registry_result_;

    if (command_success && last_registry_result_ == RegistryResult::OK) {
        out_result.state = CommandState::COMPLETED;
        out_result.executed = true;
        out_result.error_code = "none";
        last_command_state_ = CommandState::COMPLETED;
        writeCommandState(
            now_ms,
            CommandState::COMPLETED,
            last_registry_result_,
            "none",
            request.direction,
            request.speed_percent);
        return true;
    }

    out_result.state = CommandState::FAILED;
    out_result.executed = false;
    out_result.error_code = command_success ? ERR_CAPABILITY_UNAVAILABLE : ERR_DEVICE_TIMEOUT;
    last_command_state_ = CommandState::FAILED;
    writeCommandState(
        now_ms,
        CommandState::FAILED,
        last_registry_result_,
        out_result.error_code,
        request.direction,
        request.speed_percent);
    return false;
}

bool MotorService::stop(uint32_t now_ms, MotorCommandResult& out_result) {
    MotorCommandRequest request;
    request.direction = MotorDirection::STOP;
    request.speed_percent = 0.0F;
    request.timeout_ms = 0;

    out_result.registry_result = RegistryResult::OK;

    if (registry_ == 0 || device_ == 0) {
        last_registry_result_ = RegistryResult::NOT_ATTACHED;
        fillFailedResult(
            now_ms,
            request.direction,
            request.speed_percent,
            out_result,
            ERR_CAPABILITY_UNAVAILABLE);
        return false;
    }

    if (!runtimeAllowsActuatorCommands()) {
        last_registry_result_ = RegistryResult::OK;
        fillFailedResult(
            now_ms,
            request.direction,
            request.speed_percent,
            out_result,
            ERR_CAPABILITY_UNAVAILABLE);
        return false;
    }

    out_result.accepted = true;
    out_result.executed = false;
    out_result.state = CommandState::EXECUTING;
    out_result.error_code = "none";
    out_result.registry_result = RegistryResult::OK;
    last_command_state_ = CommandState::EXECUTING;

    CapabilityPayload payload;
    const bool command_success = device_->stop(now_ms, payload);
    last_registry_result_ = registry_->updateCapabilityPayloadWithResult(CAP_MOTOR_CONTROL, payload);
    out_result.registry_result = last_registry_result_;

    if (command_success && last_registry_result_ == RegistryResult::OK) {
        out_result.state = CommandState::COMPLETED;
        out_result.executed = true;
        out_result.error_code = "none";
        last_command_state_ = CommandState::COMPLETED;
        writeCommandState(
            now_ms,
            CommandState::COMPLETED,
            last_registry_result_,
            "none",
            request.direction,
            request.speed_percent);
        return true;
    }

    out_result.state = CommandState::FAILED;
    out_result.executed = false;
    out_result.error_code = command_success ? ERR_CAPABILITY_UNAVAILABLE : ERR_DEVICE_TIMEOUT;
    last_command_state_ = CommandState::FAILED;
    writeCommandState(
        now_ms,
        CommandState::FAILED,
        last_registry_result_,
        out_result.error_code,
        request.direction,
        request.speed_percent);
    return false;
}

RegistryResult MotorService::lastRegistryResult() const {
    return last_registry_result_;
}

CommandState MotorService::lastCommandState() const {
    return last_command_state_;
}

bool MotorService::isValidMotorRequest(MotorDirection direction, float speed_percent) const {
    if (speed_percent < 0.0F || speed_percent > 100.0F) {
        return false;
    }

    switch (direction) {
        case MotorDirection::STOP:
            return speed_percent == 0.0F;
        case MotorDirection::FORWARD:
        case MotorDirection::REVERSE:
            return true;
        default:
            return false;
    }
}

bool MotorService::runtimeAllowsActuatorCommands() const {
    if (runtime_ == 0) {
        return false;
    }

    const RuntimeState state = runtime_->state();
    switch (state) {
        case RuntimeState::READY:
        case RuntimeState::RUNNING:
            return true;
        case RuntimeState::BOOTING:
        case RuntimeState::INITIALIZING:
        case RuntimeState::DISCOVERING:
        case RuntimeState::REGISTERING:
        case RuntimeState::STARTING:
        case RuntimeState::ERROR_STATE:
        case RuntimeState::SAFE_MODE:
        default:
            return false;
    }
}

void MotorService::fillFailedResult(
    uint32_t now_ms,
    MotorDirection direction,
    float speed_percent,
    MotorCommandResult& out_result,
    const char* error_code) {
    out_result.state = CommandState::FAILED;
    out_result.registry_result = last_registry_result_;
    out_result.accepted = false;
    out_result.executed = false;
    out_result.error_code = error_code;
    last_command_state_ = CommandState::FAILED;
    writeCommandState(
        now_ms,
        CommandState::FAILED,
        last_registry_result_,
        error_code,
        direction,
        speed_percent);
}

void MotorService::writeCommandState(
    uint32_t now_ms,
    CommandState command_state,
    RegistryResult registry_result,
    const char* error_code,
    MotorDirection direction,
    float speed_percent) {
    if (registry_ == 0) {
        return;
    }

    CommandStateRecord record;
    record.capability_id = CAP_MOTOR_CONTROL;
    record.command_state = command_state;
    record.timestamp_ms = now_ms;
    record.registry_result = registry_result;
    record.error_code = error_code;
    record.value_float = speed_percent;
    record.value_int = static_cast<int32_t>(direction);

    registry_->updateCommandState(record);
}

}  // namespace Cyber32
