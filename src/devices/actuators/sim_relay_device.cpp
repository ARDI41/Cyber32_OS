#include "sim_relay_device.h"

#include "../../core/ids/capability_ids.h"
#include "../../core/ids/error_ids.h"

namespace Cyber32 {

const char* const SimRelayDevice::DEVICE_ID = "device-sim-relay-001";
const char* const SimRelayDevice::DEVICE_TYPE = "actuator";

SimRelayDevice::SimRelayDevice()
    : driver_(0) {
}

bool SimRelayDevice::begin(SimRelayDriver* driver) {
    if (driver == 0) {
        return false;
    }

    driver_ = driver;
    return true;
}

bool SimRelayDevice::readPayload(uint32_t now_ms, CapabilityPayload& out_payload) {
    if (driver_ == 0 || !driver_->initialized()) {
        fillUnavailablePayload(now_ms, out_payload, ERR_ACTUATOR_UNAVAILABLE);
        return false;
    }

    out_payload.capability_id = CAP_RELAY_CONTROL;
    out_payload.schema_version = 1;
    out_payload.timestamp_ms = now_ms;
    out_payload.available = Availability::AVAILABLE;
    out_payload.stale = StaleState::FRESH;
    out_payload.value_type = PayloadValueType::BOOLEAN;
    out_payload.value_float = 0.0F;
    out_payload.value_int = driver_->enabled() ? 1 : 0;
    out_payload.unit = "boolean";
    out_payload.quality = "valid";
    out_payload.error_code = "none";
    return true;
}

bool SimRelayDevice::setEnabled(
    uint32_t now_ms,
    bool enabled,
    CapabilityPayload& out_payload) {
    if (driver_ == 0 || !driver_->setEnabled(enabled)) {
        fillUnavailablePayload(now_ms, out_payload, ERR_ACTUATOR_EXECUTION_FAILED);
        return false;
    }

    return readPayload(now_ms, out_payload);
}

bool SimRelayDevice::disable(uint32_t now_ms, CapabilityPayload& out_payload) {
    if (driver_ == 0 || !driver_->disable()) {
        fillUnavailablePayload(now_ms, out_payload, ERR_ACTUATOR_EXECUTION_FAILED);
        return false;
    }

    return readPayload(now_ms, out_payload);
}

const char* SimRelayDevice::id() const {
    return DEVICE_ID;
}

const char* SimRelayDevice::type() const {
    return DEVICE_TYPE;
}

void SimRelayDevice::fillUnavailablePayload(
    uint32_t now_ms,
    CapabilityPayload& out_payload,
    const char* error_code) const {
    out_payload.capability_id = CAP_RELAY_CONTROL;
    out_payload.schema_version = 1;
    out_payload.timestamp_ms = now_ms;
    out_payload.available = Availability::UNAVAILABLE;
    out_payload.stale = StaleState::FRESH;
    out_payload.value_type = PayloadValueType::NONE;
    out_payload.value_float = 0.0F;
    out_payload.value_int = 0;
    out_payload.unit = "boolean";
    out_payload.quality = "unavailable";
    out_payload.error_code = error_code;
}

}  // namespace Cyber32
