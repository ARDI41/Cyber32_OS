#pragma once

#include <stdint.h>

#include "../../api/cyber32_api.h"
#include "../../core/event_bus/event_bus.h"
#include "../../devices/actuators/sim_motor_device.h"
#include "../../devices/actuators/sim_relay_device.h"
#include "../../devices/actuators/sim_servo_device.h"
#include "../../devices/communication/wireless_temperature_device.h"
#include "../../devices/sensors/sim_distance_device.h"
#include "../../devices/sensors/sim_temperature_device.h"
#include "../../drivers/actuators/sim_motor_driver.h"
#include "../../drivers/actuators/sim_relay_driver.h"
#include "../../drivers/actuators/sim_servo_driver.h"
#include "../../drivers/communication/sim_espnow_transport_driver.h"
#include "../../drivers/sensors/sim_distance_driver.h"
#include "../../drivers/sensors/sim_temperature_driver.h"
#include "../../hal/time/hal_time.h"
#include "../../logic/distance_logic.h"
#include "../../logic/temperature_logic.h"
#include "../../pnp/pnp_discovery.h"
#include "../../pnp/pnp_registration.h"
#include "../../registry/registry.h"
#include "../../runtime/runtime.h"
#include "../../services/distance/distance_service.h"
#include "../../services/motor/motor_service.h"
#include "../../services/relay/relay_service.h"
#include "../../services/servo/servo_service.h"
#include "../../services/temperature/temperature_service.h"
#include "../../services/wireless/wireless_service.h"
#include "../../../include/cyber32/public/node_directory.h"
#include "../../../include/cyber32/public/public_owner_types.h"

namespace Cyber32 {

class VerticalSliceValidation {
public:
    VerticalSliceValidation();

    bool begin();
    bool runOnce(uint32_t now_ms);
    bool runOnceWithRuntime(uint32_t now_ms);
    bool passed() const;
    const char* lastError() const;

private:
    struct TemperatureServiceTaskContext {
        TemperatureService* service;
        uint32_t now_ms;
        bool ran;
        bool last_result;
        const char* last_error;
    };

    struct TemperatureLogicTaskContext {
        TemperatureLogic* logic;
        bool ran;
        bool last_result;
        const char* last_error;
    };

    struct DistanceServiceTaskContext {
        DistanceService* service;
        uint32_t now_ms;
        bool ran;
        bool last_result;
        const char* last_error;
    };

    struct DistanceLogicTaskContext {
        DistanceLogic* logic;
        bool ran;
        bool last_result;
        const char* last_error;
    };

    struct ServoServiceStateTaskContext {
        ServoService* service;
        uint32_t now_ms;
        bool ran;
        bool last_result;
        const char* last_error;
    };

    struct MotorServiceStateTaskContext {
        MotorService* service;
        uint32_t now_ms;
        bool ran;
        bool last_result;
        const char* last_error;
    };

    struct MotorServiceCommandTaskContext {
        MotorService* service;
        uint32_t now_ms;
        bool ran;
        bool last_result;
        const char* last_error;
    };

    struct RelayServiceStateTaskContext {
        RelayService* service;
        uint32_t now_ms;
        bool ran;
        bool last_result;
        const char* last_error;
    };

    struct RelayServiceCommandTaskContext {
        RelayService* service;
        uint32_t now_ms;
        bool ran;
        bool last_result;
        const char* last_error;
    };

    struct WirelessServiceProcessTaskContext {
        WirelessService* service;
        uint32_t now_ms;
        bool ran;
        bool last_result;
        const char* last_error;
    };

    struct WirelessServiceTimeoutTaskContext {
        WirelessService* service;
        uint32_t now_ms;
        bool ran;
        bool last_result;
        const char* last_error;
    };

    EventBus event_bus_;
    Registry registry_;
    Runtime runtime_;
    HalTime hal_time_;
    SimTemperatureDriver driver_;
    SimTemperatureDevice device_;
    SimDistanceDriver distance_driver_;
    SimDistanceDevice distance_device_;
    SimServoDriver servo_driver_;
    SimServoDevice servo_device_;
    SimMotorDriver motor_driver_;
    SimMotorDevice motor_device_;
    SimRelayDriver relay_driver_;
    SimRelayDevice relay_device_;
    SimEspNowTransportDriver wireless_transport_driver_;
    WirelessTemperatureDevice wireless_temperature_device_;
    PnpDiscovery pnp_discovery_;
    PnpRegistration pnp_registration_;
    TemperatureService temperature_service_;
    TemperatureLogic temperature_logic_;
    DistanceService distance_service_;
    DistanceLogic distance_logic_;
    ServoService servo_service_;
    MotorService motor_service_;
    RelayService relay_service_;
    WirelessService wireless_service_;
    Cyber32Api api_;
    TemperatureServiceTaskContext service_task_context_;
    TemperatureLogicTaskContext logic_task_context_;
    DistanceServiceTaskContext distance_service_task_context_;
    DistanceLogicTaskContext distance_logic_task_context_;
    ServoServiceStateTaskContext servo_service_state_task_context_;
    MotorServiceStateTaskContext motor_service_state_task_context_;
    MotorServiceCommandTaskContext motor_service_command_task_context_;
    RelayServiceStateTaskContext relay_service_state_task_context_;
    RelayServiceCommandTaskContext relay_service_command_task_context_;
    WirelessServiceProcessTaskContext wireless_service_process_task_context_;
    WirelessServiceTimeoutTaskContext wireless_service_timeout_task_context_;
    bool passed_;
    const char* last_error_;

    static void runTemperatureServiceTask(void* context);
    static void runTemperatureLogicTask(void* context);
    static void runDistanceServiceTask(void* context);
    static void runDistanceLogicTask(void* context);
    static void runServoServiceStateTask(void* context);
    static void runMotorServiceStateTask(void* context);
    static void runMotorServiceCommandTask(void* context);
    static void runRelayServiceStateTask(void* context);
    static void runRelayServiceCommandTask(void* context);
    static void runWirelessServiceProcessTask(void* context);
    static void runWirelessServiceTimeoutTask(void* context);

    bool registerRuntimeTasks();
    bool fail(const char* error);
    bool validateRegistryState();
    bool validateTemperaturePayload(const CapabilityPayload& payload);
    bool validateDistancePayload(const CapabilityPayload& payload);
    bool validateServoPayload(const CapabilityPayload& payload, float expected_position);
    bool validateMotorPayload(
        const CapabilityPayload& payload,
        float expected_speed,
        MotorDirection expected_direction);
    bool validateRelayPayload(const CapabilityPayload& payload, bool expected_enabled);
    bool validateLogicState();
    bool validateApiState();
    bool validateServoCommandState(uint32_t now_ms);
    bool validateServoRuntimeBlockedState(RuntimeState state, uint32_t now_ms, float expected_position);
    bool validateMotorCommandState(uint32_t now_ms);
    bool validateMotorRuntimeBlockedState(
        RuntimeState state,
        uint32_t now_ms,
        float expected_speed,
        MotorDirection expected_direction);
    bool validateMotorPendingRuntimeTransitions(uint32_t now_ms);
    bool validateRelayCommandState(uint32_t now_ms);
    bool validateRuntimeSafeModeHelpers();
    bool validateRuntimeTaskState();
    bool validateRegistryResultState();
    bool validateEspNowTransportInitializationSmoke();
    bool validateSimWirelessPacketTransportAdapter();
    bool validateEspNowWirelessPacketTransportAdapter();
    bool validateWirelessServiceTransportAdapterAttachment();
    bool validateWirelessServiceProcessPacketsAdapterPath(uint32_t now_ms);
    bool validateEspNowAdapterWirelessServicePath(uint32_t now_ms);
    bool validateCapabilityProviderStorage(uint32_t now_ms);
    bool validatePublicOwnerTypeDefaults();
    bool validateNodeDirectoryEmptySkeleton();
    void copyWirelessCapabilityId(char* destination, const char* source) const;
    bool isSameText(const char* left, const char* right) const;
};

}  // namespace Cyber32
