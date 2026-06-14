#pragma once

#include "../registry/registry.h"
#include "pnp_discovery.h"

namespace Cyber32 {

class PnpRegistration {
public:
    PnpRegistration();

    bool begin(Registry* registry);
    bool registerModuleInfo(const PnpModuleInfo& info);

private:
    Registry* registry_;

    ModuleRecord createModuleRecord(const PnpModuleInfo& info) const;
    DeviceRecord createDeviceRecord(const PnpModuleInfo& info, uint8_t module_index) const;
    CapabilityPayload createInitialPayload(const char* capability_id) const;
    CapabilityRecord createCapabilityRecord(const char* capability_id, uint8_t owner_device_index) const;
    bool isSupportedCapability(const char* capability_id) const;
    bool isSameId(const char* left, const char* right) const;
};

}  // namespace Cyber32
