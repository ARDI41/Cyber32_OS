#pragma once

#include <stdint.h>

#include "../core/event_bus/event_bus.h"

namespace Cyber32 {

struct PnpModuleInfo {
    const char* module_id;
    const char* module_type;
    uint8_t metadata_level;
    const char* display_name;
    const char* device_id;
    const char* device_type;
    const char* capability_id;
    bool valid;
};

class PnpDiscovery {
public:
    PnpDiscovery();

    void attachEventBus(EventBus* bus);
    bool discoverSimulatedTemperatureModule(PnpModuleInfo& out_info);
    bool discoverSimulatedDistanceModule(PnpModuleInfo& out_info);
    bool discoverSimulatedServoModule(PnpModuleInfo& out_info);
    bool discoverSimulatedMotorModule(PnpModuleInfo& out_info);
    bool discoverSimulatedRelayModule(PnpModuleInfo& out_info);

private:
    EventBus* event_bus_;

    bool validateInfo(const PnpModuleInfo& info) const;
    bool isRequiredTextPresent(const char* value) const;
    bool startsWithCapPrefix(const char* value) const;
    void publishModuleDiscovered(const PnpModuleInfo& info) const;
};

}  // namespace Cyber32
