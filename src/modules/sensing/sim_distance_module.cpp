#include "sim_distance_module.h"

#include "../../core/ids/capability_ids.h"

namespace Cyber32 {

const char* const SimDistanceModule::MODULE_ID = "module-sim-distance-001";
const char* const SimDistanceModule::MODULE_TYPE = "sensing";
const uint8_t SimDistanceModule::METADATA_LEVEL = 1;
const char* const SimDistanceModule::DISPLAY_NAME = "Simulated Distance Module";

const char* SimDistanceModule::id() const {
    return MODULE_ID;
}

const char* SimDistanceModule::type() const {
    return MODULE_TYPE;
}

uint8_t SimDistanceModule::metadataLevel() const {
    return METADATA_LEVEL;
}

const char* SimDistanceModule::displayName() const {
    return DISPLAY_NAME;
}

const char* SimDistanceModule::deviceId() const {
    return SimDistanceDevice::DEVICE_ID;
}

const char* SimDistanceModule::deviceType() const {
    return SimDistanceDevice::DEVICE_TYPE;
}

const char* SimDistanceModule::capabilityId() const {
    return CAP_DISTANCE;
}

}  // namespace Cyber32
