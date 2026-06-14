#pragma once

#include <stdint.h>

#include "../core/event_bus/event_bus.h"
#include "command_state_record.h"
#include "registry_records.h"
#include "registry_result.h"

namespace Cyber32 {

class Registry {
public:
    static constexpr uint8_t MAX_MODULES = 8;
    static constexpr uint8_t MAX_DEVICES = 16;
    static constexpr uint8_t MAX_CAPABILITIES = 48;
    static constexpr uint8_t MAX_COMMAND_STATES = 8;
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

    uint8_t moduleCount() const;
    uint8_t deviceCount() const;
    uint8_t capabilityCount() const;

private:
    ModuleRecord modules_[MAX_MODULES];
    DeviceRecord devices_[MAX_DEVICES];
    CapabilityRecord capabilities_[MAX_CAPABILITIES];
    CommandStateRecord command_states_[MAX_COMMAND_STATES];
    uint8_t module_count_;
    uint8_t device_count_;
    uint8_t capability_count_;
    uint8_t command_state_count_;
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
    void publishCapabilityEvent(const char* event_id, const char* capability_id, uint32_t timestamp_ms);
};

}  // namespace Cyber32
