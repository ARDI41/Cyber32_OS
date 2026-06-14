#pragma once

#include "../registry/registry.h"
#include "../runtime/runtime.h"
#include "api_response.h"

namespace Cyber32 {

class ServoService;

class Cyber32Api {
public:
    Cyber32Api();

    bool begin(Registry* registry, Runtime* runtime);
    void attachServoService(ServoService* service);
    bool getSystemStatus(ApiSystemStatus& out_status);
    bool getTemperatureState(ApiCapabilityState& out_state);
    bool getDistanceState(ApiCapabilityState& out_state);
    bool getServoPositionState(ApiCapabilityState& out_state);
    bool getServoCommandState(ApiCommandStateResponse& out_response);
    bool commandServoPosition(
        uint32_t now_ms,
        const ApiServoCommandRequest& request,
        ApiServoCommandResponse& out_response);

private:
    Registry* registry_;
    Runtime* runtime_;
    ServoService* servo_service_;

    void fillUnavailableTemperatureState(ApiCapabilityState& out_state) const;
    void fillUnavailableDistanceState(ApiCapabilityState& out_state) const;
    void fillUnavailableServoPositionState(ApiCapabilityState& out_state) const;
    void fillFailedServoCommand(ApiServoCommandResponse& out_response, const char* error_code) const;
    void fillUnavailableServoCommandState(
        ApiCommandStateResponse& out_response,
        RegistryResult registry_result) const;
};

}  // namespace Cyber32
