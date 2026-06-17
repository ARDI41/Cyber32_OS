#pragma once

#include <stdint.h>

#include "../core/event_bus/event_bus.h"
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
    RegistryResult updateCapabilityProviderPayload(
        const char* provider_id,
        const CapabilityPayload& payload,
        CapabilityProviderStatus status,
        uint32_t now_ms);
    RegistryResult setActiveProvider(const char* capability_id, const char* provider_id);
    RegistryResult getActiveProvider(const char* capability_id, ActiveCapabilityProvider& out_provider) const;
    RegistryResult selectBestProvider(const char* capability_id, ActiveCapabilityProvider& out_provider) const;
    RegistryResult updateSelectedCapabilityPayload(const char* capability_id);
    RegistryResult updateBestCapabilityPayload(const char* capability_id);

    uint8_t moduleCount() const;
    uint8_t deviceCount() const;
    uint8_t capabilityCount() const;
    uint8_t capabilityProviderCount() const;
    uint8_t activeProviderCount() const;

private:
    ModuleRecord modules_[MAX_MODULES];
    DeviceRecord devices_[MAX_DEVICES];
    CapabilityRecord capabilities_[MAX_CAPABILITIES];
    CommandStateRecord command_states_[MAX_COMMAND_STATES];
    CapabilityProviderRecord capability_providers_[MAX_CAPABILITY_PROVIDERS];
    ActiveCapabilityProvider active_capability_providers_[MAX_ACTIVE_CAPABILITY_PROVIDERS];
    uint8_t module_count_;
    uint8_t device_count_;
    uint8_t capability_count_;
    uint8_t command_state_count_;
    uint8_t capability_provider_count_;
    uint8_t active_capability_provider_count_;
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
    void publishCapabilityEvent(const char* event_id, const char* capability_id, uint32_t timestamp_ms);
};

}  // namespace Cyber32
