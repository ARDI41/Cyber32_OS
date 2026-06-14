#include "sim_servo_module.h"

#include "../../core/ids/capability_ids.h"

namespace Cyber32 {

const char* const SimServoModule::MODULE_ID = "module-sim-servo-001";
const char* const SimServoModule::MODULE_TYPE = "actuator";
const uint8_t SimServoModule::METADATA_LEVEL = 1;
const char* const SimServoModule::DISPLAY_NAME = "Simulated Servo Module";

const char* SimServoModule::id() const {
    return MODULE_ID;
}

const char* SimServoModule::type() const {
    return MODULE_TYPE;
}

uint8_t SimServoModule::metadataLevel() const {
    return METADATA_LEVEL;
}

const char* SimServoModule::displayName() const {
    return DISPLAY_NAME;
}

const char* SimServoModule::deviceId() const {
    return SimServoDevice::DEVICE_ID;
}

const char* SimServoModule::deviceType() const {
    return SimServoDevice::DEVICE_TYPE;
}

const char* SimServoModule::capabilityId() const {
    return CAP_SERVO_POSITION;
}

}  // namespace Cyber32
