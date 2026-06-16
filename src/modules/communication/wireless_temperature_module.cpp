#include "wireless_temperature_module.h"

#include "../../core/ids/capability_ids.h"

namespace Cyber32 {

const char* const WirelessTemperatureModule::MODULE_ID = "module-wireless-temperature-001";
const char* const WirelessTemperatureModule::MODULE_TYPE = "wireless_node";
const uint8_t WirelessTemperatureModule::METADATA_LEVEL = 1;
const char* const WirelessTemperatureModule::DISPLAY_NAME = "Wireless Temperature Node";
const char* const WirelessTemperatureModule::TRANSPORT = "espnow";

const char* WirelessTemperatureModule::id() const {
    return MODULE_ID;
}

const char* WirelessTemperatureModule::type() const {
    return MODULE_TYPE;
}

uint8_t WirelessTemperatureModule::metadataLevel() const {
    return METADATA_LEVEL;
}

const char* WirelessTemperatureModule::displayName() const {
    return DISPLAY_NAME;
}

const char* WirelessTemperatureModule::deviceId() const {
    return WirelessTemperatureDevice::DEVICE_ID;
}

const char* WirelessTemperatureModule::deviceType() const {
    return WirelessTemperatureDevice::DEVICE_TYPE;
}

const char* WirelessTemperatureModule::capabilityId() const {
    return CAP_TEMPERATURE;
}

const char* WirelessTemperatureModule::transport() const {
    return TRANSPORT;
}

}  // namespace Cyber32
