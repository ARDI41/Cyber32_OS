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
    DeviceRecord createDeviceRecord(const PnpModuleInfo& info) const;
    CapabilityPayload createInitialTemperaturePayload() const;
    CapabilityRecord createTemperatureCapabilityRecord() const;
};

}  // namespace Cyber32
