#pragma once

#include <stdint.h>

#include "../../devices/actuators/sim_servo_device.h"

namespace Cyber32 {

class SimServoModule {
public:
    static const char* const MODULE_ID;
    static const char* const MODULE_TYPE;
    static const uint8_t METADATA_LEVEL;
    static const char* const DISPLAY_NAME;

    const char* id() const;
    const char* type() const;
    uint8_t metadataLevel() const;
    const char* displayName() const;
    const char* deviceId() const;
    const char* deviceType() const;
    const char* capabilityId() const;
};

}  // namespace Cyber32
