#pragma once

#include "../registry/registry.h"
#include "../runtime/runtime.h"
#include "api_response.h"

namespace Cyber32 {

class MotorService;
class RelayService;
class ServoService;

class Cyber32Api {
public:
    Cyber32Api();

    bool begin(Registry* registry, Runtime* runtime);
    void attachServoService(ServoService* service);
    void attachMotorService(MotorService* service);
    void attachRelayService(RelayService* service);
    bool getSystemStatus(ApiSystemStatus& out_status);
    bool getSystemIdentity(ApiSystemIdentity& out_response);
    bool getSystemFirmware(ApiSystemFirmware& out_response);
    bool getSystemRuntime(ApiSystemRuntime& out_response);
    bool getSystemModes(ApiSystemModes& out_response);
    bool getSystemMemory(ApiSystemMemory& out_response);
    bool getSystemSummary(ApiSystemSummary& out_response);
    bool getNodeList(ApiNodeList& out_response);
    bool getNodeSummary(uint8_t node_index, ApiNodeSummary& out_response);
    bool getNodeIdentity(uint8_t node_index, ApiNodeIdentity& out_response);
    bool getNodeStatus(uint8_t node_index, ApiNodeStatus& out_response);
    bool getNodePower(uint8_t node_index, ApiNodePower& out_response);
    bool getNodeSignal(uint8_t node_index, ApiNodeSignal& out_response);
    bool getNodeDiagnostics(uint8_t node_index, ApiNodeDiagnosticsSummary& out_response);
    bool getNodeCapabilities(
        uint8_t node_index,
        ApiNodeCapabilitySummary* out_capabilities,
        uint8_t max_count,
        uint8_t& out_count);
    bool getCapabilityList(ApiCapabilityList& out_response);
    bool getCapabilitySummary(uint8_t capability_index, ApiCapabilitySummary& out_response);
    bool getCapabilityIdentity(uint8_t capability_index, ApiCapabilityIdentity& out_response);
    bool getCapabilityValue(uint8_t capability_index, ApiCapabilityValue& out_response);
    bool getCapabilityAvailability(uint8_t capability_index, ApiCapabilityAvailability& out_response);
    bool getCapabilityProviderInfo(uint8_t capability_index, ApiCapabilityProviderInfo& out_response);
    bool getCapabilityQuality(uint8_t capability_index, ApiCapabilityQuality& out_response);
    bool getTemperatureState(ApiCapabilityState& out_state);
    bool getDistanceState(ApiCapabilityState& out_state);
    bool getServoPositionState(ApiCapabilityState& out_state);
    bool getMotorControlState(ApiCapabilityState& out_state);
    bool getRelayControlState(ApiCapabilityState& out_state);
    bool getServoCommandState(ApiCommandStateResponse& out_response);
    bool getMotorCommandState(ApiCommandStateResponse& out_response);
    bool getRelayCommandState(ApiCommandStateResponse& out_response);
    bool getProviderDiagnostic(
        const char* provider_id,
        ApiProviderDiagnostic& out_response);
    bool getActiveProviderDiagnostic(
        const char* capability_id,
        ApiProviderDiagnostic& out_response);
    bool getProviderSummary(ApiProviderSummary& out_response);
    bool getWirelessNodeDiagnostic(
        uint32_t node_id,
        ApiWirelessNodeDiagnostic& out_response);
    bool getWirelessNodeDiagnosticByIndex(
        uint8_t index,
        ApiWirelessNodeDiagnostic& out_response);
    bool getWirelessNodeSummary(ApiWirelessNodeSummary& out_response);
    bool getWirelessSecurityDiagnostic(
        uint32_t node_id,
        ApiWirelessSecurityDiagnostic& out_response);
    bool getWirelessSecurityDiagnosticByIndex(
        uint8_t index,
        ApiWirelessSecurityDiagnostic& out_response);
    bool getWirelessSecuritySummary(ApiWirelessSecuritySummary& out_response);
    bool commandServoPosition(
        uint32_t now_ms,
        const ApiServoCommandRequest& request,
        ApiServoCommandResponse& out_response);
    bool commandMotorControl(
        uint32_t now_ms,
        const ApiMotorCommandRequest& request,
        ApiMotorCommandResponse& out_response);
    bool commandMotorStop(uint32_t now_ms, ApiMotorCommandResponse& out_response);
    bool commandRelayControl(
        uint32_t now_ms,
        const ApiRelayCommandRequest& request,
        ApiRelayCommandResponse& out_response);
    bool commandRelayOff(uint32_t now_ms, ApiRelayCommandResponse& out_response);

private:
    Registry* registry_;
    Runtime* runtime_;
    ServoService* servo_service_;
    MotorService* motor_service_;
    RelayService* relay_service_;

    void fillUnavailableTemperatureState(ApiCapabilityState& out_state) const;
    void fillUnavailableDistanceState(ApiCapabilityState& out_state) const;
    void fillUnavailableServoPositionState(ApiCapabilityState& out_state) const;
    void fillUnavailableMotorControlState(ApiCapabilityState& out_state) const;
    void fillUnavailableRelayControlState(ApiCapabilityState& out_state) const;
    void fillFailedServoCommand(ApiServoCommandResponse& out_response, const char* error_code) const;
    void fillFailedMotorCommand(ApiMotorCommandResponse& out_response, const char* error_code) const;
    void fillFailedRelayCommand(ApiRelayCommandResponse& out_response, const char* error_code) const;
    void fillUnavailableServoCommandState(
        ApiCommandStateResponse& out_response,
        RegistryResult registry_result) const;
    void fillUnavailableMotorCommandState(
        ApiCommandStateResponse& out_response,
        RegistryResult registry_result) const;
    void fillUnavailableRelayCommandState(
        ApiCommandStateResponse& out_response,
        RegistryResult registry_result) const;
    void fillUnavailableProviderDiagnostic(
        ApiProviderDiagnostic& out_response,
        RegistryResult registry_result,
        const char* provider_id,
        const char* capability_id) const;
    void fillUnavailableProviderSummary(
        ApiProviderSummary& out_response,
        RegistryResult registry_result) const;
    void fillWirelessNodeDiagnosticFromRecord(
        const WirelessNodeAllowlistRecord& record,
        ApiWirelessNodeDiagnostic& out_response) const;
    void fillUnavailableWirelessNodeDiagnostic(
        ApiWirelessNodeDiagnostic& out_response,
        RegistryResult registry_result,
        uint32_t node_id) const;
    void fillUnavailableWirelessNodeSummary(
        ApiWirelessNodeSummary& out_response,
        RegistryResult registry_result) const;
    void fillWirelessSecurityDiagnosticFromRecord(
        const WirelessNodeSecurityDiagnosticRecord& record,
        ApiWirelessSecurityDiagnostic& out_response) const;
    void fillUnavailableWirelessSecurityDiagnostic(
        ApiWirelessSecurityDiagnostic& out_response,
        RegistryResult registry_result,
        uint32_t node_id) const;
    void fillUnavailableWirelessSecuritySummary(
        ApiWirelessSecuritySummary& out_response,
        RegistryResult registry_result) const;
};

}  // namespace Cyber32
