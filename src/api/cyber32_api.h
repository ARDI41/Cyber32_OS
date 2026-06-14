#pragma once

#include "../registry/registry.h"
#include "../runtime/runtime.h"
#include "api_response.h"

namespace Cyber32 {

class Cyber32Api {
public:
    Cyber32Api();

    bool begin(Registry* registry, Runtime* runtime);
    bool getSystemStatus(ApiSystemStatus& out_status);
    bool getTemperatureState(ApiCapabilityState& out_state);
    bool getDistanceState(ApiCapabilityState& out_state);

private:
    Registry* registry_;
    Runtime* runtime_;

    void fillUnavailableTemperatureState(ApiCapabilityState& out_state) const;
    void fillUnavailableDistanceState(ApiCapabilityState& out_state) const;
};

}  // namespace Cyber32
