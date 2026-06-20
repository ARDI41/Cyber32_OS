#pragma once

#include <stdint.h>

#include "../core/event_bus/event_bus.h"
#include "../core/types/wireless_node_allowlist_records.h"
#include "../core/types/wireless_node_security_diagnostics.h"
#include "capability_provider_record.h"
#include "command_state_record.h"
#include "registry_records.h"
#include "registry_result.h"

namespace Cyber32 {

struct ActiveCapabilityProvider {
    const char* capability_id;
    const char* provider_id;
};

class Registry {
public:
    static constexpr uint8_t MAX_MODULES = 8;
    static constexpr uint8_t MAX_DEVICES = 16;
    static constexpr uint8_t MAX_CAPABILITIES = 48;
    static constexpr uint8_t MAX_COMMAND_STATES = 8;
    static constexpr uint8_t MAX_CAPABILITY_PROVIDERS = 16;
    static constexpr uint8_t MAX_ACTIVE_CAPABILITY_PROVIDERS = 16;
    static constexpr uint8_t MAX_WIRELESS_NODE_ALLOWLIST = 16;
    static constexpr uint8_t MAX_WIRELESS_NODE_SECURITY_DIAGNOSTICS = 16;
    static constexpr uint32_t PROVIDER_STALE_TIMEOUT_MS = 5000;
    static constexpr uint32_t PROVIDER_LOST_TIMEOUT_MS = 15000;
    static constexpr int8_t NOT_FOUND = -1;

    Registry();

    void begin();
    void attachEventBus(EventBus* bus);

    bool registerModule(const ModuleRecord& record);
    bool registerDevice(const DeviceRecord& record);
    bool registerCapability(const CapabilityRecord& record);

    RegistryWriteResult registerModuleWithResult(const ModuleRecord& record);
    RegistryWriteResult registerDeviceWithResult(const DeviceRecord& record);
    RegistryWriteResult registerCapabilityWithResult(const CapabilityRecord& record);

    int8_t findCapabilityIndex(const char* capability_id) const;
    bool getCapabilityPayload(const char* capability_id, CapabilityPayload& out_payload) const;
    bool updateCapabilityPayload(const char* capability_id, const CapabilityPayload& payload);
    RegistryResult getCapabilityPayloadWithResult(const char* capability_id, CapabilityPayload& out_payload) const;
    RegistryResult updateCapabilityPayloadWithResult(const char* capability_id, const CapabilityPayload& payload);
    RegistryResult updateCommandState(const CommandStateRecord& record);
    RegistryResult getCommandState(const char* capability_id, CommandStateRecord& out_record) const;
    RegistryWriteResult registerCapabilityProviderWithResult(const CapabilityProviderRecord& record);
    RegistryResult getCapabilityProvider(const char* provider_id, CapabilityProviderRecord& out_record) const;
    RegistryResult getCapabilityProviderByIndex(uint8_t index, CapabilityProviderRecord& out_record) const;
    RegistryResult updateCapabilityProviderPayload(
        const char* provider_id,
        const CapabilityPayload& payload,
        CapabilityProviderStatus status,
        uint32_t now_ms);
    RegistryResult setActiveProvider(const char* capability_id, const char* provider_id);
    RegistryResult getActiveProvider(const char* capability_id, ActiveCapabilityProvider& out_provider) const;
    bool isActiveProvider(const char* capability_id, const char* provider_id) const;
    RegistryResult selectBestProvider(const char* capability_id, ActiveCapabilityProvider& out_provider) const;
    RegistryResult updateSelectedCapabilityPayload(const char* capability_id);
    RegistryResult updateBestCapabilityPayload(const char* capability_id);
    RegistryResult updateProviderHealth(uint32_t now_ms);
    RegistryWriteResult registerWirelessNodeAllowlistRecordWithResult(
        const WirelessNodeAllowlistRecord& record);
    RegistryResult getWirelessNodeAllowlistRecord(
        uint32_t node_id,
        WirelessNodeAllowlistRecord& out_record) const;
    RegistryResult getWirelessNodeAllowlistRecordByIndex(
        uint8_t index,
        WirelessNodeAllowlistRecord& out_record) const;
    RegistryResult getWirelessNodeAllowlistRecordByMac(
        const uint8_t mac_address[WIRELESS_MAC_ADDRESS_SIZE],
        WirelessNodeAllowlistRecord& out_record) const;
    RegistryWriteResult registerWirelessNodeSecurityDiagnosticWithResult(
        const WirelessNodeSecurityDiagnosticRecord& record);
    RegistryResult getWirelessNodeSecurityDiagnostic(
        uint32_t node_id,
        WirelessNodeSecurityDiagnosticRecord& out_record) const;
    RegistryResult getWirelessNodeSecurityDiagnosticByIndex(
        uint8_t index,
        WirelessNodeSecurityDiagnosticRecord& out_record) const;

    uint8_t moduleCount() const;
    uint8_t deviceCount() const;
    uint8_t capabilityCount() const;
    uint8_t capabilityProviderCount() const;
    uint8_t activeProviderCount() const;
    uint8_t wirelessNodeAllowlistCount() const;
    uint8_t wirelessNodeSecurityDiagnosticCount() const;

private:
    ModuleRecord modules_[MAX_MODULES];
    DeviceRecord devices_[MAX_DEVICES];
    CapabilityRecord capabilities_[MAX_CAPABILITIES];
    CommandStateRecord command_states_[MAX_COMMAND_STATES];
    CapabilityProviderRecord capability_providers_[MAX_CAPABILITY_PROVIDERS];
    ActiveCapabilityProvider active_capability_providers_[MAX_ACTIVE_CAPABILITY_PROVIDERS];
    WirelessNodeAllowlistRecord wireless_node_allowlist_[MAX_WIRELESS_NODE_ALLOWLIST];
    WirelessNodeSecurityDiagnosticRecord
        wireless_node_security_diagnostics_[MAX_WIRELESS_NODE_SECURITY_DIAGNOSTICS];
    uint8_t module_count_;
    uint8_t device_count_;
    uint8_t capability_count_;
    uint8_t command_state_count_;
    uint8_t capability_provider_count_;
    uint8_t active_capability_provider_count_;
    uint8_t wireless_node_allowlist_count_;
    uint8_t wireless_node_security_diagnostic_count_;
    EventBus* event_bus_;

    bool isTextPresent(const char* value) const;
    bool isValidModuleRecord(const ModuleRecord& record) const;
    bool isValidDeviceRecord(const DeviceRecord& record) const;
    bool isValidCapabilityRecord(const CapabilityRecord& record) const;
    bool isValidPayloadForCapability(const CapabilityPayload& payload, const CapabilityRecord& record) const;
    bool isSameId(const char* left, const char* right) const;
    bool hasModuleId(const char* module_id) const;
    bool hasDeviceId(const char* device_id) const;
    bool hasCapabilityId(const char* capability_id) const;
    int8_t findCommandStateIndex(const char* capability_id) const;
    int8_t findCapabilityProviderIndex(const char* provider_id) const;
    int8_t findActiveProviderIndex(const char* capability_id) const;
    int8_t findWirelessNodeAllowlistIndex(uint32_t node_id) const;
    int8_t findWirelessNodeSecurityDiagnosticIndex(uint32_t node_id) const;
    void publishCapabilityEvent(const char* event_id, const char* capability_id, uint32_t timestamp_ms);
};

}  // namespace Cyber32
