#include "pnp_registration.h"

#include "../core/ids/capability_ids.h"

namespace Cyber32 {

PnpRegistration::PnpRegistration()
    : registry_(0) {
}

bool PnpRegistration::begin(Registry* registry) {
    if (registry == 0) {
        return false;
    }

    registry_ = registry;
    return true;
}

bool PnpRegistration::registerModuleInfo(const PnpModuleInfo& info) {
    if (registry_ == 0) {
        return false;
    }
    if (!info.valid) {
        return false;
    }
    if (!isSupportedCapability(info.capability_id)) {
        return false;
    }

    ModuleRecord module_record = createModuleRecord(info);
    if (!registry_->registerModule(module_record)) {
        return false;
    }

    const uint8_t registered_module_index = registry_->moduleCount() - 1;

    DeviceRecord device_record = createDeviceRecord(info, registered_module_index);
    if (!registry_->registerDevice(device_record)) {
        return false;
    }

    // TODO: Replace count-derived owner index with an explicit Registry registration result before multi-provider v1.
    const uint8_t registered_device_index = registry_->deviceCount() - 1;

    CapabilityRecord capability_record = createCapabilityRecord(info.capability_id, registered_device_index);
    return registry_->registerCapability(capability_record);
}

ModuleRecord PnpRegistration::createModuleRecord(const PnpModuleInfo& info) const {
    ModuleRecord record;
    record.module_id = info.module_id;
    record.module_type = info.module_type;
    record.status = RecordStatus::AVAILABLE;
    record.device_count = 1;
    record.capability_count = 1;
    return record;
}

DeviceRecord PnpRegistration::createDeviceRecord(const PnpModuleInfo& info, uint8_t module_index) const {
    DeviceRecord record;
    record.device_id = info.device_id;
    record.device_type = info.device_type;
    record.status = RecordStatus::AVAILABLE;
    record.module_index = module_index;
    record.capability_count = 1;
    return record;
}

CapabilityPayload PnpRegistration::createInitialPayload(const char* capability_id) const {
    CapabilityPayload payload;
    payload.capability_id = capability_id;
    payload.schema_version = 1;
    payload.timestamp_ms = 0;
    payload.available = Availability::AVAILABLE;
    payload.stale = StaleState::STALE;
    payload.value_type = PayloadValueType::NONE;
    payload.value_float = 0.0F;
    payload.value_int = 0;
    payload.unit = isSameId(capability_id, CAP_DISTANCE) ? "meter" : "degree_celsius";
    payload.quality = "stale";
    payload.error_code = "none";
    return payload;
}

CapabilityRecord PnpRegistration::createCapabilityRecord(
    const char* capability_id,
    uint8_t owner_device_index) const {
    CapabilityRecord record;
    record.capability_id = capability_id;
    record.category = "sensors";
    record.kind = "sensor";
    record.data_type = PayloadValueType::FLOAT;
    record.access = "read";
    record.status = RecordStatus::AVAILABLE;
    record.owner_device_index = owner_device_index;
    record.latest_payload = createInitialPayload(capability_id);
    return record;
}

bool PnpRegistration::isSupportedCapability(const char* capability_id) const {
    return isSameId(capability_id, CAP_TEMPERATURE) ||
           isSameId(capability_id, CAP_DISTANCE);
}

bool PnpRegistration::isSameId(const char* left, const char* right) const {
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

}  // namespace Cyber32
