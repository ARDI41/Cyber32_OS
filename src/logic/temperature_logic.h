#pragma once

#include "../registry/registry.h"
#include "logic_status.h"

namespace Cyber32 {

class TemperatureLogic {
public:
    TemperatureLogic();

    bool begin(Registry* registry);
    bool evaluate();
    LogicStatus status() const;
    float lastTemperature() const;

private:
    Registry* registry_;
    LogicStatus status_;
    float last_temperature_;
};

}  // namespace Cyber32
