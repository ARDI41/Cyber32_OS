#include "vertical_slice_validation.h"

#include "../../core/ids/capability_ids.h"
#include "../../core/ids/error_ids.h"
#include "../../drivers/communication/espnow_transport_driver.h"
#include "../../logic/logic_status.h"
#include "../../services/wireless/wireless_packet_transport.h"

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

    if (!validateEspNowTransportInitializationSmoke()) {
        return false;
    }

    if (!validateSimWirelessPacketTransportAdapter()) {
        return false;
    }

    if (!validateEspNowWirelessPacketTransportAdapter()) {
        return false;
    }

    if (!validateWirelessServiceTransportAdapterAttachment()) {
        return false;
    }

    if (!validateWirelessServiceProcessPacketsAdapterPath(now_ms)) {
        return false;
    }

    if (!validateEspNowAdapterWirelessServicePath(now_ms)) {
        return false;
    }

    if (!validateCapabilityProviderStorage(now_ms)) {
        return false;
    }

    if (!validatePublicOwnerTypeDefaults()) {
        return false;
    }

    if (!validateNodeDirectoryEmptySkeleton()) {
        return false;
    }

    if (!validateNodeDirectoryControlledAddPath()) {
        return false;
    }

    if (!validateCapabilityDirectoryEmptySkeleton()) {
        return false;
    }

    if (!validateCapabilityDirectoryControlledAddPath()) {
        return false;
    }

    if (!validatePublicOwnerStoreEmptySkeleton()) {
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

    if (!validateEspNowTransportInitializationSmoke()) {
        return false;
    }

    if (!validateSimWirelessPacketTransportAdapter()) {
        return false;
    }

    if (!validateEspNowWirelessPacketTransportAdapter()) {
        return false;
    }

    if (!validateWirelessServiceTransportAdapterAttachment()) {
        return false;
    }

    if (!validateWirelessServiceProcessPacketsAdapterPath(now_ms)) {
        return false;
    }

    if (!validateEspNowAdapterWirelessServicePath(now_ms)) {
        return false;
    }

    if (!validateCapabilityProviderStorage(now_ms)) {
        return false;
    }

    if (!validatePublicOwnerTypeDefaults()) {
        return false;
    }

    if (!validateNodeDirectoryEmptySkeleton()) {
        return false;
    }

    if (!validateNodeDirectoryControlledAddPath()) {
        return false;
    }

    if (!validateCapabilityDirectoryEmptySkeleton()) {
        return false;
    }

    if (!validateCapabilityDirectoryControlledAddPath()) {
        return false;
    }

    if (!validatePublicOwnerStoreEmptySkeleton()) {
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
    ApiSystemIdentity identity;
    if (!api_.getSystemIdentity(identity)) {
        return fail("api_system_identity_failed");
    }
    if (!identity.ok) {
        return fail("api_system_identity_not_ok");
    }
    if (!isSameText(identity.error_code, "none")) {
        return fail("api_system_identity_error_invalid");
    }

    ApiSystemFirmware firmware;
    if (!api_.getSystemFirmware(firmware)) {
        return fail("api_system_firmware_failed");
    }
    if (!firmware.ok) {
        return fail("api_system_firmware_not_ok");
    }
    if (!isSameText(firmware.error_code, "none")) {
        return fail("api_system_firmware_error_invalid");
    }

    ApiSystemRuntime runtime_status;
    if (!api_.getSystemRuntime(runtime_status)) {
        return fail("api_system_runtime_failed");
    }
    if (!runtime_status.ok) {
        return fail("api_system_runtime_not_ok");
    }
    if (!isSameText(runtime_status.error_code, "none")) {
        return fail("api_system_runtime_error_invalid");
    }
    if (runtime_status.runtime_state != runtime_.state()) {
        return fail("api_system_runtime_state_invalid");
    }

    ApiSystemModes modes;
    if (!api_.getSystemModes(modes)) {
        return fail("api_system_modes_failed");
    }
    if (!modes.ok) {
        return fail("api_system_modes_not_ok");
    }
    if (!isSameText(modes.error_code, "none")) {
        return fail("api_system_modes_error_invalid");
    }

    ApiSystemMemory memory;
    if (!api_.getSystemMemory(memory)) {
        return fail("api_system_memory_failed");
    }
    if (!memory.ok) {
        return fail("api_system_memory_not_ok");
    }
    if (!isSameText(memory.error_code, "none")) {
        return fail("api_system_memory_error_invalid");
    }

    ApiSystemSummary summary;
    if (!api_.getSystemSummary(summary)) {
        return fail("api_system_summary_failed");
    }
    if (!summary.ok) {
        return fail("api_system_summary_not_ok");
    }
    if (!isSameText(summary.error_code, "none")) {
        return fail("api_system_summary_error_invalid");
    }
    if (!summary.identity.ok) {
        return fail("api_system_summary_identity_not_ok");
    }
    if (!summary.firmware.ok) {
        return fail("api_system_summary_firmware_not_ok");
    }
    if (!summary.runtime.ok) {
        return fail("api_system_summary_runtime_not_ok");
    }
    if (!summary.modes.ok) {
        return fail("api_system_summary_modes_not_ok");
    }
    if (!summary.memory.ok) {
        return fail("api_system_summary_memory_not_ok");
    }

    ApiNodeList node_list;
    if (!api_.getNodeList(node_list)) {
        return fail("api_node_list_failed");
    }
    if (!node_list.ok) {
        return fail("api_node_list_not_ok");
    }
    if (!isSameText(node_list.error_code, "none")) {
        return fail("api_node_list_error_invalid");
    }
    if (node_list.count != 0) {
        return fail("api_node_list_count_invalid");
    }
    if (node_list.count > API_MAX_NODE_SUMMARY_COUNT) {
        return fail("api_node_list_count_overflow");
    }
    if (node_list.count > NODE_DIRECTORY_MAX_PUBLIC_NODES) {
        return fail("api_node_list_node_directory_overflow");
    }

    ApiNodeList node_list_repeat;
    if (!api_.getNodeList(node_list_repeat)) {
        return fail("api_node_list_repeat_failed");
    }
    if (node_list_repeat.count != node_list.count) {
        return fail("api_node_list_repeat_mutated");
    }

    ApiNodeSummary node_summary;
    if (api_.getNodeSummary(0, node_summary)) {
        return fail("api_node_summary_unexpected_success");
    }
    if (node_summary.ok) {
        return fail("api_node_summary_unexpected_ok");
    }
    if (!isSameText(node_summary.error_code, "node_not_found")) {
        return fail("api_node_summary_error_invalid");
    }

    ApiNodeSummary node_summary_repeat;
    if (api_.getNodeSummary(0, node_summary_repeat)) {
        return fail("api_node_summary_repeat_unexpected_success");
    }
    if (!isSameText(node_summary_repeat.error_code, node_summary.error_code)) {
        return fail("api_node_summary_repeat_mutated");
    }

    ApiNodeIdentity node_identity;
    if (api_.getNodeIdentity(0, node_identity)) {
        return fail("api_node_identity_unexpected_success");
    }
    if (node_identity.ok) {
        return fail("api_node_identity_unexpected_ok");
    }
    if (!isSameText(node_identity.error_code, "node_not_found")) {
        return fail("api_node_identity_error_invalid");
    }
    if (node_identity.node_id != 0) {
        return fail("api_node_identity_node_id_invalid");
    }
    if (node_identity.has_source_mac) {
        return fail("api_node_identity_mac_present");
    }

    ApiNodeIdentity node_identity_repeat;
    if (api_.getNodeIdentity(0, node_identity_repeat)) {
        return fail("api_node_identity_repeat_unexpected_success");
    }
    if (!isSameText(node_identity_repeat.error_code, node_identity.error_code)) {
        return fail("api_node_identity_repeat_mutated");
    }

    ApiNodeStatus node_status;
    if (api_.getNodeStatus(0, node_status)) {
        return fail("api_node_status_unexpected_success");
    }
    if (node_status.ok) {
        return fail("api_node_status_unexpected_ok");
    }
    if (!isSameText(node_status.error_code, "node_not_found")) {
        return fail("api_node_status_error_invalid");
    }
    if (node_status.online || node_status.paired || node_status.trusted ||
        node_status.blocked) {
        return fail("api_node_status_flags_invalid");
    }
    if (node_status.last_seen_ms != 0) {
        return fail("api_node_status_last_seen_invalid");
    }

    ApiNodeStatus node_status_repeat;
    if (api_.getNodeStatus(0, node_status_repeat)) {
        return fail("api_node_status_repeat_unexpected_success");
    }
    if (!isSameText(node_status_repeat.error_code, node_status.error_code)) {
        return fail("api_node_status_repeat_mutated");
    }

    ApiNodePower node_power;
    if (api_.getNodePower(0, node_power)) {
        return fail("api_node_power_unexpected_success");
    }
    if (node_power.ok) {
        return fail("api_node_power_unexpected_ok");
    }
    if (!isSameText(node_power.error_code, "node_not_found")) {
        return fail("api_node_power_error_invalid");
    }
    if (node_power.has_battery_percent || node_power.has_battery_mv) {
        return fail("api_node_power_presence_invalid");
    }
    if (node_power.battery_percent != 0 || node_power.battery_mv != 0) {
        return fail("api_node_power_values_invalid");
    }

    ApiNodePower node_power_repeat;
    if (api_.getNodePower(0, node_power_repeat)) {
        return fail("api_node_power_repeat_unexpected_success");
    }
    if (!isSameText(node_power_repeat.error_code, node_power.error_code)) {
        return fail("api_node_power_repeat_mutated");
    }

    ApiNodeSignal node_signal;
    if (api_.getNodeSignal(0, node_signal)) {
        return fail("api_node_signal_unexpected_success");
    }
    if (node_signal.ok) {
        return fail("api_node_signal_unexpected_ok");
    }
    if (!isSameText(node_signal.error_code, "node_not_found")) {
        return fail("api_node_signal_error_invalid");
    }
    if (node_signal.has_rssi) {
        return fail("api_node_signal_rssi_present");
    }
    if (node_signal.rssi != 0 || node_signal.signal_quality_percent != 0) {
        return fail("api_node_signal_values_invalid");
    }

    ApiNodeSignal node_signal_repeat;
    if (api_.getNodeSignal(0, node_signal_repeat)) {
        return fail("api_node_signal_repeat_unexpected_success");
    }
    if (!isSameText(node_signal_repeat.error_code, node_signal.error_code)) {
        return fail("api_node_signal_repeat_mutated");
    }

    ApiNodeDiagnosticsSummary node_diagnostics;
    if (api_.getNodeDiagnostics(0, node_diagnostics)) {
        return fail("api_node_diagnostics_unexpected_success");
    }
    if (node_diagnostics.ok) {
        return fail("api_node_diagnostics_unexpected_ok");
    }
    if (!isSameText(node_diagnostics.error_code, "node_not_found")) {
        return fail("api_node_diagnostics_error_invalid");
    }
    if (node_diagnostics.accepted_packet_count != 0 ||
        node_diagnostics.rejected_packet_count != 0) {
        return fail("api_node_diagnostics_counts_invalid");
    }
    if (!isSameText(node_diagnostics.last_error_code, "none")) {
        return fail("api_node_diagnostics_last_error_invalid");
    }
    if (node_diagnostics.has_security_diagnostics) {
        return fail("api_node_diagnostics_security_present");
    }

    ApiNodeDiagnosticsSummary node_diagnostics_repeat;
    if (api_.getNodeDiagnostics(0, node_diagnostics_repeat)) {
        return fail("api_node_diagnostics_repeat_unexpected_success");
    }
    if (!isSameText(node_diagnostics_repeat.error_code, node_diagnostics.error_code) ||
        node_diagnostics_repeat.accepted_packet_count != node_diagnostics.accepted_packet_count ||
        node_diagnostics_repeat.rejected_packet_count != node_diagnostics.rejected_packet_count ||
        !isSameText(node_diagnostics_repeat.last_error_code, node_diagnostics.last_error_code) ||
        node_diagnostics_repeat.has_security_diagnostics !=
            node_diagnostics.has_security_diagnostics) {
        return fail("api_node_diagnostics_repeat_mutated");
    }

    ApiNodeCapabilitySummary node_capabilities[2];
    uint8_t node_capability_count = 99;
    if (api_.getNodeCapabilities(0, node_capabilities, 2, node_capability_count)) {
        return fail("api_node_capabilities_unexpected_success");
    }
    if (node_capability_count != 0) {
        return fail("api_node_capabilities_count_invalid");
    }

    uint8_t node_capability_count_repeat = 99;
    if (api_.getNodeCapabilities(0, node_capabilities, 2, node_capability_count_repeat)) {
        return fail("api_node_capabilities_repeat_unexpected_success");
    }
    if (node_capability_count_repeat != node_capability_count) {
        return fail("api_node_capabilities_repeat_mutated");
    }

    node_capabilities[0].capability_count = 77;
    uint8_t zero_max_capability_count = 99;
    if (api_.getNodeCapabilities(0, 0, 0, zero_max_capability_count)) {
        return fail("api_node_capabilities_zero_max_unexpected_success");
    }
    if (zero_max_capability_count != 0) {
        return fail("api_node_capabilities_zero_max_count_invalid");
    }
    if (node_capabilities[0].capability_count != 77) {
        return fail("api_node_capabilities_zero_max_wrote_buffer");
    }

    uint8_t null_capability_count = 99;
    if (api_.getNodeCapabilities(0, 0, 2, null_capability_count)) {
        return fail("api_node_capabilities_null_unexpected_success");
    }
    if (null_capability_count != 0) {
        return fail("api_node_capabilities_null_count_invalid");
    }

    ApiCapabilityList capability_list;
    if (!api_.getCapabilityList(capability_list)) {
        return fail("api_capability_list_failed");
    }
    if (!capability_list.ok) {
        return fail("api_capability_list_not_ok");
    }
    if (!isSameText(capability_list.error_code, "none")) {
        return fail("api_capability_list_error_invalid");
    }
    if (capability_list.count != 0) {
        return fail("api_capability_list_count_invalid");
    }
    if (capability_list.count > API_MAX_CAPABILITY_SUMMARY_COUNT) {
        return fail("api_capability_list_count_overflow");
    }
    if (capability_list.count > CAPABILITY_DIRECTORY_MAX_PUBLIC_CAPABILITIES) {
        return fail("api_capability_list_directory_bound_invalid");
    }

    ApiCapabilityList capability_list_repeat;
    if (!api_.getCapabilityList(capability_list_repeat)) {
        return fail("api_capability_list_repeat_failed");
    }
    if (capability_list_repeat.count != capability_list.count) {
        return fail("api_capability_list_repeat_mutated");
    }

    ApiCapabilitySummary capability_summary;
    if (api_.getCapabilitySummary(0, capability_summary)) {
        return fail("api_capability_summary_unexpected_success");
    }
    if (capability_summary.ok) {
        return fail("api_capability_summary_unexpected_ok");
    }
    if (!isSameText(capability_summary.error_code, "capability_not_found")) {
        return fail("api_capability_summary_error_invalid");
    }

    ApiCapabilitySummary capability_summary_repeat;
    if (api_.getCapabilitySummary(0, capability_summary_repeat)) {
        return fail("api_capability_summary_repeat_unexpected_success");
    }
    if (!isSameText(capability_summary_repeat.error_code, capability_summary.error_code)) {
        return fail("api_capability_summary_repeat_mutated");
    }

    ApiCapabilityIdentity capability_identity;
    if (api_.getCapabilityIdentity(0, capability_identity)) {
        return fail("api_capability_identity_unexpected_success");
    }
    if (capability_identity.ok) {
        return fail("api_capability_identity_unexpected_ok");
    }
    if (!isSameText(capability_identity.error_code, "capability_not_found")) {
        return fail("api_capability_identity_error_invalid");
    }
    if (capability_identity.capability_id != 0 ||
        capability_identity.friendly_name != 0 ||
        capability_identity.category != 0 ||
        capability_identity.unit != 0) {
        return fail("api_capability_identity_text_invalid");
    }
    if (capability_identity.value_type != PayloadValueType::NONE) {
        return fail("api_capability_identity_type_invalid");
    }

    ApiCapabilityIdentity capability_identity_repeat;
    if (api_.getCapabilityIdentity(0, capability_identity_repeat)) {
        return fail("api_capability_identity_repeat_unexpected_success");
    }
    if (!isSameText(capability_identity_repeat.error_code, capability_identity.error_code) ||
        capability_identity_repeat.capability_id != capability_identity.capability_id ||
        capability_identity_repeat.friendly_name != capability_identity.friendly_name ||
        capability_identity_repeat.category != capability_identity.category ||
        capability_identity_repeat.unit != capability_identity.unit ||
        capability_identity_repeat.value_type != capability_identity.value_type) {
        return fail("api_capability_identity_repeat_mutated");
    }

    ApiCapabilityValue capability_value;
    if (api_.getCapabilityValue(0, capability_value)) {
        return fail("api_capability_value_unexpected_success");
    }
    if (capability_value.ok) {
        return fail("api_capability_value_unexpected_ok");
    }
    if (!isSameText(capability_value.error_code, "capability_not_found")) {
        return fail("api_capability_value_error_invalid");
    }
    if (capability_value.value_type != PayloadValueType::NONE) {
        return fail("api_capability_value_type_invalid");
    }
    if (capability_value.value_float != 0.0F ||
        capability_value.value_int != 0 ||
        capability_value.value_bool ||
        capability_value.timestamp_ms != 0 ||
        capability_value.unit != 0) {
        return fail("api_capability_value_defaults_invalid");
    }

    ApiCapabilityValue capability_value_repeat;
    if (api_.getCapabilityValue(0, capability_value_repeat)) {
        return fail("api_capability_value_repeat_unexpected_success");
    }
    if (!isSameText(capability_value_repeat.error_code, capability_value.error_code) ||
        capability_value_repeat.value_type != capability_value.value_type ||
        capability_value_repeat.value_float != capability_value.value_float ||
        capability_value_repeat.value_int != capability_value.value_int ||
        capability_value_repeat.value_bool != capability_value.value_bool ||
        capability_value_repeat.timestamp_ms != capability_value.timestamp_ms ||
        capability_value_repeat.unit != capability_value.unit) {
        return fail("api_capability_value_repeat_mutated");
    }

    ApiCapabilityAvailability capability_availability;
    if (api_.getCapabilityAvailability(0, capability_availability)) {
        return fail("api_capability_availability_unexpected_success");
    }
    if (capability_availability.ok) {
        return fail("api_capability_availability_unexpected_ok");
    }
    if (!isSameText(capability_availability.error_code, "capability_not_found")) {
        return fail("api_capability_availability_error_invalid");
    }
    if (capability_availability.available != Availability::UNAVAILABLE ||
        capability_availability.stale != StaleState::STALE ||
        capability_availability.last_update_ms != 0 ||
        capability_availability.has_provider) {
        return fail("api_capability_availability_defaults_invalid");
    }

    ApiCapabilityAvailability capability_availability_repeat;
    if (api_.getCapabilityAvailability(0, capability_availability_repeat)) {
        return fail("api_capability_availability_repeat_unexpected_success");
    }
    if (!isSameText(capability_availability_repeat.error_code,
                    capability_availability.error_code) ||
        capability_availability_repeat.available != capability_availability.available ||
        capability_availability_repeat.stale != capability_availability.stale ||
        capability_availability_repeat.last_update_ms != capability_availability.last_update_ms ||
        capability_availability_repeat.has_provider != capability_availability.has_provider) {
        return fail("api_capability_availability_repeat_mutated");
    }

    ApiCapabilityProviderInfo capability_provider_info;
    if (api_.getCapabilityProviderInfo(0, capability_provider_info)) {
        return fail("api_capability_provider_info_unexpected_success");
    }
    if (capability_provider_info.ok) {
        return fail("api_capability_provider_info_unexpected_ok");
    }
    if (!isSameText(capability_provider_info.error_code, "capability_not_found")) {
        return fail("api_capability_provider_info_error_invalid");
    }
    if (capability_provider_info.active_provider_id != 0 ||
        capability_provider_info.provider_type != CapabilityProviderType::UNKNOWN ||
        capability_provider_info.provider_status != CapabilityProviderStatus::UNKNOWN ||
        capability_provider_info.owner_node_id != 0 ||
        capability_provider_info.has_owner_node ||
        capability_provider_info.selected) {
        return fail("api_capability_provider_info_defaults_invalid");
    }

    ApiCapabilityProviderInfo capability_provider_info_repeat;
    if (api_.getCapabilityProviderInfo(0, capability_provider_info_repeat)) {
        return fail("api_capability_provider_info_repeat_unexpected_success");
    }
    if (!isSameText(capability_provider_info_repeat.error_code,
                    capability_provider_info.error_code) ||
        capability_provider_info_repeat.active_provider_id !=
            capability_provider_info.active_provider_id ||
        capability_provider_info_repeat.provider_type != capability_provider_info.provider_type ||
        capability_provider_info_repeat.provider_status !=
            capability_provider_info.provider_status ||
        capability_provider_info_repeat.owner_node_id != capability_provider_info.owner_node_id ||
        capability_provider_info_repeat.has_owner_node != capability_provider_info.has_owner_node ||
        capability_provider_info_repeat.selected != capability_provider_info.selected) {
        return fail("api_capability_provider_info_repeat_mutated");
    }

    ApiCapabilityQuality capability_quality;
    if (api_.getCapabilityQuality(0, capability_quality)) {
        return fail("api_capability_quality_unexpected_success");
    }
    if (capability_quality.ok) {
        return fail("api_capability_quality_unexpected_ok");
    }
    if (!isSameText(capability_quality.error_code, "capability_not_found")) {
        return fail("api_capability_quality_error_invalid");
    }
    if (capability_quality.quality != 0 ||
        !isSameText(capability_quality.error_code_payload, "none") ||
        capability_quality.has_error) {
        return fail("api_capability_quality_defaults_invalid");
    }

    ApiCapabilityQuality capability_quality_repeat;
    if (api_.getCapabilityQuality(0, capability_quality_repeat)) {
        return fail("api_capability_quality_repeat_unexpected_success");
    }
    if (!isSameText(capability_quality_repeat.error_code, capability_quality.error_code) ||
        capability_quality_repeat.quality != capability_quality.quality ||
        !isSameText(capability_quality_repeat.error_code_payload,
                    capability_quality.error_code_payload) ||
        capability_quality_repeat.has_error != capability_quality.has_error) {
        return fail("api_capability_quality_repeat_mutated");
    }

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

bool VerticalSliceValidation::validateEspNowTransportInitializationSmoke() {
    EspNowTransportDriver espnow_driver;

    if (espnow_driver.initialized()) {
        return fail("espnow_initial_state_invalid");
    }
    if (espnow_driver.callbackReceived()) {
        return fail("espnow_callback_initial_state_invalid");
    }
    if (espnow_driver.lastReceivedLength() != 0) {
        return fail("espnow_callback_initial_length_invalid");
    }
    if (espnow_driver.hasRawPayload()) {
        return fail("espnow_raw_initial_state_invalid");
    }
    if (espnow_driver.rawPayloadLength() != 0) {
        return fail("espnow_raw_initial_length_invalid");
    }

    const bool begin_result = espnow_driver.begin();
    if (espnow_driver.initialized() != begin_result) {
        return fail("espnow_initialized_result_mismatch");
    }
    if (espnow_driver.callbackReceived()) {
        return fail("espnow_callback_unexpected_after_begin");
    }
    if (espnow_driver.lastReceivedLength() != 0) {
        return fail("espnow_callback_length_after_begin_invalid");
    }
    if (espnow_driver.hasRawPayload()) {
        return fail("espnow_raw_unexpected_after_begin");
    }
    if (espnow_driver.rawPayloadLength() != 0) {
        return fail("espnow_raw_length_after_begin_invalid");
    }

    if (espnow_driver.hasReceivedPacket()) {
        return fail("espnow_pending_packet_unexpected");
    }

    WirelessPacketHeader header;
    WirelessCapabilityValue value;
    WirelessNodeDiagnostics diagnostics;
    if (espnow_driver.readReceivedPacket(header, value, diagnostics)) {
        return fail("espnow_read_without_packet_succeeded");
    }

    uint8_t source_mac[WIRELESS_MAC_ADDRESS_SIZE];
    bool has_source_mac = true;
    uint8_t raw_buffer[WIRELESS_MAX_PACKET_SIZE];
    uint16_t raw_length = 0;
    if (espnow_driver.readReceivedPacket(header, value, diagnostics, source_mac, has_source_mac)) {
        return fail("espnow_mac_read_without_packet_succeeded");
    }
    if (espnow_driver.readRawPayload(raw_buffer, raw_length, source_mac, has_source_mac)) {
        return fail("espnow_empty_raw_read_succeeded");
    }
    if (raw_length != 0) {
        return fail("espnow_empty_raw_length_invalid");
    }
    espnow_driver.clearReceivedPacket();
    if (espnow_driver.hasReceivedPacket()) {
        return fail("espnow_clear_initial_left_pending_packet");
    }
    if (espnow_driver.hasRawPayload()) {
        return fail("espnow_clear_initial_left_raw_payload");
    }
    if (espnow_driver.rawPayloadLength() != 0) {
        return fail("espnow_clear_initial_left_raw_length");
    }
    if (espnow_driver.callbackReceived()) {
        return fail("espnow_callback_unexpected_after_clear");
    }
    if (espnow_driver.lastReceivedLength() != 0) {
        return fail("espnow_callback_length_after_clear_invalid");
    }

    const uint8_t valid_source_mac[WIRELESS_MAC_ADDRESS_SIZE] = {
        0xAA,
        0xBB,
        0xCC,
        0x44,
        0x55,
        0x66
    };
    const uint8_t valid_raw_payload[5] = {1, 2, 3, 4, 5};
    if (!espnow_driver.injectRawPayloadForTest(valid_source_mac, valid_raw_payload, 5)) {
        return fail("espnow_raw_inject_failed");
    }
    if (!espnow_driver.hasRawPayload()) {
        return fail("espnow_raw_payload_missing");
    }
    if (espnow_driver.rawPayloadLength() != 5) {
        return fail("espnow_raw_payload_length_invalid");
    }
    raw_length = 0;
    clearWirelessMacAddress(source_mac);
    has_source_mac = false;
    if (!espnow_driver.readRawPayload(raw_buffer, raw_length, source_mac, has_source_mac)) {
        return fail("espnow_raw_read_failed");
    }
    if (raw_length != 5) {
        return fail("espnow_raw_read_length_invalid");
    }
    for (uint8_t i = 0; i < 5; ++i) {
        if (raw_buffer[i] != valid_raw_payload[i]) {
            return fail("espnow_raw_read_byte_invalid");
        }
    }
    if (!has_source_mac) {
        return fail("espnow_raw_source_mac_missing");
    }
    if (!wirelessMacAddressEquals(source_mac, valid_source_mac)) {
        return fail("espnow_raw_source_mac_invalid");
    }
    if (espnow_driver.hasRawPayload()) {
        return fail("espnow_raw_not_cleared_after_read");
    }
    if (espnow_driver.rawPayloadLength() != 0) {
        return fail("espnow_raw_length_not_cleared_after_read");
    }

    if (espnow_driver.injectRawPayloadForTest(valid_source_mac, 0, 5)) {
        return fail("espnow_raw_null_data_accepted");
    }
    if (espnow_driver.hasRawPayload()) {
        return fail("espnow_raw_null_data_pending");
    }
    if (espnow_driver.injectRawPayloadForTest(valid_source_mac, valid_raw_payload, 0)) {
        return fail("espnow_raw_zero_length_accepted");
    }
    if (espnow_driver.hasRawPayload()) {
        return fail("espnow_raw_zero_length_pending");
    }
    uint8_t oversized_raw_payload[WIRELESS_MAX_PACKET_SIZE + 1];
    for (uint16_t i = 0; i < static_cast<uint16_t>(WIRELESS_MAX_PACKET_SIZE + 1); ++i) {
        oversized_raw_payload[i] = static_cast<uint8_t>(i);
    }
    if (espnow_driver.injectRawPayloadForTest(
            valid_source_mac,
            oversized_raw_payload,
            static_cast<uint16_t>(WIRELESS_MAX_PACKET_SIZE + 1))) {
        return fail("espnow_raw_oversize_accepted");
    }
    if (espnow_driver.hasRawPayload()) {
        return fail("espnow_raw_oversize_pending");
    }

    if (!espnow_driver.injectRawPayloadForTest(valid_source_mac, valid_raw_payload, 5)) {
        return fail("espnow_raw_clear_setup_failed");
    }

    espnow_driver.clearReceivedPacket();
    if (espnow_driver.hasReceivedPacket()) {
        return fail("espnow_clear_left_pending_packet");
    }
    if (espnow_driver.hasRawPayload()) {
        return fail("espnow_clear_left_raw_payload");
    }
    if (espnow_driver.rawPayloadLength() != 0) {
        return fail("espnow_clear_left_raw_length");
    }

    if (espnow_driver.decodePendingRawPayload()) {
        return fail("espnow_decode_without_raw_succeeded");
    }
    if (espnow_driver.hasReceivedPacket()) {
        return fail("espnow_decode_without_raw_created_packet");
    }

    enum {
        EXPECTED_STRUCTURED_PACKET_SIZE =
            sizeof(WirelessPacketHeader) +
            sizeof(WirelessCapabilityValue) +
            sizeof(WirelessNodeDiagnostics)
    };
    if (EXPECTED_STRUCTURED_PACKET_SIZE > WIRELESS_MAX_PACKET_SIZE) {
        return fail("espnow_structured_packet_size_too_large");
    }

    WirelessPacketHeader decode_header;
    decode_header.magic = WIRELESS_PACKET_MAGIC;
    decode_header.protocol_version = WIRELESS_PROTOCOL_VERSION;
    decode_header.packet_type = WirelessPacketType::CAPABILITY_VALUE;
    decode_header.flags = 0x02;
    decode_header.sequence_id = 77;
    decode_header.node_id = 1001;
    decode_header.payload_length = static_cast<uint8_t>(
        sizeof(WirelessCapabilityValue) + sizeof(WirelessNodeDiagnostics));
    decode_header.checksum = 0;

    WirelessCapabilityValue decode_value;
    copyWirelessCapabilityId(decode_value.capability_id, CAP_TEMPERATURE);
    decode_value.payload_type = WirelessPayloadType::FLOAT;
    decode_value.value_float = 42.5F;
    decode_value.value_int = 7;
    copyWirelessCapabilityId(decode_value.error_code, "none");

    WirelessNodeDiagnostics decode_diagnostics;
    decode_diagnostics.battery_present = true;
    decode_diagnostics.battery_level_percent = 88.0F;
    decode_diagnostics.battery_voltage = 3.9F;
    decode_diagnostics.signal_quality_percent = 71.0F;
    decode_header.checksum =
        calculateWirelessPacketChecksum(decode_header, decode_value, decode_diagnostics);

    uint8_t structured_raw_payload[EXPECTED_STRUCTURED_PACKET_SIZE];
    uint16_t structured_offset = 0;
    const uint8_t* decode_header_bytes =
        reinterpret_cast<const uint8_t*>(&decode_header);
    for (uint16_t i = 0; i < sizeof(WirelessPacketHeader); ++i) {
        structured_raw_payload[structured_offset] = decode_header_bytes[i];
        ++structured_offset;
    }
    const uint8_t* decode_value_bytes =
        reinterpret_cast<const uint8_t*>(&decode_value);
    for (uint16_t i = 0; i < sizeof(WirelessCapabilityValue); ++i) {
        structured_raw_payload[structured_offset] = decode_value_bytes[i];
        ++structured_offset;
    }
    const uint8_t* decode_diagnostics_bytes =
        reinterpret_cast<const uint8_t*>(&decode_diagnostics);
    for (uint16_t i = 0; i < sizeof(WirelessNodeDiagnostics); ++i) {
        structured_raw_payload[structured_offset] = decode_diagnostics_bytes[i];
        ++structured_offset;
    }

    if (!espnow_driver.injectRawPayloadForTest(
            valid_source_mac,
            structured_raw_payload,
            static_cast<uint16_t>(EXPECTED_STRUCTURED_PACKET_SIZE))) {
        return fail("espnow_decode_raw_inject_failed");
    }
    if (!espnow_driver.decodePendingRawPayload()) {
        return fail("espnow_decode_valid_failed");
    }
    if (!espnow_driver.hasReceivedPacket()) {
        return fail("espnow_decode_packet_not_pending");
    }
    WirelessPacketHeader decoded_header;
    WirelessCapabilityValue decoded_value;
    WirelessNodeDiagnostics decoded_diagnostics;
    clearWirelessMacAddress(source_mac);
    has_source_mac = false;
    if (!espnow_driver.readReceivedPacket(
            decoded_header,
            decoded_value,
            decoded_diagnostics,
            source_mac,
            has_source_mac)) {
        return fail("espnow_decode_read_failed");
    }
    if (decoded_header.magic != decode_header.magic ||
        decoded_header.protocol_version != decode_header.protocol_version ||
        decoded_header.packet_type != decode_header.packet_type ||
        decoded_header.flags != decode_header.flags ||
        decoded_header.sequence_id != decode_header.sequence_id ||
        decoded_header.node_id != decode_header.node_id ||
        decoded_header.payload_length != decode_header.payload_length ||
        decoded_header.checksum != decode_header.checksum) {
        return fail("espnow_decode_header_mismatch");
    }
    if (!isSameText(decoded_value.capability_id, CAP_TEMPERATURE) ||
        decoded_value.payload_type != decode_value.payload_type ||
        decoded_value.value_float != decode_value.value_float ||
        decoded_value.value_int != decode_value.value_int ||
        !isSameText(decoded_value.error_code, "none")) {
        return fail("espnow_decode_value_mismatch");
    }
    if (decoded_diagnostics.battery_present != decode_diagnostics.battery_present ||
        decoded_diagnostics.battery_level_percent != decode_diagnostics.battery_level_percent ||
        decoded_diagnostics.battery_voltage != decode_diagnostics.battery_voltage ||
        decoded_diagnostics.signal_quality_percent != decode_diagnostics.signal_quality_percent) {
        return fail("espnow_decode_diagnostics_mismatch");
    }
    if (!has_source_mac) {
        return fail("espnow_decode_source_mac_missing");
    }
    if (!wirelessMacAddressEquals(source_mac, valid_source_mac)) {
        return fail("espnow_decode_source_mac_mismatch");
    }
    if (espnow_driver.hasReceivedPacket()) {
        return fail("espnow_decode_packet_not_cleared_after_read");
    }

    uint8_t short_raw_payload[EXPECTED_STRUCTURED_PACKET_SIZE - 1];
    for (uint16_t i = 0; i < static_cast<uint16_t>(EXPECTED_STRUCTURED_PACKET_SIZE - 1); ++i) {
        short_raw_payload[i] = static_cast<uint8_t>(i + 1);
    }
    if (!espnow_driver.injectRawPayloadForTest(
            valid_source_mac,
            short_raw_payload,
            static_cast<uint16_t>(EXPECTED_STRUCTURED_PACKET_SIZE - 1))) {
        return fail("espnow_decode_short_inject_failed");
    }
    if (espnow_driver.decodePendingRawPayload()) {
        return fail("espnow_decode_short_succeeded");
    }
    if (espnow_driver.hasRawPayload()) {
        return fail("espnow_decode_short_left_raw");
    }
    if (espnow_driver.hasReceivedPacket()) {
        return fail("espnow_decode_short_created_packet");
    }
    raw_length = 0;
    if (espnow_driver.readRawPayload(raw_buffer, raw_length, source_mac, has_source_mac)) {
        return fail("espnow_decode_short_raw_read_succeeded");
    }

    uint8_t long_raw_payload[EXPECTED_STRUCTURED_PACKET_SIZE + 1];
    for (uint16_t i = 0; i < static_cast<uint16_t>(EXPECTED_STRUCTURED_PACKET_SIZE + 1); ++i) {
        long_raw_payload[i] = static_cast<uint8_t>(i + 3);
    }
    if (!espnow_driver.injectRawPayloadForTest(
            valid_source_mac,
            long_raw_payload,
            static_cast<uint16_t>(EXPECTED_STRUCTURED_PACKET_SIZE + 1))) {
        return fail("espnow_decode_long_inject_failed");
    }
    if (espnow_driver.decodePendingRawPayload()) {
        return fail("espnow_decode_long_succeeded");
    }
    if (espnow_driver.hasReceivedPacket()) {
        return fail("espnow_decode_long_created_packet");
    }

    return true;
}

bool VerticalSliceValidation::validateSimWirelessPacketTransportAdapter() {
    WirelessPacketTransportAdapter null_adapter =
        makeSimEspNowTransportAdapter(0);
    if (wirelessPacketTransportAdapterValid(null_adapter)) {
        return fail("sim_transport_null_adapter_valid");
    }

    SimEspNowTransportDriver sim_driver;
    sim_driver.begin();
    WirelessPacketTransportAdapter adapter =
        makeSimEspNowTransportAdapter(&sim_driver);
    if (!wirelessPacketTransportAdapterValid(adapter)) {
        return fail("sim_transport_adapter_invalid");
    }
    if (adapter.has_received_packet(adapter.context)) {
        return fail("sim_transport_adapter_empty_has_packet");
    }

    WirelessPacketHeader header;
    WirelessCapabilityValue value;
    WirelessNodeDiagnostics diagnostics;
    uint8_t source_mac[WIRELESS_MAC_ADDRESS_SIZE];
    bool has_source_mac = true;
    if (adapter.read_received_packet(
            adapter.context,
            header,
            value,
            diagnostics,
            source_mac,
            has_source_mac)) {
        return fail("sim_transport_adapter_empty_read_succeeded");
    }

    WirelessPacketHeader expected_header;
    expected_header.magic = WIRELESS_PACKET_MAGIC;
    expected_header.protocol_version = WIRELESS_PROTOCOL_VERSION;
    expected_header.packet_type = WirelessPacketType::CAPABILITY_VALUE;
    expected_header.flags = 0x01;
    expected_header.sequence_id = 11;
    expected_header.node_id = 1001;
    expected_header.payload_length = static_cast<uint8_t>(
        sizeof(WirelessCapabilityValue) + sizeof(WirelessNodeDiagnostics));
    expected_header.checksum = 0;

    WirelessCapabilityValue expected_value;
    copyWirelessCapabilityId(expected_value.capability_id, CAP_TEMPERATURE);
    expected_value.payload_type = WirelessPayloadType::FLOAT;
    expected_value.value_float = 21.25F;
    expected_value.value_int = 2;
    copyWirelessCapabilityId(expected_value.error_code, "none");

    WirelessNodeDiagnostics expected_diagnostics;
    expected_diagnostics.battery_present = true;
    expected_diagnostics.battery_level_percent = 90.0F;
    expected_diagnostics.battery_voltage = 4.0F;
    expected_diagnostics.signal_quality_percent = 80.0F;
    expected_header.checksum =
        calculateWirelessPacketChecksum(expected_header, expected_value, expected_diagnostics);

    if (!sim_driver.injectReceivedCapabilityValue(
            expected_header,
            expected_value,
            expected_diagnostics)) {
        return fail("sim_transport_adapter_no_mac_inject_failed");
    }
    if (!adapter.has_received_packet(adapter.context)) {
        return fail("sim_transport_adapter_no_mac_missing");
    }
    clearWirelessMacAddress(source_mac);
    has_source_mac = true;
    if (!adapter.read_received_packet(
            adapter.context,
            header,
            value,
            diagnostics,
            source_mac,
            has_source_mac)) {
        return fail("sim_transport_adapter_no_mac_read_failed");
    }
    if (has_source_mac) {
        return fail("sim_transport_adapter_no_mac_flag_invalid");
    }
    uint8_t empty_mac[WIRELESS_MAC_ADDRESS_SIZE];
    clearWirelessMacAddress(empty_mac);
    if (!wirelessMacAddressEquals(source_mac, empty_mac)) {
        return fail("sim_transport_adapter_no_mac_not_cleared");
    }
    if (header.magic != expected_header.magic ||
        header.protocol_version != expected_header.protocol_version ||
        header.packet_type != expected_header.packet_type ||
        header.flags != expected_header.flags ||
        header.sequence_id != expected_header.sequence_id ||
        header.node_id != expected_header.node_id ||
        header.payload_length != expected_header.payload_length ||
        header.checksum != expected_header.checksum) {
        return fail("sim_transport_adapter_no_mac_header_invalid");
    }
    if (!isSameText(value.capability_id, CAP_TEMPERATURE) ||
        value.payload_type != expected_value.payload_type ||
        value.value_float != expected_value.value_float ||
        value.value_int != expected_value.value_int ||
        !isSameText(value.error_code, "none")) {
        return fail("sim_transport_adapter_no_mac_value_invalid");
    }
    if (diagnostics.battery_present != expected_diagnostics.battery_present ||
        diagnostics.battery_level_percent != expected_diagnostics.battery_level_percent ||
        diagnostics.battery_voltage != expected_diagnostics.battery_voltage ||
        diagnostics.signal_quality_percent != expected_diagnostics.signal_quality_percent) {
        return fail("sim_transport_adapter_no_mac_diagnostics_invalid");
    }
    if (adapter.has_received_packet(adapter.context)) {
        return fail("sim_transport_adapter_no_mac_not_cleared");
    }

    const uint8_t expected_mac[WIRELESS_MAC_ADDRESS_SIZE] = {
        0xAA,
        0xBB,
        0xCC,
        0x77,
        0x88,
        0x99
    };
    expected_header.sequence_id = 12;
    expected_value.value_float = 22.5F;
    expected_header.checksum =
        calculateWirelessPacketChecksum(expected_header, expected_value, expected_diagnostics);
    if (!sim_driver.injectReceivedCapabilityValueWithMac(
            expected_mac,
            expected_header,
            expected_value,
            expected_diagnostics)) {
        return fail("sim_transport_adapter_mac_inject_failed");
    }
    clearWirelessMacAddress(source_mac);
    has_source_mac = false;
    if (!adapter.read_received_packet(
            adapter.context,
            header,
            value,
            diagnostics,
            source_mac,
            has_source_mac)) {
        return fail("sim_transport_adapter_mac_read_failed");
    }
    if (!has_source_mac) {
        return fail("sim_transport_adapter_mac_flag_invalid");
    }
    if (!wirelessMacAddressEquals(source_mac, expected_mac)) {
        return fail("sim_transport_adapter_mac_invalid");
    }
    if (header.sequence_id != expected_header.sequence_id ||
        value.value_float != expected_value.value_float ||
        diagnostics.signal_quality_percent != expected_diagnostics.signal_quality_percent) {
        return fail("sim_transport_adapter_mac_packet_invalid");
    }

    if (!sim_driver.injectReceivedCapabilityValue(
            expected_header,
            expected_value,
            expected_diagnostics)) {
        return fail("sim_transport_adapter_clear_inject_failed");
    }
    adapter.clear_received_packet(adapter.context);
    if (adapter.has_received_packet(adapter.context)) {
        return fail("sim_transport_adapter_clear_failed");
    }

    return true;
}

bool VerticalSliceValidation::validateEspNowWirelessPacketTransportAdapter() {
    WirelessPacketTransportAdapter null_adapter =
        makeEspNowTransportAdapter(0);
    if (wirelessPacketTransportAdapterValid(null_adapter)) {
        return fail("espnow_transport_null_adapter_valid");
    }

    EspNowTransportDriver espnow_driver;
    WirelessPacketTransportAdapter adapter =
        makeEspNowTransportAdapter(&espnow_driver);
    if (!wirelessPacketTransportAdapterValid(adapter)) {
        return fail("espnow_transport_adapter_invalid");
    }
    if (adapter.has_received_packet(adapter.context)) {
        return fail("espnow_transport_adapter_empty_has_packet");
    }

    WirelessPacketHeader header;
    WirelessCapabilityValue value;
    WirelessNodeDiagnostics diagnostics;
    uint8_t source_mac[WIRELESS_MAC_ADDRESS_SIZE];
    bool has_source_mac = true;
    if (adapter.read_received_packet(
            adapter.context,
            header,
            value,
            diagnostics,
            source_mac,
            has_source_mac)) {
        return fail("espnow_transport_adapter_empty_read_succeeded");
    }

    enum {
        EXPECTED_STRUCTURED_PACKET_SIZE =
            sizeof(WirelessPacketHeader) +
            sizeof(WirelessCapabilityValue) +
            sizeof(WirelessNodeDiagnostics)
    };
    if (EXPECTED_STRUCTURED_PACKET_SIZE > WIRELESS_MAX_PACKET_SIZE) {
        return fail("espnow_transport_adapter_packet_size_invalid");
    }

    WirelessPacketHeader expected_header;
    expected_header.magic = WIRELESS_PACKET_MAGIC;
    expected_header.protocol_version = WIRELESS_PROTOCOL_VERSION;
    expected_header.packet_type = WirelessPacketType::CAPABILITY_VALUE;
    expected_header.flags = 0x03;
    expected_header.sequence_id = 33;
    expected_header.node_id = 1001;
    expected_header.payload_length = static_cast<uint8_t>(
        sizeof(WirelessCapabilityValue) + sizeof(WirelessNodeDiagnostics));
    expected_header.checksum = 0;

    WirelessCapabilityValue expected_value;
    copyWirelessCapabilityId(expected_value.capability_id, CAP_TEMPERATURE);
    expected_value.payload_type = WirelessPayloadType::FLOAT;
    expected_value.value_float = 23.75F;
    expected_value.value_int = 4;
    copyWirelessCapabilityId(expected_value.error_code, "none");

    WirelessNodeDiagnostics expected_diagnostics;
    expected_diagnostics.battery_present = true;
    expected_diagnostics.battery_level_percent = 86.0F;
    expected_diagnostics.battery_voltage = 3.8F;
    expected_diagnostics.signal_quality_percent = 76.0F;
    expected_header.checksum =
        calculateWirelessPacketChecksum(expected_header, expected_value, expected_diagnostics);

    uint8_t raw_payload[EXPECTED_STRUCTURED_PACKET_SIZE];
    uint16_t raw_offset = 0;
    const uint8_t* header_bytes =
        reinterpret_cast<const uint8_t*>(&expected_header);
    for (uint16_t i = 0; i < sizeof(WirelessPacketHeader); ++i) {
        raw_payload[raw_offset] = header_bytes[i];
        ++raw_offset;
    }
    const uint8_t* value_bytes =
        reinterpret_cast<const uint8_t*>(&expected_value);
    for (uint16_t i = 0; i < sizeof(WirelessCapabilityValue); ++i) {
        raw_payload[raw_offset] = value_bytes[i];
        ++raw_offset;
    }
    const uint8_t* diagnostics_bytes =
        reinterpret_cast<const uint8_t*>(&expected_diagnostics);
    for (uint16_t i = 0; i < sizeof(WirelessNodeDiagnostics); ++i) {
        raw_payload[raw_offset] = diagnostics_bytes[i];
        ++raw_offset;
    }

    const uint8_t expected_mac[WIRELESS_MAC_ADDRESS_SIZE] = {
        0xAA,
        0xBB,
        0xCC,
        0x10,
        0x20,
        0x30
    };
    if (!espnow_driver.injectRawPayloadForTest(
            expected_mac,
            raw_payload,
            static_cast<uint16_t>(EXPECTED_STRUCTURED_PACKET_SIZE))) {
        return fail("espnow_transport_adapter_raw_inject_failed");
    }
    if (!espnow_driver.decodePendingRawPayload()) {
        return fail("espnow_transport_adapter_decode_failed");
    }
    if (!adapter.has_received_packet(adapter.context)) {
        return fail("espnow_transport_adapter_packet_missing");
    }

    clearWirelessMacAddress(source_mac);
    has_source_mac = false;
    if (!adapter.read_received_packet(
            adapter.context,
            header,
            value,
            diagnostics,
            source_mac,
            has_source_mac)) {
        return fail("espnow_transport_adapter_read_failed");
    }
    if (header.magic != expected_header.magic ||
        header.protocol_version != expected_header.protocol_version ||
        header.packet_type != expected_header.packet_type ||
        header.flags != expected_header.flags ||
        header.sequence_id != expected_header.sequence_id ||
        header.node_id != expected_header.node_id ||
        header.payload_length != expected_header.payload_length ||
        header.checksum != expected_header.checksum) {
        return fail("espnow_transport_adapter_header_invalid");
    }
    if (!isSameText(value.capability_id, CAP_TEMPERATURE) ||
        value.payload_type != expected_value.payload_type ||
        value.value_float != expected_value.value_float ||
        value.value_int != expected_value.value_int ||
        !isSameText(value.error_code, "none")) {
        return fail("espnow_transport_adapter_value_invalid");
    }
    if (diagnostics.battery_present != expected_diagnostics.battery_present ||
        diagnostics.battery_level_percent != expected_diagnostics.battery_level_percent ||
        diagnostics.battery_voltage != expected_diagnostics.battery_voltage ||
        diagnostics.signal_quality_percent != expected_diagnostics.signal_quality_percent) {
        return fail("espnow_transport_adapter_diagnostics_invalid");
    }
    if (!has_source_mac) {
        return fail("espnow_transport_adapter_mac_missing");
    }
    if (!wirelessMacAddressEquals(source_mac, expected_mac)) {
        return fail("espnow_transport_adapter_mac_invalid");
    }
    if (adapter.has_received_packet(adapter.context)) {
        return fail("espnow_transport_adapter_not_cleared_after_read");
    }

    expected_header.sequence_id = 34;
    expected_header.checksum =
        calculateWirelessPacketChecksum(expected_header, expected_value, expected_diagnostics);
    raw_offset = 0;
    header_bytes = reinterpret_cast<const uint8_t*>(&expected_header);
    for (uint16_t i = 0; i < sizeof(WirelessPacketHeader); ++i) {
        raw_payload[raw_offset] = header_bytes[i];
        ++raw_offset;
    }
    for (uint16_t i = 0; i < sizeof(WirelessCapabilityValue); ++i) {
        raw_payload[raw_offset] = value_bytes[i];
        ++raw_offset;
    }
    for (uint16_t i = 0; i < sizeof(WirelessNodeDiagnostics); ++i) {
        raw_payload[raw_offset] = diagnostics_bytes[i];
        ++raw_offset;
    }
    if (!espnow_driver.injectRawPayloadForTest(
            expected_mac,
            raw_payload,
            static_cast<uint16_t>(EXPECTED_STRUCTURED_PACKET_SIZE))) {
        return fail("espnow_transport_adapter_clear_inject_failed");
    }
    if (!espnow_driver.decodePendingRawPayload()) {
        return fail("espnow_transport_adapter_clear_decode_failed");
    }
    adapter.clear_received_packet(adapter.context);
    if (adapter.has_received_packet(adapter.context)) {
        return fail("espnow_transport_adapter_clear_failed");
    }

    return true;
}

bool VerticalSliceValidation::validateWirelessServiceTransportAdapterAttachment() {
    WirelessPacketTransportAdapter invalid_adapter;
    invalid_adapter.context = 0;
    invalid_adapter.has_received_packet = 0;
    invalid_adapter.read_received_packet = 0;
    invalid_adapter.clear_received_packet = 0;

    if (wireless_service_.attachTransportAdapter(invalid_adapter)) {
        return fail("wireless_service_invalid_adapter_attached");
    }

    SimEspNowTransportDriver adapter_driver;
    adapter_driver.begin();
    WirelessPacketTransportAdapter adapter =
        makeSimEspNowTransportAdapter(&adapter_driver);
    if (!wirelessPacketTransportAdapterValid(adapter)) {
        return fail("wireless_service_adapter_setup_invalid");
    }
    if (!wireless_service_.attachTransportAdapter(adapter)) {
        return fail("wireless_service_valid_adapter_rejected");
    }

    wireless_service_.begin();
    if (wireless_service_.attachTransportAdapter(invalid_adapter)) {
        return fail("wireless_service_invalid_adapter_after_begin_attached");
    }

    wireless_service_.attachRegistry(&registry_);
    wireless_service_.attachTransportDriver(&wireless_transport_driver_);
    wireless_service_.attachWirelessTemperatureDevice(&wireless_temperature_device_);

    return true;
}

bool VerticalSliceValidation::validateWirelessServiceProcessPacketsAdapterPath(uint32_t now_ms) {
    Registry local_registry;
    local_registry.begin();

    WirelessNodeAllowlistRecord allowlist_record;
    allowlist_record.node_id = 1001;
    allowlist_record.allow_state = WirelessNodeAllowState::ALLOWED;
    allowlist_record.trust_state = WirelessTrustState::TRUSTED;
    allowlist_record.added_at_ms = now_ms;
    allowlist_record.last_seen_ms = now_ms;
    allowlist_record.has_mac_address = true;
    allowlist_record.mac_address[0] = 0xAA;
    allowlist_record.mac_address[1] = 0xBB;
    allowlist_record.mac_address[2] = 0xCC;
    allowlist_record.mac_address[3] = 0x01;
    allowlist_record.mac_address[4] = 0x02;
    allowlist_record.mac_address[5] = 0x03;
    if (local_registry.registerWirelessNodeAllowlistRecordWithResult(allowlist_record).result !=
        RegistryResult::OK) {
        return fail("wireless_service_adapter_allowlist_register_failed");
    }

    CapabilityPayload stale_payload;
    stale_payload.capability_id = CAP_TEMPERATURE;
    stale_payload.schema_version = 1;
    stale_payload.timestamp_ms = 0;
    stale_payload.available = Availability::AVAILABLE;
    stale_payload.stale = StaleState::STALE;
    stale_payload.value_type = PayloadValueType::FLOAT;
    stale_payload.value_float = 0.0F;
    stale_payload.value_int = 0;
    stale_payload.unit = "degree_celsius";
    stale_payload.quality = "stale";
    stale_payload.error_code = "none";

    CapabilityProviderRecord provider;
    provider.provider_id = WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID;
    provider.capability_id = CAP_TEMPERATURE;
    provider.owner_module_index = 0;
    provider.owner_device_index = 0;
    provider.provider_type = CapabilityProviderType::WIRELESS;
    provider.status = CapabilityProviderStatus::STALE;
    provider.priority = 20;
    provider.last_update_ms = 0;
    provider.latest_payload = stale_payload;
    if (local_registry.registerCapabilityProviderWithResult(provider).result !=
        RegistryResult::OK) {
        return fail("wireless_service_adapter_provider_register_failed");
    }

    WirelessTemperatureDevice local_device;
    if (!local_device.begin(1001)) {
        return fail("wireless_service_adapter_device_begin_failed");
    }
    local_device.setTrustState(WirelessTrustState::TRUSTED);

    SimEspNowTransportDriver adapter_driver;
    adapter_driver.begin();
    WirelessPacketTransportAdapter adapter =
        makeSimEspNowTransportAdapter(&adapter_driver);
    if (!wirelessPacketTransportAdapterValid(adapter)) {
        return fail("wireless_service_process_adapter_invalid");
    }

    WirelessService local_service;
    local_service.begin();
    local_service.attachRegistry(&local_registry);
    if (!local_service.attachTransportAdapter(adapter)) {
        return fail("wireless_service_process_adapter_attach_failed");
    }
    local_service.attachWirelessTemperatureDevice(&local_device);

    WirelessPacketHeader header;
    header.magic = WIRELESS_PACKET_MAGIC;
    header.protocol_version = WIRELESS_PROTOCOL_VERSION;
    header.packet_type = WirelessPacketType::CAPABILITY_VALUE;
    header.flags = 0;
    header.sequence_id = 701;
    header.node_id = 1001;
    header.payload_length = static_cast<uint8_t>(
        sizeof(WirelessCapabilityValue) + sizeof(WirelessNodeDiagnostics));
    header.checksum = 0;

    WirelessCapabilityValue value;
    copyWirelessCapabilityId(value.capability_id, CAP_TEMPERATURE);
    value.payload_type = WirelessPayloadType::FLOAT;
    value.value_float = 26.5F;
    value.value_int = 0;
    copyWirelessCapabilityId(value.error_code, "none");

    WirelessNodeDiagnostics diagnostics;
    diagnostics.battery_present = true;
    diagnostics.battery_level_percent = 84.0F;
    diagnostics.battery_voltage = 3.7F;
    diagnostics.signal_quality_percent = 72.0F;
    header.checksum = calculateWirelessPacketChecksum(header, value, diagnostics);

    const uint8_t source_mac[WIRELESS_MAC_ADDRESS_SIZE] = {
        0xAA,
        0xBB,
        0xCC,
        0x01,
        0x02,
        0x03
    };
    if (!adapter_driver.injectReceivedCapabilityValueWithMac(
            source_mac,
            header,
            value,
            diagnostics)) {
        return fail("wireless_service_process_adapter_inject_failed");
    }
    if (!local_service.processPackets(now_ms)) {
        return fail(local_service.lastErrorCode());
    }
    CapabilityProviderRecord provider_out;
    if (local_registry.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            provider_out) != RegistryResult::OK) {
        return fail("wireless_service_process_adapter_provider_missing");
    }
    if (provider_out.latest_payload.value_float != 26.5F) {
        return fail("wireless_service_process_adapter_payload_invalid");
    }
    if (adapter.has_received_packet(adapter.context)) {
        return fail("wireless_service_process_adapter_packet_not_cleared");
    }

    local_service.begin();
    local_service.attachRegistry(&local_registry);
    local_service.attachTransportDriver(&adapter_driver);
    local_service.attachWirelessTemperatureDevice(&local_device);

    header.sequence_id = 702;
    value.value_float = 27.5F;
    header.checksum = calculateWirelessPacketChecksum(header, value, diagnostics);
    if (!adapter_driver.injectReceivedCapabilityValue(
            header,
            value,
            diagnostics)) {
        return fail("wireless_service_process_fallback_inject_failed");
    }
    if (!local_service.processPackets(now_ms + 1)) {
        return fail(local_service.lastErrorCode());
    }
    if (local_registry.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            provider_out) != RegistryResult::OK) {
        return fail("wireless_service_process_fallback_provider_missing");
    }
    if (provider_out.latest_payload.value_float != 27.5F) {
        return fail("wireless_service_process_fallback_payload_invalid");
    }

    header.sequence_id = 703;
    value.value_float = 28.5F;
    header.checksum = static_cast<uint16_t>(
        calculateWirelessPacketChecksum(header, value, diagnostics) + 1);
    if (!adapter_driver.injectReceivedCapabilityValue(
            header,
            value,
            diagnostics)) {
        return fail("wireless_service_process_bad_checksum_inject_failed");
    }
    if (local_service.processPackets(now_ms + 2)) {
        return fail("wireless_service_process_bad_checksum_succeeded");
    }
    if (!isSameText(local_service.lastErrorCode(), "wireless_checksum_invalid")) {
        return fail("wireless_service_process_bad_checksum_error_invalid");
    }

    header.sequence_id = 704;
    header.node_id = 2002;
    value.value_float = 29.5F;
    header.checksum = calculateWirelessPacketChecksum(header, value, diagnostics);
    if (!adapter_driver.injectReceivedCapabilityValue(
            header,
            value,
            diagnostics)) {
        return fail("wireless_service_process_unknown_node_inject_failed");
    }
    if (local_service.processPackets(now_ms + 3)) {
        return fail("wireless_service_process_unknown_node_succeeded");
    }
    if (!isSameText(local_service.lastErrorCode(), "wireless_node_not_allowed")) {
        return fail("wireless_service_process_unknown_node_error_invalid");
    }

    header.sequence_id = 702;
    header.node_id = 1001;
    value.value_float = 30.5F;
    header.checksum = calculateWirelessPacketChecksum(header, value, diagnostics);
    if (!adapter_driver.injectReceivedCapabilityValue(
            header,
            value,
            diagnostics)) {
        return fail("wireless_service_process_duplicate_inject_failed");
    }
    if (local_service.processPackets(now_ms + 4)) {
        return fail("wireless_service_process_duplicate_succeeded");
    }
    if (!isSameText(local_service.lastErrorCode(), "wireless_duplicate_sequence")) {
        return fail("wireless_service_process_duplicate_error_invalid");
    }

    if (local_registry.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            provider_out) != RegistryResult::OK) {
        return fail("wireless_service_process_policy_provider_missing");
    }
    if (provider_out.latest_payload.value_float != 27.5F) {
        return fail("wireless_service_process_policy_changed_payload");
    }

    return true;
}

bool VerticalSliceValidation::validateEspNowAdapterWirelessServicePath(uint32_t now_ms) {
    Registry local_registry;
    local_registry.begin();

    CapabilityPayload canonical_payload;
    canonical_payload.capability_id = CAP_TEMPERATURE;
    canonical_payload.schema_version = 1;
    canonical_payload.timestamp_ms = now_ms;
    canonical_payload.available = Availability::AVAILABLE;
    canonical_payload.stale = StaleState::FRESH;
    canonical_payload.value_type = PayloadValueType::FLOAT;
    canonical_payload.value_float = 22.4F;
    canonical_payload.value_int = 0;
    canonical_payload.unit = "degree_celsius";
    canonical_payload.quality = "valid";
    canonical_payload.error_code = "none";

    ModuleRecord module;
    module.module_id = "module-espnow-adapter-validation-001";
    module.module_type = "wireless_node";
    module.status = RecordStatus::AVAILABLE;
    module.device_count = 1;
    module.capability_count = 1;
    if (local_registry.registerModuleWithResult(module).result != RegistryResult::OK) {
        return fail("espnow_service_path_module_register_failed");
    }

    DeviceRecord device;
    device.device_id = "device-espnow-adapter-validation-001";
    device.device_type = "wireless_sensor";
    device.status = RecordStatus::AVAILABLE;
    device.module_index = 0;
    device.capability_count = 1;
    if (local_registry.registerDeviceWithResult(device).result != RegistryResult::OK) {
        return fail("espnow_service_path_device_register_failed");
    }

    CapabilityRecord capability;
    capability.capability_id = CAP_TEMPERATURE;
    capability.category = "sensors";
    capability.kind = "sensor";
    capability.data_type = PayloadValueType::FLOAT;
    capability.access = "read";
    capability.status = RecordStatus::AVAILABLE;
    capability.owner_device_index = 0;
    capability.latest_payload = canonical_payload;
    if (local_registry.registerCapabilityWithResult(capability).result != RegistryResult::OK) {
        return fail("espnow_service_path_capability_register_failed");
    }

    WirelessNodeAllowlistRecord allowlist_record;
    allowlist_record.node_id = 1001;
    allowlist_record.allow_state = WirelessNodeAllowState::ALLOWED;
    allowlist_record.trust_state = WirelessTrustState::TRUSTED;
    allowlist_record.added_at_ms = now_ms;
    allowlist_record.last_seen_ms = now_ms;
    allowlist_record.has_mac_address = true;
    allowlist_record.mac_address[0] = 0xAA;
    allowlist_record.mac_address[1] = 0xBB;
    allowlist_record.mac_address[2] = 0xCC;
    allowlist_record.mac_address[3] = 0x44;
    allowlist_record.mac_address[4] = 0x55;
    allowlist_record.mac_address[5] = 0x66;
    if (local_registry.registerWirelessNodeAllowlistRecordWithResult(allowlist_record).result !=
        RegistryResult::OK) {
        return fail("espnow_service_path_allowlist_register_failed");
    }

    CapabilityPayload stale_provider_payload;
    stale_provider_payload.capability_id = CAP_TEMPERATURE;
    stale_provider_payload.schema_version = 1;
    stale_provider_payload.timestamp_ms = 0;
    stale_provider_payload.available = Availability::AVAILABLE;
    stale_provider_payload.stale = StaleState::STALE;
    stale_provider_payload.value_type = PayloadValueType::FLOAT;
    stale_provider_payload.value_float = 0.0F;
    stale_provider_payload.value_int = 0;
    stale_provider_payload.unit = "degree_celsius";
    stale_provider_payload.quality = "stale";
    stale_provider_payload.error_code = "none";

    CapabilityProviderRecord provider;
    provider.provider_id = WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID;
    provider.capability_id = CAP_TEMPERATURE;
    provider.owner_module_index = 0;
    provider.owner_device_index = 0;
    provider.provider_type = CapabilityProviderType::WIRELESS;
    provider.status = CapabilityProviderStatus::STALE;
    provider.priority = 20;
    provider.last_update_ms = 0;
    provider.latest_payload = stale_provider_payload;
    if (local_registry.registerCapabilityProviderWithResult(provider).result !=
        RegistryResult::OK) {
        return fail("espnow_service_path_provider_register_failed");
    }

    WirelessNodeSecurityDiagnosticRecord security_record;
    security_record.node_id = 1001;
    security_record.has_mac_address = false;
    clearWirelessMacAddress(security_record.mac_address);
    security_record.allow_state = WirelessNodeAllowState::ALLOWED;
    security_record.trust_state = WirelessTrustState::TRUSTED;
    security_record.last_seen_ms = 0;
    security_record.last_accepted_sequence_id = 0;
    security_record.last_rejected_sequence_id = 0;
    security_record.last_error_code = "none";
    security_record.checksum_reject_count = 0;
    security_record.mac_not_allowed_reject_count = 0;
    security_record.mac_node_mismatch_reject_count = 0;
    security_record.blocked_reject_count = 0;
    security_record.not_allowed_reject_count = 0;
    security_record.untrusted_reject_count = 0;
    security_record.duplicate_sequence_reject_count = 0;
    security_record.invalid_packet_reject_count = 0;
    security_record.accepted_packet_count = 0;
    if (local_registry.registerWirelessNodeSecurityDiagnosticWithResult(security_record).result !=
        RegistryResult::OK) {
        return fail("espnow_service_path_security_register_failed");
    }

    WirelessTemperatureDevice local_device;
    if (!local_device.begin(1001)) {
        return fail("espnow_service_path_device_begin_failed");
    }
    local_device.setTrustState(WirelessTrustState::TRUSTED);

    EspNowTransportDriver espnow_driver;
    WirelessPacketTransportAdapter adapter =
        makeEspNowTransportAdapter(&espnow_driver);
    if (!wirelessPacketTransportAdapterValid(adapter)) {
        return fail("espnow_service_path_adapter_invalid");
    }

    WirelessService local_service;
    local_service.begin();
    local_service.attachRegistry(&local_registry);
    if (!local_service.attachTransportAdapter(adapter)) {
        return fail("espnow_service_path_adapter_attach_failed");
    }
    local_service.attachWirelessTemperatureDevice(&local_device);

    WirelessPacketHeader header;
    header.magic = WIRELESS_PACKET_MAGIC;
    header.protocol_version = WIRELESS_PROTOCOL_VERSION;
    header.packet_type = WirelessPacketType::CAPABILITY_VALUE;
    header.flags = 0;
    header.sequence_id = 801;
    header.node_id = 1001;
    header.payload_length = static_cast<uint8_t>(
        sizeof(WirelessCapabilityValue) + sizeof(WirelessNodeDiagnostics));
    header.checksum = 0;

    WirelessCapabilityValue value;
    copyWirelessCapabilityId(value.capability_id, CAP_TEMPERATURE);
    value.payload_type = WirelessPayloadType::FLOAT;
    value.value_float = 31.5F;
    value.value_int = 0;
    copyWirelessCapabilityId(value.error_code, "none");

    WirelessNodeDiagnostics diagnostics;
    diagnostics.battery_present = true;
    diagnostics.battery_level_percent = 82.0F;
    diagnostics.battery_voltage = 3.72F;
    diagnostics.signal_quality_percent = 74.0F;
    header.checksum = calculateWirelessPacketChecksum(header, value, diagnostics);

    enum {
        EXPECTED_STRUCTURED_PACKET_SIZE =
            sizeof(WirelessPacketHeader) +
            sizeof(WirelessCapabilityValue) +
            sizeof(WirelessNodeDiagnostics)
    };
    if (EXPECTED_STRUCTURED_PACKET_SIZE > WIRELESS_MAX_PACKET_SIZE) {
        return fail("espnow_service_path_packet_size_invalid");
    }

    uint8_t raw_payload[EXPECTED_STRUCTURED_PACKET_SIZE];
    uint16_t raw_offset = 0;
    const uint8_t* header_bytes = reinterpret_cast<const uint8_t*>(&header);
    for (uint16_t i = 0; i < sizeof(WirelessPacketHeader); ++i) {
        raw_payload[raw_offset] = header_bytes[i];
        ++raw_offset;
    }
    const uint8_t* value_bytes = reinterpret_cast<const uint8_t*>(&value);
    for (uint16_t i = 0; i < sizeof(WirelessCapabilityValue); ++i) {
        raw_payload[raw_offset] = value_bytes[i];
        ++raw_offset;
    }
    const uint8_t* diagnostics_bytes = reinterpret_cast<const uint8_t*>(&diagnostics);
    for (uint16_t i = 0; i < sizeof(WirelessNodeDiagnostics); ++i) {
        raw_payload[raw_offset] = diagnostics_bytes[i];
        ++raw_offset;
    }

    const uint8_t source_mac[WIRELESS_MAC_ADDRESS_SIZE] = {
        0xAA,
        0xBB,
        0xCC,
        0x44,
        0x55,
        0x66
    };
    if (!espnow_driver.injectRawPayloadForTest(
            source_mac,
            raw_payload,
            static_cast<uint16_t>(EXPECTED_STRUCTURED_PACKET_SIZE))) {
        return fail("espnow_service_path_raw_inject_failed");
    }
    if (!espnow_driver.decodePendingRawPayload()) {
        return fail("espnow_service_path_decode_failed");
    }
    if (!adapter.has_received_packet(adapter.context)) {
        return fail("espnow_service_path_packet_missing");
    }

    if (!local_service.processPackets(now_ms + 1)) {
        return fail(local_service.lastErrorCode());
    }

    CapabilityProviderRecord provider_out;
    if (local_registry.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            provider_out) != RegistryResult::OK) {
        return fail("espnow_service_path_provider_missing");
    }
    if (provider_out.status != CapabilityProviderStatus::AVAILABLE) {
        return fail("espnow_service_path_provider_status_invalid");
    }
    if (!isSameText(provider_out.latest_payload.capability_id, CAP_TEMPERATURE) ||
        provider_out.latest_payload.value_float != 31.5F ||
        provider_out.latest_payload.stale != StaleState::FRESH ||
        provider_out.last_update_ms != now_ms + 1) {
        return fail("espnow_service_path_provider_payload_invalid");
    }
    if (adapter.has_received_packet(adapter.context) ||
        espnow_driver.hasReceivedPacket() ||
        espnow_driver.hasRawPayload()) {
        return fail("espnow_service_path_packet_not_cleared");
    }
    WirelessNodeSecurityDiagnosticRecord security_out;
    if (local_registry.getWirelessNodeSecurityDiagnostic(1001, security_out) !=
        RegistryResult::OK) {
        return fail("espnow_service_path_security_accept_missing");
    }
    if (security_out.accepted_packet_count != 1 ||
        security_out.last_accepted_sequence_id != 801 ||
        security_out.last_seen_ms != now_ms + 1 ||
        !isSameText(security_out.last_error_code, "none") ||
        !security_out.has_mac_address ||
        !wirelessMacAddressEquals(security_out.mac_address, source_mac)) {
        return fail("espnow_service_path_security_accept_invalid");
    }

    header.sequence_id = 802;
    header.node_id = 1001;
    value.value_float = 32.5F;
    header.checksum = static_cast<uint16_t>(
        calculateWirelessPacketChecksum(header, value, diagnostics) + 1);
    raw_offset = 0;
    header_bytes = reinterpret_cast<const uint8_t*>(&header);
    for (uint16_t i = 0; i < sizeof(WirelessPacketHeader); ++i) {
        raw_payload[raw_offset] = header_bytes[i];
        ++raw_offset;
    }
    value_bytes = reinterpret_cast<const uint8_t*>(&value);
    for (uint16_t i = 0; i < sizeof(WirelessCapabilityValue); ++i) {
        raw_payload[raw_offset] = value_bytes[i];
        ++raw_offset;
    }
    diagnostics_bytes = reinterpret_cast<const uint8_t*>(&diagnostics);
    for (uint16_t i = 0; i < sizeof(WirelessNodeDiagnostics); ++i) {
        raw_payload[raw_offset] = diagnostics_bytes[i];
        ++raw_offset;
    }
    if (!espnow_driver.injectRawPayloadForTest(
            source_mac,
            raw_payload,
            static_cast<uint16_t>(EXPECTED_STRUCTURED_PACKET_SIZE))) {
        return fail("espnow_service_path_bad_checksum_inject_failed");
    }
    if (!espnow_driver.decodePendingRawPayload()) {
        return fail("espnow_service_path_bad_checksum_decode_failed");
    }
    if (local_service.processPackets(now_ms + 2)) {
        return fail("espnow_service_path_bad_checksum_succeeded");
    }
    if (!isSameText(local_service.lastErrorCode(), "wireless_checksum_invalid")) {
        return fail("espnow_service_path_bad_checksum_error_invalid");
    }
    if (local_registry.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            provider_out) != RegistryResult::OK) {
        return fail("espnow_service_path_bad_checksum_provider_missing");
    }
    if (provider_out.latest_payload.value_float != 31.5F ||
        provider_out.last_update_ms != now_ms + 1) {
        return fail("espnow_service_path_bad_checksum_changed_provider");
    }
    if (adapter.has_received_packet(adapter.context) ||
        espnow_driver.hasReceivedPacket() ||
        espnow_driver.hasRawPayload()) {
        return fail("espnow_service_path_bad_checksum_not_cleared");
    }
    if (local_registry.getWirelessNodeSecurityDiagnostic(1001, security_out) !=
        RegistryResult::OK) {
        return fail("espnow_service_path_security_checksum_missing");
    }
    if (security_out.checksum_reject_count != 1 ||
        security_out.last_rejected_sequence_id != 802 ||
        security_out.last_seen_ms != now_ms + 2 ||
        !isSameText(security_out.last_error_code, "wireless_checksum_invalid")) {
        return fail("espnow_service_path_security_checksum_invalid");
    }

    header.sequence_id = 803;
    header.node_id = 1001;
    value.value_float = 33.5F;
    header.checksum = calculateWirelessPacketChecksum(header, value, diagnostics);
    raw_offset = 0;
    header_bytes = reinterpret_cast<const uint8_t*>(&header);
    for (uint16_t i = 0; i < sizeof(WirelessPacketHeader); ++i) {
        raw_payload[raw_offset] = header_bytes[i];
        ++raw_offset;
    }
    value_bytes = reinterpret_cast<const uint8_t*>(&value);
    for (uint16_t i = 0; i < sizeof(WirelessCapabilityValue); ++i) {
        raw_payload[raw_offset] = value_bytes[i];
        ++raw_offset;
    }
    diagnostics_bytes = reinterpret_cast<const uint8_t*>(&diagnostics);
    for (uint16_t i = 0; i < sizeof(WirelessNodeDiagnostics); ++i) {
        raw_payload[raw_offset] = diagnostics_bytes[i];
        ++raw_offset;
    }
    const uint8_t unknown_mac[WIRELESS_MAC_ADDRESS_SIZE] = {
        0xAA,
        0xBB,
        0xCC,
        0x77,
        0x88,
        0x99
    };
    if (!espnow_driver.injectRawPayloadForTest(
            unknown_mac,
            raw_payload,
            static_cast<uint16_t>(EXPECTED_STRUCTURED_PACKET_SIZE))) {
        return fail("espnow_service_path_unknown_mac_inject_failed");
    }
    if (!espnow_driver.decodePendingRawPayload()) {
        return fail("espnow_service_path_unknown_mac_decode_failed");
    }
    if (local_service.processPackets(now_ms + 3)) {
        return fail("espnow_service_path_unknown_mac_succeeded");
    }
    if (!isSameText(local_service.lastErrorCode(), "wireless_mac_not_allowed")) {
        return fail("espnow_service_path_unknown_mac_error_invalid");
    }
    if (local_registry.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            provider_out) != RegistryResult::OK) {
        return fail("espnow_service_path_unknown_mac_provider_missing");
    }
    if (provider_out.latest_payload.value_float != 31.5F ||
        provider_out.last_update_ms != now_ms + 1) {
        return fail("espnow_service_path_unknown_mac_changed_provider");
    }
    if (adapter.has_received_packet(adapter.context) ||
        espnow_driver.hasReceivedPacket() ||
        espnow_driver.hasRawPayload()) {
        return fail("espnow_service_path_unknown_mac_not_cleared");
    }
    if (local_registry.getWirelessNodeSecurityDiagnostic(1001, security_out) !=
        RegistryResult::OK) {
        return fail("espnow_service_path_security_unknown_mac_missing");
    }
    if (security_out.mac_not_allowed_reject_count != 1 ||
        security_out.last_rejected_sequence_id != 803 ||
        security_out.last_seen_ms != now_ms + 3 ||
        !isSameText(security_out.last_error_code, "wireless_mac_not_allowed")) {
        return fail("espnow_service_path_security_unknown_mac_invalid");
    }

    WirelessNodeAllowlistRecord blocked_mac_record;
    blocked_mac_record.node_id = 3003;
    blocked_mac_record.allow_state = WirelessNodeAllowState::BLOCKED;
    blocked_mac_record.trust_state = WirelessTrustState::BLOCKED;
    blocked_mac_record.added_at_ms = now_ms;
    blocked_mac_record.last_seen_ms = now_ms;
    blocked_mac_record.has_mac_address = true;
    blocked_mac_record.mac_address[0] = 0xAA;
    blocked_mac_record.mac_address[1] = 0xBB;
    blocked_mac_record.mac_address[2] = 0xCC;
    blocked_mac_record.mac_address[3] = 0x30;
    blocked_mac_record.mac_address[4] = 0x03;
    blocked_mac_record.mac_address[5] = 0x03;
    if (local_registry.registerWirelessNodeAllowlistRecordWithResult(blocked_mac_record).result !=
        RegistryResult::OK) {
        return fail("espnow_service_path_blocked_mac_register_failed");
    }

    header.sequence_id = 804;
    header.node_id = 3003;
    value.value_float = 34.5F;
    header.checksum = calculateWirelessPacketChecksum(header, value, diagnostics);
    raw_offset = 0;
    header_bytes = reinterpret_cast<const uint8_t*>(&header);
    for (uint16_t i = 0; i < sizeof(WirelessPacketHeader); ++i) {
        raw_payload[raw_offset] = header_bytes[i];
        ++raw_offset;
    }
    value_bytes = reinterpret_cast<const uint8_t*>(&value);
    for (uint16_t i = 0; i < sizeof(WirelessCapabilityValue); ++i) {
        raw_payload[raw_offset] = value_bytes[i];
        ++raw_offset;
    }
    diagnostics_bytes = reinterpret_cast<const uint8_t*>(&diagnostics);
    for (uint16_t i = 0; i < sizeof(WirelessNodeDiagnostics); ++i) {
        raw_payload[raw_offset] = diagnostics_bytes[i];
        ++raw_offset;
    }
    if (!espnow_driver.injectRawPayloadForTest(
            blocked_mac_record.mac_address,
            raw_payload,
            static_cast<uint16_t>(EXPECTED_STRUCTURED_PACKET_SIZE))) {
        return fail("espnow_service_path_blocked_mac_inject_failed");
    }
    if (!espnow_driver.decodePendingRawPayload()) {
        return fail("espnow_service_path_blocked_mac_decode_failed");
    }
    if (local_service.processPackets(now_ms + 4)) {
        return fail("espnow_service_path_blocked_mac_succeeded");
    }
    if (!isSameText(local_service.lastErrorCode(), "wireless_node_blocked")) {
        return fail("espnow_service_path_blocked_mac_error_invalid");
    }
    if (local_registry.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            provider_out) != RegistryResult::OK) {
        return fail("espnow_service_path_blocked_mac_provider_missing");
    }
    if (provider_out.latest_payload.value_float != 31.5F ||
        provider_out.last_update_ms != now_ms + 1) {
        return fail("espnow_service_path_blocked_mac_changed_provider");
    }
    if (adapter.has_received_packet(adapter.context) ||
        espnow_driver.hasReceivedPacket() ||
        espnow_driver.hasRawPayload()) {
        return fail("espnow_service_path_blocked_mac_not_cleared");
    }

    WirelessNodeAllowlistRecord mismatch_mac_record;
    mismatch_mac_record.node_id = 5005;
    mismatch_mac_record.allow_state = WirelessNodeAllowState::ALLOWED;
    mismatch_mac_record.trust_state = WirelessTrustState::TRUSTED;
    mismatch_mac_record.added_at_ms = now_ms;
    mismatch_mac_record.last_seen_ms = now_ms;
    mismatch_mac_record.has_mac_address = true;
    mismatch_mac_record.mac_address[0] = 0xAA;
    mismatch_mac_record.mac_address[1] = 0xBB;
    mismatch_mac_record.mac_address[2] = 0xCC;
    mismatch_mac_record.mac_address[3] = 0x50;
    mismatch_mac_record.mac_address[4] = 0x05;
    mismatch_mac_record.mac_address[5] = 0x05;
    if (local_registry.registerWirelessNodeAllowlistRecordWithResult(mismatch_mac_record).result !=
        RegistryResult::OK) {
        return fail("espnow_service_path_mismatch_mac_register_failed");
    }

    header.sequence_id = 805;
    header.node_id = 1001;
    value.value_float = 35.5F;
    header.checksum = calculateWirelessPacketChecksum(header, value, diagnostics);
    raw_offset = 0;
    header_bytes = reinterpret_cast<const uint8_t*>(&header);
    for (uint16_t i = 0; i < sizeof(WirelessPacketHeader); ++i) {
        raw_payload[raw_offset] = header_bytes[i];
        ++raw_offset;
    }
    value_bytes = reinterpret_cast<const uint8_t*>(&value);
    for (uint16_t i = 0; i < sizeof(WirelessCapabilityValue); ++i) {
        raw_payload[raw_offset] = value_bytes[i];
        ++raw_offset;
    }
    diagnostics_bytes = reinterpret_cast<const uint8_t*>(&diagnostics);
    for (uint16_t i = 0; i < sizeof(WirelessNodeDiagnostics); ++i) {
        raw_payload[raw_offset] = diagnostics_bytes[i];
        ++raw_offset;
    }
    if (!espnow_driver.injectRawPayloadForTest(
            mismatch_mac_record.mac_address,
            raw_payload,
            static_cast<uint16_t>(EXPECTED_STRUCTURED_PACKET_SIZE))) {
        return fail("espnow_service_path_mismatch_mac_inject_failed");
    }
    if (!espnow_driver.decodePendingRawPayload()) {
        return fail("espnow_service_path_mismatch_mac_decode_failed");
    }
    if (local_service.processPackets(now_ms + 5)) {
        return fail("espnow_service_path_mismatch_mac_succeeded");
    }
    if (!isSameText(local_service.lastErrorCode(), "wireless_mac_node_mismatch")) {
        return fail("espnow_service_path_mismatch_mac_error_invalid");
    }
    if (local_registry.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            provider_out) != RegistryResult::OK) {
        return fail("espnow_service_path_mismatch_mac_provider_missing");
    }
    if (provider_out.latest_payload.value_float != 31.5F ||
        provider_out.last_update_ms != now_ms + 1) {
        return fail("espnow_service_path_mismatch_mac_changed_provider");
    }
    if (adapter.has_received_packet(adapter.context) ||
        espnow_driver.hasReceivedPacket() ||
        espnow_driver.hasRawPayload()) {
        return fail("espnow_service_path_mismatch_mac_not_cleared");
    }
    if (local_registry.getWirelessNodeSecurityDiagnostic(1001, security_out) !=
        RegistryResult::OK) {
        return fail("espnow_service_path_security_mismatch_mac_missing");
    }
    if (security_out.mac_node_mismatch_reject_count != 1 ||
        security_out.last_rejected_sequence_id != 805 ||
        security_out.last_seen_ms != now_ms + 5 ||
        !isSameText(security_out.last_error_code, "wireless_mac_node_mismatch")) {
        return fail("espnow_service_path_security_mismatch_mac_invalid");
    }

    header.sequence_id = 803;
    header.node_id = 2002;
    value.value_float = 36.5F;
    header.checksum = calculateWirelessPacketChecksum(header, value, diagnostics);
    raw_offset = 0;
    header_bytes = reinterpret_cast<const uint8_t*>(&header);
    for (uint16_t i = 0; i < sizeof(WirelessPacketHeader); ++i) {
        raw_payload[raw_offset] = header_bytes[i];
        ++raw_offset;
    }
    value_bytes = reinterpret_cast<const uint8_t*>(&value);
    for (uint16_t i = 0; i < sizeof(WirelessCapabilityValue); ++i) {
        raw_payload[raw_offset] = value_bytes[i];
        ++raw_offset;
    }
    diagnostics_bytes = reinterpret_cast<const uint8_t*>(&diagnostics);
    for (uint16_t i = 0; i < sizeof(WirelessNodeDiagnostics); ++i) {
        raw_payload[raw_offset] = diagnostics_bytes[i];
        ++raw_offset;
    }
    if (!espnow_driver.injectRawPayloadForTest(
            0,
            raw_payload,
            static_cast<uint16_t>(EXPECTED_STRUCTURED_PACKET_SIZE))) {
        return fail("espnow_service_path_unknown_node_inject_failed");
    }
    if (!espnow_driver.decodePendingRawPayload()) {
        return fail("espnow_service_path_unknown_node_decode_failed");
    }
    if (local_service.processPackets(now_ms + 6)) {
        return fail("espnow_service_path_unknown_node_succeeded");
    }
    if (!isSameText(local_service.lastErrorCode(), "wireless_node_not_allowed")) {
        return fail("espnow_service_path_unknown_node_error_invalid");
    }
    if (local_registry.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            provider_out) != RegistryResult::OK) {
        return fail("espnow_service_path_unknown_node_provider_missing");
    }
    if (provider_out.latest_payload.value_float != 31.5F ||
        provider_out.last_update_ms != now_ms + 1) {
        return fail("espnow_service_path_unknown_node_changed_provider");
    }
    if (adapter.has_received_packet(adapter.context) ||
        espnow_driver.hasReceivedPacket() ||
        espnow_driver.hasRawPayload()) {
        return fail("espnow_service_path_unknown_node_not_cleared");
    }

    header.sequence_id = 801;
    header.node_id = 1001;
    value.value_float = 37.5F;
    header.checksum = calculateWirelessPacketChecksum(header, value, diagnostics);
    raw_offset = 0;
    header_bytes = reinterpret_cast<const uint8_t*>(&header);
    for (uint16_t i = 0; i < sizeof(WirelessPacketHeader); ++i) {
        raw_payload[raw_offset] = header_bytes[i];
        ++raw_offset;
    }
    value_bytes = reinterpret_cast<const uint8_t*>(&value);
    for (uint16_t i = 0; i < sizeof(WirelessCapabilityValue); ++i) {
        raw_payload[raw_offset] = value_bytes[i];
        ++raw_offset;
    }
    diagnostics_bytes = reinterpret_cast<const uint8_t*>(&diagnostics);
    for (uint16_t i = 0; i < sizeof(WirelessNodeDiagnostics); ++i) {
        raw_payload[raw_offset] = diagnostics_bytes[i];
        ++raw_offset;
    }
    if (!espnow_driver.injectRawPayloadForTest(
            source_mac,
            raw_payload,
            static_cast<uint16_t>(EXPECTED_STRUCTURED_PACKET_SIZE))) {
        return fail("espnow_service_path_duplicate_inject_failed");
    }
    if (!espnow_driver.decodePendingRawPayload()) {
        return fail("espnow_service_path_duplicate_decode_failed");
    }
    if (local_service.processPackets(now_ms + 7)) {
        return fail("espnow_service_path_duplicate_succeeded");
    }
    if (!isSameText(local_service.lastErrorCode(), "wireless_duplicate_sequence")) {
        return fail("espnow_service_path_duplicate_error_invalid");
    }
    if (local_registry.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            provider_out) != RegistryResult::OK) {
        return fail("espnow_service_path_duplicate_provider_missing");
    }
    if (provider_out.latest_payload.value_float != 31.5F ||
        provider_out.last_update_ms != now_ms + 1) {
        return fail("espnow_service_path_duplicate_changed_provider");
    }
    if (adapter.has_received_packet(adapter.context) ||
        espnow_driver.hasReceivedPacket() ||
        espnow_driver.hasRawPayload()) {
        return fail("espnow_service_path_duplicate_not_cleared");
    }
    if (local_registry.getWirelessNodeSecurityDiagnostic(1001, security_out) !=
        RegistryResult::OK) {
        return fail("espnow_service_path_security_duplicate_missing");
    }
    if (security_out.duplicate_sequence_reject_count != 1 ||
        security_out.last_rejected_sequence_id != 801 ||
        security_out.last_seen_ms != now_ms + 7 ||
        !isSameText(security_out.last_error_code, "wireless_duplicate_sequence")) {
        return fail("espnow_service_path_security_duplicate_invalid");
    }

    WirelessNodeAllowlistRecord missing_diag_allowlist_record;
    missing_diag_allowlist_record.node_id = 7007;
    missing_diag_allowlist_record.allow_state = WirelessNodeAllowState::ALLOWED;
    missing_diag_allowlist_record.trust_state = WirelessTrustState::TRUSTED;
    missing_diag_allowlist_record.added_at_ms = now_ms;
    missing_diag_allowlist_record.last_seen_ms = now_ms;
    missing_diag_allowlist_record.has_mac_address = false;
    clearWirelessMacAddress(missing_diag_allowlist_record.mac_address);
    if (local_registry.registerWirelessNodeAllowlistRecordWithResult(
            missing_diag_allowlist_record).result != RegistryResult::OK) {
        return fail("espnow_service_path_missing_diag_allowlist_failed");
    }

    header.sequence_id = 900;
    header.node_id = 7007;
    value.value_float = 38.5F;
    header.checksum = calculateWirelessPacketChecksum(header, value, diagnostics);
    raw_offset = 0;
    header_bytes = reinterpret_cast<const uint8_t*>(&header);
    for (uint16_t i = 0; i < sizeof(WirelessPacketHeader); ++i) {
        raw_payload[raw_offset] = header_bytes[i];
        ++raw_offset;
    }
    value_bytes = reinterpret_cast<const uint8_t*>(&value);
    for (uint16_t i = 0; i < sizeof(WirelessCapabilityValue); ++i) {
        raw_payload[raw_offset] = value_bytes[i];
        ++raw_offset;
    }
    diagnostics_bytes = reinterpret_cast<const uint8_t*>(&diagnostics);
    for (uint16_t i = 0; i < sizeof(WirelessNodeDiagnostics); ++i) {
        raw_payload[raw_offset] = diagnostics_bytes[i];
        ++raw_offset;
    }
    if (!espnow_driver.injectRawPayloadForTest(
            0,
            raw_payload,
            static_cast<uint16_t>(EXPECTED_STRUCTURED_PACKET_SIZE))) {
        return fail("espnow_service_path_missing_diag_inject_failed");
    }
    if (!espnow_driver.decodePendingRawPayload()) {
        return fail("espnow_service_path_missing_diag_decode_failed");
    }
    if (!local_service.processPackets(now_ms + 8)) {
        return fail(local_service.lastErrorCode());
    }
    if (local_registry.getCapabilityProvider(
            WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID,
            provider_out) != RegistryResult::OK) {
        return fail("espnow_service_path_missing_diag_provider_missing");
    }
    if (provider_out.latest_payload.value_float != 38.5F ||
        provider_out.last_update_ms != now_ms + 8) {
        return fail("espnow_service_path_missing_diag_provider_invalid");
    }
    if (local_registry.getWirelessNodeSecurityDiagnostic(7007, security_out) !=
        RegistryResult::NOT_FOUND) {
        return fail("espnow_service_path_missing_diag_created_unexpectedly");
    }
    if (adapter.has_received_packet(adapter.context) ||
        espnow_driver.hasReceivedPacket() ||
        espnow_driver.hasRawPayload()) {
        return fail("espnow_service_path_missing_diag_not_cleared");
    }

    CapabilityPayload canonical_after;
    if (!local_registry.getCapabilityPayload(CAP_TEMPERATURE, canonical_after)) {
        return fail("espnow_service_path_canonical_missing");
    }
    if (canonical_after.value_float != 22.4F ||
        canonical_after.timestamp_ms != now_ms ||
        canonical_after.stale != StaleState::FRESH) {
        return fail("espnow_service_path_canonical_changed");
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

    if (registry_.wirelessNodeSecurityDiagnosticCount() != 0) {
        return fail("wireless_security_diagnostic_count_initial_invalid");
    }

    WirelessNodeSecurityDiagnosticRecord security_record;
    security_record.node_id = 1001;
    security_record.has_mac_address = true;
    security_record.mac_address[0] = 0xAA;
    security_record.mac_address[1] = 0xBB;
    security_record.mac_address[2] = 0xCC;
    security_record.mac_address[3] = 0x01;
    security_record.mac_address[4] = 0x02;
    security_record.mac_address[5] = 0x03;
    security_record.allow_state = WirelessNodeAllowState::ALLOWED;
    security_record.trust_state = WirelessTrustState::TRUSTED;
    security_record.last_seen_ms = now_ms;
    security_record.last_accepted_sequence_id = 0;
    security_record.last_rejected_sequence_id = 0;
    security_record.last_error_code = "none";
    security_record.checksum_reject_count = 0;
    security_record.mac_not_allowed_reject_count = 0;
    security_record.mac_node_mismatch_reject_count = 0;
    security_record.blocked_reject_count = 0;
    security_record.not_allowed_reject_count = 0;
    security_record.untrusted_reject_count = 0;
    security_record.duplicate_sequence_reject_count = 0;
    security_record.invalid_packet_reject_count = 0;
    security_record.accepted_packet_count = 0;

    RegistryWriteResult security_result =
        registry_.registerWirelessNodeSecurityDiagnosticWithResult(security_record);
    if (security_result.result != RegistryResult::OK) {
        return fail("wireless_security_diagnostic_register_result_invalid");
    }
    if (security_result.index != 0) {
        return fail("wireless_security_diagnostic_register_index_invalid");
    }
    if (registry_.wirelessNodeSecurityDiagnosticCount() != 1) {
        return fail("wireless_security_diagnostic_count_after_register_invalid");
    }

    WirelessNodeSecurityDiagnosticRecord security_out;
    if (registry_.getWirelessNodeSecurityDiagnostic(1001, security_out) != RegistryResult::OK) {
        return fail("wireless_security_diagnostic_get_result_invalid");
    }
    if (security_out.node_id != 1001 ||
        !security_out.has_mac_address ||
        !wirelessMacAddressEquals(security_out.mac_address, security_record.mac_address) ||
        security_out.allow_state != WirelessNodeAllowState::ALLOWED ||
        security_out.trust_state != WirelessTrustState::TRUSTED ||
        security_out.last_seen_ms != now_ms ||
        !isSameText(security_out.last_error_code, "none") ||
        security_out.accepted_packet_count != 0 ||
        security_out.checksum_reject_count != 0 ||
        security_out.mac_not_allowed_reject_count != 0 ||
        security_out.mac_node_mismatch_reject_count != 0 ||
        security_out.blocked_reject_count != 0 ||
        security_out.not_allowed_reject_count != 0 ||
        security_out.untrusted_reject_count != 0 ||
        security_out.duplicate_sequence_reject_count != 0 ||
        security_out.invalid_packet_reject_count != 0) {
        return fail("wireless_security_diagnostic_get_fields_invalid");
    }
    if (registry_.getWirelessNodeSecurityDiagnosticByIndex(0, security_out) !=
        RegistryResult::OK) {
        return fail("wireless_security_diagnostic_get_index_result_invalid");
    }
    if (security_out.node_id != 1001) {
        return fail("wireless_security_diagnostic_get_index_node_invalid");
    }
    if (registry_.registerWirelessNodeSecurityDiagnosticWithResult(security_record).result !=
        RegistryResult::DUPLICATE_ID) {
        return fail("wireless_security_diagnostic_duplicate_result_invalid");
    }
    if (registry_.wirelessNodeSecurityDiagnosticCount() != 1) {
        return fail("wireless_security_diagnostic_count_after_duplicate_invalid");
    }
    if (registry_.getWirelessNodeSecurityDiagnostic(9999, security_out) !=
        RegistryResult::NOT_FOUND) {
        return fail("wireless_security_diagnostic_missing_result_invalid");
    }
    WirelessNodeSecurityDiagnosticRecord invalid_security_record = security_record;
    invalid_security_record.node_id = 0;
    if (registry_.registerWirelessNodeSecurityDiagnosticWithResult(invalid_security_record).result !=
        RegistryResult::INVALID_ID) {
        return fail("wireless_security_diagnostic_invalid_register_result_invalid");
    }
    if (registry_.getWirelessNodeSecurityDiagnostic(0, security_out) !=
        RegistryResult::INVALID_ID) {
        return fail("wireless_security_diagnostic_invalid_get_result_invalid");
    }
    if (registry_.getWirelessNodeSecurityDiagnosticByIndex(1, security_out) !=
        RegistryResult::NOT_FOUND) {
        return fail("wireless_security_diagnostic_invalid_index_result_invalid");
    }
    if (incrementSaturatingUint16(0) != 1 ||
        incrementSaturatingUint16(65534U) != 65535U ||
        incrementSaturatingUint16(65535U) != 65535U) {
        return fail("wireless_security_diagnostic_saturating_increment_invalid");
    }

    uint8_t updated_mac[WIRELESS_MAC_ADDRESS_SIZE] = {0xAA, 0xBB, 0xCC, 0x10, 0x20, 0x30};
    if (registry_.updateWirelessNodeSecurityAccepted(
            1001,
            updated_mac,
            true,
            101,
            now_ms + 1) != RegistryResult::OK) {
        return fail("wireless_security_diagnostic_accept_update_failed");
    }
    if (registry_.getWirelessNodeSecurityDiagnostic(1001, security_out) != RegistryResult::OK) {
        return fail("wireless_security_diagnostic_accept_get_failed");
    }
    if (security_out.last_seen_ms != now_ms + 1 ||
        security_out.last_accepted_sequence_id != 101 ||
        !isSameText(security_out.last_error_code, "none") ||
        security_out.accepted_packet_count != 1 ||
        !security_out.has_mac_address ||
        !wirelessMacAddressEquals(security_out.mac_address, updated_mac)) {
        return fail("wireless_security_diagnostic_accept_fields_invalid");
    }

    if (registry_.updateWirelessNodeSecurityRejected(
            1001,
            updated_mac,
            true,
            201,
            "wireless_checksum_invalid",
            WirelessSecurityRejectReason::CHECKSUM_INVALID,
            now_ms + 2) != RegistryResult::OK) {
        return fail("wireless_security_diagnostic_checksum_update_failed");
    }
    if (registry_.getWirelessNodeSecurityDiagnostic(1001, security_out) != RegistryResult::OK ||
        security_out.checksum_reject_count != 1 ||
        security_out.last_rejected_sequence_id != 201 ||
        !isSameText(security_out.last_error_code, "wireless_checksum_invalid")) {
        return fail("wireless_security_diagnostic_checksum_fields_invalid");
    }

    if (registry_.updateWirelessNodeSecurityRejected(
            1001,
            updated_mac,
            true,
            202,
            "wireless_mac_not_allowed",
            WirelessSecurityRejectReason::MAC_NOT_ALLOWED,
            now_ms + 3) != RegistryResult::OK) {
        return fail("wireless_security_diagnostic_mac_update_failed");
    }
    if (registry_.getWirelessNodeSecurityDiagnostic(1001, security_out) != RegistryResult::OK ||
        security_out.mac_not_allowed_reject_count != 1 ||
        security_out.last_rejected_sequence_id != 202 ||
        !isSameText(security_out.last_error_code, "wireless_mac_not_allowed")) {
        return fail("wireless_security_diagnostic_mac_fields_invalid");
    }

    if (registry_.updateWirelessNodeSecurityRejected(
            1001,
            updated_mac,
            true,
            203,
            "wireless_mac_node_mismatch",
            WirelessSecurityRejectReason::MAC_NODE_MISMATCH,
            now_ms + 4) != RegistryResult::OK) {
        return fail("wireless_security_diagnostic_mismatch_update_failed");
    }
    if (registry_.getWirelessNodeSecurityDiagnostic(1001, security_out) != RegistryResult::OK ||
        security_out.mac_node_mismatch_reject_count != 1 ||
        security_out.last_rejected_sequence_id != 203 ||
        !isSameText(security_out.last_error_code, "wireless_mac_node_mismatch")) {
        return fail("wireless_security_diagnostic_mismatch_fields_invalid");
    }

    if (registry_.updateWirelessNodeSecurityRejected(
            1001,
            updated_mac,
            true,
            204,
            "wireless_node_blocked",
            WirelessSecurityRejectReason::NODE_BLOCKED,
            now_ms + 5) != RegistryResult::OK) {
        return fail("wireless_security_diagnostic_blocked_update_failed");
    }
    if (registry_.getWirelessNodeSecurityDiagnostic(1001, security_out) != RegistryResult::OK ||
        security_out.blocked_reject_count != 1 ||
        security_out.last_rejected_sequence_id != 204 ||
        !isSameText(security_out.last_error_code, "wireless_node_blocked")) {
        return fail("wireless_security_diagnostic_blocked_fields_invalid");
    }

    if (registry_.updateWirelessNodeSecurityRejected(
            1001,
            updated_mac,
            true,
            205,
            "wireless_node_not_allowed",
            WirelessSecurityRejectReason::NODE_NOT_ALLOWED,
            now_ms + 6) != RegistryResult::OK) {
        return fail("wireless_security_diagnostic_not_allowed_update_failed");
    }
    if (registry_.getWirelessNodeSecurityDiagnostic(1001, security_out) != RegistryResult::OK ||
        security_out.not_allowed_reject_count != 1 ||
        security_out.last_rejected_sequence_id != 205 ||
        !isSameText(security_out.last_error_code, "wireless_node_not_allowed")) {
        return fail("wireless_security_diagnostic_not_allowed_fields_invalid");
    }

    if (registry_.updateWirelessNodeSecurityRejected(
            1001,
            updated_mac,
            true,
            206,
            "wireless_untrusted",
            WirelessSecurityRejectReason::UNTRUSTED,
            now_ms + 7) != RegistryResult::OK) {
        return fail("wireless_security_diagnostic_untrusted_update_failed");
    }
    if (registry_.getWirelessNodeSecurityDiagnostic(1001, security_out) != RegistryResult::OK ||
        security_out.untrusted_reject_count != 1 ||
        security_out.last_rejected_sequence_id != 206 ||
        !isSameText(security_out.last_error_code, "wireless_untrusted")) {
        return fail("wireless_security_diagnostic_untrusted_fields_invalid");
    }

    if (registry_.updateWirelessNodeSecurityRejected(
            1001,
            updated_mac,
            true,
            207,
            "wireless_duplicate_sequence",
            WirelessSecurityRejectReason::DUPLICATE_SEQUENCE,
            now_ms + 8) != RegistryResult::OK) {
        return fail("wireless_security_diagnostic_duplicate_update_failed");
    }
    if (registry_.getWirelessNodeSecurityDiagnostic(1001, security_out) != RegistryResult::OK ||
        security_out.duplicate_sequence_reject_count != 1 ||
        security_out.last_rejected_sequence_id != 207 ||
        !isSameText(security_out.last_error_code, "wireless_duplicate_sequence")) {
        return fail("wireless_security_diagnostic_duplicate_fields_invalid");
    }

    if (registry_.updateWirelessNodeSecurityRejected(
            1001,
            updated_mac,
            true,
            208,
            "device_update_failed",
            WirelessSecurityRejectReason::INVALID_PACKET,
            now_ms + 9) != RegistryResult::OK) {
        return fail("wireless_security_diagnostic_invalid_packet_update_failed");
    }
    if (registry_.getWirelessNodeSecurityDiagnostic(1001, security_out) != RegistryResult::OK ||
        security_out.invalid_packet_reject_count != 1 ||
        security_out.last_rejected_sequence_id != 208 ||
        !isSameText(security_out.last_error_code, "device_update_failed")) {
        return fail("wireless_security_diagnostic_invalid_packet_fields_invalid");
    }

    if (registry_.updateWirelessNodeSecurityAccepted(
            9999,
            updated_mac,
            true,
            301,
            now_ms + 10) != RegistryResult::NOT_FOUND) {
        return fail("wireless_security_diagnostic_missing_accept_invalid");
    }
    if (registry_.updateWirelessNodeSecurityRejected(
            9999,
            updated_mac,
            true,
            302,
            "wireless_checksum_invalid",
            WirelessSecurityRejectReason::CHECKSUM_INVALID,
            now_ms + 11) != RegistryResult::NOT_FOUND) {
        return fail("wireless_security_diagnostic_missing_reject_invalid");
    }
    if (registry_.updateWirelessNodeSecurityAccepted(
            0,
            updated_mac,
            true,
            303,
            now_ms + 12) != RegistryResult::INVALID_ID) {
        return fail("wireless_security_diagnostic_invalid_accept_invalid");
    }
    if (registry_.updateWirelessNodeSecurityRejected(
            0,
            updated_mac,
            true,
            304,
            "wireless_checksum_invalid",
            WirelessSecurityRejectReason::CHECKSUM_INVALID,
            now_ms + 13) != RegistryResult::INVALID_ID) {
        return fail("wireless_security_diagnostic_invalid_reject_invalid");
    }

    WirelessNodeSecurityDiagnosticRecord saturation_record = security_record;
    saturation_record.node_id = 2002;
    saturation_record.checksum_reject_count = 65535U;
    RegistryWriteResult saturation_result =
        registry_.registerWirelessNodeSecurityDiagnosticWithResult(saturation_record);
    if (saturation_result.result != RegistryResult::OK) {
        return fail("wireless_security_diagnostic_saturation_register_failed");
    }
    if (registry_.updateWirelessNodeSecurityRejected(
            2002,
            updated_mac,
            true,
            401,
            "wireless_checksum_invalid",
            WirelessSecurityRejectReason::CHECKSUM_INVALID,
            now_ms + 14) != RegistryResult::OK) {
        return fail("wireless_security_diagnostic_saturation_update_failed");
    }
    if (registry_.getWirelessNodeSecurityDiagnostic(2002, security_out) != RegistryResult::OK ||
        security_out.checksum_reject_count != 65535U) {
        return fail("wireless_security_diagnostic_saturation_fields_invalid");
    }

    CapabilityPayload security_api_canonical_before;
    if (!registry_.getCapabilityPayload(CAP_TEMPERATURE, security_api_canonical_before)) {
        return fail("wireless_security_api_canonical_before_missing");
    }
    const uint8_t security_api_provider_count_before = registry_.capabilityProviderCount();

    ApiWirelessSecurityDiagnostic security_api_out;
    if (!api_.getWirelessSecurityDiagnostic(1001, security_api_out)) {
        return fail("wireless_security_api_get_failed");
    }
    if (!security_api_out.ok ||
        security_api_out.registry_result != RegistryResult::OK ||
        security_api_out.node_id != 1001 ||
        !security_api_out.has_mac_address ||
        !wirelessMacAddressEquals(security_api_out.mac_address, updated_mac) ||
        security_api_out.allow_state != WirelessNodeAllowState::ALLOWED ||
        security_api_out.trust_state != WirelessTrustState::TRUSTED ||
        security_api_out.last_seen_ms != now_ms + 9 ||
        security_api_out.last_accepted_sequence_id != 101 ||
        security_api_out.last_rejected_sequence_id != 208 ||
        !isSameText(security_api_out.last_error_code, "device_update_failed") ||
        security_api_out.accepted_packet_count != 1 ||
        security_api_out.checksum_reject_count != 1 ||
        security_api_out.mac_not_allowed_reject_count != 1 ||
        security_api_out.mac_node_mismatch_reject_count != 1 ||
        security_api_out.blocked_reject_count != 1 ||
        security_api_out.not_allowed_reject_count != 1 ||
        security_api_out.untrusted_reject_count != 1 ||
        security_api_out.duplicate_sequence_reject_count != 1 ||
        security_api_out.invalid_packet_reject_count != 1 ||
        !isSameText(security_api_out.error_code, "none")) {
        return fail("wireless_security_api_get_fields_invalid");
    }

    ApiWirelessSecurityDiagnostic security_api_second_read;
    if (!api_.getWirelessSecurityDiagnostic(1001, security_api_second_read)) {
        return fail("wireless_security_api_second_get_failed");
    }
    if (security_api_second_read.accepted_packet_count != security_api_out.accepted_packet_count ||
        security_api_second_read.checksum_reject_count != security_api_out.checksum_reject_count ||
        security_api_second_read.duplicate_sequence_reject_count !=
            security_api_out.duplicate_sequence_reject_count ||
        security_api_second_read.last_rejected_sequence_id !=
            security_api_out.last_rejected_sequence_id) {
        return fail("wireless_security_api_read_mutated_counters");
    }

    if (!api_.getWirelessSecurityDiagnosticByIndex(0, security_api_out)) {
        return fail("wireless_security_api_index_get_failed");
    }
    if (!security_api_out.ok ||
        security_api_out.registry_result != RegistryResult::OK ||
        security_api_out.node_id != 1001) {
        return fail("wireless_security_api_index_fields_invalid");
    }

    if (api_.getWirelessSecurityDiagnostic(9999, security_api_out)) {
        return fail("wireless_security_api_missing_succeeded");
    }
    if (security_api_out.registry_result != RegistryResult::NOT_FOUND) {
        return fail("wireless_security_api_missing_result_invalid");
    }
    if (api_.getWirelessSecurityDiagnostic(0, security_api_out)) {
        return fail("wireless_security_api_invalid_succeeded");
    }
    if (security_api_out.registry_result != RegistryResult::INVALID_ID) {
        return fail("wireless_security_api_invalid_result_invalid");
    }
    if (api_.getWirelessSecurityDiagnosticByIndex(99, security_api_out)) {
        return fail("wireless_security_api_index_missing_succeeded");
    }
    if (security_api_out.registry_result != RegistryResult::NOT_FOUND) {
        return fail("wireless_security_api_index_missing_result_invalid");
    }

    ApiWirelessSecuritySummary security_summary;
    if (!api_.getWirelessSecuritySummary(security_summary)) {
        return fail("wireless_security_api_summary_failed");
    }
    if (!security_summary.ok ||
        security_summary.registry_result != RegistryResult::OK ||
        security_summary.node_count != registry_.wirelessNodeSecurityDiagnosticCount() ||
        security_summary.total_accepted_packets != 1 ||
        security_summary.total_rejected_packets != 65543UL ||
        security_summary.total_checksum_rejects != 65536UL ||
        security_summary.total_mac_rejects != 2 ||
        security_summary.total_duplicate_rejects != 1 ||
        !isSameText(security_summary.error_code, "none")) {
        return fail("wireless_security_api_summary_fields_invalid");
    }

    CapabilityPayload security_api_canonical_after;
    if (!registry_.getCapabilityPayload(CAP_TEMPERATURE, security_api_canonical_after)) {
        return fail("wireless_security_api_canonical_after_missing");
    }
    if (security_api_canonical_after.value_float != security_api_canonical_before.value_float ||
        security_api_canonical_after.timestamp_ms != security_api_canonical_before.timestamp_ms ||
        registry_.capabilityProviderCount() != security_api_provider_count_before) {
        return fail("wireless_security_api_read_changed_state");
    }

    ApiCapabilityState security_api_temperature_state;
    if (!api_.getTemperatureState(security_api_temperature_state)) {
        return fail("wireless_security_api_temperature_failed");
    }
    if (security_api_temperature_state.payload.value_float !=
        security_api_canonical_before.value_float) {
        return fail("wireless_security_api_temperature_changed");
    }

    {
        WirelessPacketHeader test_header;
        test_header.magic = WIRELESS_PACKET_MAGIC;
        test_header.protocol_version = WIRELESS_PROTOCOL_VERSION;
        test_header.packet_type = WirelessPacketType::CAPABILITY_VALUE;
        test_header.flags = 0;
        test_header.sequence_id = 700;
        test_header.node_id = 1001;
        test_header.payload_length = 0;

        WirelessCapabilityValue test_value;
        copyWirelessCapabilityId(test_value.capability_id, CAP_TEMPERATURE);
        test_value.payload_type = WirelessPayloadType::FLOAT;
        test_value.value_float = 21.5F;
        test_value.value_int = 0;
        copyWirelessCapabilityId(test_value.error_code, "none");

        WirelessNodeDiagnostics test_diagnostics;
        test_diagnostics.battery_present = true;
        test_diagnostics.battery_level_percent = 80.0F;
        test_diagnostics.battery_voltage = 3.7F;
        test_diagnostics.signal_quality_percent = 70.0F;

        test_header.checksum =
            calculateWirelessPacketChecksum(test_header, test_value, test_diagnostics);
        if (!wireless_transport_driver_.injectReceivedCapabilityValue(
                test_header,
                test_value,
                test_diagnostics)) {
            return fail("wireless_transport_no_mac_inject_failed");
        }

        WirelessPacketHeader read_header;
        WirelessCapabilityValue read_value;
        WirelessNodeDiagnostics read_diagnostics;
        if (!wireless_transport_driver_.readReceivedPacket(
                read_header,
                read_value,
                read_diagnostics)) {
            return fail("wireless_transport_no_mac_read_failed");
        }
        if (read_header.sequence_id != test_header.sequence_id ||
            read_header.node_id != test_header.node_id ||
            read_value.value_float != test_value.value_float ||
            read_diagnostics.signal_quality_percent != test_diagnostics.signal_quality_percent) {
            return fail("wireless_transport_no_mac_read_invalid");
        }

        test_header.sequence_id = 701;
        test_header.checksum =
            calculateWirelessPacketChecksum(test_header, test_value, test_diagnostics);
        if (!wireless_transport_driver_.injectReceivedCapabilityValue(
                test_header,
                test_value,
                test_diagnostics)) {
            return fail("wireless_transport_overload_no_mac_inject_failed");
        }

        uint8_t source_mac[WIRELESS_MAC_ADDRESS_SIZE];
        uint8_t empty_mac[WIRELESS_MAC_ADDRESS_SIZE];
        clearWirelessMacAddress(source_mac);
        clearWirelessMacAddress(empty_mac);
        bool has_source_mac = true;
        if (!wireless_transport_driver_.readReceivedPacket(
                read_header,
                read_value,
                read_diagnostics,
                source_mac,
                has_source_mac)) {
            return fail("wireless_transport_overload_no_mac_read_failed");
        }
        if (has_source_mac) {
            return fail("wireless_transport_overload_no_mac_flag_invalid");
        }
        if (!wirelessMacAddressEquals(source_mac, empty_mac)) {
            return fail("wireless_transport_overload_no_mac_not_cleared");
        }

        uint8_t expected_mac[WIRELESS_MAC_ADDRESS_SIZE] = {0xAA, 0xBB, 0xCC, 0x11, 0x22, 0x33};
        test_header.sequence_id = 702;
        test_value.value_float = 22.5F;
        test_header.checksum =
            calculateWirelessPacketChecksum(test_header, test_value, test_diagnostics);
        if (!wireless_transport_driver_.injectReceivedCapabilityValueWithMac(
                expected_mac,
                test_header,
                test_value,
                test_diagnostics)) {
            return fail("wireless_transport_mac_inject_failed");
        }
        clearWirelessMacAddress(source_mac);
        has_source_mac = false;
        if (!wireless_transport_driver_.readReceivedPacket(
                read_header,
                read_value,
                read_diagnostics,
                source_mac,
                has_source_mac)) {
            return fail("wireless_transport_mac_read_failed");
        }
        if (!has_source_mac) {
            return fail("wireless_transport_mac_flag_invalid");
        }
        if (!wirelessMacAddressEquals(source_mac, expected_mac)) {
            return fail("wireless_transport_mac_value_invalid");
        }
        if (read_header.sequence_id != test_header.sequence_id ||
            read_value.value_float != test_value.value_float) {
            return fail("wireless_transport_mac_packet_invalid");
        }

        test_header.sequence_id = 703;
        test_header.checksum =
            calculateWirelessPacketChecksum(test_header, test_value, test_diagnostics);
        if (!wireless_transport_driver_.injectReceivedCapabilityValueWithMac(
                expected_mac,
                test_header,
                test_value,
                test_diagnostics)) {
            return fail("wireless_transport_clear_mac_inject_failed");
        }
        wireless_transport_driver_.clearReceivedPacket();
        test_header.sequence_id = 704;
        test_header.checksum =
            calculateWirelessPacketChecksum(test_header, test_value, test_diagnostics);
        if (!wireless_transport_driver_.injectReceivedCapabilityValue(
                test_header,
                test_value,
                test_diagnostics)) {
            return fail("wireless_transport_clear_no_mac_inject_failed");
        }
        clearWirelessMacAddress(source_mac);
        has_source_mac = true;
        if (!wireless_transport_driver_.readReceivedPacket(
                read_header,
                read_value,
                read_diagnostics,
                source_mac,
                has_source_mac)) {
            return fail("wireless_transport_clear_no_mac_read_failed");
        }
        if (has_source_mac || !wirelessMacAddressEquals(source_mac, empty_mac)) {
            return fail("wireless_transport_clear_mac_not_reset");
        }

        test_header.sequence_id = 705;
        test_header.checksum =
            calculateWirelessPacketChecksum(test_header, test_value, test_diagnostics);
        if (!wireless_transport_driver_.injectReceivedCapabilityValueWithMac(
                expected_mac,
                test_header,
                test_value,
                test_diagnostics)) {
            return fail("wireless_transport_begin_mac_inject_failed");
        }
        wireless_transport_driver_.begin();
        test_header.sequence_id = 706;
        test_header.checksum =
            calculateWirelessPacketChecksum(test_header, test_value, test_diagnostics);
        if (!wireless_transport_driver_.injectReceivedCapabilityValue(
                test_header,
                test_value,
                test_diagnostics)) {
            return fail("wireless_transport_begin_no_mac_inject_failed");
        }
        clearWirelessMacAddress(source_mac);
        has_source_mac = true;
        if (!wireless_transport_driver_.readReceivedPacket(
                read_header,
                read_value,
                read_diagnostics,
                source_mac,
                has_source_mac)) {
            return fail("wireless_transport_begin_no_mac_read_failed");
        }
        if (has_source_mac || !wirelessMacAddressEquals(source_mac, empty_mac)) {
            return fail("wireless_transport_begin_mac_not_reset");
        }
    }

    WirelessNodeAllowlistRecord mac_allowlist_record = allowlist_record;
    mac_allowlist_record.node_id = 5005;
    mac_allowlist_record.has_mac_address = true;
    mac_allowlist_record.mac_address[0] = 0xAA;
    mac_allowlist_record.mac_address[1] = 0xBB;
    mac_allowlist_record.mac_address[2] = 0xCC;
    mac_allowlist_record.mac_address[3] = 0x01;
    mac_allowlist_record.mac_address[4] = 0x02;
    mac_allowlist_record.mac_address[5] = 0x03;
    if (registry_.registerWirelessNodeAllowlistRecordWithResult(mac_allowlist_record).result !=
        RegistryResult::OK) {
        return fail("wireless_mac_register_result_invalid");
    }

    uint8_t valid_mac[WIRELESS_MAC_ADDRESS_SIZE] = {0xAA, 0xBB, 0xCC, 0x01, 0x02, 0x03};
    if (registry_.getWirelessNodeAllowlistRecordByMac(valid_mac, allowlist_out) !=
        RegistryResult::OK) {
        return fail("wireless_mac_lookup_result_invalid");
    }
    if (allowlist_out.node_id != 5005) {
        return fail("wireless_mac_lookup_node_invalid");
    }
    if (!allowlist_out.has_mac_address) {
        return fail("wireless_mac_lookup_flag_invalid");
    }

    uint8_t missing_mac[WIRELESS_MAC_ADDRESS_SIZE] = {0xAA, 0xBB, 0xCC, 0x09, 0x08, 0x07};
    if (registry_.getWirelessNodeAllowlistRecordByMac(missing_mac, allowlist_out) !=
        RegistryResult::NOT_FOUND) {
        return fail("wireless_mac_missing_result_invalid");
    }
    if (registry_.getWirelessNodeAllowlistRecordByMac(0, allowlist_out) !=
        RegistryResult::INVALID_ID) {
        return fail("wireless_mac_null_result_invalid");
    }

    WirelessNodeAllowlistRecord no_mac_allowlist_record = allowlist_record;
    no_mac_allowlist_record.node_id = 6006;
    no_mac_allowlist_record.has_mac_address = false;
    no_mac_allowlist_record.mac_address[0] = 0xAA;
    no_mac_allowlist_record.mac_address[1] = 0xBB;
    no_mac_allowlist_record.mac_address[2] = 0xCC;
    no_mac_allowlist_record.mac_address[3] = 0x04;
    no_mac_allowlist_record.mac_address[4] = 0x05;
    no_mac_allowlist_record.mac_address[5] = 0x06;
    if (registry_.registerWirelessNodeAllowlistRecordWithResult(no_mac_allowlist_record).result !=
        RegistryResult::OK) {
        return fail("wireless_mac_no_mac_register_result_invalid");
    }
    uint8_t ignored_mac[WIRELESS_MAC_ADDRESS_SIZE] = {0xAA, 0xBB, 0xCC, 0x04, 0x05, 0x06};
    if (registry_.getWirelessNodeAllowlistRecordByMac(ignored_mac, allowlist_out) !=
        RegistryResult::NOT_FOUND) {
        return fail("wireless_mac_no_mac_ignored_invalid");
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
    if (node_summary.allowed_count != 4) {
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

bool VerticalSliceValidation::validatePublicOwnerTypeDefaults() {
    if (static_cast<unsigned char>(PublicLifecycleState::NONE) != 0 ||
        static_cast<unsigned char>(PublicTrustState::UNKNOWN) != 0 ||
        static_cast<unsigned char>(PublicAvailabilityState::UNKNOWN) != 0 ||
        static_cast<unsigned char>(PublicFreshnessState::UNKNOWN) != 0 ||
        static_cast<unsigned char>(PublicHealthState::UNKNOWN) != 0) {
        return fail("public_owner_enum_defaults_invalid");
    }

    PublicNodeRecord node;
    if (node.valid || node.node_id != 0 || node.source_type != 0 ||
        node.lifecycle_state != PublicLifecycleState::NONE ||
        node.visibility_state != PublicVisibilityState::NONE ||
        node.trust_state != PublicTrustState::UNKNOWN ||
        node.capability_count != 0 ||
        node.freshness_state != PublicFreshnessState::UNKNOWN ||
        node.diagnostics_available || node.last_seen_ms != 0) {
        return fail("public_node_default_invalid");
    }
    node.node_id = 42;
    node.valid = true;
    node.reset();
    if (node.valid || node.node_id != 0) {
        return fail("public_node_reset_invalid");
    }

    PublicCapabilityRecord capability;
    if (capability.valid || capability.capability_id != 0 ||
        capability.capability_instance_id != 0 || capability.owner_node_id != 0 ||
        capability.category != 0 ||
        capability.lifecycle_state != PublicLifecycleState::NONE ||
        capability.visibility_state != PublicVisibilityState::NONE ||
        capability.value_availability_state != PublicAvailabilityState::UNKNOWN ||
        capability.freshness_state != PublicFreshnessState::UNKNOWN ||
        capability.provider_available || capability.diagnostics_available) {
        return fail("public_capability_default_invalid");
    }

    PublicNodeCapabilityLink link;
    if (link.active || link.link_id != 0 || link.node_id != 0 ||
        link.capability_instance_id != 0 ||
        link.link_visibility_state != PublicVisibilityState::NONE ||
        link.link_freshness_state != PublicFreshnessState::UNKNOWN ||
        link.diagnostics_available || link.display_order != 0) {
        return fail("public_node_capability_link_default_invalid");
    }

    PublicCapabilityValueSnapshot value;
    if (value.capability_instance_id != 0 || value.value_type != PayloadValueType::NONE ||
        value.value_valid || value.value_available || value.value_stale ||
        value.unavailable_reason != PublicUnavailableReason::NO_OWNER ||
        value.numeric_value != 0.0F || value.bool_value || value.last_update_ms != 0 ||
        value.unit_metadata_ref != 0 || value.quality_ref != 0) {
        return fail("public_capability_value_default_invalid");
    }

    PublicNodeDiagnosticsSnapshot node_diagnostics;
    if (node_diagnostics.node_id != 0 || node_diagnostics.diagnostics_available ||
        node_diagnostics.health_known || node_diagnostics.health_ok ||
        node_diagnostics.stale || node_diagnostics.last_update_ms != 0 ||
        node_diagnostics.accepted_count != 0 || node_diagnostics.rejected_count != 0 ||
        node_diagnostics.unavailable_reason != PublicUnavailableReason::NO_OWNER) {
        return fail("public_node_diagnostics_default_invalid");
    }

    PublicCapabilityDiagnosticsSnapshot capability_diagnostics;
    if (capability_diagnostics.capability_instance_id != 0 ||
        capability_diagnostics.diagnostics_available ||
        capability_diagnostics.health_known || capability_diagnostics.health_ok ||
        capability_diagnostics.stale || capability_diagnostics.last_update_ms != 0 ||
        capability_diagnostics.accepted_count != 0 ||
        capability_diagnostics.rejected_count != 0 ||
        capability_diagnostics.unavailable_reason != PublicUnavailableReason::NO_OWNER) {
        return fail("public_capability_diagnostics_default_invalid");
    }

    PublicProviderDiagnosticsSnapshot provider_diagnostics;
    if (provider_diagnostics.provider_id != 0 ||
        provider_diagnostics.diagnostics_available ||
        provider_diagnostics.health_known || provider_diagnostics.health_ok ||
        provider_diagnostics.stale || provider_diagnostics.last_update_ms != 0 ||
        provider_diagnostics.accepted_count != 0 ||
        provider_diagnostics.rejected_count != 0 ||
        provider_diagnostics.unavailable_reason != PublicUnavailableReason::NO_OWNER) {
        return fail("public_provider_diagnostics_default_invalid");
    }

    return true;
}

bool VerticalSliceValidation::validateNodeDirectoryEmptySkeleton() {
    NodeDirectory directory;
    if (directory.count() != 0) {
        return fail("node_directory_default_count_invalid");
    }
    if (!directory.isEmpty()) {
        return fail("node_directory_default_not_empty");
    }
    if (directory.capacity() == 0 ||
        directory.capacity() != NODE_DIRECTORY_MAX_PUBLIC_NODES) {
        return fail("node_directory_capacity_invalid");
    }

    PublicNodeRecord out_record;
    out_record.node_id = 77;
    out_record.valid = true;
    out_record.lifecycle_state = PublicLifecycleState::AVAILABLE;
    if (directory.readByIndex(0, out_record)) {
        return fail("node_directory_empty_read_succeeded");
    }
    if (out_record.valid || out_record.node_id != 0 ||
        out_record.lifecycle_state != PublicLifecycleState::NONE ||
        out_record.visibility_state != PublicVisibilityState::NONE ||
        out_record.trust_state != PublicTrustState::UNKNOWN ||
        out_record.capability_count != 0 ||
        out_record.diagnostics_available ||
        out_record.last_seen_ms != 0) {
        return fail("node_directory_empty_read_record_invalid");
    }
    if (directory.count() != 0 || !directory.isEmpty()) {
        return fail("node_directory_empty_read_mutated");
    }

    if (directory.readByIndex(directory.capacity(), out_record)) {
        return fail("node_directory_out_of_range_read_succeeded");
    }
    if (out_record.valid || out_record.node_id != 0) {
        return fail("node_directory_out_of_range_record_invalid");
    }
    if (directory.count() != 0) {
        return fail("node_directory_out_of_range_mutated");
    }

    for (uint8_t attempt = 0; attempt < 3; ++attempt) {
        if (directory.readByIndex(0, out_record)) {
            return fail("node_directory_repeated_empty_read_succeeded");
        }
        if (directory.count() != 0 || !directory.isEmpty() || out_record.valid) {
            return fail("node_directory_repeated_empty_read_mutated");
        }
    }

    directory.reset();
    if (directory.count() != 0 || !directory.isEmpty()) {
        return fail("node_directory_reset_invalid");
    }
    if (directory.readByIndex(0, out_record)) {
        return fail("node_directory_reset_read_succeeded");
    }
    if (out_record.valid || out_record.node_id != 0) {
        return fail("node_directory_reset_record_invalid");
    }

    return true;
}

bool VerticalSliceValidation::validateNodeDirectoryControlledAddPath() {
    NodeDirectory directory;
    if (directory.count() != 0 || !directory.isEmpty()) {
        return fail("node_directory_add_default_invalid");
    }

    PublicNodeRecord default_record;
    if (directory.addNode(default_record)) {
        return fail("node_directory_add_default_succeeded");
    }
    if (directory.count() != 0 || !directory.isEmpty()) {
        return fail("node_directory_add_default_mutated");
    }

    PublicNodeRecord test_node;
    // Validation-only owner-backed test data. This is not discovery, hardware, or packet data.
    test_node.node_id = 1001;
    test_node.source_type = 1;
    test_node.lifecycle_state = PublicLifecycleState::AVAILABLE;
    test_node.visibility_state = PublicVisibilityState::PUBLIC;
    test_node.trust_state = PublicTrustState::UNKNOWN;
    test_node.capability_count = 0;
    test_node.freshness_state = PublicFreshnessState::UNKNOWN;
    test_node.diagnostics_available = false;
    test_node.last_seen_ms = 0;
    test_node.valid = true;

    if (!directory.addNode(test_node)) {
        return fail("node_directory_add_valid_failed");
    }
    if (directory.count() != 1 || directory.isEmpty()) {
        return fail("node_directory_add_valid_count_invalid");
    }

    PublicNodeRecord out_record;
    if (!directory.readByIndex(0, out_record)) {
        return fail("node_directory_add_read_failed");
    }
    if (!out_record.valid ||
        out_record.node_id != test_node.node_id ||
        out_record.source_type != test_node.source_type ||
        out_record.lifecycle_state != test_node.lifecycle_state ||
        out_record.visibility_state != test_node.visibility_state ||
        out_record.capability_count != 0 ||
        out_record.diagnostics_available) {
        return fail("node_directory_add_read_record_invalid");
    }

    if (directory.addNode(test_node)) {
        return fail("node_directory_add_duplicate_succeeded");
    }
    if (directory.count() != 1) {
        return fail("node_directory_add_duplicate_mutated");
    }

    PublicNodeRecord invalid_node = test_node;
    invalid_node.valid = false;
    invalid_node.node_id = 2002;
    if (directory.addNode(invalid_node)) {
        return fail("node_directory_add_invalid_valid_flag_succeeded");
    }
    if (directory.count() != 1) {
        return fail("node_directory_add_invalid_valid_flag_mutated");
    }

    invalid_node = test_node;
    invalid_node.node_id = 0;
    if (directory.addNode(invalid_node)) {
        return fail("node_directory_add_invalid_node_id_succeeded");
    }
    if (directory.count() != 1) {
        return fail("node_directory_add_invalid_node_id_mutated");
    }

    invalid_node = test_node;
    invalid_node.node_id = 2003;
    invalid_node.lifecycle_state = PublicLifecycleState::NONE;
    if (directory.addNode(invalid_node)) {
        return fail("node_directory_add_invalid_lifecycle_succeeded");
    }
    if (directory.count() != 1) {
        return fail("node_directory_add_invalid_lifecycle_mutated");
    }

    invalid_node = test_node;
    invalid_node.node_id = 2004;
    invalid_node.visibility_state = PublicVisibilityState::NONE;
    if (directory.addNode(invalid_node)) {
        return fail("node_directory_add_invalid_visibility_succeeded");
    }
    if (directory.count() != 1) {
        return fail("node_directory_add_invalid_visibility_mutated");
    }

    NodeDirectory full_directory;
    for (PublicNodeIndex index = 0; index < NODE_DIRECTORY_MAX_PUBLIC_NODES; ++index) {
        PublicNodeRecord fill_node = test_node;
        fill_node.node_id = static_cast<PublicNodeId>(3000 + index);
        if (!full_directory.addNode(fill_node)) {
            return fail("node_directory_add_fill_failed");
        }
    }
    if (full_directory.count() != NODE_DIRECTORY_MAX_PUBLIC_NODES) {
        return fail("node_directory_add_full_count_invalid");
    }

    PublicNodeRecord overflow_node = test_node;
    overflow_node.node_id = 9001;
    if (full_directory.addNode(overflow_node)) {
        return fail("node_directory_add_full_succeeded");
    }
    if (full_directory.count() != NODE_DIRECTORY_MAX_PUBLIC_NODES) {
        return fail("node_directory_add_full_mutated");
    }

    directory.reset();
    if (directory.count() != 0 || !directory.isEmpty()) {
        return fail("node_directory_add_reset_invalid");
    }
    if (directory.readByIndex(0, out_record)) {
        return fail("node_directory_add_reset_read_succeeded");
    }
    if (out_record.valid || out_record.node_id != 0) {
        return fail("node_directory_add_reset_record_invalid");
    }

    PublicOwnerStore store;
    if (!store.mutableNodes().addNode(test_node)) {
        return fail("public_owner_store_mutable_node_add_failed");
    }
    if (store.nodes().count() != 1) {
        return fail("public_owner_store_mutable_node_count_invalid");
    }
    if (!store.nodes().readByIndex(0, out_record) ||
        out_record.node_id != test_node.node_id ||
        !out_record.valid) {
        return fail("public_owner_store_mutable_node_read_invalid");
    }
    store.reset();
    if (store.nodes().count() != 0 || !store.nodes().isEmpty()) {
        return fail("public_owner_store_mutable_node_reset_invalid");
    }

    return true;
}

bool VerticalSliceValidation::validateCapabilityDirectoryEmptySkeleton() {
    CapabilityDirectory directory;
    if (directory.count() != 0) {
        return fail("capability_directory_default_count_invalid");
    }
    if (!directory.isEmpty()) {
        return fail("capability_directory_default_not_empty");
    }
    if (directory.capacity() == 0 ||
        directory.capacity() != CAPABILITY_DIRECTORY_MAX_PUBLIC_CAPABILITIES) {
        return fail("capability_directory_capacity_invalid");
    }

    PublicCapabilityRecord out_record;
    out_record.capability_id = 1234;
    out_record.capability_instance_id = 88;
    out_record.owner_node_id = 77;
    out_record.valid = true;
    out_record.lifecycle_state = PublicLifecycleState::AVAILABLE;
    out_record.visibility_state = PublicVisibilityState::PUBLIC;
    out_record.value_availability_state = PublicAvailabilityState::AVAILABLE;
    out_record.freshness_state = PublicFreshnessState::FRESH;
    out_record.provider_available = true;
    out_record.diagnostics_available = true;
    if (directory.readByIndex(0, out_record)) {
        return fail("capability_directory_empty_read_succeeded");
    }
    if (out_record.valid || out_record.capability_id != 0 ||
        out_record.capability_instance_id != 0 || out_record.owner_node_id != 0 ||
        out_record.category != 0 ||
        out_record.lifecycle_state != PublicLifecycleState::NONE ||
        out_record.visibility_state != PublicVisibilityState::NONE ||
        out_record.value_availability_state != PublicAvailabilityState::UNKNOWN ||
        out_record.freshness_state != PublicFreshnessState::UNKNOWN ||
        out_record.provider_available || out_record.diagnostics_available) {
        return fail("capability_directory_empty_read_record_invalid");
    }
    if (directory.count() != 0 || !directory.isEmpty()) {
        return fail("capability_directory_empty_read_mutated");
    }

    if (directory.readByIndex(directory.capacity(), out_record)) {
        return fail("capability_directory_out_of_range_read_succeeded");
    }
    if (out_record.valid || out_record.capability_id != 0 ||
        out_record.capability_instance_id != 0 || out_record.owner_node_id != 0) {
        return fail("capability_directory_out_of_range_record_invalid");
    }
    if (directory.count() != 0) {
        return fail("capability_directory_out_of_range_mutated");
    }

    for (uint8_t attempt = 0; attempt < 3; ++attempt) {
        if (directory.readByIndex(0, out_record)) {
            return fail("capability_directory_repeated_empty_read_succeeded");
        }
        if (directory.count() != 0 || !directory.isEmpty() ||
            out_record.valid || out_record.capability_id != 0 ||
            out_record.capability_instance_id != 0) {
            return fail("capability_directory_repeated_empty_read_mutated");
        }
    }

    directory.reset();
    if (directory.count() != 0 || !directory.isEmpty()) {
        return fail("capability_directory_reset_invalid");
    }
    if (directory.readByIndex(0, out_record)) {
        return fail("capability_directory_reset_read_succeeded");
    }
    if (out_record.valid || out_record.capability_id != 0 ||
        out_record.capability_instance_id != 0) {
        return fail("capability_directory_reset_record_invalid");
    }

    return true;
}

bool VerticalSliceValidation::validateCapabilityDirectoryControlledAddPath() {
    CapabilityDirectory directory;
    if (directory.count() != 0 || !directory.isEmpty()) {
        return fail("capability_directory_add_default_invalid");
    }

    PublicCapabilityRecord default_record;
    if (directory.addCapability(default_record)) {
        return fail("capability_directory_add_default_succeeded");
    }
    if (directory.count() != 0 || !directory.isEmpty()) {
        return fail("capability_directory_add_default_mutated");
    }

    PublicCapabilityRecord test_capability;
    // Validation-only owner-backed test data. This is not hardware, provider, packet, or Registry data.
    test_capability.capability_id = 9001;
    test_capability.capability_instance_id = 7001;
    test_capability.owner_node_id = 0;
    test_capability.category = 1;
    test_capability.lifecycle_state = PublicLifecycleState::AVAILABLE;
    test_capability.visibility_state = PublicVisibilityState::PUBLIC;
    test_capability.value_availability_state = PublicAvailabilityState::UNKNOWN;
    test_capability.freshness_state = PublicFreshnessState::UNKNOWN;
    test_capability.provider_available = false;
    test_capability.diagnostics_available = false;
    test_capability.valid = true;

    if (!directory.addCapability(test_capability)) {
        return fail("capability_directory_add_valid_failed");
    }
    if (directory.count() != 1 || directory.isEmpty()) {
        return fail("capability_directory_add_valid_count_invalid");
    }

    PublicCapabilityRecord out_record;
    if (!directory.readByIndex(0, out_record)) {
        return fail("capability_directory_add_read_failed");
    }
    if (!out_record.valid ||
        out_record.capability_id != test_capability.capability_id ||
        out_record.capability_instance_id != test_capability.capability_instance_id ||
        out_record.owner_node_id != 0 ||
        out_record.lifecycle_state != test_capability.lifecycle_state ||
        out_record.visibility_state != test_capability.visibility_state ||
        out_record.value_availability_state != PublicAvailabilityState::UNKNOWN ||
        out_record.provider_available ||
        out_record.diagnostics_available) {
        return fail("capability_directory_add_read_record_invalid");
    }

    if (directory.addCapability(test_capability)) {
        return fail("capability_directory_add_duplicate_succeeded");
    }
    if (directory.count() != 1) {
        return fail("capability_directory_add_duplicate_mutated");
    }

    PublicCapabilityRecord invalid_capability = test_capability;
    invalid_capability.valid = false;
    invalid_capability.capability_instance_id = 7002;
    if (directory.addCapability(invalid_capability)) {
        return fail("capability_directory_add_invalid_valid_flag_succeeded");
    }
    if (directory.count() != 1) {
        return fail("capability_directory_add_invalid_valid_flag_mutated");
    }

    invalid_capability = test_capability;
    invalid_capability.capability_id = 0;
    invalid_capability.capability_instance_id = 7003;
    if (directory.addCapability(invalid_capability)) {
        return fail("capability_directory_add_invalid_capability_id_succeeded");
    }
    if (directory.count() != 1) {
        return fail("capability_directory_add_invalid_capability_id_mutated");
    }

    invalid_capability = test_capability;
    invalid_capability.capability_instance_id = 0;
    if (directory.addCapability(invalid_capability)) {
        return fail("capability_directory_add_invalid_instance_id_succeeded");
    }
    if (directory.count() != 1) {
        return fail("capability_directory_add_invalid_instance_id_mutated");
    }

    invalid_capability = test_capability;
    invalid_capability.capability_instance_id = 7004;
    invalid_capability.lifecycle_state = PublicLifecycleState::NONE;
    if (directory.addCapability(invalid_capability)) {
        return fail("capability_directory_add_invalid_lifecycle_succeeded");
    }
    if (directory.count() != 1) {
        return fail("capability_directory_add_invalid_lifecycle_mutated");
    }

    invalid_capability = test_capability;
    invalid_capability.capability_instance_id = 7005;
    invalid_capability.visibility_state = PublicVisibilityState::NONE;
    if (directory.addCapability(invalid_capability)) {
        return fail("capability_directory_add_invalid_visibility_succeeded");
    }
    if (directory.count() != 1) {
        return fail("capability_directory_add_invalid_visibility_mutated");
    }

    CapabilityDirectory full_directory;
    for (PublicCapabilityIndex index = 0; index < CAPABILITY_DIRECTORY_MAX_PUBLIC_CAPABILITIES; ++index) {
        PublicCapabilityRecord fill_capability = test_capability;
        fill_capability.capability_instance_id =
            static_cast<PublicCapabilityInstanceId>(8000 + index);
        if (!full_directory.addCapability(fill_capability)) {
            return fail("capability_directory_add_fill_failed");
        }
    }
    if (full_directory.count() != CAPABILITY_DIRECTORY_MAX_PUBLIC_CAPABILITIES) {
        return fail("capability_directory_add_full_count_invalid");
    }

    PublicCapabilityRecord overflow_capability = test_capability;
    overflow_capability.capability_instance_id = 9901;
    if (full_directory.addCapability(overflow_capability)) {
        return fail("capability_directory_add_full_succeeded");
    }
    if (full_directory.count() != CAPABILITY_DIRECTORY_MAX_PUBLIC_CAPABILITIES) {
        return fail("capability_directory_add_full_mutated");
    }

    directory.reset();
    if (directory.count() != 0 || !directory.isEmpty()) {
        return fail("capability_directory_add_reset_invalid");
    }
    if (directory.readByIndex(0, out_record)) {
        return fail("capability_directory_add_reset_read_succeeded");
    }
    if (out_record.valid || out_record.capability_id != 0 ||
        out_record.capability_instance_id != 0) {
        return fail("capability_directory_add_reset_record_invalid");
    }

    PublicOwnerStore store;
    if (!store.mutableCapabilities().addCapability(test_capability)) {
        return fail("public_owner_store_mutable_capability_add_failed");
    }
    if (store.capabilities().count() != 1) {
        return fail("public_owner_store_mutable_capability_count_invalid");
    }
    if (!store.capabilities().readByIndex(0, out_record) ||
        out_record.capability_instance_id != test_capability.capability_instance_id ||
        !out_record.valid) {
        return fail("public_owner_store_mutable_capability_read_invalid");
    }
    store.reset();
    if (store.capabilities().count() != 0 || !store.capabilities().isEmpty()) {
        return fail("public_owner_store_mutable_capability_reset_invalid");
    }

    return true;
}

bool VerticalSliceValidation::validatePublicOwnerStoreEmptySkeleton() {
    PublicOwnerStore store;
    if (store.nodes().count() != 0 || !store.nodes().isEmpty()) {
        return fail("public_owner_store_default_nodes_invalid");
    }
    if (store.capabilities().count() != 0 || !store.capabilities().isEmpty()) {
        return fail("public_owner_store_default_capabilities_invalid");
    }
    if (store.nodes().capacity() != NODE_DIRECTORY_MAX_PUBLIC_NODES) {
        return fail("public_owner_store_node_capacity_invalid");
    }
    if (store.capabilities().capacity() != CAPABILITY_DIRECTORY_MAX_PUBLIC_CAPABILITIES) {
        return fail("public_owner_store_capability_capacity_invalid");
    }

    PublicNodeRecord node_record;
    node_record.node_id = 1001;
    node_record.valid = true;
    if (store.nodes().readByIndex(0, node_record)) {
        return fail("public_owner_store_empty_node_read_succeeded");
    }
    if (node_record.valid || node_record.node_id != 0) {
        return fail("public_owner_store_empty_node_record_invalid");
    }
    if (store.nodes().count() != 0 || !store.nodes().isEmpty()) {
        return fail("public_owner_store_node_read_mutated");
    }

    PublicCapabilityRecord capability_record;
    if (store.capabilities().readByIndex(0, capability_record)) {
        return fail("public_owner_store_empty_capability_read_succeeded");
    }
    if (capability_record.valid || capability_record.capability_id != 0 ||
        capability_record.capability_instance_id != 0) {
        return fail("public_owner_store_empty_capability_record_invalid");
    }
    if (store.capabilities().count() != 0 || !store.capabilities().isEmpty()) {
        return fail("public_owner_store_capability_read_mutated");
    }

    for (uint8_t attempt = 0; attempt < 3; ++attempt) {
        const NodeDirectory& nodes = store.nodes();
        const CapabilityDirectory& capabilities = store.capabilities();
        if (nodes.count() != 0 || !nodes.isEmpty() ||
            capabilities.count() != 0 || !capabilities.isEmpty()) {
            return fail("public_owner_store_repeated_read_mutated");
        }
    }

    NodeDirectory& mutable_nodes = store.mutableNodes();
    CapabilityDirectory& mutable_capabilities = store.mutableCapabilities();
    if (mutable_nodes.count() != 0 || !mutable_nodes.isEmpty()) {
        return fail("public_owner_store_mutable_nodes_created_record");
    }
    if (mutable_capabilities.count() != 0 || !mutable_capabilities.isEmpty()) {
        return fail("public_owner_store_mutable_capabilities_created_record");
    }

    store.reset();
    if (store.nodes().count() != 0 || !store.nodes().isEmpty()) {
        return fail("public_owner_store_reset_nodes_invalid");
    }
    if (store.capabilities().count() != 0 || !store.capabilities().isEmpty()) {
        return fail("public_owner_store_reset_capabilities_invalid");
    }
    if (store.nodes().readByIndex(0, node_record)) {
        return fail("public_owner_store_reset_node_read_succeeded");
    }
    if (store.capabilities().readByIndex(0, capability_record)) {
        return fail("public_owner_store_reset_capability_read_succeeded");
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
