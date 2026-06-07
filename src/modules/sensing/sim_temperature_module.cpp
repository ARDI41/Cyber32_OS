#include "sim_temperature_module.h"

#include "../../core/ids/capability_ids.h"

namespace Cyber32 {

const char* const SimTemperatureModule::MODULE_ID = "module-sim-temperature-001";
const char* const SimTemperatureModule::MODULE_TYPE = "sensing";
const uint8_t SimTemperatureModule::METADATA_LEVEL = 1;
const char* const SimTemperatureModule::DISPLAY_NAME = "Simulated Temperature Module";

const char* SimTemperatureModule::id() const {
    return MODULE_ID;
}

const char* SimTemperatureModule::type() const {
    return MODULE_TYPE;
}

uint8_t SimTemperatureModule::metadataLevel() const {
    return METADATA_LEVEL;
}

const char* SimTemperatureModule::displayName() const {
    return DISPLAY_NAME;
}

const char* SimTemperatureModule::deviceId() const {
    return SimTemperatureDevice::DEVICE_ID;
}

const char* SimTemperatureModule::deviceType() const {
    return SimTemperatureDevice::DEVICE_TYPE;
}

const char* SimTemperatureModule::capabilityId() const {
    return CAP_TEMPERATURE;
}

}  // namespace Cyber32
