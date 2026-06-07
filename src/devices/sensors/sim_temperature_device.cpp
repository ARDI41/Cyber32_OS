#include "sim_temperature_device.h"

#include "../../core/ids/capability_ids.h"
#include "../../core/ids/error_ids.h"

namespace Cyber32 {

const char* const SimTemperatureDevice::DEVICE_ID = "device-sim-temperature-001";
const char* const SimTemperatureDevice::DEVICE_TYPE = "sensor";

SimTemperatureDevice::SimTemperatureDevice()
    : driver_(0) {
}

bool SimTemperatureDevice::begin(SimTemperatureDriver* driver) {
    if (driver == 0) {
        return false;
    }

    driver_ = driver;
    return true;
}

bool SimTemperatureDevice::readPayload(uint32_t now_ms, CapabilityPayload& out_payload) {
    fillBasePayload(now_ms, out_payload);

    float temperature = 0.0F;
    if (driver_ != 0 && driver_->readTemperature(temperature)) {
        out_payload.available = Availability::AVAILABLE;
        out_payload.stale = StaleState::FRESH;
        out_payload.value_type = PayloadValueType::FLOAT;
        out_payload.value_float = temperature;
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

const char* SimTemperatureDevice::id() const {
    return DEVICE_ID;
}

const char* SimTemperatureDevice::type() const {
    return DEVICE_TYPE;
}

void SimTemperatureDevice::fillBasePayload(uint32_t now_ms, CapabilityPayload& out_payload) const {
    out_payload.capability_id = CAP_TEMPERATURE;
    out_payload.schema_version = 1;
    out_payload.timestamp_ms = now_ms;
    out_payload.available = Availability::UNAVAILABLE;
    out_payload.stale = StaleState::FRESH;
    out_payload.value_type = PayloadValueType::NONE;
    out_payload.value_float = 0.0F;
    out_payload.value_int = 0;
    out_payload.unit = "degree_celsius";
    out_payload.quality = "unavailable";
    out_payload.error_code = ERR_DEVICE_TIMEOUT;
}

}  // namespace Cyber32
