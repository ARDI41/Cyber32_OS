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

    ModuleRecord module_record = createModuleRecord(info);
    if (!registry_->registerModule(module_record)) {
        return false;
    }

    DeviceRecord device_record = createDeviceRecord(info);
    if (!registry_->registerDevice(device_record)) {
        return false;
    }

    CapabilityRecord capability_record = createTemperatureCapabilityRecord();
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

DeviceRecord PnpRegistration::createDeviceRecord(const PnpModuleInfo& info) const {
    DeviceRecord record;
    record.device_id = info.device_id;
    record.device_type = info.device_type;
    record.status = RecordStatus::AVAILABLE;
    record.module_index = 0;
    record.capability_count = 1;
    return record;
}

CapabilityPayload PnpRegistration::createInitialTemperaturePayload() const {
    CapabilityPayload payload;
    payload.capability_id = CAP_TEMPERATURE;
    payload.schema_version = 1;
    payload.timestamp_ms = 0;
    payload.available = Availability::AVAILABLE;
    payload.stale = StaleState::STALE;
    payload.value_type = PayloadValueType::NONE;
    payload.value_float = 0.0F;
    payload.value_int = 0;
    payload.unit = "degree_celsius";
    payload.quality = "stale";
    payload.error_code = "none";
    return payload;
}

CapabilityRecord PnpRegistration::createTemperatureCapabilityRecord() const {
    CapabilityRecord record;
    record.capability_id = CAP_TEMPERATURE;
    record.category = "sensors";
    record.kind = "sensor";
    record.data_type = PayloadValueType::FLOAT;
    record.access = "read";
    record.status = RecordStatus::AVAILABLE;
    record.owner_device_index = 0;
    record.latest_payload = createInitialTemperaturePayload();
    return record;
}

}  // namespace Cyber32
