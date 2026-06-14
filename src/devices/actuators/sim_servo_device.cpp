#include "sim_servo_device.h"

#include "../../core/ids/capability_ids.h"
#include "../../core/ids/error_ids.h"

namespace Cyber32 {

const char* const SimServoDevice::DEVICE_ID = "device-sim-servo-001";
const char* const SimServoDevice::DEVICE_TYPE = "actuator";

SimServoDevice::SimServoDevice()
    : driver_(0) {
}

bool SimServoDevice::begin(SimServoDriver* driver) {
    if (driver == 0) {
        return false;
    }

    driver_ = driver;
    return true;
}

bool SimServoDevice::readPayload(uint32_t now_ms, CapabilityPayload& out_payload) {
    fillBasePayload(now_ms, out_payload);

    if (driver_ != 0 && driver_->initialized()) {
        out_payload.available = Availability::AVAILABLE;
        out_payload.stale = StaleState::FRESH;
        out_payload.value_type = PayloadValueType::FLOAT;
        out_payload.value_float = driver_->currentPosition();
        out_payload.value_int = 0;
        out_payload.quality = "valid";
        out_payload.error_code = "none";
        return true;
    }

    return false;
}

bool SimServoDevice::setPosition(uint32_t now_ms, float position_degrees, CapabilityPayload& out_payload) {
    if (driver_ == 0 || !driver_->setPosition(position_degrees)) {
        fillBasePayload(now_ms, out_payload);
        return false;
    }

    return readPayload(now_ms, out_payload);
}

const char* SimServoDevice::id() const {
    return DEVICE_ID;
}

const char* SimServoDevice::type() const {
    return DEVICE_TYPE;
}

void SimServoDevice::fillBasePayload(uint32_t now_ms, CapabilityPayload& out_payload) const {
    out_payload.capability_id = CAP_SERVO_POSITION;
    out_payload.schema_version = 1;
    out_payload.timestamp_ms = now_ms;
    out_payload.available = Availability::UNAVAILABLE;
    out_payload.stale = StaleState::FRESH;
    out_payload.value_type = PayloadValueType::NONE;
    out_payload.value_float = 0.0F;
    out_payload.value_int = 0;
    out_payload.unit = "degree";
    out_payload.quality = "unavailable";
    out_payload.error_code = ERR_DEVICE_TIMEOUT;
}

}  // namespace Cyber32
