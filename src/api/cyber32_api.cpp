#include "cyber32_api.h"

#include "../core/ids/capability_ids.h"
#include "../core/ids/error_ids.h"
#include "../services/motor/motor_service.h"
#include "../services/relay/relay_service.h"
#include "../services/servo/servo_service.h"

namespace Cyber32 {

Cyber32Api::Cyber32Api()
    : registry_(0),
      runtime_(0),
      servo_service_(0),
      motor_service_(0),
      relay_service_(0) {
}

bool Cyber32Api::begin(Registry* registry, Runtime* runtime) {
    if (registry == 0 || runtime == 0) {
        return false;
    }

    registry_ = registry;
    runtime_ = runtime;
    return true;
}

void Cyber32Api::attachServoService(ServoService* service) {
    servo_service_ = service;
}

void Cyber32Api::attachMotorService(MotorService* service) {
    motor_service_ = service;
}

void Cyber32Api::attachRelayService(RelayService* service) {
    relay_service_ = service;
}

bool Cyber32Api::getSystemStatus(ApiSystemStatus& out_status) {
    if (registry_ == 0 || runtime_ == 0) {
        out_status.ok = false;
        out_status.runtime_state = RuntimeState::ERROR_STATE;
        out_status.module_count = 0;
        out_status.device_count = 0;
        out_status.capability_count = 0;
        out_status.latest_error_code = ERR_CAPABILITY_UNAVAILABLE;
        return false;
    }

    out_status.ok = true;
    out_status.runtime_state = runtime_->state();
    out_status.module_count = registry_->moduleCount();
    out_status.device_count = registry_->deviceCount();
    out_status.capability_count = registry_->capabilityCount();
    out_status.latest_error_code = "none";
    return true;
}

bool Cyber32Api::getTemperatureState(ApiCapabilityState& out_state) {
    if (registry_ == 0) {
        fillUnavailableTemperatureState(out_state);
        return false;
    }

    CapabilityPayload payload;
    const RegistryResult result =
        registry_->getCapabilityPayloadWithResult(CAP_TEMPERATURE, payload);
    if (result != RegistryResult::OK) {
        fillUnavailableTemperatureState(out_state);
        return false;
    }

    out_state.ok = true;
    out_state.payload = payload;
    out_state.error_code = "none";
    return true;
}

bool Cyber32Api::getDistanceState(ApiCapabilityState& out_state) {
    if (registry_ == 0) {
        fillUnavailableDistanceState(out_state);
        return false;
    }

    CapabilityPayload payload;
    const RegistryResult result =
        registry_->getCapabilityPayloadWithResult(CAP_DISTANCE, payload);
    if (result != RegistryResult::OK) {
        fillUnavailableDistanceState(out_state);
        return false;
    }

    out_state.ok = true;
    out_state.payload = payload;
    out_state.error_code = "none";
    return true;
}

bool Cyber32Api::getServoPositionState(ApiCapabilityState& out_state) {
    if (registry_ == 0) {
        fillUnavailableServoPositionState(out_state);
        return false;
    }

    CapabilityPayload payload;
    const RegistryResult result =
        registry_->getCapabilityPayloadWithResult(CAP_SERVO_POSITION, payload);
    if (result != RegistryResult::OK) {
        fillUnavailableServoPositionState(out_state);
        return false;
    }

    out_state.ok = true;
    out_state.payload = payload;
    out_state.error_code = "none";
    return true;
}

bool Cyber32Api::getMotorControlState(ApiCapabilityState& out_state) {
    if (registry_ == 0) {
        fillUnavailableMotorControlState(out_state);
        return false;
    }

    CapabilityPayload payload;
    const RegistryResult result =
        registry_->getCapabilityPayloadWithResult(CAP_MOTOR_CONTROL, payload);
    if (result != RegistryResult::OK) {
        fillUnavailableMotorControlState(out_state);
        return false;
    }

    out_state.ok = true;
    out_state.payload = payload;
    out_state.error_code = "none";
    return true;
}

bool Cyber32Api::getRelayControlState(ApiCapabilityState& out_state) {
    if (registry_ == 0) {
        fillUnavailableRelayControlState(out_state);
        return false;
    }

    CapabilityPayload payload;
    const RegistryResult result =
        registry_->getCapabilityPayloadWithResult(CAP_RELAY_CONTROL, payload);
    if (result != RegistryResult::OK) {
        fillUnavailableRelayControlState(out_state);
        return false;
    }

    out_state.ok = true;
    out_state.payload = payload;
    out_state.error_code = "none";
    return true;
}

bool Cyber32Api::getServoCommandState(ApiCommandStateResponse& out_response) {
    if (registry_ == 0) {
        fillUnavailableServoCommandState(out_response, RegistryResult::NOT_ATTACHED);
        return false;
    }

    CommandStateRecord record;
    const RegistryResult result = registry_->getCommandState(CAP_SERVO_POSITION, record);
    if (result != RegistryResult::OK) {
        fillUnavailableServoCommandState(out_response, result);
        return false;
    }

    out_response.ok = true;
    out_response.command_state = record.command_state;
    out_response.registry_result = record.registry_result;
    out_response.capability_id = record.capability_id;
    out_response.error_code = record.error_code;
    out_response.value_float = record.value_float;
    out_response.value_int = record.value_int;
    out_response.timestamp_ms = record.timestamp_ms;
    return true;
}

bool Cyber32Api::getMotorCommandState(ApiCommandStateResponse& out_response) {
    if (registry_ == 0) {
        fillUnavailableMotorCommandState(out_response, RegistryResult::NOT_ATTACHED);
        return false;
    }

    CommandStateRecord record;
    const RegistryResult result = registry_->getCommandState(CAP_MOTOR_CONTROL, record);
    if (result != RegistryResult::OK) {
        fillUnavailableMotorCommandState(out_response, result);
        return false;
    }

    out_response.ok = true;
    out_response.command_state = record.command_state;
    out_response.registry_result = record.registry_result;
    out_response.capability_id = record.capability_id;
    out_response.error_code = record.error_code;
    out_response.value_float = record.value_float;
    out_response.value_int = record.value_int;
    out_response.timestamp_ms = record.timestamp_ms;
    return true;
}

bool Cyber32Api::getRelayCommandState(ApiCommandStateResponse& out_response) {
    if (registry_ == 0) {
        fillUnavailableRelayCommandState(out_response, RegistryResult::NOT_ATTACHED);
        return false;
    }

    CommandStateRecord record;
    const RegistryResult result = registry_->getCommandState(CAP_RELAY_CONTROL, record);
    if (result != RegistryResult::OK) {
        fillUnavailableRelayCommandState(out_response, result);
        return false;
    }

    out_response.ok = true;
    out_response.command_state = record.command_state;
    out_response.registry_result = record.registry_result;
    out_response.capability_id = record.capability_id;
    out_response.error_code = record.error_code;
    out_response.value_float = record.value_float;
    out_response.value_int = record.value_int;
    out_response.timestamp_ms = record.timestamp_ms;
    return true;
}

bool Cyber32Api::commandServoPosition(
    uint32_t now_ms,
    const ApiServoCommandRequest& request,
    ApiServoCommandResponse& out_response) {
    if (servo_service_ == 0) {
        fillFailedServoCommand(out_response, ERR_CAPABILITY_UNAVAILABLE);
        return false;
    }

    ServoCommandRequest service_request;
    service_request.position_degrees = request.position_degrees;
    service_request.timeout_ms = request.timeout_ms;

    ServoCommandResult service_result;
    const bool success = servo_service_->setPosition(now_ms, service_request, service_result);

    out_response.ok = success;
    out_response.command_state = service_result.state;
    out_response.accepted = service_result.accepted;
    out_response.executed = service_result.executed;
    out_response.error_code = service_result.error_code;
    return success;
}

bool Cyber32Api::commandMotorControl(
    uint32_t now_ms,
    const ApiMotorCommandRequest& request,
    ApiMotorCommandResponse& out_response) {
    if (motor_service_ == 0) {
        fillFailedMotorCommand(out_response, ERR_CAPABILITY_UNAVAILABLE);
        return false;
    }

    MotorCommandRequest service_request;
    service_request.direction = request.direction;
    service_request.speed_percent = request.speed_percent;
    service_request.timeout_ms = request.timeout_ms;

    MotorCommandResult service_result;
    const bool success = motor_service_->setMotor(now_ms, service_request, service_result);

    out_response.ok = success;
    out_response.command_state = service_result.state;
    out_response.accepted = service_result.accepted;
    out_response.executed = service_result.executed;
    out_response.error_code = service_result.error_code;
    return success;
}

bool Cyber32Api::commandMotorStop(uint32_t now_ms, ApiMotorCommandResponse& out_response) {
    if (motor_service_ == 0) {
        fillFailedMotorCommand(out_response, ERR_CAPABILITY_UNAVAILABLE);
        return false;
    }

    MotorCommandResult service_result;
    const bool success = motor_service_->stop(now_ms, service_result);

    out_response.ok = success;
    out_response.command_state = service_result.state;
    out_response.accepted = service_result.accepted;
    out_response.executed = service_result.executed;
    out_response.error_code = service_result.error_code;
    return success;
}

bool Cyber32Api::commandRelayControl(
    uint32_t now_ms,
    const ApiRelayCommandRequest& request,
    ApiRelayCommandResponse& out_response) {
    if (relay_service_ == 0) {
        fillFailedRelayCommand(out_response, ERR_ACTUATOR_UNAVAILABLE);
        return false;
    }

    RelayCommandRequest service_request;
    service_request.enabled = request.enabled;
    service_request.timeout_ms = request.timeout_ms;

    RelayCommandResult service_result;
    const bool success = relay_service_->setEnabled(now_ms, service_request, service_result);

    out_response.ok = success;
    out_response.command_state = service_result.state;
    out_response.accepted = service_result.accepted;
    out_response.executed = service_result.executed;
    out_response.error_code = service_result.error_code;
    return success;
}

bool Cyber32Api::commandRelayOff(uint32_t now_ms, ApiRelayCommandResponse& out_response) {
    if (relay_service_ == 0) {
        fillFailedRelayCommand(out_response, ERR_ACTUATOR_UNAVAILABLE);
        return false;
    }

    RelayCommandResult service_result;
    const bool success = relay_service_->disable(now_ms, service_result);

    out_response.ok = success;
    out_response.command_state = service_result.state;
    out_response.accepted = service_result.accepted;
    out_response.executed = service_result.executed;
    out_response.error_code = service_result.error_code;
    return success;
}

void Cyber32Api::fillUnavailableTemperatureState(ApiCapabilityState& out_state) const {
    out_state.ok = false;
    out_state.payload.capability_id = CAP_TEMPERATURE;
    out_state.payload.schema_version = 1;
    out_state.payload.timestamp_ms = 0;
    out_state.payload.available = Availability::UNAVAILABLE;
    out_state.payload.stale = StaleState::STALE;
    out_state.payload.value_type = PayloadValueType::NONE;
    out_state.payload.value_float = 0.0F;
    out_state.payload.value_int = 0;
    out_state.payload.unit = "degree_celsius";
    out_state.payload.quality = "unavailable";
    out_state.payload.error_code = ERR_CAPABILITY_UNAVAILABLE;
    out_state.error_code = ERR_CAPABILITY_UNAVAILABLE;
}

void Cyber32Api::fillUnavailableDistanceState(ApiCapabilityState& out_state) const {
    out_state.ok = false;
    out_state.payload.capability_id = CAP_DISTANCE;
    out_state.payload.schema_version = 1;
    out_state.payload.timestamp_ms = 0;
    out_state.payload.available = Availability::UNAVAILABLE;
    out_state.payload.stale = StaleState::STALE;
    out_state.payload.value_type = PayloadValueType::NONE;
    out_state.payload.value_float = 0.0F;
    out_state.payload.value_int = 0;
    out_state.payload.unit = "meter";
    out_state.payload.quality = "unavailable";
    out_state.payload.error_code = ERR_CAPABILITY_UNAVAILABLE;
    out_state.error_code = ERR_CAPABILITY_UNAVAILABLE;
}

void Cyber32Api::fillUnavailableServoPositionState(ApiCapabilityState& out_state) const {
    out_state.ok = false;
    out_state.payload.capability_id = CAP_SERVO_POSITION;
    out_state.payload.schema_version = 1;
    out_state.payload.timestamp_ms = 0;
    out_state.payload.available = Availability::UNAVAILABLE;
    out_state.payload.stale = StaleState::STALE;
    out_state.payload.value_type = PayloadValueType::NONE;
    out_state.payload.value_float = 0.0F;
    out_state.payload.value_int = 0;
    out_state.payload.unit = "degree";
    out_state.payload.quality = "unavailable";
    out_state.payload.error_code = ERR_CAPABILITY_UNAVAILABLE;
    out_state.error_code = ERR_CAPABILITY_UNAVAILABLE;
}

void Cyber32Api::fillUnavailableMotorControlState(ApiCapabilityState& out_state) const {
    out_state.ok = false;
    out_state.payload.capability_id = CAP_MOTOR_CONTROL;
    out_state.payload.schema_version = 1;
    out_state.payload.timestamp_ms = 0;
    out_state.payload.available = Availability::UNAVAILABLE;
    out_state.payload.stale = StaleState::STALE;
    out_state.payload.value_type = PayloadValueType::NONE;
    out_state.payload.value_float = 0.0F;
    out_state.payload.value_int = 0;
    out_state.payload.unit = "percent";
    out_state.payload.quality = "unavailable";
    out_state.payload.error_code = ERR_CAPABILITY_UNAVAILABLE;
    out_state.error_code = ERR_CAPABILITY_UNAVAILABLE;
}

void Cyber32Api::fillUnavailableRelayControlState(ApiCapabilityState& out_state) const {
    out_state.ok = false;
    out_state.payload.capability_id = CAP_RELAY_CONTROL;
    out_state.payload.schema_version = 1;
    out_state.payload.timestamp_ms = 0;
    out_state.payload.available = Availability::UNAVAILABLE;
    out_state.payload.stale = StaleState::STALE;
    out_state.payload.value_type = PayloadValueType::NONE;
    out_state.payload.value_float = 0.0F;
    out_state.payload.value_int = 0;
    out_state.payload.unit = "boolean";
    out_state.payload.quality = "unavailable";
    out_state.payload.error_code = ERR_ACTUATOR_UNAVAILABLE;
    out_state.error_code = ERR_ACTUATOR_UNAVAILABLE;
}

void Cyber32Api::fillFailedServoCommand(
    ApiServoCommandResponse& out_response,
    const char* error_code) const {
    out_response.ok = false;
    out_response.command_state = CommandState::FAILED;
    out_response.accepted = false;
    out_response.executed = false;
    out_response.error_code = error_code;
}

void Cyber32Api::fillFailedMotorCommand(
    ApiMotorCommandResponse& out_response,
    const char* error_code) const {
    out_response.ok = false;
    out_response.command_state = CommandState::FAILED;
    out_response.accepted = false;
    out_response.executed = false;
    out_response.error_code = error_code;
}

void Cyber32Api::fillFailedRelayCommand(
    ApiRelayCommandResponse& out_response,
    const char* error_code) const {
    out_response.ok = false;
    out_response.command_state = CommandState::FAILED;
    out_response.accepted = false;
    out_response.executed = false;
    out_response.error_code = error_code;
}

void Cyber32Api::fillUnavailableServoCommandState(
    ApiCommandStateResponse& out_response,
    RegistryResult registry_result) const {
    out_response.ok = false;
    out_response.command_state = CommandState::FAILED;
    out_response.registry_result = registry_result;
    out_response.capability_id = CAP_SERVO_POSITION;
    out_response.error_code = ERR_CAPABILITY_UNAVAILABLE;
    out_response.value_float = 0.0F;
    out_response.value_int = 0;
    out_response.timestamp_ms = 0;
}

void Cyber32Api::fillUnavailableMotorCommandState(
    ApiCommandStateResponse& out_response,
    RegistryResult registry_result) const {
    out_response.ok = false;
    out_response.command_state = CommandState::FAILED;
    out_response.registry_result = registry_result;
    out_response.capability_id = CAP_MOTOR_CONTROL;
    out_response.error_code = ERR_CAPABILITY_UNAVAILABLE;
    out_response.value_float = 0.0F;
    out_response.value_int = 0;
    out_response.timestamp_ms = 0;
}

void Cyber32Api::fillUnavailableRelayCommandState(
    ApiCommandStateResponse& out_response,
    RegistryResult registry_result) const {
    out_response.ok = false;
    out_response.command_state = CommandState::FAILED;
    out_response.registry_result = registry_result;
    out_response.capability_id = CAP_RELAY_CONTROL;
    out_response.error_code = ERR_ACTUATOR_UNAVAILABLE;
    out_response.value_float = 0.0F;
    out_response.value_int = 0;
    out_response.timestamp_ms = 0;
}

}  // namespace Cyber32
