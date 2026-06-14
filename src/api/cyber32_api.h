#pragma once

#include "../registry/registry.h"
#include "../runtime/runtime.h"
#include "api_response.h"

namespace Cyber32 {

class MotorService;
class ServoService;

class Cyber32Api {
public:
    Cyber32Api();

    bool begin(Registry* registry, Runtime* runtime);
    void attachServoService(ServoService* service);
    void attachMotorService(MotorService* service);
    bool getSystemStatus(ApiSystemStatus& out_status);
    bool getTemperatureState(ApiCapabilityState& out_state);
    bool getDistanceState(ApiCapabilityState& out_state);
    bool getServoPositionState(ApiCapabilityState& out_state);
    bool getMotorControlState(ApiCapabilityState& out_state);
    bool getServoCommandState(ApiCommandStateResponse& out_response);
    bool getMotorCommandState(ApiCommandStateResponse& out_response);
    bool commandServoPosition(
        uint32_t now_ms,
        const ApiServoCommandRequest& request,
        ApiServoCommandResponse& out_response);
    bool commandMotorControl(
        uint32_t now_ms,
        const ApiMotorCommandRequest& request,
        ApiMotorCommandResponse& out_response);
    bool commandMotorStop(uint32_t now_ms, ApiMotorCommandResponse& out_response);

private:
    Registry* registry_;
    Runtime* runtime_;
    ServoService* servo_service_;
    MotorService* motor_service_;

    void fillUnavailableTemperatureState(ApiCapabilityState& out_state) const;
    void fillUnavailableDistanceState(ApiCapabilityState& out_state) const;
    void fillUnavailableServoPositionState(ApiCapabilityState& out_state) const;
    void fillUnavailableMotorControlState(ApiCapabilityState& out_state) const;
    void fillFailedServoCommand(ApiServoCommandResponse& out_response, const char* error_code) const;
    void fillFailedMotorCommand(ApiMotorCommandResponse& out_response, const char* error_code) const;
    void fillUnavailableServoCommandState(
        ApiCommandStateResponse& out_response,
        RegistryResult registry_result) const;
    void fillUnavailableMotorCommandState(
        ApiCommandStateResponse& out_response,
        RegistryResult registry_result) const;
};

}  // namespace Cyber32
