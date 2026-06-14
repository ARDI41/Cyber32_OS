#pragma once

#include <stdint.h>

#include "../../devices/sensors/sim_distance_device.h"
#include "../../registry/registry.h"

namespace Cyber32 {

class DistanceService {
public:
    DistanceService();

    bool begin(Registry* registry, SimDistanceDevice* device);
    bool update(uint32_t now_ms);
    const char* id() const;
    RegistryResult lastRegistryResult() const;

private:
    Registry* registry_;
    SimDistanceDevice* device_;
    RegistryResult last_registry_result_;
};

}  // namespace Cyber32
