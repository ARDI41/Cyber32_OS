#include "vertical_slice_validation.h"

#include "../../core/ids/capability_ids.h"
#include "../../logic/logic_status.h"

namespace Cyber32 {

VerticalSliceValidation::VerticalSliceValidation()
    : service_task_context_(),
      logic_task_context_(),
      distance_service_task_context_(),
      distance_logic_task_context_(),
      servo_service_state_task_context_(),
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

    servo_driver_.begin();
    if (!servo_device_.begin(&servo_driver_)) {
        return fail("servo_device_begin_failed");
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

    PnpModuleInfo servo_module_info;
    if (!pnp_discovery_.discoverSimulatedServoModule(servo_module_info)) {
        return fail("servo_pnp_discovery_failed");
    }

    if (!pnp_registration_.registerModuleInfo(servo_module_info)) {
        return fail("servo_pnp_registration_failed");
    }

    if (!temperature_service_.begin(&registry_, &device_)) {
        return fail("temperature_service_begin_failed");
    }

    if (!distance_service_.begin(&registry_, &distance_device_)) {
        return fail("distance_service_begin_failed");
    }

    if (!servo_service_.begin(&registry_, &servo_device_)) {
        return fail("servo_service_begin_failed");
    }
    servo_service_.attachRuntime(&runtime_);

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
    api_.attachServoService(&servo_service_);
    runtime_.setState(RuntimeState::READY);

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

    if (!servo_service_.updateState(now_ms)) {
        return fail("servo_update_failed");
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

    if (!registry_.getCapabilityPayload(CAP_SERVO_POSITION, payload)) {
        return fail("servo_payload_missing");
    }
    if (!validateServoPayload(payload, 90.0F)) {
        return false;
    }

    if (!validateLogicState()) {
        return false;
    }

    if (!validateApiState()) {
        return false;
    }

    if (!validateServoCommandState(now_ms)) {
        return false;
    }

    if (!validateRegistryResultState()) {
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

    servo_service_state_task_context_.now_ms = now_ms;
    servo_service_state_task_context_.ran = false;
    servo_service_state_task_context_.last_result = false;
    servo_service_state_task_context_.last_error = "not_run";

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

    if (!registry_.getCapabilityPayload(CAP_SERVO_POSITION, payload)) {
        return fail("servo_payload_missing");
    }
    if (!validateServoPayload(payload, 90.0F)) {
        return false;
    }

    if (!validateLogicState()) {
        return false;
    }

    if (!validateApiState()) {
        return false;
    }

    if (!validateServoCommandState(now_ms)) {
        return false;
    }

    if (!validateRegistryResultState()) {
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

void VerticalSliceValidation::runServoServiceStateTask(void* context) {
    ServoServiceStateTaskContext* task_context =
        static_cast<ServoServiceStateTaskContext*>(context);
    if (task_context == 0 || task_context->service == 0) {
        return;
    }

    task_context->ran = true;
    task_context->last_result = task_context->service->updateState(task_context->now_ms);
    task_context->last_error = task_context->last_result ? "none" : "servo_update_failed";
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

    servo_service_state_task_context_.service = &servo_service_;
    servo_service_state_task_context_.now_ms = 0;
    servo_service_state_task_context_.ran = false;
    servo_service_state_task_context_.last_result = false;
    servo_service_state_task_context_.last_error = "not_run";

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

    RuntimeTask servo_service_state_task;
    servo_service_state_task.task_id = "task.servo_service.update_state";
    servo_service_state_task.enabled = true;
    servo_service_state_task.period_ms = 250;
    servo_service_state_task.next_run_ms = 0;
    servo_service_state_task.last_run_ms = 0;
    servo_service_state_task.callback = &VerticalSliceValidation::runServoServiceStateTask;
    servo_service_state_task.context = &servo_service_state_task_context_;

    if (!runtime_.registerTask(servo_service_state_task)) {
        return fail("servo_service_state_task_register_failed");
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
    if (registry_.moduleCount() != 3) {
        return fail("module_count_invalid");
    }
    if (registry_.deviceCount() != 3) {
        return fail("device_count_invalid");
    }
    if (registry_.capabilityCount() != 3) {
        return fail("capability_count_invalid");
    }
    if (registry_.findCapabilityIndex(CAP_TEMPERATURE) == Registry::NOT_FOUND) {
        return fail("temperature_capability_missing");
    }
    if (registry_.findCapabilityIndex(CAP_DISTANCE) == Registry::NOT_FOUND) {
        return fail("distance_capability_missing");
    }
    if (registry_.findCapabilityIndex(CAP_SERVO_POSITION) == Registry::NOT_FOUND) {
        return fail("servo_capability_missing");
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

bool VerticalSliceValidation::validateServoPayload(
    const CapabilityPayload& payload,
    float expected_position) {
    if (payload.available != Availability::AVAILABLE) {
        return fail("servo_unavailable");
    }
    if (payload.value_type != PayloadValueType::FLOAT) {
        return fail("servo_type_invalid");
    }
    if (payload.value_float != expected_position) {
        return fail("servo_value_invalid");
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
    if (!validateDistancePayload(state.payload)) {
        return false;
    }

    if (!api_.getServoPositionState(state)) {
        return fail("api_servo_failed");
    }
    if (!state.ok) {
        return fail("api_servo_not_ok");
    }
    return validateServoPayload(state.payload, 90.0F);
}

bool VerticalSliceValidation::validateServoCommandState(uint32_t now_ms) {
    runtime_.setState(RuntimeState::READY);

    ApiServoCommandRequest command_request;
    command_request.position_degrees = 45.0F;
    command_request.timeout_ms = 1000;

    ApiServoCommandResponse command_response;
    if (!api_.commandServoPosition(now_ms, command_request, command_response)) {
        return fail("api_servo_command_failed");
    }
    if (!command_response.ok) {
        return fail("api_servo_command_not_ok");
    }
    if (command_response.command_state != CommandState::COMPLETED) {
        return fail("api_servo_command_state_invalid");
    }
    if (!command_response.accepted) {
        return fail("api_servo_command_not_accepted");
    }
    if (!command_response.executed) {
        return fail("api_servo_command_not_executed");
    }

    ApiCommandStateResponse command_state_response;
    if (!api_.getServoCommandState(command_state_response)) {
        return fail("api_servo_command_state_read_failed");
    }
    if (!command_state_response.ok) {
        return fail("api_servo_command_state_not_ok");
    }
    if (!isSameText(command_state_response.capability_id, CAP_SERVO_POSITION)) {
        return fail("api_servo_command_state_capability_invalid");
    }
    if (command_state_response.command_state != CommandState::COMPLETED) {
        return fail("api_servo_command_state_not_completed");
    }
    if (command_state_response.value_float != 45.0F) {
        return fail("api_servo_command_state_value_invalid");
    }
    if (!isSameText(command_state_response.error_code, "none")) {
        return fail("api_servo_command_state_error_invalid");
    }

    ApiCapabilityState servo_state;
    if (!api_.getServoPositionState(servo_state)) {
        return fail("api_servo_after_command_failed");
    }
    if (!validateServoPayload(servo_state.payload, 45.0F)) {
        return false;
    }

    command_request.position_degrees = 999.0F;
    command_request.timeout_ms = 1000;
    if (api_.commandServoPosition(now_ms, command_request, command_response)) {
        return fail("api_servo_invalid_command_succeeded");
    }
    if (command_response.ok) {
        return fail("api_servo_invalid_command_ok");
    }
    if (command_response.accepted) {
        return fail("api_servo_invalid_command_accepted");
    }
    if (command_response.executed) {
        return fail("api_servo_invalid_command_executed");
    }
    if (command_response.command_state != CommandState::FAILED) {
        return fail("api_servo_invalid_command_state");
    }

    if (!api_.getServoCommandState(command_state_response)) {
        return fail("api_servo_invalid_command_state_read_failed");
    }
    if (!command_state_response.ok) {
        return fail("api_servo_invalid_command_state_not_ok");
    }
    if (!isSameText(command_state_response.capability_id, CAP_SERVO_POSITION)) {
        return fail("api_servo_invalid_command_capability_invalid");
    }
    if (command_state_response.command_state != CommandState::FAILED) {
        return fail("api_servo_invalid_command_state_invalid");
    }
    if (command_state_response.value_float != 999.0F) {
        return fail("api_servo_invalid_command_value_invalid");
    }
    if (isSameText(command_state_response.error_code, "none")) {
        return fail("api_servo_invalid_command_error_invalid");
    }

    if (!api_.getServoPositionState(servo_state)) {
        return fail("api_servo_after_invalid_failed");
    }
    if (!validateServoPayload(servo_state.payload, 45.0F)) {
        return false;
    }

    if (!validateServoRuntimeBlockedState(RuntimeState::BOOTING, now_ms, 45.0F)) {
        return false;
    }
    if (!validateServoRuntimeBlockedState(RuntimeState::INITIALIZING, now_ms, 45.0F)) {
        return false;
    }
    if (!validateServoRuntimeBlockedState(RuntimeState::DISCOVERING, now_ms, 45.0F)) {
        return false;
    }
    if (!validateServoRuntimeBlockedState(RuntimeState::REGISTERING, now_ms, 45.0F)) {
        return false;
    }
    if (!validateServoRuntimeBlockedState(RuntimeState::STARTING, now_ms, 45.0F)) {
        return false;
    }
    if (!validateServoRuntimeBlockedState(RuntimeState::ERROR_STATE, now_ms, 45.0F)) {
        return false;
    }
    if (!validateServoRuntimeBlockedState(RuntimeState::SAFE_MODE, now_ms, 45.0F)) {
        return false;
    }

    runtime_.setState(RuntimeState::RUNNING);
    command_request.position_degrees = 90.0F;
    command_request.timeout_ms = 1000;
    if (!api_.commandServoPosition(now_ms, command_request, command_response)) {
        return fail("api_servo_running_command_failed");
    }
    if (!command_response.ok) {
        return fail("api_servo_running_command_not_ok");
    }
    if (command_response.command_state != CommandState::COMPLETED) {
        return fail("api_servo_running_command_state_invalid");
    }
    if (!command_response.accepted || !command_response.executed) {
        return fail("api_servo_running_command_flags_invalid");
    }
    if (!api_.getServoPositionState(servo_state)) {
        return fail("api_servo_running_state_failed");
    }
    return validateServoPayload(servo_state.payload, 90.0F);
}

bool VerticalSliceValidation::validateServoRuntimeBlockedState(
    RuntimeState state,
    uint32_t now_ms,
    float expected_position) {
    runtime_.setState(state);

    ApiServoCommandRequest command_request;
    command_request.position_degrees = 90.0F;
    command_request.timeout_ms = 1000;

    ApiServoCommandResponse command_response;
    if (api_.commandServoPosition(now_ms, command_request, command_response)) {
        return fail("api_servo_blocked_command_succeeded");
    }
    if (command_response.ok) {
        return fail("api_servo_blocked_command_ok");
    }
    if (command_response.command_state != CommandState::FAILED) {
        return fail("api_servo_blocked_command_state_invalid");
    }
    if (command_response.accepted) {
        return fail("api_servo_blocked_command_accepted");
    }
    if (command_response.executed) {
        return fail("api_servo_blocked_command_executed");
    }
    if (isSameText(command_response.error_code, "none")) {
        return fail("api_servo_blocked_command_error_invalid");
    }

    ApiCommandStateResponse command_state_response;
    if (!api_.getServoCommandState(command_state_response)) {
        return fail("api_servo_blocked_command_state_read_failed");
    }
    if (!command_state_response.ok) {
        return fail("api_servo_blocked_command_state_not_ok");
    }
    if (command_state_response.command_state != CommandState::FAILED) {
        return fail("api_servo_blocked_latest_state_invalid");
    }

    ApiCapabilityState servo_state;
    if (!api_.getServoPositionState(servo_state)) {
        return fail("api_servo_blocked_payload_read_failed");
    }
    return validateServoPayload(servo_state.payload, expected_position);
}

bool VerticalSliceValidation::validateRuntimeTaskState() {
    if (runtime_.taskCount() < 5) {
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
    if (!servo_service_state_task_context_.ran) {
        return fail("servo_service_state_task_not_run");
    }
    if (!servo_service_state_task_context_.last_result) {
        return fail(servo_service_state_task_context_.last_error);
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

bool VerticalSliceValidation::validateRegistryResultState() {
    CapabilityPayload temperature_payload;
    if (registry_.getCapabilityPayloadWithResult(CAP_TEMPERATURE, temperature_payload) !=
        RegistryResult::OK) {
        return fail("temperature_result_not_ok");
    }

    CapabilityPayload distance_payload;
    if (registry_.getCapabilityPayloadWithResult(CAP_DISTANCE, distance_payload) !=
        RegistryResult::OK) {
        return fail("distance_result_not_ok");
    }

    CapabilityPayload servo_payload;
    if (registry_.getCapabilityPayloadWithResult(CAP_SERVO_POSITION, servo_payload) !=
        RegistryResult::OK) {
        return fail("servo_result_not_ok");
    }

    CapabilityPayload missing_payload;
    if (registry_.getCapabilityPayloadWithResult("CAP_MISSING_TEST", missing_payload) !=
        RegistryResult::NOT_FOUND) {
        return fail("missing_get_result_invalid");
    }

    CapabilityRecord duplicate_record;
    duplicate_record.capability_id = CAP_DISTANCE;
    duplicate_record.category = "sensors";
    duplicate_record.kind = "sensor";
    duplicate_record.data_type = PayloadValueType::FLOAT;
    duplicate_record.access = "read";
    duplicate_record.status = RecordStatus::AVAILABLE;
    duplicate_record.owner_device_index = 1;
    duplicate_record.latest_payload = distance_payload;

    if (registry_.registerCapabilityWithResult(duplicate_record).result !=
        RegistryResult::DUPLICATE_ID) {
        return fail("duplicate_capability_result_invalid");
    }

    CapabilityPayload missing_update_payload = distance_payload;
    missing_update_payload.capability_id = "CAP_MISSING_TEST";
    if (registry_.updateCapabilityPayloadWithResult("CAP_MISSING_TEST", missing_update_payload) !=
        RegistryResult::NOT_FOUND) {
        return fail("missing_update_result_invalid");
    }

    return true;
}

bool VerticalSliceValidation::isSameText(const char* left, const char* right) const {
    if (left == 0 || right == 0) {
        return false;
    }

    while (*left != '\0' && *right != '\0') {
        if (*left != *right) {
            return false;
        }
        ++left;
        ++right;
    }

    return *left == '\0' && *right == '\0';
}

}  // namespace Cyber32
