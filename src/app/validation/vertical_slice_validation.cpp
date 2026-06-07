#include "vertical_slice_validation.h"

#include "../../core/ids/capability_ids.h"
#include "../../logic/logic_status.h"

namespace Cyber32 {

VerticalSliceValidation::VerticalSliceValidation()
    : passed_(false),
      last_error_("not_run") {
}

bool VerticalSliceValidation::begin() {
    passed_ = false;
    last_error_ = "begin_failed";

    event_bus_.clear();

    registry_.begin();
    registry_.attachEventBus(&event_bus_);

    runtime_.begin();
    runtime_.attachEventBus(&event_bus_);
    runtime_.attachRegistry(&registry_);

    hal_time_.begin();

    driver_.begin();
    if (!device_.begin(&driver_)) {
        return fail("device_begin_failed");
    }

    pnp_discovery_.attachEventBus(&event_bus_);

    PnpModuleInfo module_info;
    if (!pnp_discovery_.discoverSimulatedTemperatureModule(module_info)) {
        return fail("pnp_discovery_failed");
    }

    if (!pnp_registration_.begin(&registry_)) {
        return fail("pnp_registration_begin_failed");
    }
    if (!pnp_registration_.registerModuleInfo(module_info)) {
        return fail("pnp_registration_failed");
    }

    if (!temperature_service_.begin(&registry_, &device_)) {
        return fail("temperature_service_begin_failed");
    }

    if (!temperature_logic_.begin(&registry_)) {
        return fail("temperature_logic_begin_failed");
    }

    if (!api_.begin(&registry_, &runtime_)) {
        return fail("api_begin_failed");
    }

    last_error_ = "none";
    return true;
}

bool VerticalSliceValidation::runOnce(uint32_t now_ms) {
    passed_ = false;

    if (!temperature_service_.update(now_ms)) {
        return fail("temperature_update_failed");
    }

    if (!temperature_logic_.evaluate()) {
        return fail("logic_evaluate_failed");
    }

    if (!validateRegistryState()) {
        return false;
    }

    CapabilityPayload payload;
    if (!registry_.getCapabilityPayload(CAP_TEMPERATURE, payload)) {
        return fail("capability_payload_missing");
    }
    if (!validateTemperaturePayload(payload)) {
        return false;
    }

    if (!validateLogicState()) {
        return false;
    }

    if (!validateApiState()) {
        return false;
    }

    passed_ = true;
    last_error_ = "none";
    return true;
}

bool VerticalSliceValidation::passed() const {
    return passed_;
}

const char* VerticalSliceValidation::lastError() const {
    return last_error_;
}

bool VerticalSliceValidation::fail(const char* error) {
    passed_ = false;
    last_error_ = error;
    return false;
}

bool VerticalSliceValidation::validateRegistryState() {
    if (registry_.moduleCount() != 1) {
        return fail("module_count_invalid");
    }
    if (registry_.deviceCount() != 1) {
        return fail("device_count_invalid");
    }
    if (registry_.capabilityCount() != 1) {
        return fail("capability_count_invalid");
    }
    if (registry_.findCapabilityIndex(CAP_TEMPERATURE) == Registry::NOT_FOUND) {
        return fail("capability_missing");
    }
    return true;
}

bool VerticalSliceValidation::validateTemperaturePayload(const CapabilityPayload& payload) {
    if (payload.available != Availability::AVAILABLE) {
        return fail("temperature_unavailable");
    }
    if (payload.value_type != PayloadValueType::FLOAT) {
        return fail("temperature_type_invalid");
    }
    if (payload.value_float != 22.4F) {
        return fail("temperature_value_invalid");
    }
    return true;
}

bool VerticalSliceValidation::validateLogicState() {
    if (temperature_logic_.status() != LogicStatus::TEMPERATURE_SEEN) {
        return fail("logic_status_invalid");
    }
    return true;
}

bool VerticalSliceValidation::validateApiState() {
    ApiCapabilityState state;
    if (!api_.getTemperatureState(state)) {
        return fail("api_temperature_failed");
    }
    if (!state.ok) {
        return fail("api_temperature_not_ok");
    }
    return validateTemperaturePayload(state.payload);
}

}  // namespace Cyber32
