#include "vertical_slice_validation.h"

#include "../../core/ids/capability_ids.h"
#include "../../logic/logic_status.h"

namespace Cyber32 {

VerticalSliceValidation::VerticalSliceValidation()
    : service_task_context_(),
      logic_task_context_(),
      distance_service_task_context_(),
      distance_logic_task_context_(),
      passed_(false),
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

    distance_driver_.begin();
    if (!distance_device_.begin(&distance_driver_)) {
        return fail("distance_device_begin_failed");
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

    PnpModuleInfo distance_module_info;
    if (!pnp_discovery_.discoverSimulatedDistanceModule(distance_module_info)) {
        return fail("distance_pnp_discovery_failed");
    }

    if (!pnp_registration_.registerModuleInfo(distance_module_info)) {
        return fail("distance_pnp_registration_failed");
    }

    if (!temperature_service_.begin(&registry_, &device_)) {
        return fail("temperature_service_begin_failed");
    }

    if (!distance_service_.begin(&registry_, &distance_device_)) {
        return fail("distance_service_begin_failed");
    }

    if (!temperature_logic_.begin(&registry_)) {
        return fail("temperature_logic_begin_failed");
    }

    if (!distance_logic_.begin(&registry_)) {
        return fail("distance_logic_begin_failed");
    }

    if (!registerRuntimeTasks()) {
        return false;
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

    if (!distance_service_.update(now_ms)) {
        return fail("distance_update_failed");
    }

    if (!temperature_logic_.evaluate()) {
        return fail("logic_evaluate_failed");
    }

    if (!distance_logic_.evaluate()) {
        return fail("distance_logic_evaluate_failed");
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

    if (!registry_.getCapabilityPayload(CAP_DISTANCE, payload)) {
        return fail("distance_payload_missing");
    }
    if (!validateDistancePayload(payload)) {
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

bool VerticalSliceValidation::runOnceWithRuntime(uint32_t now_ms) {
    passed_ = false;

    service_task_context_.now_ms = now_ms;
    service_task_context_.ran = false;
    service_task_context_.last_result = false;
    service_task_context_.last_error = "not_run";

    distance_service_task_context_.now_ms = now_ms;
    distance_service_task_context_.ran = false;
    distance_service_task_context_.last_result = false;
    distance_service_task_context_.last_error = "not_run";

    logic_task_context_.ran = false;
    logic_task_context_.last_result = false;
    logic_task_context_.last_error = "not_run";

    distance_logic_task_context_.ran = false;
    distance_logic_task_context_.last_result = false;
    distance_logic_task_context_.last_error = "not_run";

    runtime_.update(now_ms);

    if (!validateRuntimeTaskState()) {
        return false;
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

    if (!registry_.getCapabilityPayload(CAP_DISTANCE, payload)) {
        return fail("distance_payload_missing");
    }
    if (!validateDistancePayload(payload)) {
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

void VerticalSliceValidation::runTemperatureServiceTask(void* context) {
    TemperatureServiceTaskContext* task_context =
        static_cast<TemperatureServiceTaskContext*>(context);
    if (task_context == 0 || task_context->service == 0) {
        return;
    }

    task_context->ran = true;
    task_context->last_result = task_context->service->update(task_context->now_ms);
    task_context->last_error = task_context->last_result ? "none" : "temperature_update_failed";
}

void VerticalSliceValidation::runTemperatureLogicTask(void* context) {
    TemperatureLogicTaskContext* task_context =
        static_cast<TemperatureLogicTaskContext*>(context);
    if (task_context == 0 || task_context->logic == 0) {
        return;
    }

    task_context->ran = true;
    task_context->last_result = task_context->logic->evaluate();
    task_context->last_error = task_context->last_result ? "none" : "logic_evaluate_failed";
}

void VerticalSliceValidation::runDistanceServiceTask(void* context) {
    DistanceServiceTaskContext* task_context =
        static_cast<DistanceServiceTaskContext*>(context);
    if (task_context == 0 || task_context->service == 0) {
        return;
    }

    task_context->ran = true;
    task_context->last_result = task_context->service->update(task_context->now_ms);
    task_context->last_error = task_context->last_result ? "none" : "distance_update_failed";
}

void VerticalSliceValidation::runDistanceLogicTask(void* context) {
    DistanceLogicTaskContext* task_context =
        static_cast<DistanceLogicTaskContext*>(context);
    if (task_context == 0 || task_context->logic == 0) {
        return;
    }

    task_context->ran = true;
    task_context->last_result = task_context->logic->evaluate();
    task_context->last_error = task_context->last_result ? "none" : "distance_logic_evaluate_failed";
}

bool VerticalSliceValidation::registerRuntimeTasks() {
    service_task_context_.service = &temperature_service_;
    service_task_context_.now_ms = 0;
    service_task_context_.ran = false;
    service_task_context_.last_result = false;
    service_task_context_.last_error = "not_run";

    logic_task_context_.logic = &temperature_logic_;
    logic_task_context_.ran = false;
    logic_task_context_.last_result = false;
    logic_task_context_.last_error = "not_run";

    distance_service_task_context_.service = &distance_service_;
    distance_service_task_context_.now_ms = 0;
    distance_service_task_context_.ran = false;
    distance_service_task_context_.last_result = false;
    distance_service_task_context_.last_error = "not_run";

    distance_logic_task_context_.logic = &distance_logic_;
    distance_logic_task_context_.ran = false;
    distance_logic_task_context_.last_result = false;
    distance_logic_task_context_.last_error = "not_run";

    RuntimeTask service_task;
    service_task.task_id = "task.temperature_service.update";
    service_task.enabled = true;
    service_task.period_ms = 1000;
    service_task.next_run_ms = 0;
    service_task.last_run_ms = 0;
    service_task.callback = &VerticalSliceValidation::runTemperatureServiceTask;
    service_task.context = &service_task_context_;

    if (!runtime_.registerTask(service_task)) {
        return fail("service_task_register_failed");
    }

    RuntimeTask distance_service_task;
    distance_service_task.task_id = "task.distance_service.update";
    distance_service_task.enabled = true;
    distance_service_task.period_ms = 250;
    distance_service_task.next_run_ms = 0;
    distance_service_task.last_run_ms = 0;
    distance_service_task.callback = &VerticalSliceValidation::runDistanceServiceTask;
    distance_service_task.context = &distance_service_task_context_;

    if (!runtime_.registerTask(distance_service_task)) {
        return fail("distance_service_task_register_failed");
    }

    RuntimeTask logic_task;
    logic_task.task_id = "task.temperature_logic.evaluate";
    logic_task.enabled = true;
    logic_task.period_ms = 1000;
    logic_task.next_run_ms = 0;
    logic_task.last_run_ms = 0;
    logic_task.callback = &VerticalSliceValidation::runTemperatureLogicTask;
    logic_task.context = &logic_task_context_;

    if (!runtime_.registerTask(logic_task)) {
        return fail("logic_task_register_failed");
    }

    RuntimeTask distance_logic_task;
    distance_logic_task.task_id = "task.distance_logic.evaluate";
    distance_logic_task.enabled = true;
    distance_logic_task.period_ms = 250;
    distance_logic_task.next_run_ms = 0;
    distance_logic_task.last_run_ms = 0;
    distance_logic_task.callback = &VerticalSliceValidation::runDistanceLogicTask;
    distance_logic_task.context = &distance_logic_task_context_;

    if (!runtime_.registerTask(distance_logic_task)) {
        return fail("distance_logic_task_register_failed");
    }

    return true;
}

bool VerticalSliceValidation::fail(const char* error) {
    passed_ = false;
    last_error_ = error;
    return false;
}

bool VerticalSliceValidation::validateRegistryState() {
    if (registry_.moduleCount() != 2) {
        return fail("module_count_invalid");
    }
    if (registry_.deviceCount() != 2) {
        return fail("device_count_invalid");
    }
    if (registry_.capabilityCount() != 2) {
        return fail("capability_count_invalid");
    }
    if (registry_.findCapabilityIndex(CAP_TEMPERATURE) == Registry::NOT_FOUND) {
        return fail("temperature_capability_missing");
    }
    if (registry_.findCapabilityIndex(CAP_DISTANCE) == Registry::NOT_FOUND) {
        return fail("distance_capability_missing");
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

bool VerticalSliceValidation::validateDistancePayload(const CapabilityPayload& payload) {
    if (payload.available != Availability::AVAILABLE) {
        return fail("distance_unavailable");
    }
    if (payload.value_type != PayloadValueType::FLOAT) {
        return fail("distance_type_invalid");
    }
    if (payload.value_float != 1.25F) {
        return fail("distance_value_invalid");
    }
    return true;
}

bool VerticalSliceValidation::validateLogicState() {
    if (temperature_logic_.status() != LogicStatus::TEMPERATURE_SEEN) {
        return fail("logic_status_invalid");
    }
    if (distance_logic_.status() != LogicStatus::DISTANCE_SEEN) {
        return fail("distance_logic_status_invalid");
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
    if (!validateTemperaturePayload(state.payload)) {
        return false;
    }

    if (!api_.getDistanceState(state)) {
        return fail("api_distance_failed");
    }
    if (!state.ok) {
        return fail("api_distance_not_ok");
    }
    return validateDistancePayload(state.payload);
}

bool VerticalSliceValidation::validateRuntimeTaskState() {
    if (runtime_.taskCount() < 4) {
        return fail("runtime_task_count_invalid");
    }
    if (!service_task_context_.ran) {
        return fail("service_task_not_run");
    }
    if (!service_task_context_.last_result) {
        return fail(service_task_context_.last_error);
    }
    if (!distance_service_task_context_.ran) {
        return fail("distance_service_task_not_run");
    }
    if (!distance_service_task_context_.last_result) {
        return fail(distance_service_task_context_.last_error);
    }
    if (!logic_task_context_.ran) {
        return fail("logic_task_not_run");
    }
    if (!logic_task_context_.last_result) {
        return fail(logic_task_context_.last_error);
    }
    if (!distance_logic_task_context_.ran) {
        return fail("distance_logic_task_not_run");
    }
    if (!distance_logic_task_context_.last_result) {
        return fail(distance_logic_task_context_.last_error);
    }
    return true;
}

}  // namespace Cyber32
