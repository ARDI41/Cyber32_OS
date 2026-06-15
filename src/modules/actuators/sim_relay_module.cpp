#include "sim_relay_module.h"

#include "../../core/ids/capability_ids.h"

namespace Cyber32 {

const char* const SimRelayModule::MODULE_ID = "module-sim-relay-001";
const char* const SimRelayModule::MODULE_TYPE = "actuator";
const uint8_t SimRelayModule::METADATA_LEVEL = 1;
const char* const SimRelayModule::DISPLAY_NAME = "Simulated Relay Module";

const char* SimRelayModule::id() const {
    return MODULE_ID;
}

const char* SimRelayModule::type() const {
    return MODULE_TYPE;
}

uint8_t SimRelayModule::metadataLevel() const {
    return METADATA_LEVEL;
}

const char* SimRelayModule::displayName() const {
    return DISPLAY_NAME;
}

const char* SimRelayModule::deviceId() const {
    return SimRelayDevice::DEVICE_ID;
}

const char* SimRelayModule::deviceType() const {
    return SimRelayDevice::DEVICE_TYPE;
}

const char* SimRelayModule::capabilityId() const {
    return CAP_RELAY_CONTROL;
}

}  // namespace Cyber32
