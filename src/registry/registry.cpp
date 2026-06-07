#include "registry.h"

#include "../core/ids/event_ids.h"

namespace Cyber32 {

Registry::Registry()
    : module_count_(0),
      device_count_(0),
      capability_count_(0),
      event_bus_(0) {
}

void Registry::begin() {
    module_count_ = 0;
    device_count_ = 0;
    capability_count_ = 0;
    event_bus_ = 0;
}

void Registry::attachEventBus(EventBus* bus) {
    event_bus_ = bus;
}

bool Registry::registerModule(const ModuleRecord& record) {
    if (module_count_ >= MAX_MODULES || hasModuleId(record.module_id)) {
        return false;
    }

    modules_[module_count_] = record;
    ++module_count_;
    return true;
}

bool Registry::registerDevice(const DeviceRecord& record) {
    if (device_count_ >= MAX_DEVICES || hasDeviceId(record.device_id)) {
        return false;
    }

    devices_[device_count_] = record;
    ++device_count_;
    return true;
}

bool Registry::registerCapability(const CapabilityRecord& record) {
    if (capability_count_ >= MAX_CAPABILITIES || hasCapabilityId(record.capability_id)) {
        return false;
    }

    capabilities_[capability_count_] = record;
    ++capability_count_;
    publishCapabilityEvent(EVT_CAPABILITY_REGISTERED, record.capability_id, record.latest_payload.timestamp_ms);
    return true;
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
    const int8_t index = findCapabilityIndex(capability_id);
    if (index == NOT_FOUND) {
        return false;
    }

    out_payload = capabilities_[index].latest_payload;
    return true;
}

bool Registry::updateCapabilityPayload(const char* capability_id, const CapabilityPayload& payload) {
    const int8_t index = findCapabilityIndex(capability_id);
    if (index == NOT_FOUND) {
        return false;
    }

    capabilities_[index].latest_payload = payload;
    publishCapabilityEvent(EVT_CAPABILITY_VALUE_UPDATED, capability_id, payload.timestamp_ms);
    return true;
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
