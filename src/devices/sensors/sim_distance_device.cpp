#include "sim_distance_device.h"

#include "../../core/ids/capability_ids.h"
#include "../../core/ids/error_ids.h"

namespace Cyber32 {

const char* const SimDistanceDevice::DEVICE_ID = "device-sim-distance-001";
const char* const SimDistanceDevice::DEVICE_TYPE = "sensor";

SimDistanceDevice::SimDistanceDevice()
    : driver_(0) {
}

bool SimDistanceDevice::begin(SimDistanceDriver* driver) {
    if (driver == 0) {
        return false;
    }

    driver_ = driver;
    return true;
}

bool SimDistanceDevice::readPayload(uint32_t now_ms, CapabilityPayload& out_payload) {
    fillBasePayload(now_ms, out_payload);

    float distance = 0.0F;
    if (driver_ != 0 && driver_->readDistance(distance)) {
        out_payload.available = Availability::AVAILABLE;
        out_payload.stale = StaleState::FRESH;
        out_payload.value_type = PayloadValueType::FLOAT;
        out_payload.value_float = distance;
        out_payload.value_int = 0;
        out_payload.quality = "valid";
        out_payload.error_code = "none";
        return true;
    }

    out_payload.available = Availability::UNAVAILABLE;
    out_payload.stale = StaleState::FRESH;
    out_payload.value_type = PayloadValueType::NONE;
    out_payload.value_float = 0.0F;
    out_payload.value_int = 0;
    out_payload.quality = "unavailable";
    out_payload.error_code = ERR_DEVICE_TIMEOUT;
    return false;
}

const char* SimDistanceDevice::id() const {
    return DEVICE_ID;
}

const char* SimDistanceDevice::type() const {
    return DEVICE_TYPE;
}

void SimDistanceDevice::fillBasePayload(uint32_t now_ms, CapabilityPayload& out_payload) const {
    out_payload.capability_id = CAP_DISTANCE;
    out_payload.schema_version = 1;
    out_payload.timestamp_ms = now_ms;
    out_payload.available = Availability::UNAVAILABLE;
    out_payload.stale = StaleState::FRESH;
    out_payload.value_type = PayloadValueType::NONE;
    out_payload.value_float = 0.0F;
    out_payload.value_int = 0;
    out_payload.unit = "meter";
    out_payload.quality = "unavailable";
    out_payload.error_code = ERR_DEVICE_TIMEOUT;
}

}  // namespace Cyber32
