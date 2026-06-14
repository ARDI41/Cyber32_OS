#pragma once

#include "../registry/registry.h"
#include "logic_status.h"

namespace Cyber32 {

class DistanceLogic {
public:
    DistanceLogic();

    bool begin(Registry* registry);
    bool evaluate();
    LogicStatus status() const;
    float lastDistance() const;

private:
    Registry* registry_;
    LogicStatus status_;
    float last_distance_;
};

}  // namespace Cyber32
