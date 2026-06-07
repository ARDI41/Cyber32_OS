#pragma once

#include <stdint.h>

#include "../../devices/sensors/sim_temperature_device.h"
#include "../../registry/registry.h"

namespace Cyber32 {

class TemperatureService {
public:
    TemperatureService();

    bool begin(Registry* registry, SimTemperatureDevice* device);
    bool update(uint32_t now_ms);
    const char* id() const;

private:
    Registry* registry_;
    SimTemperatureDevice* device_;
};

}  // namespace Cyber32
