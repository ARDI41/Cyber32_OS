#include "servo_service.h"

#include "../../core/ids/capability_ids.h"
#include "../../core/ids/error_ids.h"
#include "../../core/ids/service_ids.h"

namespace Cyber32 {

ServoService::ServoService()
    : registry_(0),
      device_(0),
      runtime_(0),
      last_registry_result_(RegistryResult::NOT_ATTACHED),
      last_command_state_(CommandState::REQUEST) {
}

bool ServoService::begin(Registry* registry, SimServoDevice* device) {
    if (registry == 0 || device == 0) {
        return false;
    }

    registry_ = registry;
    device_ = device;
    last_registry_result_ = RegistryResult::OK;
    last_command_state_ = CommandState::REQUEST;
    return true;
}

void ServoService::attachRuntime(Runtime* runtime) {
    runtime_ = runtime;
}

const char* ServoService::id() const {
    return SERVICE_SERVO;
}

bool ServoService::updateState(uint32_t now_ms) {
    if (registry_ == 0 || device_ == 0) {
        last_registry_result_ = RegistryResult::NOT_ATTACHED;
        return false;
    }

    CapabilityPayload payload;
    const bool read_success = device_->readPayload(now_ms, payload);
    last_registry_result_ = registry_->updateCapabilityPayloadWithResult(CAP_SERVO_POSITION, payload);

    return read_success && last_registry_result_ == RegistryResult::OK;
}

bool ServoService::setPosition(
    uint32_t now_ms,
    const ServoCommandRequest& request,
    ServoCommandResult& out_result) {
    out_result.registry_result = RegistryResult::OK;

    if (registry_ == 0 || device_ == 0) {
        last_registry_result_ = RegistryResult::NOT_ATTACHED;
        fillFailedResult(now_ms, request.position_degrees, out_result, ERR_CAPABILITY_UNAVAILABLE);
        return false;
    }

    if (!runtimeAllowsActuatorCommands()) {
        last_registry_result_ = RegistryResult::OK;
        fillFailedResult(now_ms, request.position_degrees, out_result, ERR_CAPABILITY_UNAVAILABLE);
        return false;
    }

    if (!isValidPosition(request.position_degrees)) {
        last_registry_result_ = RegistryResult::INVALID_RECORD;
        fillFailedResult(now_ms, request.position_degrees, out_result, ERR_CAPABILITY_UNAVAILABLE);
        return false;
    }

    out_result.accepted = true;
    out_result.executed = false;
    out_result.state = CommandState::EXECUTING;
    out_result.error_code = "none";
    out_result.registry_result = RegistryResult::OK;
    last_command_state_ = CommandState::EXECUTING;

    CapabilityPayload payload;
    const bool command_success = device_->setPosition(now_ms, request.position_degrees, payload);
    last_registry_result_ = registry_->updateCapabilityPayloadWithResult(CAP_SERVO_POSITION, payload);
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
            request.position_degrees);
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
        request.position_degrees);
    return false;
}

RegistryResult ServoService::lastRegistryResult() const {
    return last_registry_result_;
}

CommandState ServoService::lastCommandState() const {
    return last_command_state_;
}

bool ServoService::isValidPosition(float position_degrees) const {
    return position_degrees >= 0.0F && position_degrees <= 180.0F;
}

bool ServoService::runtimeAllowsActuatorCommands() const {
    if (runtime_ == 0) {
        return false;
    }

    const RuntimeState state = runtime_->state();
    return state == RuntimeState::READY || state == RuntimeState::RUNNING;
}

void ServoService::fillFailedResult(
    uint32_t now_ms,
    float position_degrees,
    ServoCommandResult& out_result,
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
        position_degrees);
}

void ServoService::writeCommandState(
    uint32_t now_ms,
    CommandState command_state,
    RegistryResult registry_result,
    const char* error_code,
    float value_float) {
    if (registry_ == 0) {
        return;
    }

    CommandStateRecord record;
    record.capability_id = CAP_SERVO_POSITION;
    record.command_state = command_state;
    record.timestamp_ms = now_ms;
    record.registry_result = registry_result;
    record.error_code = error_code;
    record.value_float = value_float;
    record.value_int = 0;

    registry_->updateCommandState(record);
}

}  // namespace Cyber32
