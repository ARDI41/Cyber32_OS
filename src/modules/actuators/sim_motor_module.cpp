#include "sim_motor_module.h"

#include "../../core/ids/capability_ids.h"

namespace Cyber32 {

const char* const SimMotorModule::MODULE_ID = "module-sim-motor-001";
const char* const SimMotorModule::MODULE_TYPE = "actuator";
const uint8_t SimMotorModule::METADATA_LEVEL = 1;
const char* const SimMotorModule::DISPLAY_NAME = "Simulated Motor Module";

const char* SimMotorModule::id() const {
    return MODULE_ID;
}

const char* SimMotorModule::type() const {
    return MODULE_TYPE;
}

uint8_t SimMotorModule::metadataLevel() const {
    return METADATA_LEVEL;
}

const char* SimMotorModule::displayName() const {
    return DISPLAY_NAME;
}

const char* SimMotorModule::deviceId() const {
    return SimMotorDevice::DEVICE_ID;
}

const char* SimMotorModule::deviceType() const {
    return SimMotorDevice::DEVICE_TYPE;
}

const char* SimMotorModule::capabilityId() const {
    return CAP_MOTOR_CONTROL;
}

}  // namespace Cyber32
