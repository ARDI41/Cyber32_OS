#pragma once

#include <stdint.h>

#include "../../core/types/motor_types.h"
#include "../../drivers/actuators/sim_motor_driver.h"
#include "../../registry/registry_records.h"

namespace Cyber32 {

class SimMotorDevice {
public:
    static const char* const DEVICE_ID;
    static const char* const DEVICE_TYPE;

    SimMotorDevice();

    bool begin(SimMotorDriver* driver);
    bool readPayload(uint32_t now_ms, CapabilityPayload& out_payload);
    bool setMotor(uint32_t now_ms, MotorDirection direction, float speed_percent, CapabilityPayload& out_payload);
    bool stop(uint32_t now_ms, CapabilityPayload& out_payload);
    const char* id() const;
    const char* type() const;

private:
    SimMotorDriver* driver_;

    void fillBasePayload(uint32_t now_ms, CapabilityPayload& out_payload) const;
};

}  // namespace Cyber32
