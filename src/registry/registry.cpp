#include "registry.h"

#include "../core/ids/event_ids.h"

namespace Cyber32 {

Registry::Registry()
    : module_count_(0),
      device_count_(0),
      capability_count_(0),
      command_state_count_(0),
      capability_provider_count_(0),
      active_capability_provider_count_(0),
      event_bus_(0) {
}

void Registry::begin() {
    module_count_ = 0;
    device_count_ = 0;
    capability_count_ = 0;
    command_state_count_ = 0;
    capability_provider_count_ = 0;
    active_capability_provider_count_ = 0;
    event_bus_ = 0;
}

void Registry::attachEventBus(EventBus* bus) {
    event_bus_ = bus;
}

bool Registry::registerModule(const ModuleRecord& record) {
    return registerModuleWithResult(record).result == RegistryResult::OK;
}

bool Registry::registerDevice(const DeviceRecord& record) {
    return registerDeviceWithResult(record).result == RegistryResult::OK;
}

bool Registry::registerCapability(const CapabilityRecord& record) {
    return registerCapabilityWithResult(record).result == RegistryResult::OK;
}

RegistryWriteResult Registry::registerModuleWithResult(const ModuleRecord& record) {
    RegistryWriteResult result = {RegistryResult::OK, 255};

    if (!isTextPresent(record.module_id)) {
        result.result = RegistryResult::INVALID_ID;
        return result;
    }
    if (!isValidModuleRecord(record)) {
        result.result = RegistryResult::INVALID_RECORD;
        return result;
    }
    if (hasModuleId(record.module_id)) {
        result.result = RegistryResult::DUPLICATE_ID;
        return result;
    }
    if (module_count_ >= MAX_MODULES) {
        result.result = RegistryResult::TABLE_FULL;
        return result;
    }

    const uint8_t assigned_index = module_count_;
    modules_[assigned_index] = record;
    ++module_count_;

    result.result = RegistryResult::OK;
    result.index = assigned_index;
    return result;
}

RegistryWriteResult Registry::registerDeviceWithResult(const DeviceRecord& record) {
    RegistryWriteResult result = {RegistryResult::OK, 255};

    if (!isTextPresent(record.device_id)) {
        result.result = RegistryResult::INVALID_ID;
        return result;
    }
    if (!isValidDeviceRecord(record)) {
        result.result = RegistryResult::INVALID_RECORD;
        return result;
    }
    if (hasDeviceId(record.device_id)) {
        result.result = RegistryResult::DUPLICATE_ID;
        return result;
    }
    if (device_count_ >= MAX_DEVICES) {
        result.result = RegistryResult::TABLE_FULL;
        return result;
    }

    const uint8_t assigned_index = device_count_;
    devices_[assigned_index] = record;
    ++device_count_;

    result.result = RegistryResult::OK;
    result.index = assigned_index;
    return result;
}

RegistryWriteResult Registry::registerCapabilityWithResult(const CapabilityRecord& record) {
    RegistryWriteResult result = {RegistryResult::OK, 255};

    if (!isTextPresent(record.capability_id)) {
        result.result = RegistryResult::INVALID_ID;
        return result;
    }
    if (!isValidCapabilityRecord(record)) {
        result.result = RegistryResult::INVALID_RECORD;
        return result;
    }
    if (hasCapabilityId(record.capability_id)) {
        result.result = RegistryResult::DUPLICATE_ID;
        return result;
    }
    if (capability_count_ >= MAX_CAPABILITIES) {
        result.result = RegistryResult::TABLE_FULL;
        return result;
    }

    const uint8_t assigned_index = capability_count_;
    capabilities_[assigned_index] = record;
    ++capability_count_;
    publishCapabilityEvent(EVT_CAPABILITY_REGISTERED, record.capability_id, record.latest_payload.timestamp_ms);

    result.result = RegistryResult::OK;
    result.index = assigned_index;
    return result;
}

int8_t Registry::findCapabilityIndex(const char* capability_id) const {
    for (uint8_t i = 0; i < capability_count_; ++i) {
        if (isSameId(capabilities_[i].capability_id, capability_id)) {
            return static_cast<int8_t>(i);
        }
    }

    return NOT_FOUND;
}

bool Registry::getCapabilityPayload(const char* capability_id, CapabilityPayload& out_payload) const {
    return getCapabilityPayloadWithResult(capability_id, out_payload) == RegistryResult::OK;
}

bool Registry::updateCapabilityPayload(const char* capability_id, const CapabilityPayload& payload) {
    return updateCapabilityPayloadWithResult(capability_id, payload) == RegistryResult::OK;
}

RegistryResult Registry::getCapabilityPayloadWithResult(
    const char* capability_id,
    CapabilityPayload& out_payload) const {
    if (!isTextPresent(capability_id)) {
        return RegistryResult::INVALID_ID;
    }

    const int8_t index = findCapabilityIndex(capability_id);
    if (index == NOT_FOUND) {
        return RegistryResult::NOT_FOUND;
    }

    out_payload = capabilities_[index].latest_payload;
    return RegistryResult::OK;
}

RegistryResult Registry::updateCapabilityPayloadWithResult(
    const char* capability_id,
    const CapabilityPayload& payload) {
    if (!isTextPresent(capability_id)) {
        return RegistryResult::INVALID_ID;
    }
    if (!isTextPresent(payload.capability_id) || !isSameId(capability_id, payload.capability_id)) {
        return RegistryResult::INVALID_RECORD;
    }

    const int8_t index = findCapabilityIndex(capability_id);
    if (index == NOT_FOUND) {
        return RegistryResult::NOT_FOUND;
    }

    const CapabilityRecord& capability = capabilities_[index];
    if (!isValidPayloadForCapability(payload, capability)) {
        return RegistryResult::TYPE_MISMATCH;
    }

    capabilities_[index].latest_payload = payload;
    publishCapabilityEvent(EVT_CAPABILITY_VALUE_UPDATED, capability_id, payload.timestamp_ms);
    return RegistryResult::OK;
}

RegistryResult Registry::updateCommandState(const CommandStateRecord& record) {
    if (!isTextPresent(record.capability_id)) {
        return RegistryResult::INVALID_ID;
    }
    if (findCapabilityIndex(record.capability_id) == NOT_FOUND) {
        return RegistryResult::NOT_FOUND;
    }

    const int8_t existing_index = findCommandStateIndex(record.capability_id);
    if (existing_index != NOT_FOUND) {
        command_states_[existing_index] = record;
        return RegistryResult::OK;
    }

    if (command_state_count_ >= MAX_COMMAND_STATES) {
        return RegistryResult::TABLE_FULL;
    }

    command_states_[command_state_count_] = record;
    ++command_state_count_;
    return RegistryResult::OK;
}

RegistryResult Registry::getCommandState(
    const char* capability_id,
    CommandStateRecord& out_record) const {
    if (!isTextPresent(capability_id)) {
        return RegistryResult::INVALID_ID;
    }

    const int8_t index = findCommandStateIndex(capability_id);
    if (index == NOT_FOUND) {
        return RegistryResult::NOT_FOUND;
    }

    out_record = command_states_[index];
    return RegistryResult::OK;
}

RegistryWriteResult Registry::registerCapabilityProviderWithResult(
    const CapabilityProviderRecord& record) {
    RegistryWriteResult result = {RegistryResult::OK, 255};

    if (!isTextPresent(record.provider_id)) {
        result.result = RegistryResult::INVALID_ID;
        return result;
    }
    if (!isTextPresent(record.capability_id)) {
        result.result = RegistryResult::INVALID_RECORD;
        return result;
    }
    if (findCapabilityProviderIndex(record.provider_id) != NOT_FOUND) {
        result.result = RegistryResult::DUPLICATE_ID;
        return result;
    }
    if (capability_provider_count_ >= MAX_CAPABILITY_PROVIDERS) {
        result.result = RegistryResult::TABLE_FULL;
        return result;
    }

    const uint8_t assigned_index = capability_provider_count_;
    capability_providers_[assigned_index] = record;
    ++capability_provider_count_;

    result.result = RegistryResult::OK;
    result.index = assigned_index;
    return result;
}

RegistryResult Registry::getCapabilityProvider(
    const char* provider_id,
    CapabilityProviderRecord& out_record) const {
    if (!isTextPresent(provider_id)) {
        return RegistryResult::INVALID_ID;
    }

    const int8_t index = findCapabilityProviderIndex(provider_id);
    if (index == NOT_FOUND) {
        return RegistryResult::NOT_FOUND;
    }

    out_record = capability_providers_[index];
    return RegistryResult::OK;
}

RegistryResult Registry::updateCapabilityProviderPayload(
    const char* provider_id,
    const CapabilityPayload& payload,
    CapabilityProviderStatus status,
    uint32_t now_ms) {
    if (!isTextPresent(provider_id)) {
        return RegistryResult::INVALID_ID;
    }

    const int8_t index = findCapabilityProviderIndex(provider_id);
    if (index == NOT_FOUND) {
        return RegistryResult::NOT_FOUND;
    }
    if (!isTextPresent(payload.capability_id) ||
        !isSameId(payload.capability_id, capability_providers_[index].capability_id)) {
        return RegistryResult::INVALID_RECORD;
    }

    capability_providers_[index].latest_payload = payload;
    capability_providers_[index].status = status;
    capability_providers_[index].last_update_ms = now_ms;
    return RegistryResult::OK;
}

RegistryResult Registry::setActiveProvider(const char* capability_id, const char* provider_id) {
    if (!isTextPresent(capability_id)) {
        return RegistryResult::INVALID_ID;
    }
    if (!isTextPresent(provider_id)) {
        return RegistryResult::INVALID_ID;
    }

    const int8_t provider_index = findCapabilityProviderIndex(provider_id);
    if (provider_index == NOT_FOUND) {
        return RegistryResult::NOT_FOUND;
    }
    if (!isSameId(capability_providers_[provider_index].capability_id, capability_id)) {
        return RegistryResult::INVALID_RECORD;
    }

    const int8_t active_index = findActiveProviderIndex(capability_id);
    if (active_index != NOT_FOUND) {
        active_capability_providers_[active_index].provider_id = provider_id;
        return RegistryResult::OK;
    }

    if (active_capability_provider_count_ >= MAX_ACTIVE_CAPABILITY_PROVIDERS) {
        return RegistryResult::TABLE_FULL;
    }

    active_capability_providers_[active_capability_provider_count_].capability_id = capability_id;
    active_capability_providers_[active_capability_provider_count_].provider_id = provider_id;
    ++active_capability_provider_count_;
    return RegistryResult::OK;
}

RegistryResult Registry::getActiveProvider(
    const char* capability_id,
    ActiveCapabilityProvider& out_provider) const {
    if (!isTextPresent(capability_id)) {
        return RegistryResult::INVALID_ID;
    }

    const int8_t index = findActiveProviderIndex(capability_id);
    if (index == NOT_FOUND) {
        return RegistryResult::NOT_FOUND;
    }

    out_provider = active_capability_providers_[index];
    return RegistryResult::OK;
}

RegistryResult Registry::selectBestProvider(
    const char* capability_id,
    ActiveCapabilityProvider& out_provider) const {
    if (!isTextPresent(capability_id)) {
        return RegistryResult::INVALID_ID;
    }

    int8_t selected_index = NOT_FOUND;
    for (uint8_t i = 0; i < capability_provider_count_; ++i) {
        const CapabilityProviderRecord& candidate = capability_providers_[i];
        if (!isSameId(candidate.capability_id, capability_id)) {
            continue;
        }
        if (candidate.status != CapabilityProviderStatus::AVAILABLE) {
            continue;
        }

        if (selected_index == NOT_FOUND) {
            selected_index = static_cast<int8_t>(i);
            continue;
        }

        const CapabilityProviderRecord& selected = capability_providers_[selected_index];
        if (candidate.priority > selected.priority ||
            (candidate.priority == selected.priority &&
             candidate.last_update_ms > selected.last_update_ms)) {
            selected_index = static_cast<int8_t>(i);
        }
    }

    if (selected_index == NOT_FOUND) {
        return RegistryResult::NOT_FOUND;
    }

    out_provider.capability_id = capability_providers_[selected_index].capability_id;
    out_provider.provider_id = capability_providers_[selected_index].provider_id;
    return RegistryResult::OK;
}

uint8_t Registry::moduleCount() const {
    return module_count_;
}

uint8_t Registry::deviceCount() const {
    return device_count_;
}

uint8_t Registry::capabilityCount() const {
    return capability_count_;
}

uint8_t Registry::capabilityProviderCount() const {
    return capability_provider_count_;
}

uint8_t Registry::activeProviderCount() const {
    return active_capability_provider_count_;
}

bool Registry::isSameId(const char* left, const char* right) const {
    if (left == 0 || right == 0) {
        return false;
    }

    while (*left != '\0' && *right != '\0') {
        if (*left != *right) {
            return false;
        }
        ++left;
        ++right;
    }

    return *left == '\0' && *right == '\0';
}

bool Registry::isTextPresent(const char* value) const {
    return value != 0 && value[0] != '\0';
}

bool Registry::isValidModuleRecord(const ModuleRecord& record) const {
    return isTextPresent(record.module_id) &&
           isTextPresent(record.module_type);
}

bool Registry::isValidDeviceRecord(const DeviceRecord& record) const {
    return isTextPresent(record.device_id) &&
           isTextPresent(record.device_type) &&
           record.module_index < module_count_;
}

bool Registry::isValidCapabilityRecord(const CapabilityRecord& record) const {
    return isTextPresent(record.capability_id) &&
           record.capability_id[0] == 'C' &&
           record.capability_id[1] == 'A' &&
           record.capability_id[2] == 'P' &&
           record.capability_id[3] == '_' &&
           isTextPresent(record.category) &&
           isTextPresent(record.kind) &&
           isTextPresent(record.access) &&
           record.owner_device_index < device_count_ &&
           isTextPresent(record.latest_payload.capability_id) &&
           isSameId(record.capability_id, record.latest_payload.capability_id) &&
           isValidPayloadForCapability(record.latest_payload, record);
}

bool Registry::isValidPayloadForCapability(
    const CapabilityPayload& payload,
    const CapabilityRecord& record) const {
    if (!isSameId(payload.capability_id, record.capability_id)) {
        return false;
    }
    if (payload.value_type == PayloadValueType::NONE) {
        return payload.available != Availability::AVAILABLE ||
               payload.stale == StaleState::STALE;
    }
    return payload.value_type == record.data_type;
}

bool Registry::hasModuleId(const char* module_id) const {
    for (uint8_t i = 0; i < module_count_; ++i) {
        if (isSameId(modules_[i].module_id, module_id)) {
            return true;
        }
    }

    return false;
}

bool Registry::hasDeviceId(const char* device_id) const {
    for (uint8_t i = 0; i < device_count_; ++i) {
        if (isSameId(devices_[i].device_id, device_id)) {
            return true;
        }
    }

    return false;
}

bool Registry::hasCapabilityId(const char* capability_id) const {
    return findCapabilityIndex(capability_id) != NOT_FOUND;
}

int8_t Registry::findCommandStateIndex(const char* capability_id) const {
    for (uint8_t i = 0; i < command_state_count_; ++i) {
        if (isSameId(command_states_[i].capability_id, capability_id)) {
            return static_cast<int8_t>(i);
        }
    }

    return NOT_FOUND;
}

int8_t Registry::findCapabilityProviderIndex(const char* provider_id) const {
    for (uint8_t i = 0; i < capability_provider_count_; ++i) {
        if (isSameId(capability_providers_[i].provider_id, provider_id)) {
            return static_cast<int8_t>(i);
        }
    }

    return NOT_FOUND;
}

int8_t Registry::findActiveProviderIndex(const char* capability_id) const {
    for (uint8_t i = 0; i < active_capability_provider_count_; ++i) {
        if (isSameId(active_capability_providers_[i].capability_id, capability_id)) {
            return static_cast<int8_t>(i);
        }
    }

    return NOT_FOUND;
}

void Registry::publishCapabilityEvent(const char* event_id, const char* capability_id, uint32_t timestamp_ms) {
    if (event_bus_ == 0) {
        return;
    }

    EventRecord event = {
        event_id,
        EventPriority::NORMAL,
        timestamp_ms,
        "registry",
        "registry",
        capability_id,
        0,
        0.0F,
        0
    };

    event_bus_->publish(event);
}

}  // namespace Cyber32
