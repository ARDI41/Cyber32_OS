#include "pnp_discovery.h"

#include "../core/ids/event_ids.h"
#include "../modules/sensing/sim_distance_module.h"
#include "../modules/sensing/sim_temperature_module.h"

namespace Cyber32 {

PnpDiscovery::PnpDiscovery()
    : event_bus_(0) {
}

void PnpDiscovery::attachEventBus(EventBus* bus) {
    event_bus_ = bus;
}

bool PnpDiscovery::discoverSimulatedTemperatureModule(PnpModuleInfo& out_info) {
    SimTemperatureModule module;

    out_info.module_id = module.id();
    out_info.module_type = module.type();
    out_info.metadata_level = module.metadataLevel();
    out_info.display_name = module.displayName();
    out_info.device_id = module.deviceId();
    out_info.device_type = module.deviceType();
    out_info.capability_id = module.capabilityId();
    out_info.valid = false;

    if (!validateInfo(out_info)) {
        return false;
    }

    out_info.valid = true;
    publishModuleDiscovered(out_info);
    return true;
}

bool PnpDiscovery::discoverSimulatedDistanceModule(PnpModuleInfo& out_info) {
    SimDistanceModule module;

    out_info.module_id = module.id();
    out_info.module_type = module.type();
    out_info.metadata_level = module.metadataLevel();
    out_info.display_name = module.displayName();
    out_info.device_id = module.deviceId();
    out_info.device_type = module.deviceType();
    out_info.capability_id = module.capabilityId();
    out_info.valid = false;

    if (!validateInfo(out_info)) {
        return false;
    }

    out_info.valid = true;
    publishModuleDiscovered(out_info);
    return true;
}

bool PnpDiscovery::validateInfo(const PnpModuleInfo& info) const {
    if (!isRequiredTextPresent(info.module_id)) {
        return false;
    }
    if (!isRequiredTextPresent(info.module_type)) {
        return false;
    }
    if (!isRequiredTextPresent(info.display_name)) {
        return false;
    }
    if (!isRequiredTextPresent(info.device_id)) {
        return false;
    }
    if (!isRequiredTextPresent(info.device_type)) {
        return false;
    }
    if (!isRequiredTextPresent(info.capability_id)) {
        return false;
    }
    if (info.metadata_level != 1) {
        return false;
    }
    return startsWithCapPrefix(info.capability_id);
}

bool PnpDiscovery::isRequiredTextPresent(const char* value) const {
    return value != 0 && value[0] != '\0';
}

bool PnpDiscovery::startsWithCapPrefix(const char* value) const {
    return value != 0 &&
           value[0] == 'C' &&
           value[1] == 'A' &&
           value[2] == 'P' &&
           value[3] == '_';
}

void PnpDiscovery::publishModuleDiscovered(const PnpModuleInfo& info) const {
    if (event_bus_ == 0) {
        return;
    }

    EventRecord event;
    event.event_id = EVT_MODULE_DISCOVERED;
    event.priority = EventPriority::NORMAL;
    event.timestamp_ms = 0;
    event.source_layer = "PNP";
    event.source_id = info.module_id;
    event.target_id = info.capability_id;
    event.value_type = 0;
    event.value_float = 0.0F;
    event.value_int = static_cast<int32_t>(info.metadata_level);

    event_bus_->publish(event);
}

}  // namespace Cyber32
