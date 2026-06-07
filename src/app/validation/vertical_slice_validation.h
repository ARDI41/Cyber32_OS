#pragma once

#include <stdint.h>

#include "../../api/cyber32_api.h"
#include "../../core/event_bus/event_bus.h"
#include "../../devices/sensors/sim_temperature_device.h"
#include "../../drivers/sensors/sim_temperature_driver.h"
#include "../../hal/time/hal_time.h"
#include "../../logic/temperature_logic.h"
#include "../../pnp/pnp_discovery.h"
#include "../../pnp/pnp_registration.h"
#include "../../registry/registry.h"
#include "../../runtime/runtime.h"
#include "../../services/temperature/temperature_service.h"

namespace Cyber32 {

class VerticalSliceValidation {
public:
    VerticalSliceValidation();

    bool begin();
    bool runOnce(uint32_t now_ms);
    bool passed() const;
    const char* lastError() const;

private:
    EventBus event_bus_;
    Registry registry_;
    Runtime runtime_;
    HalTime hal_time_;
    SimTemperatureDriver driver_;
    SimTemperatureDevice device_;
    PnpDiscovery pnp_discovery_;
    PnpRegistration pnp_registration_;
    TemperatureService temperature_service_;
    TemperatureLogic temperature_logic_;
    Cyber32Api api_;
    bool passed_;
    const char* last_error_;

    bool fail(const char* error);
    bool validateRegistryState();
    bool validateTemperaturePayload(const CapabilityPayload& payload);
    bool validateLogicState();
    bool validateApiState();
};

}  // namespace Cyber32
