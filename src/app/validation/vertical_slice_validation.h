#pragma once

#include <stdint.h>

#include "../../api/cyber32_api.h"
#include "../../core/event_bus/event_bus.h"
#include "../../devices/sensors/sim_distance_device.h"
#include "../../devices/sensors/sim_temperature_device.h"
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
#include "../../services/temperature/temperature_service.h"

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

    EventBus event_bus_;
    Registry registry_;
    Runtime runtime_;
    HalTime hal_time_;
    SimTemperatureDriver driver_;
    SimTemperatureDevice device_;
    SimDistanceDriver distance_driver_;
    SimDistanceDevice distance_device_;
    PnpDiscovery pnp_discovery_;
    PnpRegistration pnp_registration_;
    TemperatureService temperature_service_;
    TemperatureLogic temperature_logic_;
    DistanceService distance_service_;
    DistanceLogic distance_logic_;
    Cyber32Api api_;
    TemperatureServiceTaskContext service_task_context_;
    TemperatureLogicTaskContext logic_task_context_;
    DistanceServiceTaskContext distance_service_task_context_;
    DistanceLogicTaskContext distance_logic_task_context_;
    bool passed_;
    const char* last_error_;

    static void runTemperatureServiceTask(void* context);
    static void runTemperatureLogicTask(void* context);
    static void runDistanceServiceTask(void* context);
    static void runDistanceLogicTask(void* context);

    bool registerRuntimeTasks();
    bool fail(const char* error);
    bool validateRegistryState();
    bool validateTemperaturePayload(const CapabilityPayload& payload);
    bool validateDistancePayload(const CapabilityPayload& payload);
    bool validateLogicState();
    bool validateApiState();
    bool validateRuntimeTaskState();
};

}  // namespace Cyber32
