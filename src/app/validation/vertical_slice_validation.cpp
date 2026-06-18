#include "vertical_slice_validation.h"

#include "../../core/ids/capability_ids.h"
#include "../../core/ids/error_ids.h"
#include "../../logic/logic_status.h"

namespace Cyber32 {

VerticalSliceValidation::VerticalSliceValidation()
    : service_task_context_(),
      logic_task_context_(),
      distance_service_task_context_(),
      distance_logic_task_context_(),
      servo_service_state_task_context_(),
      motor_service_state_task_context_(),
      motor_service_command_task_context_(),
      relay_service_state_task_context_(),
      relay_service_command_task_context_(),
      wireless_service_process_task_context_(),
      wireless_service_timeout_task_context_(),
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

    motor_driver_.begin();
    if (!motor_device_.begin(&motor_driver_)) {
        return fail("motor_device_begin_failed");
    }

    relay_driver_.begin();
    if (!relay_device_.begin(&relay_driver_)) {
        return fail("relay_device_begin_failed");
    }

    wireless_transport_driver_.begin();
    if (!wireless_temperature_device_.begin(1001)) {
        return fail("wireless_temperature_device_begin_failed");
    }
    wireless_service_.begin();
    wireless_service_.attachRegistry(&registry_);
    wireless_service_.attachTransportDriver(&wireless_transport_driver_);
    wireless_service_.attachWirelessTemperatureDevice(&wireless_temperature_device_);

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

    PnpModuleInfo motor_module_info;
    if (!pnp_discovery_.discoverSimulatedMotorModule(motor_module_info)) {
        return fail("motor_pnp_discovery_failed");
    }

    if (!pnp_registration_.registerModuleInfo(motor_module_info)) {
        return fail("motor_pnp_registration_failed");
    }

    PnpModuleInfo relay_module_info;
    if (!pnp_discovery_.discoverSimulatedRelayModule(relay_module_info)) {
        return fail("relay_pnp_discovery_failed");
    }

    if (!pnp_registration_.registerModuleInfo(relay_module_info)) {
        return fail("relay_pnp_registration_failed");
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

    if (!motor_service_.begin(&registry_, &motor_device_)) {
        return fail("motor_service_begin_failed");
    }
    motor_service_.attachRuntime(&runtime_);

    if (!relay_service_.begin(&registry_, &relay_device_)) {
        return fail("relay_service_begin_failed");
    }
    relay_service_.attachRuntime(&runtime_);

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
    api_.attachMotorService(&motor_service_);
    api_.attachRelayService(&relay_service_);
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

    if (!motor_service_.updateState(now_ms)) {
        return fail("motor_update_failed");
    }

    if (!relay_service_.updateState(now_ms)) {
        return fail("relay_update_failed");
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

    if (!validateRuntimeSafeModeHelpers()) {
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

    if (!registry_.getCapabilityPayload(CAP_MOTOR_CONTROL, payload)) {
        return fail("motor_payload_missing");
    }
    if (!validateMotorPayload(payload, 0.0F, MotorDirection::STOP)) {
        return false;
    }

    if (!registry_.getCapabilityPayload(CAP_RELAY_CONTROL, payload)) {
        return fail("relay_payload_missing");
    }
    if (!validateRelayPayload(payload, false)) {
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

    if (!validateMotorCommandState(now_ms)) {
        return false;
    }

    if (!validateRelayCommandState(now_ms)) {
        return false;
    }

    if (!validateRegistryResultState()) {
        return false;
    }

    if (!validateCapabilityProviderStorage(now_ms)) {
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

    motor_service_state_task_context_.now_ms = now_ms;
    motor_service_state_task_context_.ran = false;
    motor_service_state_task_context_.last_result = false;
    motor_service_state_task_context_.last_error = "not_run";

    motor_service_command_task_context_.now_ms = now_ms;
    motor_service_command_task_context_.ran = false;
    motor_service_command_task_context_.last_result = false;
    motor_service_command_task_context_.last_error = "not_run";

    relay_service_state_task_context_.now_ms = now_ms;
    relay_service_state_task_context_.ran = false;
    relay_service_state_task_context_.last_result = false;
    relay_service_state_task_context_.last_error = "not_run";

    relay_service_command_task_context_.now_ms = now_ms;
    relay_service_command_task_context_.ran = false;
    relay_service_command_task_context_.last_result = false;
    relay_service_command_task_context_.last_error = "not_run";

    wireless_service_process_task_context_.now_ms = now_ms;
    wireless_service_process_task_context_.ran = false;
    wireless_service_process_task_context_.last_result = false;
    wireless_service_process_task_context_.last_error = "not_run";

    wireless_service_timeout_task_context_.now_ms = now_ms;
    wireless_service_timeout_task_context_.ran = false;
    wireless_service_timeout_task_context_.last_result = false;
    wireless_service_timeout_task_context_.last_error = "not_run";

    runtime_.update(now_ms);

    if (!validateRuntimeTaskState()) {
        return false;
    }

    if (!validateRegistryState()) {
        return false;
    }

    if (!validateRuntimeSafeModeHelpers()) {
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

    if (!registry_.getCapabilityPayload(CAP_MOTOR_CONTROL, payload)) {
        return fail("motor_payload_missing");
    }
    if (!validateMotorPayload(payload, 0.0F, MotorDirection::STOP)) {
        return false;
    }

    if (!registry_.getCapabilityPayload(CAP_RELAY_CONTROL, payload)) {
        return fail("relay_payload_missing");
    }
    if (!validateRelayPayload(payload, false)) {
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

    if (!validateMotorCommandState(now_ms)) {
        return false;
    }

    if (!validateRelayCommandState(now_ms)) {
        return false;
    }

    if (!validateRegistryResultState()) {
        return false;
    }

    if (!validateCapabilityProviderStorage(now_ms)) {
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

void VerticalSliceValidation::runMotorServiceStateTask(void* context) {
    MotorServiceStateTaskContext* task_context =
        static_cast<MotorServiceStateTaskContext*>(context);
    if (task_context == 0 || task_context->service == 0) {
        return;
    }

    task_context->ran = true;
    task_context->last_result = task_context->service->updateState(task_context->now_ms);
    task_context->last_error = task_context->last_result ? "none" : "motor_update_failed";
}

void VerticalSliceValidation::runMotorServiceCommandTask(void* context) {
    MotorServiceCommandTaskContext* task_context =
        static_cast<MotorServiceCommandTaskContext*>(context);
    if (task_context == 0 || task_context->service == 0) {
        return;
    }

    task_context->ran = true;
    task_context->last_result = task_context->service->executePendingCommand(task_context->now_ms);
    task_context->last_error = task_context->last_result ? "none" : "motor_command_execute_failed";
}

void VerticalSliceValidation::runRelayServiceStateTask(void* context) {
    RelayServiceStateTaskContext* task_context =
        static_cast<RelayServiceStateTaskContext*>(context);
    if (task_context == 0 || task_context->service == 0) {
        return;
    }

    task_context->ran = true;
    task_context->last_result = task_context->service->updateState(task_context->now_ms);
    task_context->last_error = task_context->last_result ? "none" : "relay_update_failed";
}

void VerticalSliceValidation::runRelayServiceCommandTask(void* context) {
    RelayServiceCommandTaskContext* task_context =
        static_cast<RelayServiceCommandTaskContext*>(context);
    if (task_context == 0 || task_context->service == 0) {
        return;
    }

    task_context->ran = true;
    task_context->last_result = task_context->service->executePendingCommand(task_context->now_ms);
    task_context->last_error = task_context->last_result ? "none" : "relay_command_execute_failed";
}

void VerticalSliceValidation::runWirelessServiceProcessTask(void* context) {
    WirelessServiceProcessTaskContext* task_context =
        static_cast<WirelessServiceProcessTaskContext*>(context);
    if (task_context == 0 || task_context->service == 0) {
        return;
    }

    task_context->ran = true;
    task_context->last_result = task_context->service->processPackets(task_context->now_ms);
    task_context->last_error =
        task_context->last_result ? "none" : task_context->service->lastErrorCode();
}

void VerticalSliceValidation::runWirelessServiceTimeoutTask(void* context) {
    WirelessServiceTimeoutTaskContext* task_context =
        static_cast<WirelessServiceTimeoutTaskContext*>(context);
    if (task_context == 0 || task_context->service == 0) {
        return;
    }

    task_context->ran = true;
    task_context->last_result = task_context->service->checkTimeouts(task_context->now_ms);
    task_context->last_error =
        task_context->last_result ? "none" : task_context->service->lastErrorCode();
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

    motor_service_state_task_context_.service = &motor_service_;
    motor_service_state_task_context_.now_ms = 0;
    motor_service_state_task_context_.ran = false;
    motor_service_state_task_context_.last_result = false;
    motor_service_state_task_context_.last_error = "not_run";

    motor_service_command_task_context_.service = &motor_service_;
    motor_service_command_task_context_.now_ms = 0;
    motor_service_command_task_context_.ran = false;
    motor_service_command_task_context_.last_result = false;
    motor_service_command_task_context_.last_error = "not_run";

    relay_service_state_task_context_.service = &relay_service_;
    relay_service_state_task_context_.now_ms = 0;
    relay_service_state_task_context_.ran = false;
    relay_service_state_task_context_.last_result = false;
    relay_service_state_task_context_.last_error = "not_run";

    relay_service_command_task_context_.service = &relay_service_;
    relay_service_command_task_context_.now_ms = 0;
    relay_service_command_task_context_.ran = false;
    relay_service_command_task_context_.last_result = false;
    relay_service_command_task_context_.last_error = "not_run";

    wireless_service_process_task_context_.service = &wireless_service_;
    wireless_service_process_task_context_.now_ms = 0;
    wireless_service_process_task_context_.ran = false;
    wireless_service_process_task_context_.last_result = false;
    wireless_service_process_task_context_.last_error = "not_run";

    wireless_service_timeout_task_context_.service = &wireless_service_;
    wireless_service_timeout_task_context_.now_ms = 0;
    wireless_service_timeout_task_context_.ran = false;
    wireless_service_timeout_task_context_.last_result = false;
    wireless_service_timeout_task_context_.last_error = "not_run";

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

    RuntimeTask motor_service_state_task;
    motor_service_state_task.task_id = "task.motor_service.update_state";
    motor_service_state_task.enabled = true;
    motor_service_state_task.period_ms = 250;
    motor_service_state_task.next_run_ms = 0;
    motor_service_state_task.last_run_ms = 0;
    motor_service_state_task.callback = &VerticalSliceValidation::runMotorServiceStateTask;
    motor_service_state_task.context = &motor_service_state_task_context_;

    if (!runtime_.registerTask(motor_service_state_task)) {
        return fail("motor_service_state_task_register_failed");
    }

    RuntimeTask motor_service_command_task;
    motor_service_command_task.task_id = "task.motor_service.execute_command";
    motor_service_command_task.enabled = true;
    motor_service_command_task.period_ms = 10;
    motor_service_command_task.next_run_ms = 0;
    motor_service_command_task.last_run_ms = 0;
    motor_service_command_task.callback = &VerticalSliceValidation::runMotorServiceCommandTask;
    motor_service_command_task.context = &motor_service_command_task_context_;

    if (!runtime_.registerTask(motor_service_command_task)) {
        return fail("motor_service_command_task_register_failed");
    }

    RuntimeTask relay_service_state_task;
    relay_service_state_task.task_id = "task.relay_service.update_state";
    relay_service_state_task.enabled = true;
    relay_service_state_task.period_ms = 250;
    relay_service_state_task.next_run_ms = 0;
    relay_service_state_task.last_run_ms = 0;
    relay_service_state_task.callback = &VerticalSliceValidation::runRelayServiceStateTask;
    relay_service_state_task.context = &relay_service_state_task_context_;

    if (!runtime_.registerTask(relay_service_state_task)) {
        return fail("relay_service_state_task_register_failed");
    }

    RuntimeTask relay_service_command_task;
    relay_service_command_task.task_id = "task.relay_service.execute_command";
    relay_service_command_task.enabled = true;
    relay_service_command_task.period_ms = 10;
    relay_service_command_task.next_run_ms = 0;
    relay_service_command_task.last_run_ms = 0;
    relay_service_command_task.callback = &VerticalSliceValidation::runRelayServiceCommandTask;
    relay_service_command_task.context = &relay_service_command_task_context_;

    if (!runtime_.registerTask(relay_service_command_task)) {
        return fail("relay_service_command_task_register_failed");
    }

    RuntimeTask wireless_service_process_task;
    wireless_service_process_task.task_id = "task.wireless_service.process_packets";
    wireless_service_process_task.enabled = true;
    wireless_service_process_task.period_ms = 250;
    wireless_service_process_task.next_run_ms = 0;
    wireless_service_process_task.last_run_ms = 0;
    wireless_service_process_task.callback = &VerticalSliceValidation::runWirelessServiceProcessTask;
    wireless_service_process_task.context = &wireless_service_process_task_context_;

    if (!runtime_.registerTask(wireless_service_process_task)) {
        return fail("wireless_service_process_task_register_failed");
    }

    RuntimeTask wireless_service_timeout_task;
    wireless_service_timeout_task.task_id = "task.wireless_service.check_timeouts";
    wireless_service_timeout_task.enabled = true;
    wireless_service_timeout_task.period_ms = 1000;
    wireless_service_timeout_task.next_run_ms = 0;
    wireless_service_timeout_task.last_run_ms = 0;
    wireless_service_timeout_task.callback = &VerticalSliceValidation::runWirelessServiceTimeoutTask;
    wireless_service_timeout_task.context = &wireless_service_timeout_task_context_;

    if (!runtime_.registerTask(wireless_service_timeout_task)) {
        return fail("wireless_service_timeout_task_register_failed");
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
    if (registry_.moduleCount() != 5) {
        return fail("module_count_invalid");
    }
    if (registry_.deviceCount() != 5) {
        return fail("device_count_invalid");
    }
    if (registry_.capabilityCount() != 5) {
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
    if (registry_.findCapabilityIndex(CAP_MOTOR_CONTROL) == Registry::NOT_FOUND) {
        return fail("motor_capability_missing");
    }
    if (registry_.findCapabilityIndex(CAP_RELAY_CONTROL) == Registry::NOT_FOUND) {
        return fail("relay_capability_missing");
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

bool VerticalSliceValidation::validateMotorPayload(
    const CapabilityPayload& payload,
    float expected_speed,
    MotorDirection expected_direction) {
    if (payload.available != Availability::AVAILABLE) {
        return fail("motor_unavailable");
    }
    if (payload.value_type != PayloadValueType::FLOAT) {
        return fail("motor_type_invalid");
    }
    if (payload.value_float != expected_speed) {
        return fail("motor_speed_invalid");
    }
    if (payload.value_int != static_cast<int32_t>(expected_direction)) {
        return fail("motor_direction_invalid");
    }
    return true;
}

bool VerticalSliceValidation::validateRelayPayload(
    const CapabilityPayload& payload,
    bool expected_enabled) {
    if (payload.available != Availability::AVAILABLE) {
        return fail("relay_unavailable");
    }
    if (payload.value_type != PayloadValueType::BOOLEAN) {
        return fail("relay_type_invalid");
    }
    if (payload.value_int != (expected_enabled ? 1 : 0)) {
        return fail("relay_value_invalid");
    }
    if (!isSameText(payload.unit, "boolean")) {
        return fail("relay_unit_invalid");
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
    if (!validateServoPayload(state.payload, 90.0F)) {
        return false;
    }

    if (!api_.getMotorControlState(state)) {
        return fail("api_motor_failed");
    }
    if (!state.ok) {
        return fail("api_motor_not_ok");
    }
    if (!validateMotorPayload(state.payload, 0.0F, MotorDirection::STOP)) {
        return false;
    }

    if (!api_.getRelayControlState(state)) {
        return fail("api_relay_failed");
    }
    if (!state.ok) {
        return fail("api_relay_not_ok");
    }
    return validateRelayPayload(state.payload, false);
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
    if (state == RuntimeState::SAFE_MODE) {
        if (!runtime_.enterSafeMode()) {
            return fail("runtime_enter_safe_mode_failed");
        }
    } else {
        runtime_.setState(state);
    }

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

bool VerticalSliceValidation::validateMotorCommandState(uint32_t now_ms) {
    runtime_.setState(RuntimeState::READY);

    ApiMotorCommandRequest command_request;
    command_request.direction = MotorDirection::FORWARD;
    command_request.speed_percent = 50.0F;
    command_request.timeout_ms = 1000;

    ApiMotorCommandResponse command_response;
    if (!api_.commandMotorControl(now_ms, command_request, command_response)) {
        return fail("api_motor_command_failed");
    }
    if (!command_response.ok) {
        return fail("api_motor_command_not_ok");
    }
    if (command_response.command_state != CommandState::ACCEPTED) {
        return fail("api_motor_command_state_invalid");
    }
    if (!command_response.accepted) {
        return fail("api_motor_command_not_accepted");
    }
    if (command_response.executed) {
        return fail("api_motor_command_executed_too_early");
    }

    ApiCapabilityState motor_state;
    if (!api_.getMotorControlState(motor_state)) {
        return fail("api_motor_after_accept_failed");
    }
    if (!validateMotorPayload(motor_state.payload, 0.0F, MotorDirection::STOP)) {
        return false;
    }

    ApiCommandStateResponse command_state_response;
    if (!api_.getMotorCommandState(command_state_response)) {
        return fail("api_motor_command_state_read_failed");
    }
    if (!command_state_response.ok) {
        return fail("api_motor_command_state_not_ok");
    }
    if (!isSameText(command_state_response.capability_id, CAP_MOTOR_CONTROL)) {
        return fail("api_motor_command_state_capability_invalid");
    }
    if (command_state_response.command_state != CommandState::ACCEPTED) {
        return fail("api_motor_command_state_not_accepted");
    }
    if (command_state_response.value_float != 50.0F) {
        return fail("api_motor_command_state_value_invalid");
    }
    if (command_state_response.value_int != static_cast<int32_t>(MotorDirection::FORWARD)) {
        return fail("api_motor_command_state_direction_invalid");
    }
    if (!isSameText(command_state_response.error_code, "none")) {
        return fail("api_motor_command_state_error_invalid");
    }

    const uint32_t execution_ms = now_ms + 10;
    motor_service_command_task_context_.now_ms = execution_ms;
    motor_service_command_task_context_.ran = false;
    motor_service_command_task_context_.last_result = false;
    motor_service_command_task_context_.last_error = "not_run";
    runtime_.update(execution_ms);

    if (!motor_service_command_task_context_.ran) {
        return fail("motor_command_task_not_run");
    }
    if (!motor_service_command_task_context_.last_result) {
        return fail(motor_service_command_task_context_.last_error);
    }

    if (!api_.getMotorCommandState(command_state_response)) {
        return fail("api_motor_completed_command_state_read_failed");
    }
    if (!command_state_response.ok) {
        return fail("api_motor_completed_command_state_not_ok");
    }
    if (command_state_response.command_state != CommandState::COMPLETED) {
        return fail("api_motor_command_state_not_completed");
    }
    if (command_state_response.value_float != 50.0F) {
        return fail("api_motor_completed_command_state_value_invalid");
    }
    if (command_state_response.value_int != static_cast<int32_t>(MotorDirection::FORWARD)) {
        return fail("api_motor_completed_command_state_direction_invalid");
    }
    if (!isSameText(command_state_response.error_code, "none")) {
        return fail("api_motor_completed_command_state_error_invalid");
    }

    if (!api_.getMotorControlState(motor_state)) {
        return fail("api_motor_after_command_failed");
    }
    if (!validateMotorPayload(motor_state.payload, 50.0F, MotorDirection::FORWARD)) {
        return false;
    }

    if (!api_.commandMotorStop(now_ms, command_response)) {
        return fail("api_motor_stop_failed");
    }
    if (!command_response.ok) {
        return fail("api_motor_stop_not_ok");
    }
    if (command_response.command_state != CommandState::COMPLETED) {
        return fail("api_motor_stop_state_invalid");
    }
    if (!command_response.accepted || !command_response.executed) {
        return fail("api_motor_stop_flags_invalid");
    }
    if (!api_.getMotorControlState(motor_state)) {
        return fail("api_motor_after_stop_failed");
    }
    if (!validateMotorPayload(motor_state.payload, 0.0F, MotorDirection::STOP)) {
        return false;
    }

    command_request.direction = MotorDirection::FORWARD;
    command_request.speed_percent = 40.0F;
    command_request.timeout_ms = 1;
    if (!api_.commandMotorControl(now_ms, command_request, command_response)) {
        return fail("api_motor_timeout_command_accept_failed");
    }
    if (!command_response.ok) {
        return fail("api_motor_timeout_command_not_ok");
    }
    if (command_response.command_state != CommandState::ACCEPTED) {
        return fail("api_motor_timeout_command_state_invalid");
    }
    if (!command_response.accepted) {
        return fail("api_motor_timeout_command_not_accepted");
    }
    if (command_response.executed) {
        return fail("api_motor_timeout_command_executed_too_early");
    }
    if (!isSameText(command_response.error_code, "none")) {
        return fail("api_motor_timeout_accept_error_invalid");
    }

    const uint32_t timeout_execution_ms = now_ms + 20;
    motor_service_command_task_context_.now_ms = timeout_execution_ms;
    motor_service_command_task_context_.ran = false;
    motor_service_command_task_context_.last_result = true;
    motor_service_command_task_context_.last_error = "not_run";
    runtime_.update(timeout_execution_ms);

    if (!motor_service_command_task_context_.ran) {
        return fail("motor_timeout_command_task_not_run");
    }
    if (motor_service_command_task_context_.last_result) {
        return fail("motor_timeout_command_completed");
    }

    if (!api_.getMotorCommandState(command_state_response)) {
        return fail("api_motor_timeout_command_state_read_failed");
    }
    if (!command_state_response.ok) {
        return fail("api_motor_timeout_command_state_not_ok");
    }
    if (command_state_response.command_state != CommandState::TIMED_OUT) {
        return fail("api_motor_timeout_command_state_invalid");
    }
    if (command_state_response.value_float != 40.0F) {
        return fail("api_motor_timeout_command_value_invalid");
    }
    if (command_state_response.value_int != static_cast<int32_t>(MotorDirection::FORWARD)) {
        return fail("api_motor_timeout_command_direction_invalid");
    }
    if (!isSameText(command_state_response.error_code, ERR_COMMAND_TIMEOUT)) {
        return fail("api_motor_timeout_command_error_invalid");
    }

    if (!api_.getMotorControlState(motor_state)) {
        return fail("api_motor_after_timeout_failed");
    }
    if (!validateMotorPayload(motor_state.payload, 0.0F, MotorDirection::STOP)) {
        return false;
    }

    runtime_.setState(RuntimeState::RUNNING);
    command_request.direction = MotorDirection::REVERSE;
    command_request.speed_percent = 25.0F;
    command_request.timeout_ms = 1000;
    if (!api_.commandMotorControl(now_ms, command_request, command_response)) {
        return fail("api_motor_reverse_command_failed");
    }
    if (!command_response.ok) {
        return fail("api_motor_reverse_command_not_ok");
    }
    if (command_response.command_state != CommandState::ACCEPTED) {
        return fail("api_motor_reverse_command_state_invalid");
    }
    if (!command_response.accepted || command_response.executed) {
        return fail("api_motor_reverse_command_flags_invalid");
    }

    const uint32_t reverse_execution_ms = now_ms + 30;
    motor_service_command_task_context_.now_ms = reverse_execution_ms;
    motor_service_command_task_context_.ran = false;
    motor_service_command_task_context_.last_result = false;
    motor_service_command_task_context_.last_error = "not_run";
    runtime_.update(reverse_execution_ms);

    if (!motor_service_command_task_context_.ran) {
        return fail("motor_reverse_command_task_not_run");
    }
    if (!motor_service_command_task_context_.last_result) {
        return fail(motor_service_command_task_context_.last_error);
    }

    if (!api_.getMotorCommandState(command_state_response)) {
        return fail("api_motor_reverse_command_state_read_failed");
    }
    if (!command_state_response.ok) {
        return fail("api_motor_reverse_command_state_not_ok");
    }
    if (command_state_response.command_state != CommandState::COMPLETED) {
        return fail("api_motor_reverse_command_not_completed");
    }
    if (command_state_response.value_float != 25.0F) {
        return fail("api_motor_reverse_command_value_invalid");
    }
    if (command_state_response.value_int != static_cast<int32_t>(MotorDirection::REVERSE)) {
        return fail("api_motor_reverse_command_direction_invalid");
    }

    if (!api_.getMotorControlState(motor_state)) {
        return fail("api_motor_after_reverse_failed");
    }
    if (!validateMotorPayload(motor_state.payload, 25.0F, MotorDirection::REVERSE)) {
        return false;
    }

    if (!api_.commandMotorStop(now_ms, command_response)) {
        return fail("api_motor_stop_after_reverse_failed");
    }
    if (!command_response.ok) {
        return fail("api_motor_stop_after_reverse_not_ok");
    }
    if (command_response.command_state != CommandState::COMPLETED) {
        return fail("api_motor_stop_after_reverse_state_invalid");
    }
    if (!api_.getMotorControlState(motor_state)) {
        return fail("api_motor_after_reverse_stop_failed");
    }
    if (!validateMotorPayload(motor_state.payload, 0.0F, MotorDirection::STOP)) {
        return false;
    }

    motor_driver_.setFailureMode(true);
    command_request.direction = MotorDirection::FORWARD;
    command_request.speed_percent = 30.0F;
    command_request.timeout_ms = 1000;
    if (!api_.commandMotorControl(now_ms, command_request, command_response)) {
        motor_driver_.setFailureMode(false);
        return fail("api_motor_failure_command_accept_failed");
    }
    if (!command_response.ok) {
        motor_driver_.setFailureMode(false);
        return fail("api_motor_failure_command_not_ok");
    }
    if (command_response.command_state != CommandState::ACCEPTED) {
        motor_driver_.setFailureMode(false);
        return fail("api_motor_failure_command_state_invalid");
    }
    if (!command_response.accepted || command_response.executed) {
        motor_driver_.setFailureMode(false);
        return fail("api_motor_failure_command_flags_invalid");
    }

    const uint32_t failure_execution_ms = now_ms + 50;
    motor_service_command_task_context_.now_ms = failure_execution_ms;
    motor_service_command_task_context_.ran = false;
    motor_service_command_task_context_.last_result = true;
    motor_service_command_task_context_.last_error = "not_run";
    runtime_.update(failure_execution_ms);

    motor_driver_.setFailureMode(false);

    if (!motor_service_command_task_context_.ran) {
        return fail("motor_failure_command_task_not_run");
    }
    if (motor_service_command_task_context_.last_result) {
        return fail("motor_failure_command_completed");
    }

    if (!api_.getMotorCommandState(command_state_response)) {
        return fail("api_motor_failure_command_state_read_failed");
    }
    if (!command_state_response.ok) {
        return fail("api_motor_failure_command_state_not_ok");
    }
    if (command_state_response.command_state != CommandState::FAILED) {
        return fail("api_motor_failure_command_state_invalid");
    }
    if (command_state_response.value_float != 30.0F) {
        return fail("api_motor_failure_command_value_invalid");
    }
    if (command_state_response.value_int != static_cast<int32_t>(MotorDirection::FORWARD)) {
        return fail("api_motor_failure_command_direction_invalid");
    }
    if (!isSameText(command_state_response.error_code, ERR_ACTUATOR_EXECUTION_FAILED)) {
        return fail("api_motor_failure_command_error_invalid");
    }

    if (!api_.getMotorControlState(motor_state)) {
        return fail("api_motor_after_failure_failed");
    }
    if (motor_state.payload.value_float == 30.0F &&
        motor_state.payload.value_int == static_cast<int32_t>(MotorDirection::FORWARD)) {
        return fail("api_motor_failure_payload_unsafe");
    }
    if (!validateMotorPayload(motor_state.payload, 0.0F, MotorDirection::STOP)) {
        return false;
    }

    command_request.direction = MotorDirection::FORWARD;
    command_request.speed_percent = 40.0F;
    command_request.timeout_ms = 1000;
    if (!api_.commandMotorControl(now_ms, command_request, command_response)) {
        return fail("api_motor_replace_first_accept_failed");
    }
    if (!command_response.ok) {
        return fail("api_motor_replace_first_not_ok");
    }
    if (command_response.command_state != CommandState::ACCEPTED) {
        return fail("api_motor_replace_first_state_invalid");
    }
    if (!command_response.accepted || command_response.executed) {
        return fail("api_motor_replace_first_flags_invalid");
    }

    command_request.direction = MotorDirection::FORWARD;
    command_request.speed_percent = 60.0F;
    command_request.timeout_ms = 1000;
    if (api_.commandMotorControl(now_ms, command_request, command_response)) {
        return fail("api_motor_pending_motion_succeeded");
    }
    if (command_response.ok) {
        return fail("api_motor_pending_motion_ok");
    }
    if (command_response.accepted) {
        return fail("api_motor_pending_motion_accepted");
    }
    if (command_response.executed) {
        return fail("api_motor_pending_motion_executed");
    }
    if (command_response.command_state != CommandState::FAILED) {
        return fail("api_motor_pending_motion_state_invalid");
    }
    if (!isSameText(command_response.error_code, ERR_PENDING_COMMAND_EXISTS)) {
        return fail("api_motor_pending_motion_error_invalid");
    }
    if (!api_.getMotorCommandState(command_state_response)) {
        return fail("api_motor_pending_motion_state_read_failed");
    }
    if (!command_state_response.ok) {
        return fail("api_motor_pending_motion_state_not_ok");
    }
    if (command_state_response.command_state != CommandState::FAILED) {
        return fail("api_motor_pending_motion_latest_state_invalid");
    }
    if (command_state_response.value_float != 60.0F) {
        return fail("api_motor_pending_motion_value_invalid");
    }
    if (command_state_response.value_int != static_cast<int32_t>(MotorDirection::FORWARD)) {
        return fail("api_motor_pending_motion_direction_invalid");
    }
    if (!isSameText(command_state_response.error_code, ERR_PENDING_COMMAND_EXISTS)) {
        return fail("api_motor_pending_motion_latest_error_invalid");
    }

    command_request.direction = MotorDirection::STOP;
    command_request.speed_percent = 0.0F;
    command_request.timeout_ms = 1000;
    if (!api_.commandMotorControl(now_ms, command_request, command_response)) {
        return fail("api_motor_replace_stop_failed");
    }
    if (!command_response.ok) {
        return fail("api_motor_replace_stop_not_ok");
    }
    if (command_response.command_state != CommandState::ACCEPTED) {
        return fail("api_motor_replace_stop_state_invalid");
    }
    if (!command_response.accepted || command_response.executed) {
        return fail("api_motor_replace_stop_flags_invalid");
    }

    if (!api_.getMotorControlState(motor_state)) {
        return fail("api_motor_replace_pending_payload_read_failed");
    }
    if (!validateMotorPayload(motor_state.payload, 0.0F, MotorDirection::STOP)) {
        return false;
    }

    const uint32_t replacement_execution_ms = now_ms + 70;
    motor_service_command_task_context_.now_ms = replacement_execution_ms;
    motor_service_command_task_context_.ran = false;
    motor_service_command_task_context_.last_result = false;
    motor_service_command_task_context_.last_error = "not_run";
    runtime_.update(replacement_execution_ms);

    if (!motor_service_command_task_context_.ran) {
        return fail("motor_replace_stop_task_not_run");
    }
    if (!motor_service_command_task_context_.last_result) {
        return fail(motor_service_command_task_context_.last_error);
    }

    if (!api_.getMotorCommandState(command_state_response)) {
        return fail("api_motor_replace_stop_state_read_failed");
    }
    if (!command_state_response.ok) {
        return fail("api_motor_replace_stop_state_not_ok");
    }
    if (command_state_response.command_state != CommandState::COMPLETED) {
        return fail("api_motor_replace_stop_not_completed");
    }
    if (command_state_response.value_float != 0.0F) {
        return fail("api_motor_replace_stop_value_invalid");
    }
    if (command_state_response.value_int != static_cast<int32_t>(MotorDirection::STOP)) {
        return fail("api_motor_replace_stop_direction_invalid");
    }

    if (!api_.getMotorControlState(motor_state)) {
        return fail("api_motor_after_replace_stop_failed");
    }
    if (!validateMotorPayload(motor_state.payload, 0.0F, MotorDirection::STOP)) {
        return false;
    }

    command_request.direction = MotorDirection::FORWARD;
    command_request.speed_percent = 999.0F;
    command_request.timeout_ms = 1000;
    if (api_.commandMotorControl(now_ms, command_request, command_response)) {
        return fail("api_motor_invalid_command_succeeded");
    }
    if (command_response.ok) {
        return fail("api_motor_invalid_command_ok");
    }
    if (command_response.accepted) {
        return fail("api_motor_invalid_command_accepted");
    }
    if (command_response.executed) {
        return fail("api_motor_invalid_command_executed");
    }
    if (command_response.command_state != CommandState::FAILED) {
        return fail("api_motor_invalid_command_state");
    }
    if (!isSameText(command_response.error_code, ERR_COMMAND_INVALID_SPEED)) {
        return fail("api_motor_invalid_command_response_error_invalid");
    }
    if (!api_.getMotorCommandState(command_state_response)) {
        return fail("api_motor_invalid_command_state_read_failed");
    }
    if (!command_state_response.ok) {
        return fail("api_motor_invalid_command_state_not_ok");
    }
    if (command_state_response.command_state != CommandState::FAILED) {
        return fail("api_motor_invalid_latest_state_invalid");
    }
    if (command_state_response.value_float != 999.0F) {
        return fail("api_motor_invalid_command_value_invalid");
    }
    if (!isSameText(command_state_response.error_code, ERR_COMMAND_INVALID_SPEED)) {
        return fail("api_motor_invalid_command_error_invalid");
    }
    if (!api_.getMotorControlState(motor_state)) {
        return fail("api_motor_after_invalid_failed");
    }
    if (!validateMotorPayload(motor_state.payload, 0.0F, MotorDirection::STOP)) {
        return false;
    }

    command_request.direction = MotorDirection::FORWARD;
    command_request.speed_percent = 10.0F;
    command_request.timeout_ms = 0;
    if (api_.commandMotorControl(now_ms, command_request, command_response)) {
        return fail("api_motor_invalid_timeout_succeeded");
    }
    if (command_response.ok) {
        return fail("api_motor_invalid_timeout_ok");
    }
    if (command_response.accepted) {
        return fail("api_motor_invalid_timeout_accepted");
    }
    if (command_response.executed) {
        return fail("api_motor_invalid_timeout_executed");
    }
    if (command_response.command_state != CommandState::FAILED) {
        return fail("api_motor_invalid_timeout_state");
    }
    if (!isSameText(command_response.error_code, ERR_COMMAND_INVALID_TIMEOUT)) {
        return fail("api_motor_invalid_timeout_response_error_invalid");
    }
    if (!api_.getMotorCommandState(command_state_response)) {
        return fail("api_motor_invalid_timeout_state_read_failed");
    }
    if (!command_state_response.ok) {
        return fail("api_motor_invalid_timeout_state_not_ok");
    }
    if (command_state_response.command_state != CommandState::FAILED) {
        return fail("api_motor_invalid_timeout_latest_state_invalid");
    }
    if (command_state_response.value_float != 10.0F) {
        return fail("api_motor_invalid_timeout_value_invalid");
    }
    if (command_state_response.value_int != static_cast<int32_t>(MotorDirection::FORWARD)) {
        return fail("api_motor_invalid_timeout_direction_invalid");
    }
    if (!isSameText(command_state_response.error_code, ERR_COMMAND_INVALID_TIMEOUT)) {
        return fail("api_motor_invalid_timeout_error_invalid");
    }
    if (!api_.getMotorControlState(motor_state)) {
        return fail("api_motor_after_invalid_timeout_failed");
    }
    if (!validateMotorPayload(motor_state.payload, 0.0F, MotorDirection::STOP)) {
        return false;
    }

    const MotorDirection invalid_direction = static_cast<MotorDirection>(99);
    command_request.direction = invalid_direction;
    command_request.speed_percent = 10.0F;
    command_request.timeout_ms = 1000;
    if (api_.commandMotorControl(now_ms, command_request, command_response)) {
        return fail("api_motor_invalid_direction_succeeded");
    }
    if (command_response.ok) {
        return fail("api_motor_invalid_direction_ok");
    }
    if (command_response.accepted) {
        return fail("api_motor_invalid_direction_accepted");
    }
    if (command_response.executed) {
        return fail("api_motor_invalid_direction_executed");
    }
    if (command_response.command_state != CommandState::FAILED) {
        return fail("api_motor_invalid_direction_state");
    }
    if (!isSameText(command_response.error_code, ERR_COMMAND_INVALID_DIRECTION)) {
        return fail("api_motor_invalid_direction_response_error_invalid");
    }
    if (!api_.getMotorCommandState(command_state_response)) {
        return fail("api_motor_invalid_direction_state_read_failed");
    }
    if (!command_state_response.ok) {
        return fail("api_motor_invalid_direction_state_not_ok");
    }
    if (command_state_response.command_state != CommandState::FAILED) {
        return fail("api_motor_invalid_direction_latest_state_invalid");
    }
    if (command_state_response.value_float != 10.0F) {
        return fail("api_motor_invalid_direction_value_invalid");
    }
    if (command_state_response.value_int != static_cast<int32_t>(invalid_direction)) {
        return fail("api_motor_invalid_direction_value_int_invalid");
    }
    if (!isSameText(command_state_response.error_code, ERR_COMMAND_INVALID_DIRECTION)) {
        return fail("api_motor_invalid_direction_error_invalid");
    }
    if (!api_.getMotorControlState(motor_state)) {
        return fail("api_motor_after_invalid_direction_failed");
    }
    if (!validateMotorPayload(motor_state.payload, 0.0F, MotorDirection::STOP)) {
        return false;
    }

    if (!validateMotorPendingRuntimeTransitions(now_ms)) {
        return false;
    }

    if (!validateMotorRuntimeBlockedState(
            RuntimeState::SAFE_MODE,
            now_ms + 120,
            0.0F,
            MotorDirection::STOP)) {
        return false;
    }

    runtime_.setState(RuntimeState::RUNNING);
    return true;
}

bool VerticalSliceValidation::validateMotorPendingRuntimeTransitions(uint32_t now_ms) {
    runtime_.setState(RuntimeState::READY);

    ApiCapabilityState motor_state;
    if (!api_.getMotorControlState(motor_state)) {
        return fail("api_motor_transition_initial_read_failed");
    }
    if (!validateMotorPayload(motor_state.payload, 0.0F, MotorDirection::STOP)) {
        return false;
    }

    ApiMotorCommandRequest command_request;
    command_request.direction = MotorDirection::FORWARD;
    command_request.speed_percent = 35.0F;
    command_request.timeout_ms = 1000;

    ApiMotorCommandResponse command_response;
    if (!api_.commandMotorControl(now_ms, command_request, command_response)) {
        return fail("api_motor_safe_transition_accept_failed");
    }
    if (!command_response.ok) {
        return fail("api_motor_safe_transition_not_ok");
    }
    if (command_response.command_state != CommandState::ACCEPTED) {
        return fail("api_motor_safe_transition_state_invalid");
    }
    if (!command_response.accepted || command_response.executed) {
        return fail("api_motor_safe_transition_flags_invalid");
    }

    if (!runtime_.enterSafeMode()) {
        return fail("runtime_safe_transition_enter_failed");
    }

    const uint32_t safe_transition_execution_ms = now_ms + 80;
    motor_service_command_task_context_.now_ms = safe_transition_execution_ms;
    motor_service_command_task_context_.ran = false;
    motor_service_command_task_context_.last_result = true;
    motor_service_command_task_context_.last_error = "not_run";
    runtime_.update(safe_transition_execution_ms);

    if (!motor_service_command_task_context_.ran) {
        return fail("motor_safe_transition_task_not_run");
    }
    if (motor_service_command_task_context_.last_result) {
        return fail("motor_safe_transition_completed");
    }

    ApiCommandStateResponse command_state_response;
    if (!api_.getMotorCommandState(command_state_response)) {
        return fail("api_motor_safe_transition_state_read_failed");
    }
    if (!command_state_response.ok) {
        return fail("api_motor_safe_transition_state_not_ok");
    }
    if (command_state_response.command_state != CommandState::FAILED) {
        return fail("api_motor_safe_transition_not_failed");
    }
    if (command_state_response.value_float != 35.0F) {
        return fail("api_motor_safe_transition_value_invalid");
    }
    if (command_state_response.value_int != static_cast<int32_t>(MotorDirection::FORWARD)) {
        return fail("api_motor_safe_transition_direction_invalid");
    }
    if (!isSameText(command_state_response.error_code, ERR_SAFE_MODE_BLOCKED)) {
        return fail("api_motor_safe_transition_error_invalid");
    }
    if (motor_service_.hasPendingCommand()) {
        return fail("motor_safe_transition_pending_not_cleared");
    }

    if (!api_.getMotorControlState(motor_state)) {
        return fail("api_motor_safe_transition_payload_read_failed");
    }
    if (!validateMotorPayload(motor_state.payload, 0.0F, MotorDirection::STOP)) {
        return false;
    }

    if (!runtime_.exitSafeMode()) {
        return fail("runtime_safe_transition_exit_failed");
    }

    command_request.direction = MotorDirection::FORWARD;
    command_request.speed_percent = 45.0F;
    command_request.timeout_ms = 1000;
    if (!api_.commandMotorControl(now_ms + 81, command_request, command_response)) {
        return fail("api_motor_error_transition_accept_failed");
    }
    if (!command_response.ok) {
        return fail("api_motor_error_transition_not_ok");
    }
    if (command_response.command_state != CommandState::ACCEPTED) {
        return fail("api_motor_error_transition_state_invalid");
    }
    if (!command_response.accepted || command_response.executed) {
        return fail("api_motor_error_transition_flags_invalid");
    }

    runtime_.setState(RuntimeState::ERROR_STATE);

    const uint32_t error_transition_execution_ms = now_ms + 90;
    motor_service_command_task_context_.now_ms = error_transition_execution_ms;
    motor_service_command_task_context_.ran = false;
    motor_service_command_task_context_.last_result = true;
    motor_service_command_task_context_.last_error = "not_run";
    runtime_.update(error_transition_execution_ms);

    if (!motor_service_command_task_context_.ran) {
        return fail("motor_error_transition_task_not_run");
    }
    if (motor_service_command_task_context_.last_result) {
        return fail("motor_error_transition_completed");
    }

    if (!api_.getMotorCommandState(command_state_response)) {
        return fail("api_motor_error_transition_state_read_failed");
    }
    if (!command_state_response.ok) {
        return fail("api_motor_error_transition_state_not_ok");
    }
    if (command_state_response.command_state != CommandState::FAILED) {
        return fail("api_motor_error_transition_not_failed");
    }
    if (command_state_response.value_float != 45.0F) {
        return fail("api_motor_error_transition_value_invalid");
    }
    if (command_state_response.value_int != static_cast<int32_t>(MotorDirection::FORWARD)) {
        return fail("api_motor_error_transition_direction_invalid");
    }
    if (!isSameText(command_state_response.error_code, ERR_RUNTIME_NOT_READY)) {
        return fail("api_motor_error_transition_error_invalid");
    }
    if (motor_service_.hasPendingCommand()) {
        return fail("motor_error_transition_pending_not_cleared");
    }

    if (!api_.getMotorControlState(motor_state)) {
        return fail("api_motor_error_transition_payload_read_failed");
    }
    if (!validateMotorPayload(motor_state.payload, 0.0F, MotorDirection::STOP)) {
        return false;
    }

    runtime_.setState(RuntimeState::READY);
    if (!runtime_.enterSafeMode()) {
        return fail("runtime_safe_stop_enter_failed");
    }

    command_request.direction = MotorDirection::STOP;
    command_request.speed_percent = 0.0F;
    command_request.timeout_ms = 1000;
    if (!api_.commandMotorControl(now_ms + 91, command_request, command_response)) {
        return fail("api_motor_safe_transition_stop_failed");
    }
    if (!command_response.ok) {
        return fail("api_motor_safe_transition_stop_not_ok");
    }
    if (command_response.command_state != CommandState::ACCEPTED) {
        return fail("api_motor_safe_transition_stop_state_invalid");
    }
    if (!command_response.accepted || command_response.executed) {
        return fail("api_motor_safe_transition_stop_flags_invalid");
    }

    const uint32_t safe_stop_execution_ms = now_ms + 100;
    motor_service_command_task_context_.now_ms = safe_stop_execution_ms;
    motor_service_command_task_context_.ran = false;
    motor_service_command_task_context_.last_result = false;
    motor_service_command_task_context_.last_error = "not_run";
    runtime_.update(safe_stop_execution_ms);

    if (!motor_service_command_task_context_.ran) {
        return fail("motor_safe_transition_stop_task_not_run");
    }
    if (!motor_service_command_task_context_.last_result) {
        return fail(motor_service_command_task_context_.last_error);
    }

    if (!api_.getMotorCommandState(command_state_response)) {
        return fail("api_motor_safe_transition_stop_state_read_failed");
    }
    if (!command_state_response.ok) {
        return fail("api_motor_safe_transition_stop_state_not_ok");
    }
    if (command_state_response.command_state != CommandState::COMPLETED) {
        return fail("api_motor_safe_transition_stop_not_completed");
    }
    if (command_state_response.value_float != 0.0F) {
        return fail("api_motor_safe_transition_stop_value_invalid");
    }
    if (command_state_response.value_int != static_cast<int32_t>(MotorDirection::STOP)) {
        return fail("api_motor_safe_transition_stop_direction_invalid");
    }
    if (!isSameText(command_state_response.error_code, "none")) {
        return fail("api_motor_safe_transition_stop_error_invalid");
    }
    if (motor_service_.hasPendingCommand()) {
        return fail("motor_safe_transition_stop_pending_not_cleared");
    }

    if (!api_.getMotorControlState(motor_state)) {
        return fail("api_motor_safe_transition_stop_payload_read_failed");
    }
    if (!validateMotorPayload(motor_state.payload, 0.0F, MotorDirection::STOP)) {
        return false;
    }

    runtime_.setState(RuntimeState::READY);
    return true;
}

bool VerticalSliceValidation::validateMotorRuntimeBlockedState(
    RuntimeState state,
    uint32_t now_ms,
    float expected_speed,
    MotorDirection expected_direction) {
    if (state == RuntimeState::SAFE_MODE) {
        if (!runtime_.enterSafeMode()) {
            return fail("runtime_enter_safe_mode_failed");
        }
    } else {
        runtime_.setState(state);
    }

    ApiMotorCommandRequest command_request;
    command_request.direction = MotorDirection::FORWARD;
    command_request.speed_percent = 25.0F;
    command_request.timeout_ms = 1000;

    ApiMotorCommandResponse command_response;
    if (api_.commandMotorControl(now_ms, command_request, command_response)) {
        return fail("api_motor_blocked_command_succeeded");
    }
    if (command_response.ok) {
        return fail("api_motor_blocked_command_ok");
    }
    if (command_response.command_state != CommandState::FAILED) {
        return fail("api_motor_blocked_command_state_invalid");
    }
    if (command_response.accepted) {
        return fail("api_motor_blocked_command_accepted");
    }
    if (command_response.executed) {
        return fail("api_motor_blocked_command_executed");
    }
    const char* expected_error_code = state == RuntimeState::SAFE_MODE
        ? ERR_SAFE_MODE_BLOCKED
        : ERR_RUNTIME_NOT_READY;

    if (!isSameText(command_response.error_code, expected_error_code)) {
        return fail("api_motor_blocked_command_error_invalid");
    }

    ApiCommandStateResponse command_state_response;
    if (!api_.getMotorCommandState(command_state_response)) {
        return fail("api_motor_blocked_command_state_read_failed");
    }
    if (!command_state_response.ok) {
        return fail("api_motor_blocked_command_state_not_ok");
    }
    if (command_state_response.command_state != CommandState::FAILED) {
        return fail("api_motor_blocked_latest_state_invalid");
    }
    if (!isSameText(command_state_response.error_code, expected_error_code)) {
        return fail("api_motor_blocked_latest_error_invalid");
    }

    ApiCapabilityState motor_state;
    if (!api_.getMotorControlState(motor_state)) {
        return fail("api_motor_blocked_payload_read_failed");
    }
    if (!validateMotorPayload(motor_state.payload, expected_speed, expected_direction)) {
        return false;
    }

    if (state != RuntimeState::SAFE_MODE) {
        return true;
    }

    command_request.direction = MotorDirection::STOP;
    command_request.speed_percent = 0.0F;
    command_request.timeout_ms = 1000;

    if (!api_.commandMotorControl(now_ms, command_request, command_response)) {
        return fail("api_motor_safe_mode_stop_failed");
    }
    if (!command_response.ok) {
        return fail("api_motor_safe_mode_stop_not_ok");
    }
    if (command_response.command_state != CommandState::ACCEPTED) {
        return fail("api_motor_safe_mode_stop_state_invalid");
    }
    if (!command_response.accepted) {
        return fail("api_motor_safe_mode_stop_not_accepted");
    }
    if (command_response.executed) {
        return fail("api_motor_safe_mode_stop_executed_too_early");
    }

    if (!api_.getMotorControlState(motor_state)) {
        return fail("api_motor_safe_mode_stop_pending_read_failed");
    }
    if (!validateMotorPayload(motor_state.payload, expected_speed, expected_direction)) {
        return false;
    }

    const uint32_t execution_ms = now_ms + 20;
    motor_service_command_task_context_.now_ms = execution_ms;
    motor_service_command_task_context_.ran = false;
    motor_service_command_task_context_.last_result = false;
    motor_service_command_task_context_.last_error = "not_run";
    runtime_.update(execution_ms);

    if (!motor_service_command_task_context_.ran) {
        return fail("motor_safe_mode_stop_task_not_run");
    }
    if (!motor_service_command_task_context_.last_result) {
        return fail(motor_service_command_task_context_.last_error);
    }

    if (!api_.getMotorCommandState(command_state_response)) {
        return fail("api_motor_safe_mode_stop_state_read_failed");
    }
    if (!command_state_response.ok) {
        return fail("api_motor_safe_mode_stop_state_not_ok");
    }
    if (command_state_response.command_state != CommandState::COMPLETED) {
        return fail("api_motor_safe_mode_stop_not_completed");
    }
    if (command_state_response.value_float != 0.0F) {
        return fail("api_motor_safe_mode_stop_value_invalid");
    }
    if (command_state_response.value_int != static_cast<int32_t>(MotorDirection::STOP)) {
        return fail("api_motor_safe_mode_stop_direction_invalid");
    }
    if (!isSameText(command_state_response.error_code, "none")) {
        return fail("api_motor_safe_mode_stop_error_invalid");
    }

    if (!api_.getMotorControlState(motor_state)) {
        return fail("api_motor_safe_mode_stop_payload_read_failed");
    }
    return validateMotorPayload(motor_state.payload, 0.0F, MotorDirection::STOP);
}

bool VerticalSliceValidation::validateRelayCommandState(uint32_t now_ms) {
    runtime_.setState(RuntimeState::READY);
    const uint32_t relay_base_ms = now_ms + 1000;

    ApiCapabilityState relay_state;
    if (!api_.getRelayControlState(relay_state)) {
        return fail("api_relay_initial_state_failed");
    }
    if (!validateRelayPayload(relay_state.payload, false)) {
        return false;
    }

    ApiRelayCommandRequest command_request;
    command_request.enabled = true;
    command_request.timeout_ms = 1000;

    ApiRelayCommandResponse command_response;
    if (!api_.commandRelayControl(relay_base_ms, command_request, command_response)) {
        return fail("api_relay_on_accept_failed");
    }
    if (!command_response.ok) {
        return fail("api_relay_on_not_ok");
    }
    if (command_response.command_state != CommandState::ACCEPTED) {
        return fail("api_relay_on_state_invalid");
    }
    if (!command_response.accepted || command_response.executed) {
        return fail("api_relay_on_flags_invalid");
    }

    if (!api_.getRelayControlState(relay_state)) {
        return fail("api_relay_on_pending_state_failed");
    }
    if (!validateRelayPayload(relay_state.payload, false)) {
        return false;
    }

    const uint32_t on_execution_ms = relay_base_ms + 20;
    relay_service_command_task_context_.now_ms = on_execution_ms;
    relay_service_command_task_context_.ran = false;
    relay_service_command_task_context_.last_result = false;
    relay_service_command_task_context_.last_error = "not_run";
    runtime_.update(on_execution_ms);

    if (!relay_service_command_task_context_.ran) {
        return fail("relay_on_task_not_run");
    }
    if (!relay_service_command_task_context_.last_result) {
        return fail(relay_service_command_task_context_.last_error);
    }

    ApiCommandStateResponse command_state_response;
    if (!api_.getRelayCommandState(command_state_response)) {
        return fail("api_relay_on_command_state_read_failed");
    }
    if (!command_state_response.ok) {
        return fail("api_relay_on_command_state_not_ok");
    }
    if (!isSameText(command_state_response.capability_id, CAP_RELAY_CONTROL)) {
        return fail("api_relay_on_command_capability_invalid");
    }
    if (command_state_response.command_state != CommandState::COMPLETED) {
        return fail("api_relay_on_not_completed");
    }
    if (command_state_response.value_int != 1) {
        return fail("api_relay_on_command_value_invalid");
    }
    if (!isSameText(command_state_response.error_code, "none")) {
        return fail("api_relay_on_command_error_invalid");
    }

    if (!api_.getRelayControlState(relay_state)) {
        return fail("api_relay_after_on_failed");
    }
    if (!validateRelayPayload(relay_state.payload, true)) {
        return false;
    }

    if (!api_.commandRelayOff(relay_base_ms + 40, command_response)) {
        return fail("api_relay_off_accept_failed");
    }
    if (!command_response.ok) {
        return fail("api_relay_off_not_ok");
    }
    if (command_response.command_state != CommandState::ACCEPTED) {
        return fail("api_relay_off_state_invalid");
    }
    if (!command_response.accepted || command_response.executed) {
        return fail("api_relay_off_flags_invalid");
    }

    if (!api_.getRelayControlState(relay_state)) {
        return fail("api_relay_off_pending_state_failed");
    }
    if (!validateRelayPayload(relay_state.payload, true)) {
        return false;
    }

    const uint32_t off_execution_ms = relay_base_ms + 60;
    relay_service_command_task_context_.now_ms = off_execution_ms;
    relay_service_command_task_context_.ran = false;
    relay_service_command_task_context_.last_result = false;
    relay_service_command_task_context_.last_error = "not_run";
    runtime_.update(off_execution_ms);

    if (!relay_service_command_task_context_.ran) {
        return fail("relay_off_task_not_run");
    }
    if (!relay_service_command_task_context_.last_result) {
        return fail(relay_service_command_task_context_.last_error);
    }

    if (!api_.getRelayCommandState(command_state_response)) {
        return fail("api_relay_off_command_state_read_failed");
    }
    if (!command_state_response.ok) {
        return fail("api_relay_off_command_state_not_ok");
    }
    if (command_state_response.command_state != CommandState::COMPLETED) {
        return fail("api_relay_off_not_completed");
    }
    if (command_state_response.value_int != 0) {
        return fail("api_relay_off_command_value_invalid");
    }
    if (!isSameText(command_state_response.error_code, "none")) {
        return fail("api_relay_off_command_error_invalid");
    }

    if (!api_.getRelayControlState(relay_state)) {
        return fail("api_relay_after_off_failed");
    }
    if (!validateRelayPayload(relay_state.payload, false)) {
        return false;
    }

    command_request.enabled = true;
    command_request.timeout_ms = 0;
    if (api_.commandRelayControl(relay_base_ms + 80, command_request, command_response)) {
        return fail("api_relay_invalid_timeout_succeeded");
    }
    if (command_response.ok) {
        return fail("api_relay_invalid_timeout_ok");
    }
    if (command_response.command_state != CommandState::FAILED) {
        return fail("api_relay_invalid_timeout_state_invalid");
    }
    if (command_response.accepted || command_response.executed) {
        return fail("api_relay_invalid_timeout_flags_invalid");
    }
    if (!isSameText(command_response.error_code, ERR_COMMAND_INVALID_TIMEOUT)) {
        return fail("api_relay_invalid_timeout_error_invalid");
    }

    if (!api_.getRelayControlState(relay_state)) {
        return fail("api_relay_after_invalid_timeout_failed");
    }
    if (!validateRelayPayload(relay_state.payload, false)) {
        return false;
    }

    command_request.enabled = true;
    command_request.timeout_ms = 1;
    if (!api_.commandRelayControl(relay_base_ms + 90, command_request, command_response)) {
        return fail("api_relay_timeout_accept_failed");
    }
    if (!command_response.ok) {
        return fail("api_relay_timeout_not_ok");
    }
    if (command_response.command_state != CommandState::ACCEPTED) {
        return fail("api_relay_timeout_accept_state_invalid");
    }
    if (!command_response.accepted || command_response.executed) {
        return fail("api_relay_timeout_accept_flags_invalid");
    }
    if (!api_.getRelayControlState(relay_state)) {
        return fail("api_relay_timeout_pending_state_failed");
    }
    if (!validateRelayPayload(relay_state.payload, false)) {
        return false;
    }

    const uint32_t timeout_execution_ms = relay_base_ms + 92;
    relay_service_command_task_context_.now_ms = timeout_execution_ms;
    relay_service_command_task_context_.ran = false;
    relay_service_command_task_context_.last_result = true;
    relay_service_command_task_context_.last_error = "not_run";
    runtime_.update(timeout_execution_ms);

    if (!relay_service_command_task_context_.ran) {
        return fail("relay_timeout_task_not_run");
    }
    if (relay_service_command_task_context_.last_result) {
        return fail("relay_timeout_command_completed");
    }
    if (!api_.getRelayCommandState(command_state_response)) {
        return fail("api_relay_timeout_command_state_read_failed");
    }
    if (!command_state_response.ok) {
        return fail("api_relay_timeout_command_state_not_ok");
    }
    if (command_state_response.command_state != CommandState::TIMED_OUT) {
        return fail("api_relay_timeout_state_invalid");
    }
    if (!isSameText(command_state_response.error_code, ERR_COMMAND_TIMEOUT)) {
        return fail("api_relay_timeout_error_invalid");
    }
    if (!api_.getRelayControlState(relay_state)) {
        return fail("api_relay_after_timeout_failed");
    }
    if (!validateRelayPayload(relay_state.payload, false)) {
        return false;
    }

    runtime_.setState(RuntimeState::READY);
    command_request.enabled = true;
    command_request.timeout_ms = 1000;
    if (!api_.commandRelayControl(relay_base_ms + 95, command_request, command_response)) {
        return fail("api_relay_pending_safe_mode_accept_failed");
    }
    if (!command_response.ok) {
        return fail("api_relay_pending_safe_mode_not_ok");
    }
    if (command_response.command_state != CommandState::ACCEPTED) {
        return fail("api_relay_pending_safe_mode_accept_state_invalid");
    }
    if (!command_response.accepted || command_response.executed) {
        return fail("api_relay_pending_safe_mode_accept_flags_invalid");
    }
    if (!api_.getRelayControlState(relay_state)) {
        return fail("api_relay_pending_safe_mode_state_failed");
    }
    if (!validateRelayPayload(relay_state.payload, false)) {
        return false;
    }
    if (!runtime_.enterSafeMode()) {
        return fail("runtime_relay_pending_safe_mode_enter_failed");
    }

    const uint32_t pending_safe_mode_execution_ms = relay_base_ms + 96;
    relay_service_command_task_context_.now_ms = pending_safe_mode_execution_ms;
    relay_service_command_task_context_.ran = false;
    relay_service_command_task_context_.last_result = true;
    relay_service_command_task_context_.last_error = "not_run";
    runtime_.update(pending_safe_mode_execution_ms);

    if (!relay_service_command_task_context_.ran) {
        return fail("relay_pending_safe_mode_task_not_run");
    }
    if (relay_service_command_task_context_.last_result) {
        return fail("relay_pending_safe_mode_command_completed");
    }
    if (!api_.getRelayCommandState(command_state_response)) {
        return fail("api_relay_pending_safe_mode_command_state_read_failed");
    }
    if (!command_state_response.ok) {
        return fail("api_relay_pending_safe_mode_command_state_not_ok");
    }
    if (command_state_response.command_state != CommandState::FAILED) {
        return fail("api_relay_pending_safe_mode_state_invalid");
    }
    if (!isSameText(command_state_response.error_code, ERR_SAFE_MODE_BLOCKED)) {
        return fail("api_relay_pending_safe_mode_error_invalid");
    }
    if (!api_.getRelayControlState(relay_state)) {
        return fail("api_relay_after_pending_safe_mode_failed");
    }
    if (!validateRelayPayload(relay_state.payload, false)) {
        return false;
    }
    runtime_.setState(RuntimeState::READY);

    command_request.enabled = true;
    command_request.timeout_ms = 1000;
    if (!api_.commandRelayControl(relay_base_ms + 97, command_request, command_response)) {
        return fail("api_relay_pending_error_accept_failed");
    }
    if (!command_response.ok) {
        return fail("api_relay_pending_error_not_ok");
    }
    if (command_response.command_state != CommandState::ACCEPTED) {
        return fail("api_relay_pending_error_accept_state_invalid");
    }
    if (!command_response.accepted || command_response.executed) {
        return fail("api_relay_pending_error_accept_flags_invalid");
    }
    if (!api_.getRelayControlState(relay_state)) {
        return fail("api_relay_pending_error_state_failed");
    }
    if (!validateRelayPayload(relay_state.payload, false)) {
        return false;
    }
    runtime_.setState(RuntimeState::ERROR_STATE);

    const uint32_t pending_error_execution_ms = relay_base_ms + 98;
    relay_service_command_task_context_.now_ms = pending_error_execution_ms;
    relay_service_command_task_context_.ran = false;
    relay_service_command_task_context_.last_result = true;
    relay_service_command_task_context_.last_error = "not_run";
    runtime_.update(pending_error_execution_ms);

    if (!relay_service_command_task_context_.ran) {
        return fail("relay_pending_error_task_not_run");
    }
    if (relay_service_command_task_context_.last_result) {
        return fail("relay_pending_error_command_completed");
    }
    if (!api_.getRelayCommandState(command_state_response)) {
        return fail("api_relay_pending_error_command_state_read_failed");
    }
    if (!command_state_response.ok) {
        return fail("api_relay_pending_error_command_state_not_ok");
    }
    if (command_state_response.command_state != CommandState::FAILED) {
        return fail("api_relay_pending_error_state_invalid");
    }
    if (!isSameText(command_state_response.error_code, ERR_RUNTIME_NOT_READY)) {
        return fail("api_relay_pending_error_error_invalid");
    }
    if (!api_.getRelayControlState(relay_state)) {
        return fail("api_relay_after_pending_error_failed");
    }
    if (!validateRelayPayload(relay_state.payload, false)) {
        return false;
    }
    runtime_.setState(RuntimeState::READY);

    relay_driver_.setFailureMode(true);
    command_request.enabled = true;
    command_request.timeout_ms = 1000;
    if (!api_.commandRelayControl(relay_base_ms + 100, command_request, command_response)) {
        relay_driver_.setFailureMode(false);
        return fail("api_relay_failure_accept_failed");
    }
    if (!command_response.ok) {
        relay_driver_.setFailureMode(false);
        return fail("api_relay_failure_not_ok");
    }
    if (command_response.command_state != CommandState::ACCEPTED) {
        relay_driver_.setFailureMode(false);
        return fail("api_relay_failure_accept_state_invalid");
    }

    const uint32_t failure_execution_ms = relay_base_ms + 120;
    relay_service_command_task_context_.now_ms = failure_execution_ms;
    relay_service_command_task_context_.ran = false;
    relay_service_command_task_context_.last_result = true;
    relay_service_command_task_context_.last_error = "not_run";
    runtime_.update(failure_execution_ms);

    relay_driver_.setFailureMode(false);

    if (!relay_service_command_task_context_.ran) {
        return fail("relay_failure_task_not_run");
    }
    if (relay_service_command_task_context_.last_result) {
        return fail("relay_failure_command_completed");
    }
    if (!api_.getRelayCommandState(command_state_response)) {
        return fail("api_relay_failure_command_state_read_failed");
    }
    if (!command_state_response.ok) {
        return fail("api_relay_failure_command_state_not_ok");
    }
    if (command_state_response.command_state != CommandState::FAILED) {
        return fail("api_relay_failure_state_invalid");
    }
    if (!isSameText(command_state_response.error_code, ERR_ACTUATOR_EXECUTION_FAILED)) {
        return fail("api_relay_failure_error_invalid");
    }
    if (!api_.getRelayControlState(relay_state)) {
        return fail("api_relay_after_failure_failed");
    }
    if (relay_state.payload.value_int == 1) {
        return fail("api_relay_failure_payload_unsafe");
    }

    const uint32_t failure_recovery_ms = relay_base_ms + 140;
    relay_service_state_task_context_.now_ms = failure_recovery_ms;
    relay_service_state_task_context_.ran = false;
    relay_service_state_task_context_.last_result = false;
    relay_service_state_task_context_.last_error = "not_run";
    runtime_.update(failure_recovery_ms);

    if (!relay_service_state_task_context_.ran) {
        return fail("relay_failure_recovery_task_not_run");
    }
    if (!relay_service_state_task_context_.last_result) {
        return fail(relay_service_state_task_context_.last_error);
    }

    if (!api_.getRelayControlState(relay_state)) {
        return fail("api_relay_after_failure_recovery_failed");
    }
    if (!validateRelayPayload(relay_state.payload, false)) {
        return false;
    }

    if (!runtime_.enterSafeMode()) {
        return fail("runtime_relay_safe_mode_enter_failed");
    }

    command_request.enabled = true;
    command_request.timeout_ms = 1000;
    if (api_.commandRelayControl(relay_base_ms + 160, command_request, command_response)) {
        return fail("api_relay_safe_mode_on_succeeded");
    }
    if (command_response.ok) {
        return fail("api_relay_safe_mode_on_ok");
    }
    if (command_response.command_state != CommandState::FAILED) {
        return fail("api_relay_safe_mode_on_state_invalid");
    }
    if (command_response.accepted || command_response.executed) {
        return fail("api_relay_safe_mode_on_flags_invalid");
    }
    if (!isSameText(command_response.error_code, ERR_SAFE_MODE_BLOCKED)) {
        return fail("api_relay_safe_mode_on_error_invalid");
    }
    if (!api_.getRelayControlState(relay_state)) {
        return fail("api_relay_safe_mode_on_payload_read_failed");
    }
    if (!validateRelayPayload(relay_state.payload, false)) {
        return false;
    }

    if (!api_.commandRelayOff(relay_base_ms + 180, command_response)) {
        return fail("api_relay_safe_mode_off_accept_failed");
    }
    if (!command_response.ok) {
        return fail("api_relay_safe_mode_off_not_ok");
    }
    if (command_response.command_state != CommandState::ACCEPTED) {
        return fail("api_relay_safe_mode_off_state_invalid");
    }
    if (!command_response.accepted || command_response.executed) {
        return fail("api_relay_safe_mode_off_flags_invalid");
    }

    const uint32_t safe_off_execution_ms = relay_base_ms + 200;
    relay_service_command_task_context_.now_ms = safe_off_execution_ms;
    relay_service_command_task_context_.ran = false;
    relay_service_command_task_context_.last_result = false;
    relay_service_command_task_context_.last_error = "not_run";
    runtime_.update(safe_off_execution_ms);

    if (!relay_service_command_task_context_.ran) {
        return fail("relay_safe_mode_off_task_not_run");
    }
    if (!relay_service_command_task_context_.last_result) {
        return fail(relay_service_command_task_context_.last_error);
    }
    if (!api_.getRelayCommandState(command_state_response)) {
        return fail("api_relay_safe_mode_off_state_read_failed");
    }
    if (!command_state_response.ok) {
        return fail("api_relay_safe_mode_off_state_not_ok");
    }
    if (command_state_response.command_state != CommandState::COMPLETED) {
        return fail("api_relay_safe_mode_off_not_completed");
    }
    if (!api_.getRelayControlState(relay_state)) {
        return fail("api_relay_safe_mode_off_payload_read_failed");
    }
    if (!validateRelayPayload(relay_state.payload, false)) {
        return false;
    }

    runtime_.setState(RuntimeState::ERROR_STATE);
    command_request.enabled = true;
    command_request.timeout_ms = 1000;
    if (api_.commandRelayControl(relay_base_ms + 220, command_request, command_response)) {
        return fail("api_relay_error_state_on_succeeded");
    }
    if (command_response.ok) {
        return fail("api_relay_error_state_on_ok");
    }
    if (command_response.command_state != CommandState::FAILED) {
        return fail("api_relay_error_state_on_state_invalid");
    }
    if (command_response.accepted || command_response.executed) {
        return fail("api_relay_error_state_on_flags_invalid");
    }
    if (!isSameText(command_response.error_code, ERR_RUNTIME_NOT_READY)) {
        return fail("api_relay_error_state_on_error_invalid");
    }
    if (!api_.getRelayControlState(relay_state)) {
        return fail("api_relay_error_state_payload_read_failed");
    }
    if (!validateRelayPayload(relay_state.payload, false)) {
        return false;
    }

    runtime_.setState(RuntimeState::READY);
    return true;
}

bool VerticalSliceValidation::validateRuntimeSafeModeHelpers() {
    runtime_.setState(RuntimeState::READY);
    if (runtime_.isSafeMode()) {
        return fail("runtime_ready_safe_mode_invalid");
    }

    if (!runtime_.enterSafeMode()) {
        return fail("runtime_enter_safe_mode_failed");
    }
    if (runtime_.state() != RuntimeState::SAFE_MODE) {
        return fail("runtime_safe_mode_state_invalid");
    }
    if (!runtime_.isSafeMode()) {
        return fail("runtime_is_safe_mode_false");
    }

    if (!runtime_.exitSafeMode()) {
        return fail("runtime_exit_safe_mode_failed");
    }
    if (runtime_.state() != RuntimeState::READY) {
        return fail("runtime_exit_safe_mode_state_invalid");
    }
    if (runtime_.isSafeMode()) {
        return fail("runtime_exit_safe_mode_flag_invalid");
    }

    runtime_.setState(RuntimeState::RUNNING);
    if (runtime_.exitSafeMode()) {
        return fail("runtime_exit_non_safe_mode_succeeded");
    }
    if (runtime_.state() != RuntimeState::RUNNING) {
        return fail("runtime_exit_non_safe_mode_state_changed");
    }

    runtime_.setState(RuntimeState::READY);
    return true;
}

bool VerticalSliceValidation::validateRuntimeTaskState() {
    if (runtime_.taskCount() < 11) {
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
    if (!motor_service_state_task_context_.ran) {
        return fail("motor_service_state_task_not_run");
    }
    if (!motor_service_state_task_context_.last_result) {
        return fail(motor_service_state_task_context_.last_error);
    }
    if (!relay_service_state_task_context_.ran) {
        return fail("relay_service_state_task_not_run");
    }
    if (!relay_service_state_task_context_.last_result) {
        return fail(relay_service_state_task_context_.last_error);
    }
    if (!relay_service_command_task_context_.ran) {
        return fail("relay_service_command_task_not_run");
    }
    if (!wireless_service_process_task_context_.ran) {
        return fail("wireless_service_process_task_not_run");
    }
    if (!wireless_service_timeout_task_context_.ran) {
        return fail("wireless_service_timeout_task_not_run");
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

    CapabilityPayload motor_payload;
    if (registry_.getCapabilityPayloadWithResult(CAP_MOTOR_CONTROL, motor_payload) !=
        RegistryResult::OK) {
        return fail("motor_result_not_ok");
    }

    CapabilityPayload relay_payload;
    if (registry_.getCapabilityPayloadWithResult(CAP_RELAY_CONTROL, relay_payload) !=
        RegistryResult::OK) {
        return fail("relay_result_not_ok");
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

bool VerticalSliceValidation::validateCapabilityProviderStorage(uint32_t now_ms) {
    if (registry_.capabilityProviderCount() != 0) {
        return fail("provider_count_initial_invalid");
    }
    if (registry_.wirelessNodeAllowlistCount() != 0) {
        return fail("wireless_allowlist_count_initial_invalid");
    }

    CapabilityPayload temperature_payload;
    if (!registry_.getCapabilityPayload(CAP_TEMPERATURE, temperature_payload)) {
        return fail("provider_temperature_payload_missing");
    }

    WirelessNodeAllowlistRecord allowlist_record;
    allowlist_record.node_id = 1001;
    allowlist_record.allow_state = WirelessNodeAllowState::ALLOWED;
    allowlist_record.trust_state = WirelessTrustState::TRUSTED;
    allowlist_record.added_at_ms = now_ms;
    allowlist_record.last_seen_ms = now_ms;

    RegistryWriteResult allowlist_result =
        registry_.registerWirelessNodeAllowlistRecordWithResult(allowlist_record);
    if (allowlist_result.result != RegistryResult::OK) {
        return fail("wireless_allowlist_register_result_invalid");
    }
    if (allowlist_result.index != 0) {
        return fail("wireless_allowlist_register_index_invalid");
    }
    if (registry_.wirelessNodeAllowlistCount() != 1) {
        return fail("wireless_allowlist_count_after_register_invalid");
    }

    WirelessNodeAllowlistRecord allowlist_out;
    if (registry_.getWirelessNodeAllowlistRecord(1001, allowlist_out) != RegistryResult::OK) {
        return fail("wireless_allowlist_get_result_invalid");
    }
    if (allowlist_out.node_id != 1001) {
        return fail("wireless_allowlist_get_node_invalid");
    }
    if (allowlist_out.allow_state != WirelessNodeAllowState::ALLOWED) {
        return fail("wireless_allowlist_get_state_invalid");
    }
    if (allowlist_out.trust_state != WirelessTrustState::TRUSTED) {
        return fail("wireless_allowlist_get_trust_invalid");
    }
    if (registry_.getWirelessNodeAllowlistRecordByIndex(0, allowlist_out) != RegistryResult::OK) {
        return fail("wireless_allowlist_get_index_result_invalid");
    }
    if (allowlist_out.node_id != 1001) {
        return fail("wireless_allowlist_get_index_node_invalid");
    }
    if (registry_.registerWirelessNodeAllowlistRecordWithResult(allowlist_record).result !=
        RegistryResult::DUPLICATE_ID) {
        return fail("wireless_allowlist_duplicate_result_invalid");
    }
    if (registry_.wirelessNodeAllowlistCount() != 1) {
        return fail("wireless_allowlist_count_after_duplicate_invalid");
    }
    if (registry_.getWirelessNodeAllowlistRecord(9999, allowlist_out) != RegistryResult::NOT_FOUND) {
        return fail("wireless_allowlist_missing_result_invalid");
    }
    WirelessNodeAllowlistRecord invalid_allowlist_record = allowlist_record;
    invalid_allowlist_record.node_id = 0;
    if (registry_.registerWirelessNodeAllowlistRecordWithResult(invalid_allowlist_record).result !=
        RegistryResult::INVALID_ID) {
        return fail("wireless_allowlist_invalid_register_result_invalid");
    }
    if (registry_.getWirelessNodeAllowlistRecord(0, allowlist_out) != RegistryResult::INVALID_ID) {
        return fail("wireless_allowlist_invalid_get_result_invalid");
    }
    if (registry_.getWirelessNodeAllowlistRecordByIndex(1, allowlist_out) != RegistryResult::NOT_FOUND) {
        return fail("wireless_allowlist_invalid_index_result_invalid");
    }

    CapabilityProviderRecord provider;
    provider.provider_id = "provider-sim-temperature-001";
    provider.capability_id = CAP_TEMPERATURE;
    provider.owner_module_index = 0;
    provider.owner_device_index = 0;
    provider.provider_type = CapabilityProviderType::SIMULATED;
    provider.status = CapabilityProviderStatus::AVAILABLE;
    provider.priority = 10;
    provider.last_update_ms = now_ms;
    provider.latest_payload = temperature_payload;

    RegistryWriteResult provider_result = registry_.registerCapabilityProviderWithResult(provider);
    if (provider_result.result != RegistryResult::OK) {
        return fail("provider_register_result_invalid");
    }
    if (provider_result.index != 0) {
        return fail("provider_register_index_invalid");
    }
    if (registry_.capabilityProviderCount() != 1) {
        return fail("provider_count_after_register_invalid");
    }

    CapabilityProviderRecord out_record;
    if (registry_.getCapabilityProvider("provider-sim-temperature-001", out_record) !=
        RegistryResult::OK) {
        return fail("provider_get_result_invalid");
    }
    if (!isSameText(out_record.capability_id, CAP_TEMPERATURE)) {
        return fail("provider_capability_id_invalid");
    }
    if (out_record.provider_type != CapabilityProviderType::SIMULATED) {
        return fail("provider_type_invalid");
    }
    if (out_record.status != CapabilityProviderStatus::AVAILABLE) {
        return fail("provider_status_invalid");
    }
    if (out_record.latest_payload.value_float != 22.4F) {
        return fail("provider_sim_temperature_value_invalid");
    }

    if (registry_.registerCapabilityProviderWithResult(provider).result !=
        RegistryResult::DUPLICATE_ID) {
        return fail("provider_duplicate_result_invalid");
    }
    if (registry_.capabilityProviderCount() != 1) {
        return fail("provider_count_after_duplicate_invalid");
    }

    if (registry_.getCapabilityProvider("provider-missing", out_record) != RegistryResult::NOT_FOUND) {
        return fail("provider_missing_result_invalid");
    }

    if (registry_.activeProviderCount() != 0) {
        return fail("active_provider_count_initial_invalid");
    }
    if (registry_.updateSelectedCapabilityPayload(CAP_DISTANCE) != RegistryResult::NOT_FOUND) {
        return fail("selected_payload_no_active_result_invalid");
    }
    if (registry_.updateSelectedCapabilityPayload(0) != RegistryResult::INVALID_ID) {
        return fail("selected_payload_null_id_result_invalid");
    }
    if (registry_.updateSelectedCapabilityPayload("") != RegistryResult::INVALID_ID) {
        return fail("selected_payload_empty_id_result_invalid");
    }

    if (registry_.setActiveProvider(CAP_TEMPERATURE, "provider-sim-temperature-001") !=
        RegistryResult::OK) {
        return fail("active_provider_set_result_invalid");
    }
    if (registry_.activeProviderCount() != 1) {
        return fail("active_provider_count_after_set_invalid");
    }

    ActiveCapabilityProvider active_provider;
    if (registry_.getActiveProvider(CAP_TEMPERATURE, active_provider) != RegistryResult::OK) {
        return fail("active_provider_get_result_invalid");
    }
    if (!isSameText(active_provider.capability_id, CAP_TEMPERATURE)) {
        return fail("active_provider_capability_id_invalid");
    }
    if (!isSameText(active_provider.provider_id, "provider-sim-temperature-001")) {
        return fail("active_provider_id_invalid");
    }

    CapabilityProviderRecord wireless_provider = provider;
    CapabilityPayload wireless_initial_payload;
    wireless_initial_payload.capability_id = CAP_TEMPERATURE;
    wireless_initial_payload.schema_version = 1;
    wireless_initial_payload.timestamp_ms = 0;
    wireless_initial_payload.available = Availability::UNAVAILABLE;
    wireless_initial_payload.stale = StaleState::STALE;
    wireless_initial_payload.value_type = PayloadValueType::NONE;
    wireless_initial_payload.value_float = 0.0F;
    wireless_initial_payload.value_int = 0;
    wireless_initial_payload.unit = "degree_celsius";
    wireless_initial_payload.quality = "stale";
    wireless_initial_payload.error_code = "none";

    wireless_provider.provider_id = WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID;
    wireless_provider.provider_type = CapabilityProviderType::WIRELESS;
    wireless_provider.status = CapabilityProviderStatus::STALE;
    wireless_provider.priority = 20;
    wireless_provider.latest_payload = wireless_initial_payload;
    RegistryWriteResult wireless_result =
        registry_.registerCapabilityProviderWithResult(wireless_provider);
    if (wireless_result.result != RegistryResult::OK) {
        return fail("wireless_provider_register_result_invalid");
    }

    WirelessPacketHeader header;
    header.magic = WIRELESS_PACKET_MAGIC;
    header.protocol_version = WIRELESS_PROTOCOL_VERSION;
    header.packet_type = WirelessPacketType::CAPABILITY_VALUE;
    header.flags = 0;
    header.sequence_id = 1;
    header.node_id = 1001;
    header.payload_length = 0;

    WirelessCapabilityValue value;
    copyWirelessCapabilityId(value.capability_id, CAP_TEMPERATURE);
    value.payload_type = WirelessPayloadType::FLOAT;
    value.value_float = 23.5F;
    value.value_int = 0;
    copyWirelessCapabilityId(value.error_code, "none");

    WirelessNodeDiagnostics diagnostics;
    diagnostics.battery_present = true;
    diagnostics.battery_level_percent = 87.0F;
    diagnostics.battery_voltage = 0.0F;
    diagnostics.signal_quality_percent = 75.0F;

    header.checksum = calculateWirelessPacketChecksum(header, value, diagnostics);
    if (!wireless_transport_driver_.injectReceivedCapabilityValue(header, value, diagnostics)) {
        return fail("wireless_packet_inject_failed");
    }
    if (!wireless_service_.processPackets(now_ms)) {
        return fail("wireless_process_packet_failed");
    }
    if (wireless_transport_driver_.hasReceivedPacket()) {
        return fail("wireless_packet_slot_not_cleared");
    }

    CapabilityProviderRecord wireless_out_record;
    if (registry_.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            wireless_out_record) != RegistryResult::OK) {
        return fail("wireless_provider_get_after_process_failed");
    }
    if (wireless_out_record.status != CapabilityProviderStatus::AVAILABLE) {
        return fail("wireless_provider_status_invalid");
    }
    if (!isSameText(wireless_out_record.latest_payload.capability_id, CAP_TEMPERATURE)) {
        return fail("wireless_provider_payload_capability_invalid");
    }
    if (wireless_out_record.latest_payload.value_float != 23.5F) {
        return fail("wireless_provider_payload_value_invalid");
    }
    if (!isSameText(wireless_out_record.latest_payload.unit, "degree_celsius")) {
        return fail("wireless_provider_payload_unit_invalid");
    }
    if (wireless_out_record.last_update_ms != now_ms) {
        return fail("wireless_provider_last_update_invalid");
    }

    if (!registry_.getCapabilityPayload(CAP_TEMPERATURE, temperature_payload)) {
        return fail("wireless_canonical_temperature_missing");
    }
    if (!validateTemperaturePayload(temperature_payload)) {
        return false;
    }

    wireless_temperature_device_.setTrustState(WirelessTrustState::TRUSTED);
    header.packet_type = WirelessPacketType::CAPABILITY_VALUE;
    header.sequence_id = 19;
    value.value_float = 30.0F;
    header.checksum = calculateWirelessPacketChecksum(header, value, diagnostics);
    if (!wireless_transport_driver_.injectReceivedCapabilityValue(header, value, diagnostics)) {
        return fail("wireless_checksum_valid_inject_failed");
    }
    if (!wireless_service_.processPackets(now_ms + 19)) {
        return fail("wireless_checksum_valid_process_failed");
    }
    if (registry_.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            wireless_out_record) != RegistryResult::OK) {
        return fail("wireless_checksum_valid_provider_get_failed");
    }
    if (wireless_out_record.latest_payload.value_float != 30.0F) {
        return fail("wireless_checksum_valid_provider_invalid");
    }
    if (!registry_.getCapabilityPayload(CAP_TEMPERATURE, temperature_payload)) {
        return fail("wireless_checksum_valid_canonical_missing");
    }
    if (!validateTemperaturePayload(temperature_payload)) {
        return false;
    }

    header.sequence_id = 18;
    value.value_float = 31.0F;
    header.checksum =
        static_cast<uint16_t>(calculateWirelessPacketChecksum(header, value, diagnostics) + 1);
    if (!wireless_transport_driver_.injectReceivedCapabilityValue(header, value, diagnostics)) {
        return fail("wireless_checksum_invalid_inject_failed");
    }
    if (wireless_service_.processPackets(now_ms + 18)) {
        return fail("wireless_checksum_invalid_process_succeeded");
    }
    if (!isSameText(wireless_service_.lastErrorCode(), "wireless_checksum_invalid")) {
        return fail("wireless_checksum_invalid_error_invalid");
    }
    if (wireless_transport_driver_.hasReceivedPacket()) {
        return fail("wireless_checksum_invalid_packet_not_cleared");
    }
    if (registry_.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            wireless_out_record) != RegistryResult::OK) {
        return fail("wireless_checksum_invalid_provider_get_failed");
    }
    if (wireless_out_record.latest_payload.value_float != 30.0F) {
        return fail("wireless_checksum_invalid_provider_changed");
    }
    if (!registry_.getCapabilityPayload(CAP_TEMPERATURE, temperature_payload)) {
        return fail("wireless_checksum_invalid_canonical_missing");
    }
    if (!validateTemperaturePayload(temperature_payload)) {
        return false;
    }

    wireless_temperature_device_.setTrustState(WirelessTrustState::TRUSTED);
    header.sequence_id = 100;
    value.value_float = 31.0F;
    header.checksum = calculateWirelessPacketChecksum(header, value, diagnostics);
    if (!wireless_transport_driver_.injectReceivedCapabilityValue(header, value, diagnostics)) {
        return fail("wireless_sequence_first_inject_failed");
    }
    if (!wireless_service_.processPackets(now_ms + 100)) {
        return fail("wireless_sequence_first_process_failed");
    }
    if (registry_.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            wireless_out_record) != RegistryResult::OK) {
        return fail("wireless_sequence_first_provider_get_failed");
    }
    if (wireless_out_record.latest_payload.value_float != 31.0F) {
        return fail("wireless_sequence_first_provider_invalid");
    }

    header.sequence_id = 100;
    value.value_float = 32.0F;
    header.checksum = calculateWirelessPacketChecksum(header, value, diagnostics);
    if (!wireless_transport_driver_.injectReceivedCapabilityValue(header, value, diagnostics)) {
        return fail("wireless_sequence_duplicate_inject_failed");
    }
    if (wireless_service_.processPackets(now_ms + 101)) {
        return fail("wireless_sequence_duplicate_process_succeeded");
    }
    if (!isSameText(wireless_service_.lastErrorCode(), "wireless_duplicate_sequence")) {
        return fail("wireless_sequence_duplicate_error_invalid");
    }
    if (wireless_transport_driver_.hasReceivedPacket()) {
        return fail("wireless_sequence_duplicate_packet_not_cleared");
    }
    if (registry_.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            wireless_out_record) != RegistryResult::OK) {
        return fail("wireless_sequence_duplicate_provider_get_failed");
    }
    if (wireless_out_record.latest_payload.value_float != 31.0F) {
        return fail("wireless_sequence_duplicate_provider_changed");
    }
    if (!registry_.getCapabilityPayload(CAP_TEMPERATURE, temperature_payload)) {
        return fail("wireless_sequence_duplicate_canonical_missing");
    }
    if (!validateTemperaturePayload(temperature_payload)) {
        return false;
    }

    header.sequence_id = 101;
    value.value_float = 33.0F;
    header.checksum = calculateWirelessPacketChecksum(header, value, diagnostics);
    if (!wireless_transport_driver_.injectReceivedCapabilityValue(header, value, diagnostics)) {
        return fail("wireless_sequence_next_inject_failed");
    }
    if (!wireless_service_.processPackets(now_ms + 102)) {
        return fail("wireless_sequence_next_process_failed");
    }
    if (registry_.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            wireless_out_record) != RegistryResult::OK) {
        return fail("wireless_sequence_next_provider_get_failed");
    }
    if (wireless_out_record.latest_payload.value_float != 33.0F) {
        return fail("wireless_sequence_next_provider_invalid");
    }

    header.node_id = 1001;
    header.sequence_id = 102;
    value.value_float = 34.0F;
    header.checksum = calculateWirelessPacketChecksum(header, value, diagnostics);
    if (!wireless_transport_driver_.injectReceivedCapabilityValue(header, value, diagnostics)) {
        return fail("wireless_allow_allowed_inject_failed");
    }
    if (!wireless_service_.processPackets(now_ms + 103)) {
        return fail("wireless_allow_allowed_process_failed");
    }
    if (registry_.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            wireless_out_record) != RegistryResult::OK) {
        return fail("wireless_allow_allowed_provider_get_failed");
    }
    if (wireless_out_record.latest_payload.value_float != 34.0F) {
        return fail("wireless_allow_allowed_provider_invalid");
    }

    header.node_id = 2002;
    header.sequence_id = 103;
    value.value_float = 35.0F;
    header.checksum = calculateWirelessPacketChecksum(header, value, diagnostics);
    if (!wireless_transport_driver_.injectReceivedCapabilityValue(header, value, diagnostics)) {
        return fail("wireless_allow_unknown_inject_failed");
    }
    if (wireless_service_.processPackets(now_ms + 104)) {
        return fail("wireless_allow_unknown_process_succeeded");
    }
    if (!isSameText(wireless_service_.lastErrorCode(), "wireless_node_not_allowed")) {
        return fail("wireless_allow_unknown_error_invalid");
    }
    if (wireless_transport_driver_.hasReceivedPacket()) {
        return fail("wireless_allow_unknown_packet_not_cleared");
    }
    if (registry_.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            wireless_out_record) != RegistryResult::OK) {
        return fail("wireless_allow_unknown_provider_get_failed");
    }
    if (wireless_out_record.latest_payload.value_float != 34.0F) {
        return fail("wireless_allow_unknown_provider_changed");
    }
    if (!registry_.getCapabilityPayload(CAP_TEMPERATURE, temperature_payload)) {
        return fail("wireless_allow_unknown_canonical_missing");
    }
    if (!validateTemperaturePayload(temperature_payload)) {
        return false;
    }

    WirelessNodeAllowlistRecord blocked_allowlist_record = allowlist_record;
    blocked_allowlist_record.node_id = 3003;
    blocked_allowlist_record.allow_state = WirelessNodeAllowState::BLOCKED;
    blocked_allowlist_record.trust_state = WirelessTrustState::BLOCKED;
    blocked_allowlist_record.added_at_ms = now_ms + 105;
    blocked_allowlist_record.last_seen_ms = now_ms + 105;
    if (registry_.registerWirelessNodeAllowlistRecordWithResult(blocked_allowlist_record).result !=
        RegistryResult::OK) {
        return fail("wireless_allow_blocked_register_failed");
    }
    header.node_id = 3003;
    header.sequence_id = 104;
    value.value_float = 36.0F;
    header.checksum = calculateWirelessPacketChecksum(header, value, diagnostics);
    if (!wireless_transport_driver_.injectReceivedCapabilityValue(header, value, diagnostics)) {
        return fail("wireless_allow_blocked_inject_failed");
    }
    if (wireless_service_.processPackets(now_ms + 105)) {
        return fail("wireless_allow_blocked_process_succeeded");
    }
    if (!isSameText(wireless_service_.lastErrorCode(), "wireless_node_blocked")) {
        return fail("wireless_allow_blocked_error_invalid");
    }
    if (wireless_transport_driver_.hasReceivedPacket()) {
        return fail("wireless_allow_blocked_packet_not_cleared");
    }
    if (registry_.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            wireless_out_record) != RegistryResult::OK) {
        return fail("wireless_allow_blocked_provider_get_failed");
    }
    if (wireless_out_record.latest_payload.value_float != 34.0F) {
        return fail("wireless_allow_blocked_provider_changed");
    }
    if (!registry_.getCapabilityPayload(CAP_TEMPERATURE, temperature_payload)) {
        return fail("wireless_allow_blocked_canonical_missing");
    }
    if (!validateTemperaturePayload(temperature_payload)) {
        return false;
    }

    WirelessNodeAllowlistRecord allowed_untrusted_record = allowlist_record;
    allowed_untrusted_record.node_id = 4004;
    allowed_untrusted_record.allow_state = WirelessNodeAllowState::ALLOWED;
    allowed_untrusted_record.trust_state = WirelessTrustState::TRUSTED;
    allowed_untrusted_record.added_at_ms = now_ms + 106;
    allowed_untrusted_record.last_seen_ms = now_ms + 106;
    if (registry_.registerWirelessNodeAllowlistRecordWithResult(allowed_untrusted_record).result !=
        RegistryResult::OK) {
        return fail("wireless_allow_trust_register_failed");
    }
    wireless_temperature_device_.setTrustState(WirelessTrustState::BLOCKED);
    header.node_id = 4004;
    header.sequence_id = 105;
    value.value_float = 37.0F;
    header.checksum = calculateWirelessPacketChecksum(header, value, diagnostics);
    if (!wireless_transport_driver_.injectReceivedCapabilityValue(header, value, diagnostics)) {
        return fail("wireless_allow_trust_inject_failed");
    }
    if (wireless_service_.processPackets(now_ms + 106)) {
        return fail("wireless_allow_trust_process_succeeded");
    }
    if (!isSameText(wireless_service_.lastErrorCode(), "wireless_untrusted")) {
        return fail("wireless_allow_trust_error_invalid");
    }
    if (wireless_transport_driver_.hasReceivedPacket()) {
        return fail("wireless_allow_trust_packet_not_cleared");
    }
    if (registry_.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            wireless_out_record) != RegistryResult::OK) {
        return fail("wireless_allow_trust_provider_get_failed");
    }
    if (wireless_out_record.latest_payload.value_float != 34.0F) {
        return fail("wireless_allow_trust_provider_changed");
    }
    if (!registry_.getCapabilityPayload(CAP_TEMPERATURE, temperature_payload)) {
        return fail("wireless_allow_trust_canonical_missing");
    }
    if (!validateTemperaturePayload(temperature_payload)) {
        return false;
    }

    ApiCapabilityState wireless_node_diag_temperature_before;
    if (!api_.getTemperatureState(wireless_node_diag_temperature_before)) {
        return fail("wireless_node_diag_temperature_before_missing");
    }

    ApiWirelessNodeDiagnostic node_diagnostic;
    if (!api_.getWirelessNodeDiagnostic(1001, node_diagnostic)) {
        return fail("wireless_node_diag_get_failed");
    }
    if (!node_diagnostic.ok) {
        return fail("wireless_node_diag_not_ok");
    }
    if (node_diagnostic.registry_result != RegistryResult::OK) {
        return fail("wireless_node_diag_result_invalid");
    }
    if (node_diagnostic.node_id != 1001) {
        return fail("wireless_node_diag_node_invalid");
    }
    if (node_diagnostic.allow_state != WirelessNodeAllowState::ALLOWED) {
        return fail("wireless_node_diag_allow_invalid");
    }
    if (node_diagnostic.trust_state != WirelessTrustState::TRUSTED) {
        return fail("wireless_node_diag_trust_invalid");
    }

    if (!api_.getWirelessNodeDiagnosticByIndex(0, node_diagnostic)) {
        return fail("wireless_node_diag_index_get_failed");
    }
    if (!node_diagnostic.ok) {
        return fail("wireless_node_diag_index_not_ok");
    }
    if (node_diagnostic.node_id != 1001) {
        return fail("wireless_node_diag_index_node_invalid");
    }

    ApiWirelessNodeSummary node_summary;
    if (!api_.getWirelessNodeSummary(node_summary)) {
        return fail("wireless_node_summary_get_failed");
    }
    if (!node_summary.ok) {
        return fail("wireless_node_summary_not_ok");
    }
    if (node_summary.registry_result != RegistryResult::OK) {
        return fail("wireless_node_summary_result_invalid");
    }
    if (node_summary.node_count != registry_.wirelessNodeAllowlistCount()) {
        return fail("wireless_node_summary_count_invalid");
    }
    if (node_summary.allowed_count != 2) {
        return fail("wireless_node_summary_allowed_invalid");
    }
    if (node_summary.blocked_count != 1) {
        return fail("wireless_node_summary_blocked_invalid");
    }
    if (node_summary.unknown_count != 0) {
        return fail("wireless_node_summary_unknown_invalid");
    }

    if (api_.getWirelessNodeDiagnostic(9999, node_diagnostic)) {
        return fail("wireless_node_diag_missing_succeeded");
    }
    if (node_diagnostic.registry_result != RegistryResult::NOT_FOUND) {
        return fail("wireless_node_diag_missing_result_invalid");
    }
    if (api_.getWirelessNodeDiagnostic(0, node_diagnostic)) {
        return fail("wireless_node_diag_invalid_succeeded");
    }
    if (node_diagnostic.registry_result != RegistryResult::INVALID_ID) {
        return fail("wireless_node_diag_invalid_result_invalid");
    }
    if (api_.getWirelessNodeDiagnosticByIndex(99, node_diagnostic)) {
        return fail("wireless_node_diag_index_missing_succeeded");
    }
    if (node_diagnostic.registry_result != RegistryResult::NOT_FOUND) {
        return fail("wireless_node_diag_index_missing_result_invalid");
    }

    if (registry_.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            wireless_out_record) != RegistryResult::OK) {
        return fail("wireless_node_diag_provider_get_failed");
    }
    if (wireless_out_record.latest_payload.value_float != 34.0F) {
        return fail("wireless_node_diag_provider_changed");
    }

    ApiCapabilityState wireless_node_diag_temperature_after;
    if (!api_.getTemperatureState(wireless_node_diag_temperature_after)) {
        return fail("wireless_node_diag_temperature_after_missing");
    }
    if (wireless_node_diag_temperature_after.payload.value_float !=
        wireless_node_diag_temperature_before.payload.value_float) {
        return fail("wireless_node_diag_temperature_changed");
    }

    ActiveCapabilityProvider wireless_node_diag_active_provider;
    if (registry_.getActiveProvider(CAP_TEMPERATURE, wireless_node_diag_active_provider) !=
        RegistryResult::OK) {
        return fail("wireless_node_diag_active_get_failed");
    }
    if (!isSameText(wireless_node_diag_active_provider.provider_id, "provider-sim-temperature-001")) {
        return fail("wireless_node_diag_active_changed");
    }

    wireless_temperature_device_.setTrustState(WirelessTrustState::TRUSTED);
    header.node_id = 1001;

    wireless_temperature_device_.setTrustState(WirelessTrustState::TRUSTED);
    header.packet_type = WirelessPacketType::CAPABILITY_VALUE;
    header.sequence_id = 20;
    value.value_float = 26.0F;
    header.checksum = calculateWirelessPacketChecksum(header, value, diagnostics);
    if (!wireless_transport_driver_.injectReceivedCapabilityValue(header, value, diagnostics)) {
        return fail("wireless_trust_trusted_inject_failed");
    }
    if (!wireless_service_.processPackets(now_ms + 20)) {
        return fail("wireless_trust_trusted_process_failed");
    }
    if (registry_.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            wireless_out_record) != RegistryResult::OK) {
        return fail("wireless_trust_trusted_provider_get_failed");
    }
    if (wireless_out_record.latest_payload.value_float != 26.0F) {
        return fail("wireless_trust_trusted_provider_invalid");
    }
    if (!registry_.getCapabilityPayload(CAP_TEMPERATURE, temperature_payload)) {
        return fail("wireless_trust_canonical_before_missing");
    }
    if (!validateTemperaturePayload(temperature_payload)) {
        return false;
    }

    wireless_temperature_device_.setTrustState(WirelessTrustState::UNTRUSTED);
    header.sequence_id = 21;
    value.value_float = 27.0F;
    header.checksum = calculateWirelessPacketChecksum(header, value, diagnostics);
    if (!wireless_transport_driver_.injectReceivedCapabilityValue(header, value, diagnostics)) {
        return fail("wireless_trust_untrusted_inject_failed");
    }
    if (wireless_service_.processPackets(now_ms + 21)) {
        return fail("wireless_trust_untrusted_process_succeeded");
    }
    if (!isSameText(wireless_service_.lastErrorCode(), "wireless_untrusted")) {
        return fail("wireless_trust_untrusted_error_invalid");
    }
    if (wireless_transport_driver_.hasReceivedPacket()) {
        return fail("wireless_trust_untrusted_packet_not_cleared");
    }
    if (registry_.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            wireless_out_record) != RegistryResult::OK) {
        return fail("wireless_trust_untrusted_provider_get_failed");
    }
    if (wireless_out_record.latest_payload.value_float != 26.0F) {
        return fail("wireless_trust_untrusted_provider_changed");
    }
    if (!registry_.getCapabilityPayload(CAP_TEMPERATURE, temperature_payload)) {
        return fail("wireless_trust_untrusted_canonical_missing");
    }
    if (!validateTemperaturePayload(temperature_payload)) {
        return false;
    }

    wireless_temperature_device_.setTrustState(WirelessTrustState::BLOCKED);
    header.sequence_id = 22;
    value.value_float = 28.0F;
    header.checksum = calculateWirelessPacketChecksum(header, value, diagnostics);
    if (!wireless_transport_driver_.injectReceivedCapabilityValue(header, value, diagnostics)) {
        return fail("wireless_trust_blocked_inject_failed");
    }
    if (wireless_service_.processPackets(now_ms + 22)) {
        return fail("wireless_trust_blocked_process_succeeded");
    }
    if (!isSameText(wireless_service_.lastErrorCode(), "wireless_untrusted")) {
        return fail("wireless_trust_blocked_error_invalid");
    }
    if (wireless_transport_driver_.hasReceivedPacket()) {
        return fail("wireless_trust_blocked_packet_not_cleared");
    }
    if (registry_.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            wireless_out_record) != RegistryResult::OK) {
        return fail("wireless_trust_blocked_provider_get_failed");
    }
    if (wireless_out_record.latest_payload.value_float != 26.0F) {
        return fail("wireless_trust_blocked_provider_changed");
    }
    if (!registry_.getCapabilityPayload(CAP_TEMPERATURE, temperature_payload)) {
        return fail("wireless_trust_blocked_canonical_missing");
    }
    if (!validateTemperaturePayload(temperature_payload)) {
        return false;
    }

    wireless_temperature_device_.setTrustState(WirelessTrustState::UNKNOWN);
    header.sequence_id = 23;
    value.value_float = 29.0F;
    header.checksum = calculateWirelessPacketChecksum(header, value, diagnostics);
    if (!wireless_transport_driver_.injectReceivedCapabilityValue(header, value, diagnostics)) {
        return fail("wireless_trust_unknown_inject_failed");
    }
    if (wireless_service_.processPackets(now_ms + 23)) {
        return fail("wireless_trust_unknown_process_succeeded");
    }
    if (!isSameText(wireless_service_.lastErrorCode(), "wireless_untrusted")) {
        return fail("wireless_trust_unknown_error_invalid");
    }
    if (wireless_transport_driver_.hasReceivedPacket()) {
        return fail("wireless_trust_unknown_packet_not_cleared");
    }
    if (registry_.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            wireless_out_record) != RegistryResult::OK) {
        return fail("wireless_trust_unknown_provider_get_failed");
    }
    if (wireless_out_record.latest_payload.value_float != 26.0F) {
        return fail("wireless_trust_unknown_provider_changed");
    }
    if (!registry_.getCapabilityPayload(CAP_TEMPERATURE, temperature_payload)) {
        return fail("wireless_trust_unknown_canonical_missing");
    }
    if (!validateTemperaturePayload(temperature_payload)) {
        return false;
    }

    wireless_temperature_device_.setTrustState(WirelessTrustState::TRUSTED);
    header.sequence_id = 24;
    value.value_float = 23.5F;
    header.checksum = calculateWirelessPacketChecksum(header, value, diagnostics);
    if (!wireless_transport_driver_.injectReceivedCapabilityValue(header, value, diagnostics)) {
        return fail("wireless_trust_restore_inject_failed");
    }
    if (!wireless_service_.processPackets(now_ms + 24)) {
        return fail("wireless_trust_restore_process_failed");
    }
    if (registry_.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            wireless_out_record) != RegistryResult::OK) {
        return fail("wireless_trust_restore_provider_get_failed");
    }
    if (wireless_out_record.latest_payload.value_float != 23.5F) {
        return fail("wireless_trust_restore_provider_invalid");
    }

    header.sequence_id = 2;
    header.packet_type = WirelessPacketType::NODE_HEARTBEAT;
    header.checksum = calculateWirelessPacketChecksum(header, value, diagnostics);
    if (!wireless_transport_driver_.injectReceivedCapabilityValue(header, value, diagnostics)) {
        return fail("wireless_invalid_packet_inject_failed");
    }
    if (wireless_service_.processPackets(now_ms + 1)) {
        return fail("wireless_invalid_packet_process_succeeded");
    }
    if (registry_.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            wireless_out_record) != RegistryResult::OK) {
        return fail("wireless_provider_get_after_invalid_failed");
    }
    if (wireless_out_record.latest_payload.value_float != 23.5F) {
        return fail("wireless_provider_changed_after_invalid");
    }

    ActiveCapabilityProvider selected_provider;
    if (registry_.selectBestProvider(CAP_TEMPERATURE, selected_provider) != RegistryResult::OK) {
        return fail("provider_select_result_invalid");
    }
    if (!isSameText(selected_provider.provider_id, WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID)) {
        return fail("provider_select_priority_invalid");
    }

    if (registry_.setActiveProvider(CAP_TEMPERATURE, WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID) !=
        RegistryResult::OK) {
        return fail("active_provider_update_result_invalid");
    }
    if (registry_.activeProviderCount() != 1) {
        return fail("active_provider_count_after_update_invalid");
    }
    if (registry_.getActiveProvider(CAP_TEMPERATURE, active_provider) != RegistryResult::OK) {
        return fail("active_provider_get_after_update_invalid");
    }
    if (!isSameText(active_provider.provider_id, WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID)) {
        return fail("active_provider_update_id_invalid");
    }
    if (registry_.updateSelectedCapabilityPayload(CAP_TEMPERATURE) != RegistryResult::OK) {
        return fail("selected_payload_update_result_invalid");
    }
    if (!registry_.getCapabilityPayload(CAP_TEMPERATURE, temperature_payload)) {
        return fail("selected_payload_temperature_missing");
    }
    if (temperature_payload.value_float != 23.5F) {
        return fail("selected_payload_temperature_value_invalid");
    }
    if (registry_.updateBestCapabilityPayload(0) != RegistryResult::INVALID_ID) {
        return fail("best_payload_null_id_result_invalid");
    }
    if (registry_.updateBestCapabilityPayload("") != RegistryResult::INVALID_ID) {
        return fail("best_payload_empty_id_result_invalid");
    }
    if (registry_.updateBestCapabilityPayload(CAP_TEMPERATURE) != RegistryResult::OK) {
        return fail("best_payload_wireless_result_invalid");
    }
    if (registry_.getActiveProvider(CAP_TEMPERATURE, active_provider) != RegistryResult::OK) {
        return fail("best_payload_wireless_active_missing");
    }
    if (!isSameText(active_provider.provider_id, WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID)) {
        return fail("best_payload_wireless_active_invalid");
    }
    if (!registry_.getCapabilityPayload(CAP_TEMPERATURE, temperature_payload)) {
        return fail("best_payload_wireless_temperature_missing");
    }
    if (temperature_payload.value_float != 23.5F) {
        return fail("best_payload_wireless_temperature_invalid");
    }

    if (registry_.updateCapabilityProviderPayload(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            wireless_out_record.latest_payload,
            CapabilityProviderStatus::LOST,
            now_ms + 2) != RegistryResult::OK) {
        return fail("best_payload_wireless_lost_update_failed");
    }
    if (registry_.updateBestCapabilityPayload(CAP_TEMPERATURE) != RegistryResult::OK) {
        return fail("best_payload_failover_result_invalid");
    }
    if (registry_.getActiveProvider(CAP_TEMPERATURE, active_provider) != RegistryResult::OK) {
        return fail("best_payload_failover_active_missing");
    }
    if (!isSameText(active_provider.provider_id, "provider-sim-temperature-001")) {
        return fail("best_payload_failover_active_invalid");
    }
    if (!registry_.getCapabilityPayload(CAP_TEMPERATURE, temperature_payload)) {
        return fail("best_payload_failover_temperature_missing");
    }
    if (temperature_payload.value_float != 22.4F) {
        return fail("best_payload_failover_temperature_invalid");
    }

    CapabilityPayload recovered_wireless_payload = wireless_out_record.latest_payload;
    recovered_wireless_payload.timestamp_ms = now_ms + 3;
    recovered_wireless_payload.value_float = 24.0F;
    if (registry_.updateCapabilityProviderPayload(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            recovered_wireless_payload,
            CapabilityProviderStatus::AVAILABLE,
            now_ms + 3) != RegistryResult::OK) {
        return fail("best_payload_wireless_recovery_update_failed");
    }
    if (registry_.updateBestCapabilityPayload(CAP_TEMPERATURE) != RegistryResult::OK) {
        return fail("best_payload_recovery_result_invalid");
    }
    if (registry_.getActiveProvider(CAP_TEMPERATURE, active_provider) != RegistryResult::OK) {
        return fail("best_payload_recovery_active_missing");
    }
    if (!isSameText(active_provider.provider_id, WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID)) {
        return fail("best_payload_recovery_active_invalid");
    }
    if (!registry_.getCapabilityPayload(CAP_TEMPERATURE, temperature_payload)) {
        return fail("best_payload_recovery_temperature_missing");
    }
    if (temperature_payload.value_float != 24.0F) {
        return fail("best_payload_recovery_temperature_invalid");
    }

    CapabilityPayload sim_unavailable_payload = provider.latest_payload;
    if (registry_.updateCapabilityProviderPayload(
            "provider-sim-temperature-001",
            sim_unavailable_payload,
            CapabilityProviderStatus::UNAVAILABLE,
            now_ms + 4) != RegistryResult::OK) {
        return fail("best_payload_sim_unavailable_update_failed");
    }
    if (registry_.updateCapabilityProviderPayload(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            recovered_wireless_payload,
            CapabilityProviderStatus::LOST,
            now_ms + 4) != RegistryResult::OK) {
        return fail("best_payload_wireless_lost_again_failed");
    }
    if (registry_.updateBestCapabilityPayload(CAP_TEMPERATURE) != RegistryResult::NOT_FOUND) {
        return fail("best_payload_no_eligible_result_invalid");
    }
    if (!registry_.getCapabilityPayload(CAP_TEMPERATURE, temperature_payload)) {
        return fail("best_payload_no_eligible_temperature_missing");
    }
    if (temperature_payload.value_float != 24.0F) {
        return fail("best_payload_no_eligible_temperature_changed");
    }
    if (registry_.updateCapabilityProviderPayload(
            "provider-sim-temperature-001",
            provider.latest_payload,
            CapabilityProviderStatus::AVAILABLE,
            now_ms + 5) != RegistryResult::OK) {
        return fail("best_payload_sim_restore_failed");
    }
    if (registry_.updateCapabilityProviderPayload(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            recovered_wireless_payload,
            CapabilityProviderStatus::AVAILABLE,
            now_ms + 5) != RegistryResult::OK) {
        return fail("best_payload_wireless_restore_failed");
    }

    const uint32_t health_base_ms = now_ms + 1000;
    CapabilityPayload health_wireless_payload = wireless_out_record.latest_payload;
    health_wireless_payload.timestamp_ms = health_base_ms;
    health_wireless_payload.value_float = 23.5F;
    if (registry_.updateCapabilityProviderPayload(
            "provider-sim-temperature-001",
            provider.latest_payload,
            CapabilityProviderStatus::AVAILABLE,
            health_base_ms) != RegistryResult::OK) {
        return fail("health_sim_available_update_failed");
    }
    if (registry_.updateCapabilityProviderPayload(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            health_wireless_payload,
            CapabilityProviderStatus::AVAILABLE,
            health_base_ms) != RegistryResult::OK) {
        return fail("health_wireless_available_update_failed");
    }
    if (registry_.updateBestCapabilityPayload(CAP_TEMPERATURE) != RegistryResult::OK) {
        return fail("health_initial_best_update_failed");
    }
    if (!registry_.getCapabilityPayload(CAP_TEMPERATURE, temperature_payload)) {
        return fail("health_initial_temperature_missing");
    }
    if (temperature_payload.value_float != 23.5F) {
        return fail("health_initial_temperature_invalid");
    }

    if (registry_.updateProviderHealth(health_base_ms + 1) != RegistryResult::OK) {
        return fail("health_fresh_scan_failed");
    }
    if (registry_.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            wireless_out_record) != RegistryResult::OK) {
        return fail("health_fresh_wireless_get_failed");
    }
    if (wireless_out_record.status != CapabilityProviderStatus::AVAILABLE) {
        return fail("health_fresh_wireless_status_invalid");
    }

    if (registry_.updateProviderHealth(
            health_base_ms + Registry::PROVIDER_STALE_TIMEOUT_MS) != RegistryResult::OK) {
        return fail("health_stale_scan_failed");
    }
    if (registry_.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            wireless_out_record) != RegistryResult::OK) {
        return fail("health_stale_wireless_get_failed");
    }
    if (wireless_out_record.status != CapabilityProviderStatus::STALE) {
        return fail("health_stale_wireless_status_invalid");
    }

    if (registry_.updateProviderHealth(
            health_base_ms + Registry::PROVIDER_LOST_TIMEOUT_MS) != RegistryResult::OK) {
        return fail("health_lost_scan_failed");
    }
    if (registry_.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            wireless_out_record) != RegistryResult::OK) {
        return fail("health_lost_wireless_get_failed");
    }
    if (wireless_out_record.status != CapabilityProviderStatus::LOST) {
        return fail("health_lost_wireless_status_invalid");
    }

    if (registry_.updateProviderHealth(
            health_base_ms + Registry::PROVIDER_LOST_TIMEOUT_MS + 1000) != RegistryResult::OK) {
        return fail("health_lost_repeat_scan_failed");
    }
    if (registry_.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            wireless_out_record) != RegistryResult::OK) {
        return fail("health_lost_repeat_wireless_get_failed");
    }
    if (wireless_out_record.status != CapabilityProviderStatus::LOST) {
        return fail("health_lost_repeat_status_invalid");
    }

    if (!registry_.getCapabilityPayload(CAP_TEMPERATURE, temperature_payload)) {
        return fail("health_canonical_after_scan_missing");
    }
    if (temperature_payload.value_float != 23.5F) {
        return fail("health_canonical_rewritten_by_scan");
    }

    if (registry_.updateCapabilityProviderPayload(
            "provider-sim-temperature-001",
            provider.latest_payload,
            CapabilityProviderStatus::UNAVAILABLE,
            health_base_ms + Registry::PROVIDER_LOST_TIMEOUT_MS + 1100) != RegistryResult::OK) {
        return fail("health_sim_unavailable_update_failed");
    }
    if (registry_.updateProviderHealth(
            health_base_ms + Registry::PROVIDER_LOST_TIMEOUT_MS + 2100) != RegistryResult::OK) {
        return fail("health_unavailable_scan_failed");
    }
    if (registry_.getCapabilityProvider("provider-sim-temperature-001", out_record) !=
        RegistryResult::OK) {
        return fail("health_unavailable_sim_get_failed");
    }
    if (out_record.status != CapabilityProviderStatus::UNAVAILABLE) {
        return fail("health_unavailable_status_invalid");
    }

    if (registry_.updateCapabilityProviderPayload(
            "provider-sim-temperature-001",
            provider.latest_payload,
            CapabilityProviderStatus::DISABLED,
            health_base_ms + Registry::PROVIDER_LOST_TIMEOUT_MS + 2200) != RegistryResult::OK) {
        return fail("health_sim_disabled_update_failed");
    }
    if (registry_.updateProviderHealth(
            health_base_ms + Registry::PROVIDER_LOST_TIMEOUT_MS + 3200) != RegistryResult::OK) {
        return fail("health_disabled_scan_failed");
    }
    if (registry_.getCapabilityProvider("provider-sim-temperature-001", out_record) !=
        RegistryResult::OK) {
        return fail("health_disabled_sim_get_failed");
    }
    if (out_record.status != CapabilityProviderStatus::DISABLED) {
        return fail("health_disabled_status_invalid");
    }

    if (registry_.updateCapabilityProviderPayload(
            "provider-sim-temperature-001",
            provider.latest_payload,
            CapabilityProviderStatus::AVAILABLE,
            health_base_ms + Registry::PROVIDER_LOST_TIMEOUT_MS + 3300) != RegistryResult::OK) {
        return fail("health_sim_restore_available_failed");
    }
    if (registry_.selectBestProvider(CAP_TEMPERATURE, selected_provider) != RegistryResult::OK) {
        return fail("health_select_after_lost_failed");
    }
    if (!isSameText(selected_provider.provider_id, "provider-sim-temperature-001")) {
        return fail("health_lost_ignored_by_select_invalid");
    }
    if (registry_.updateBestCapabilityPayload(CAP_TEMPERATURE) != RegistryResult::OK) {
        return fail("health_failover_best_update_failed");
    }
    if (!registry_.getCapabilityPayload(CAP_TEMPERATURE, temperature_payload)) {
        return fail("health_failover_temperature_missing");
    }
    if (temperature_payload.value_float != 22.4F) {
        return fail("health_failover_temperature_invalid");
    }

    recovered_wireless_payload.timestamp_ms = health_base_ms + Registry::PROVIDER_LOST_TIMEOUT_MS + 3400;
    recovered_wireless_payload.value_float = 24.0F;
    if (registry_.updateCapabilityProviderPayload(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            recovered_wireless_payload,
            CapabilityProviderStatus::AVAILABLE,
            health_base_ms + Registry::PROVIDER_LOST_TIMEOUT_MS + 3400) != RegistryResult::OK) {
        return fail("health_wireless_recovery_update_failed");
    }
    if (registry_.updateBestCapabilityPayload(CAP_TEMPERATURE) != RegistryResult::OK) {
        return fail("health_recovery_best_update_failed");
    }
    if (!registry_.getCapabilityPayload(CAP_TEMPERATURE, temperature_payload)) {
        return fail("health_recovery_temperature_missing");
    }
    if (temperature_payload.value_float != 24.0F) {
        return fail("health_recovery_temperature_invalid");
    }
    if (registry_.updateCapabilityProviderPayload(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            recovered_wireless_payload,
            CapabilityProviderStatus::AVAILABLE,
            now_ms + 5) != RegistryResult::OK) {
        return fail("health_wireless_timestamp_restore_failed");
    }

    const uint32_t wireless_timeout_base_ms = now_ms + 50000;
    CapabilityPayload timeout_wireless_payload = wireless_out_record.latest_payload;
    timeout_wireless_payload.timestamp_ms = wireless_timeout_base_ms;
    timeout_wireless_payload.value_float = 23.5F;
    if (registry_.updateCapabilityProviderPayload(
            "provider-sim-temperature-001",
            provider.latest_payload,
            CapabilityProviderStatus::AVAILABLE,
            wireless_timeout_base_ms + Registry::PROVIDER_STALE_TIMEOUT_MS) != RegistryResult::OK) {
        return fail("wireless_timeout_sim_setup_failed");
    }
    if (registry_.updateCapabilityProviderPayload(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            timeout_wireless_payload,
            CapabilityProviderStatus::AVAILABLE,
            wireless_timeout_base_ms) != RegistryResult::OK) {
        return fail("wireless_timeout_wireless_setup_failed");
    }
    if (registry_.updateBestCapabilityPayload(CAP_TEMPERATURE) != RegistryResult::OK) {
        return fail("wireless_timeout_initial_best_failed");
    }
    if (!registry_.getCapabilityPayload(CAP_TEMPERATURE, temperature_payload)) {
        return fail("wireless_timeout_initial_temperature_missing");
    }
    if (temperature_payload.value_float != 23.5F) {
        return fail("wireless_timeout_initial_temperature_invalid");
    }
    if (!wireless_service_.checkTimeouts(wireless_timeout_base_ms + Registry::PROVIDER_STALE_TIMEOUT_MS)) {
        return fail("wireless_timeout_stale_check_failed");
    }
    if (registry_.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            wireless_out_record) != RegistryResult::OK) {
        return fail("wireless_timeout_stale_provider_get_failed");
    }
    if (wireless_out_record.status != CapabilityProviderStatus::STALE) {
        return fail("wireless_timeout_provider_not_stale");
    }
    if (registry_.updateCapabilityProviderPayload(
            "provider-sim-temperature-001",
            provider.latest_payload,
            CapabilityProviderStatus::AVAILABLE,
            wireless_timeout_base_ms + Registry::PROVIDER_LOST_TIMEOUT_MS) != RegistryResult::OK) {
        return fail("wireless_timeout_sim_refresh_failed");
    }
    if (!wireless_service_.checkTimeouts(wireless_timeout_base_ms + Registry::PROVIDER_LOST_TIMEOUT_MS)) {
        return fail("wireless_timeout_check_failed");
    }
    if (registry_.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            wireless_out_record) != RegistryResult::OK) {
        return fail("wireless_timeout_provider_get_failed");
    }
    if (wireless_out_record.status != CapabilityProviderStatus::LOST) {
        return fail("wireless_timeout_provider_not_lost");
    }
    if (registry_.getActiveProvider(CAP_TEMPERATURE, active_provider) != RegistryResult::OK) {
        return fail("wireless_timeout_active_get_failed");
    }
    if (!isSameText(active_provider.provider_id, "provider-sim-temperature-001")) {
        return fail("wireless_timeout_active_invalid");
    }
    if (!registry_.getCapabilityPayload(CAP_TEMPERATURE, temperature_payload)) {
        return fail("wireless_timeout_temperature_missing");
    }
    if (temperature_payload.value_float != 22.4F) {
        return fail("wireless_timeout_temperature_invalid");
    }

    if (registry_.updateCapabilityProviderPayload(
            "provider-sim-temperature-001",
            provider.latest_payload,
            CapabilityProviderStatus::UNAVAILABLE,
            wireless_timeout_base_ms + Registry::PROVIDER_LOST_TIMEOUT_MS + 1) != RegistryResult::OK) {
        return fail("wireless_timeout_sim_unavailable_failed");
    }
    if (registry_.updateCapabilityProviderPayload(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            timeout_wireless_payload,
            CapabilityProviderStatus::LOST,
            wireless_timeout_base_ms + Registry::PROVIDER_LOST_TIMEOUT_MS + 1) != RegistryResult::OK) {
        return fail("wireless_timeout_wireless_lost_failed");
    }
    if (wireless_service_.checkTimeouts(wireless_timeout_base_ms + Registry::PROVIDER_LOST_TIMEOUT_MS + 2)) {
        return fail("wireless_timeout_no_provider_succeeded");
    }
    if (isSameText(wireless_service_.lastErrorCode(), "none")) {
        return fail("wireless_timeout_no_provider_error_invalid");
    }
    if (!registry_.getCapabilityPayload(CAP_TEMPERATURE, temperature_payload)) {
        return fail("wireless_timeout_no_provider_temperature_missing");
    }
    if (temperature_payload.value_float != 22.4F) {
        return fail("wireless_timeout_no_provider_temperature_changed");
    }

    header.packet_type = WirelessPacketType::CAPABILITY_VALUE;
    header.sequence_id = 3;
    header.node_id = 1001;
    copyWirelessCapabilityId(value.capability_id, CAP_TEMPERATURE);
    value.payload_type = WirelessPayloadType::FLOAT;
    value.value_float = 24.0F;
    value.value_int = 0;
    copyWirelessCapabilityId(value.error_code, "none");
    const uint32_t wireless_recovery_ms =
        wireless_timeout_base_ms + Registry::PROVIDER_LOST_TIMEOUT_MS + 3;
    header.checksum = calculateWirelessPacketChecksum(header, value, diagnostics);
    if (!wireless_transport_driver_.injectReceivedCapabilityValue(header, value, diagnostics)) {
        return fail("wireless_timeout_recovery_inject_failed");
    }
    if (!wireless_service_.processPackets(wireless_recovery_ms)) {
        return fail("wireless_timeout_recovery_process_failed");
    }
    if (!wireless_service_.checkTimeouts(wireless_recovery_ms)) {
        return fail("wireless_timeout_recovery_check_failed");
    }
    if (registry_.getActiveProvider(CAP_TEMPERATURE, active_provider) != RegistryResult::OK) {
        return fail("wireless_timeout_recovery_active_get_failed");
    }
    if (!isSameText(active_provider.provider_id, WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID)) {
        return fail("wireless_timeout_recovery_active_invalid");
    }
    if (!registry_.getCapabilityPayload(CAP_TEMPERATURE, temperature_payload)) {
        return fail("wireless_timeout_recovery_temperature_missing");
    }
    if (temperature_payload.value_float != 24.0F) {
        return fail("wireless_timeout_recovery_temperature_invalid");
    }

    const uint32_t runtime_wireless_process_ms = now_ms + 70000;
    CapabilityPayload runtime_wireless_payload = wireless_out_record.latest_payload;
    runtime_wireless_payload.timestamp_ms = runtime_wireless_process_ms;
    runtime_wireless_payload.value_float = 23.5F;
    if (registry_.updateCapabilityProviderPayload(
            "provider-sim-temperature-001",
            provider.latest_payload,
            CapabilityProviderStatus::AVAILABLE,
            runtime_wireless_process_ms) != RegistryResult::OK) {
        return fail("runtime_wireless_sim_setup_failed");
    }
    if (registry_.updateCapabilityProviderPayload(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            runtime_wireless_payload,
            CapabilityProviderStatus::AVAILABLE,
            runtime_wireless_process_ms) != RegistryResult::OK) {
        return fail("runtime_wireless_provider_setup_failed");
    }

    header.packet_type = WirelessPacketType::CAPABILITY_VALUE;
    header.sequence_id = 4;
    header.node_id = 1001;
    copyWirelessCapabilityId(value.capability_id, CAP_TEMPERATURE);
    value.payload_type = WirelessPayloadType::FLOAT;
    value.value_float = 25.0F;
    value.value_int = 0;
    copyWirelessCapabilityId(value.error_code, "none");
    header.checksum = calculateWirelessPacketChecksum(header, value, diagnostics);
    if (!wireless_transport_driver_.injectReceivedCapabilityValue(header, value, diagnostics)) {
        return fail("runtime_wireless_packet_inject_failed");
    }
    wireless_service_process_task_context_.now_ms = runtime_wireless_process_ms;
    wireless_service_process_task_context_.ran = false;
    wireless_service_process_task_context_.last_result = false;
    wireless_service_process_task_context_.last_error = "not_run";
    wireless_service_timeout_task_context_.now_ms = runtime_wireless_process_ms;
    wireless_service_timeout_task_context_.ran = false;
    wireless_service_timeout_task_context_.last_result = false;
    wireless_service_timeout_task_context_.last_error = "not_run";
    runtime_.update(runtime_wireless_process_ms);
    if (!wireless_service_process_task_context_.ran) {
        return fail("runtime_wireless_process_task_not_run");
    }
    if (!wireless_service_process_task_context_.last_result) {
        return fail(wireless_service_process_task_context_.last_error);
    }
    if (wireless_transport_driver_.hasReceivedPacket()) {
        return fail("runtime_wireless_packet_not_cleared");
    }
    if (registry_.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            wireless_out_record) != RegistryResult::OK) {
        return fail("runtime_wireless_provider_get_failed");
    }
    if (wireless_out_record.latest_payload.value_float != 25.0F) {
        return fail("runtime_wireless_provider_value_invalid");
    }
    if (!wireless_service_timeout_task_context_.ran) {
        return fail("runtime_wireless_timeout_task_not_run");
    }
    if (!wireless_service_timeout_task_context_.last_result) {
        return fail(wireless_service_timeout_task_context_.last_error);
    }
    if (!registry_.getCapabilityPayload(CAP_TEMPERATURE, temperature_payload)) {
        return fail("runtime_wireless_temperature_missing");
    }
    if (temperature_payload.value_float != 25.0F) {
        return fail("runtime_wireless_temperature_invalid");
    }

    const uint32_t runtime_wireless_timeout_base_ms = runtime_wireless_process_ms + 5000;
    CapabilityPayload timeout_task_wireless_payload = wireless_out_record.latest_payload;
    timeout_task_wireless_payload.timestamp_ms = runtime_wireless_timeout_base_ms;
    timeout_task_wireless_payload.value_float = 25.0F;
    if (registry_.updateCapabilityProviderPayload(
            "provider-sim-temperature-001",
            provider.latest_payload,
            CapabilityProviderStatus::AVAILABLE,
            runtime_wireless_timeout_base_ms + Registry::PROVIDER_LOST_TIMEOUT_MS) != RegistryResult::OK) {
        return fail("runtime_wireless_timeout_sim_setup_failed");
    }
    if (registry_.updateCapabilityProviderPayload(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            timeout_task_wireless_payload,
            CapabilityProviderStatus::STALE,
            runtime_wireless_timeout_base_ms) != RegistryResult::OK) {
        return fail("runtime_wireless_timeout_provider_setup_failed");
    }
    if (registry_.updateBestCapabilityPayload(CAP_TEMPERATURE) != RegistryResult::OK) {
        return fail("runtime_wireless_timeout_initial_best_failed");
    }
    if (!registry_.getCapabilityPayload(CAP_TEMPERATURE, temperature_payload)) {
        return fail("runtime_wireless_timeout_initial_missing");
    }
    if (temperature_payload.value_float != 25.0F) {
        return fail("runtime_wireless_timeout_initial_invalid");
    }
    wireless_service_process_task_context_.now_ms =
        runtime_wireless_timeout_base_ms + Registry::PROVIDER_LOST_TIMEOUT_MS;
    wireless_service_process_task_context_.ran = false;
    wireless_service_process_task_context_.last_result = false;
    wireless_service_process_task_context_.last_error = "not_run";
    wireless_service_timeout_task_context_.now_ms =
        runtime_wireless_timeout_base_ms + Registry::PROVIDER_LOST_TIMEOUT_MS;
    wireless_service_timeout_task_context_.ran = false;
    wireless_service_timeout_task_context_.last_result = false;
    wireless_service_timeout_task_context_.last_error = "not_run";
    runtime_.update(runtime_wireless_timeout_base_ms + Registry::PROVIDER_LOST_TIMEOUT_MS);
    if (!wireless_service_timeout_task_context_.ran) {
        return fail("runtime_wireless_timeout_not_run");
    }
    if (!wireless_service_timeout_task_context_.last_result) {
        return fail(wireless_service_timeout_task_context_.last_error);
    }
    if (registry_.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            wireless_out_record) != RegistryResult::OK) {
        return fail("runtime_wireless_timeout_provider_get_failed");
    }
    if (wireless_out_record.status != CapabilityProviderStatus::LOST) {
        return fail("runtime_wireless_timeout_provider_not_lost");
    }
    if (registry_.getActiveProvider(CAP_TEMPERATURE, active_provider) != RegistryResult::OK) {
        return fail("runtime_wireless_timeout_active_get_failed");
    }
    if (!isSameText(active_provider.provider_id, "provider-sim-temperature-001")) {
        return fail("runtime_wireless_timeout_active_invalid");
    }
    if (!registry_.getCapabilityPayload(CAP_TEMPERATURE, temperature_payload)) {
        return fail("runtime_wireless_timeout_temperature_missing");
    }
    if (temperature_payload.value_float != 22.4F) {
        return fail("runtime_wireless_timeout_temperature_invalid");
    }

    if (registry_.updateCapabilityProviderPayload(
            "provider-sim-temperature-001",
            provider.latest_payload,
            CapabilityProviderStatus::AVAILABLE,
            now_ms + 5) != RegistryResult::OK) {
        return fail("wireless_timeout_sim_restore_failed");
    }
    recovered_wireless_payload.timestamp_ms = now_ms + 5;
    recovered_wireless_payload.value_float = 24.0F;
    if (registry_.updateCapabilityProviderPayload(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            recovered_wireless_payload,
            CapabilityProviderStatus::AVAILABLE,
            now_ms + 5) != RegistryResult::OK) {
        return fail("wireless_timeout_wireless_restore_failed");
    }
    if (registry_.updateBestCapabilityPayload(CAP_TEMPERATURE) != RegistryResult::OK) {
        return fail("wireless_timeout_best_restore_failed");
    }

    CapabilityProviderRecord newer_provider = provider;
    newer_provider.provider_id = "provider-newer-temperature-001";
    newer_provider.provider_type = CapabilityProviderType::WIRELESS;
    newer_provider.status = CapabilityProviderStatus::AVAILABLE;
    newer_provider.priority = 20;
    newer_provider.last_update_ms = now_ms + 20;
    RegistryWriteResult newer_result =
        registry_.registerCapabilityProviderWithResult(newer_provider);
    if (newer_result.result != RegistryResult::OK) {
        return fail("newer_provider_register_result_invalid");
    }
    if (registry_.selectBestProvider(CAP_TEMPERATURE, selected_provider) != RegistryResult::OK) {
        return fail("provider_select_tie_result_invalid");
    }
    if (!isSameText(selected_provider.provider_id, "provider-newer-temperature-001")) {
        return fail("provider_select_tie_invalid");
    }

    CapabilityProviderRecord unavailable_provider = provider;
    unavailable_provider.provider_id = "provider-unavailable-temperature-001";
    unavailable_provider.status = CapabilityProviderStatus::UNAVAILABLE;
    unavailable_provider.priority = 250;
    unavailable_provider.last_update_ms = now_ms + 30;
    RegistryWriteResult unavailable_result =
        registry_.registerCapabilityProviderWithResult(unavailable_provider);
    if (unavailable_result.result != RegistryResult::OK) {
        return fail("unavailable_provider_register_result_invalid");
    }
    if (registry_.selectBestProvider(CAP_TEMPERATURE, selected_provider) != RegistryResult::OK) {
        return fail("provider_select_unavailable_result_invalid");
    }
    if (!isSameText(selected_provider.provider_id, "provider-newer-temperature-001")) {
        return fail("provider_select_unavailable_invalid");
    }
    if (registry_.getActiveProvider(CAP_TEMPERATURE, active_provider) != RegistryResult::OK) {
        return fail("active_provider_get_after_select_invalid");
    }
    if (!isSameText(active_provider.provider_id, WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID)) {
        return fail("active_provider_changed_by_select");
    }

    if (registry_.setActiveProvider(CAP_TEMPERATURE, "provider-missing") !=
        RegistryResult::NOT_FOUND) {
        return fail("active_provider_missing_result_invalid");
    }

    if (registry_.selectBestProvider("CAP_MISSING_PROVIDER_TEST", selected_provider) !=
        RegistryResult::NOT_FOUND) {
        return fail("provider_select_missing_result_invalid");
    }
    if (registry_.selectBestProvider(0, selected_provider) != RegistryResult::INVALID_ID) {
        return fail("provider_select_null_result_invalid");
    }
    if (registry_.selectBestProvider("", selected_provider) != RegistryResult::INVALID_ID) {
        return fail("provider_select_empty_result_invalid");
    }

    CapabilityPayload distance_payload;
    if (!registry_.getCapabilityPayload(CAP_DISTANCE, distance_payload)) {
        return fail("provider_distance_payload_missing");
    }

    CapabilityProviderRecord distance_provider = provider;
    distance_provider.provider_id = "provider-distance-001";
    distance_provider.capability_id = CAP_DISTANCE;
    distance_provider.owner_module_index = 1;
    distance_provider.owner_device_index = 1;
    distance_provider.latest_payload = distance_payload;
    RegistryWriteResult distance_provider_result =
        registry_.registerCapabilityProviderWithResult(distance_provider);
    if (distance_provider_result.result != RegistryResult::OK) {
        return fail("distance_provider_register_result_invalid");
    }

    if (registry_.setActiveProvider(CAP_TEMPERATURE, "provider-distance-001") !=
        RegistryResult::INVALID_RECORD) {
        return fail("active_provider_mismatch_result_invalid");
    }
    if (registry_.getActiveProvider(CAP_DISTANCE, active_provider) != RegistryResult::NOT_FOUND) {
        return fail("active_provider_missing_mapping_invalid");
    }

    if (registry_.setActiveProvider(CAP_TEMPERATURE, "provider-unavailable-temperature-001") !=
        RegistryResult::OK) {
        return fail("selected_unavailable_active_set_invalid");
    }
    if (registry_.updateSelectedCapabilityPayload(CAP_TEMPERATURE) != RegistryResult::UNAVAILABLE) {
        return fail("selected_unavailable_result_invalid");
    }

    CapabilityProviderRecord mismatch_provider = provider;
    mismatch_provider.provider_id = "provider-mismatch-temperature-001";
    mismatch_provider.latest_payload = distance_payload;
    RegistryWriteResult mismatch_result =
        registry_.registerCapabilityProviderWithResult(mismatch_provider);
    if (mismatch_result.result != RegistryResult::OK) {
        return fail("mismatch_provider_register_result_invalid");
    }
    if (registry_.setActiveProvider(CAP_TEMPERATURE, "provider-mismatch-temperature-001") !=
        RegistryResult::OK) {
        return fail("selected_mismatch_active_set_invalid");
    }
    if (registry_.updateSelectedCapabilityPayload(CAP_TEMPERATURE) != RegistryResult::INVALID_RECORD) {
        return fail("selected_mismatch_result_invalid");
    }

    char missing_provider_id[] = "provider-volatile-temperature-001";
    CapabilityProviderRecord volatile_provider = provider;
    volatile_provider.provider_id = missing_provider_id;
    RegistryWriteResult volatile_result =
        registry_.registerCapabilityProviderWithResult(volatile_provider);
    if (volatile_result.result != RegistryResult::OK) {
        return fail("volatile_provider_register_result_invalid");
    }
    if (registry_.setActiveProvider(CAP_TEMPERATURE, "provider-volatile-temperature-001") !=
        RegistryResult::OK) {
        return fail("volatile_provider_active_set_invalid");
    }
    missing_provider_id[0] = 'x';
    if (registry_.updateSelectedCapabilityPayload(CAP_TEMPERATURE) != RegistryResult::NOT_FOUND) {
        return fail("selected_missing_provider_result_invalid");
    }

    if (registry_.setActiveProvider(CAP_TEMPERATURE, WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID) !=
        RegistryResult::OK) {
        return fail("selected_restore_active_provider_failed");
    }
    if (registry_.updateSelectedCapabilityPayload(CAP_TEMPERATURE) != RegistryResult::OK) {
        return fail("selected_restore_payload_failed");
    }

    ApiCapabilityState temperature_api_before_diagnostics;
    if (!api_.getTemperatureState(temperature_api_before_diagnostics)) {
        return fail("provider_diag_temperature_before_missing");
    }
    if (temperature_api_before_diagnostics.payload.value_float != 24.0F) {
        return fail("provider_diag_temperature_before_invalid");
    }

    ApiProviderDiagnostic provider_diagnostic;
    if (!api_.getProviderDiagnostic(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            provider_diagnostic)) {
        return fail("provider_diag_wireless_get_failed");
    }
    if (!provider_diagnostic.ok) {
        return fail("provider_diag_wireless_not_ok");
    }
    if (provider_diagnostic.registry_result != RegistryResult::OK) {
        return fail("provider_diag_wireless_result_invalid");
    }
    if (!isSameText(provider_diagnostic.provider_id, WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID)) {
        return fail("provider_diag_wireless_id_invalid");
    }
    if (!isSameText(provider_diagnostic.capability_id, CAP_TEMPERATURE)) {
        return fail("provider_diag_wireless_capability_invalid");
    }
    if (provider_diagnostic.provider_type != CapabilityProviderType::WIRELESS) {
        return fail("provider_diag_wireless_type_invalid");
    }
    if (!isSameText(provider_diagnostic.latest_payload.capability_id, CAP_TEMPERATURE)) {
        return fail("provider_diag_wireless_payload_capability_invalid");
    }

    if (registry_.setActiveProvider(CAP_TEMPERATURE, WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID) !=
        RegistryResult::OK) {
        return fail("provider_diag_active_setup_failed");
    }
    if (!api_.getActiveProviderDiagnostic(CAP_TEMPERATURE, provider_diagnostic)) {
        return fail("provider_diag_active_get_failed");
    }
    if (!provider_diagnostic.ok || !provider_diagnostic.active) {
        return fail("provider_diag_active_flag_invalid");
    }
    if (!isSameText(provider_diagnostic.provider_id, WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID)) {
        return fail("provider_diag_active_id_invalid");
    }

    ApiProviderSummary provider_summary;
    if (!api_.getProviderSummary(provider_summary)) {
        return fail("provider_summary_get_failed");
    }
    if (!provider_summary.ok) {
        return fail("provider_summary_not_ok");
    }
    if (provider_summary.registry_result != RegistryResult::OK) {
        return fail("provider_summary_result_invalid");
    }
    if (provider_summary.provider_count != registry_.capabilityProviderCount()) {
        return fail("provider_summary_count_invalid");
    }
    if (provider_summary.active_provider_count != registry_.activeProviderCount()) {
        return fail("provider_summary_active_count_invalid");
    }

    if (api_.getProviderDiagnostic("provider-missing", provider_diagnostic)) {
        return fail("provider_diag_missing_succeeded");
    }
    if (provider_diagnostic.registry_result != RegistryResult::NOT_FOUND) {
        return fail("provider_diag_missing_result_invalid");
    }
    if (api_.getProviderDiagnostic(0, provider_diagnostic)) {
        return fail("provider_diag_null_succeeded");
    }
    if (provider_diagnostic.registry_result != RegistryResult::INVALID_ID) {
        return fail("provider_diag_null_result_invalid");
    }
    if (api_.getProviderDiagnostic("", provider_diagnostic)) {
        return fail("provider_diag_empty_succeeded");
    }
    if (provider_diagnostic.registry_result != RegistryResult::INVALID_ID) {
        return fail("provider_diag_empty_result_invalid");
    }
    if (api_.getActiveProviderDiagnostic(0, provider_diagnostic)) {
        return fail("provider_diag_active_null_succeeded");
    }
    if (provider_diagnostic.registry_result != RegistryResult::INVALID_ID) {
        return fail("provider_diag_active_null_result_invalid");
    }
    if (api_.getActiveProviderDiagnostic("", provider_diagnostic)) {
        return fail("provider_diag_active_empty_succeeded");
    }
    if (provider_diagnostic.registry_result != RegistryResult::INVALID_ID) {
        return fail("provider_diag_active_empty_result_invalid");
    }
    if (api_.getActiveProviderDiagnostic(CAP_DISTANCE, provider_diagnostic)) {
        return fail("provider_diag_distance_active_succeeded");
    }
    if (provider_diagnostic.registry_result != RegistryResult::NOT_FOUND) {
        return fail("provider_diag_distance_active_result_invalid");
    }

    ApiCapabilityState temperature_api_after_diagnostics;
    if (!api_.getTemperatureState(temperature_api_after_diagnostics)) {
        return fail("provider_diag_temperature_after_missing");
    }
    if (temperature_api_after_diagnostics.payload.value_float !=
        temperature_api_before_diagnostics.payload.value_float) {
        return fail("provider_diag_temperature_changed");
    }
    if (registry_.getActiveProvider(CAP_TEMPERATURE, active_provider) != RegistryResult::OK) {
        return fail("provider_diag_active_after_missing");
    }
    if (!isSameText(active_provider.provider_id, WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID)) {
        return fail("provider_diag_active_changed");
    }

    CapabilityProviderRecord invalid_provider = provider;
    invalid_provider.provider_id = 0;
    if (registry_.registerCapabilityProviderWithResult(invalid_provider).result !=
        RegistryResult::INVALID_ID) {
        return fail("provider_null_id_result_invalid");
    }

    invalid_provider = provider;
    invalid_provider.provider_id = "";
    if (registry_.registerCapabilityProviderWithResult(invalid_provider).result !=
        RegistryResult::INVALID_ID) {
        return fail("provider_empty_id_result_invalid");
    }

    invalid_provider = provider;
    invalid_provider.provider_id = "provider-invalid-capability-001";
    invalid_provider.capability_id = 0;
    if (registry_.registerCapabilityProviderWithResult(invalid_provider).result !=
        RegistryResult::INVALID_RECORD) {
        return fail("provider_null_capability_result_invalid");
    }

    invalid_provider = provider;
    invalid_provider.provider_id = "provider-invalid-capability-002";
    invalid_provider.capability_id = "";
    if (registry_.registerCapabilityProviderWithResult(invalid_provider).result !=
        RegistryResult::INVALID_RECORD) {
        return fail("provider_empty_capability_result_invalid");
    }

    if (!registry_.getCapabilityPayload(CAP_TEMPERATURE, temperature_payload)) {
        return fail("provider_temperature_payload_changed");
    }
    if (temperature_payload.available != Availability::AVAILABLE) {
        return fail("provider_temperature_unavailable_after_selected");
    }
    if (temperature_payload.value_type != PayloadValueType::FLOAT) {
        return fail("provider_temperature_type_after_selected");
    }
    if (temperature_payload.value_float != 24.0F) {
        return fail("provider_temperature_selected_value_invalid");
    }
    if (registry_.moduleCount() != 5) {
        return fail("provider_module_count_changed");
    }
    if (registry_.deviceCount() != 5) {
        return fail("provider_device_count_changed");
    }
    if (registry_.capabilityCount() != 5) {
        return fail("provider_capability_count_changed");
    }

    return true;
}

void VerticalSliceValidation::copyWirelessCapabilityId(char* destination, const char* source) const {
    if (destination == 0) {
        return;
    }

    uint8_t index = 0;
    while (index < WIRELESS_CAPABILITY_ID_SIZE - 1 &&
           source != 0 &&
           source[index] != '\0') {
        destination[index] = source[index];
        ++index;
    }

    while (index < WIRELESS_CAPABILITY_ID_SIZE) {
        destination[index] = '\0';
        ++index;
    }
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
