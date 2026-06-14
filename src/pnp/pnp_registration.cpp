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
    RegistryWriteResult module_result = registry_->registerModuleWithResult(module_record);
    if (module_result.result != RegistryResult::OK) {
        return false;
    }

    DeviceRecord device_record = createDeviceRecord(info, module_result.index);
    RegistryWriteResult device_result = registry_->registerDeviceWithResult(device_record);
    if (device_result.result != RegistryResult::OK) {
        return false;
    }

    CapabilityRecord capability_record = createCapabilityRecord(info.capability_id, device_result.index);
    RegistryWriteResult capability_result = registry_->registerCapabilityWithResult(capability_record);
    return capability_result.result == RegistryResult::OK;
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
    if (isSameId(capability_id, CAP_DISTANCE)) {
        payload.unit = "meter";
    } else if (isSameId(capability_id, CAP_SERVO_POSITION)) {
        payload.unit = "degree";
    } else {
        payload.unit = "degree_celsius";
    }
    payload.quality = "stale";
    payload.error_code = "none";
    return payload;
}

CapabilityRecord PnpRegistration::createCapabilityRecord(
    const char* capability_id,
    uint8_t owner_device_index) const {
    CapabilityRecord record;
    record.capability_id = capability_id;
    if (isSameId(capability_id, CAP_SERVO_POSITION)) {
        record.category = "actuators";
        record.kind = "actuator";
        record.access = "read_write";
    } else {
        record.category = "sensors";
        record.kind = "sensor";
        record.access = "read";
    }
    record.data_type = PayloadValueType::FLOAT;
    record.status = RecordStatus::AVAILABLE;
    record.owner_device_index = owner_device_index;
    record.latest_payload = createInitialPayload(capability_id);
    return record;
}

bool PnpRegistration::isSupportedCapability(const char* capability_id) const {
    return isSameId(capability_id, CAP_TEMPERATURE) ||
           isSameId(capability_id, CAP_DISTANCE) ||
           isSameId(capability_id, CAP_SERVO_POSITION);
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
