#include "sim_motor_device.h"

#include "../../core/ids/capability_ids.h"
#include "../../core/ids/error_ids.h"

namespace Cyber32 {

const char* const SimMotorDevice::DEVICE_ID = "device-sim-motor-001";
const char* const SimMotorDevice::DEVICE_TYPE = "actuator";

SimMotorDevice::SimMotorDevice()
    : driver_(0) {
}

bool SimMotorDevice::begin(SimMotorDriver* driver) {
    if (driver == 0) {
        return false;
    }

    driver_ = driver;
    return true;
}

bool SimMotorDevice::readPayload(uint32_t now_ms, CapabilityPayload& out_payload) {
    fillBasePayload(now_ms, out_payload);

    if (driver_ != 0 && driver_->initialized()) {
        out_payload.available = Availability::AVAILABLE;
        out_payload.stale = StaleState::FRESH;
        out_payload.value_type = PayloadValueType::FLOAT;
        out_payload.value_float = driver_->currentSpeedPercent();
        out_payload.value_int = static_cast<int32_t>(driver_->currentDirection());
        out_payload.quality = "valid";
        out_payload.error_code = "none";
        return true;
    }

    return false;
}

bool SimMotorDevice::setMotor(uint32_t now_ms, MotorDirection direction, float speed_percent, CapabilityPayload& out_payload) {
    if (driver_ == 0 || !driver_->setMotor(direction, speed_percent)) {
        fillBasePayload(now_ms, out_payload);
        return false;
    }

    return readPayload(now_ms, out_payload);
}

bool SimMotorDevice::stop(uint32_t now_ms, CapabilityPayload& out_payload) {
    if (driver_ == 0 || !driver_->stop()) {
        fillBasePayload(now_ms, out_payload);
        return false;
    }

    return readPayload(now_ms, out_payload);
}

const char* SimMotorDevice::id() const {
    return DEVICE_ID;
}

const char* SimMotorDevice::type() const {
    return DEVICE_TYPE;
}

void SimMotorDevice::fillBasePayload(uint32_t now_ms, CapabilityPayload& out_payload) const {
    out_payload.capability_id = CAP_MOTOR_CONTROL;
    out_payload.schema_version = 1;
    out_payload.timestamp_ms = now_ms;
    out_payload.available = Availability::UNAVAILABLE;
    out_payload.stale = StaleState::FRESH;
    out_payload.value_type = PayloadValueType::NONE;
    out_payload.value_float = 0.0F;
    out_payload.value_int = 0;
    out_payload.unit = "percent";
    out_payload.quality = "unavailable";
    out_payload.error_code = ERR_DEVICE_TIMEOUT;
}

}  // namespace Cyber32
