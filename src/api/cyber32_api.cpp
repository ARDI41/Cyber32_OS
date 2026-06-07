#include "cyber32_api.h"

#include "../core/ids/capability_ids.h"
#include "../core/ids/error_ids.h"

namespace Cyber32 {

Cyber32Api::Cyber32Api()
    : registry_(0),
      runtime_(0) {
}

bool Cyber32Api::begin(Registry* registry, Runtime* runtime) {
    if (registry == 0 || runtime == 0) {
        return false;
    }

    registry_ = registry;
    runtime_ = runtime;
    return true;
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

    if (!registry_->getCapabilityPayload(CAP_TEMPERATURE, out_state.payload)) {
        fillUnavailableTemperatureState(out_state);
        return false;
    }

    out_state.ok = true;
    out_state.error_code = "none";
    return true;
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

}  // namespace Cyber32
