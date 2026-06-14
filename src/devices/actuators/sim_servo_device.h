#pragma once

#include <stdint.h>

#include "../../drivers/actuators/sim_servo_driver.h"
#include "../../registry/registry_records.h"

namespace Cyber32 {

class SimServoDevice {
public:
    static const char* const DEVICE_ID;
    static const char* const DEVICE_TYPE;

    SimServoDevice();

    bool begin(SimServoDriver* driver);
    bool readPayload(uint32_t now_ms, CapabilityPayload& out_payload);
    bool setPosition(uint32_t now_ms, float position_degrees, CapabilityPayload& out_payload);
    const char* id() const;
    const char* type() const;

private:
    SimServoDriver* driver_;

    void fillBasePayload(uint32_t now_ms, CapabilityPayload& out_payload) const;
};

}  // namespace Cyber32
