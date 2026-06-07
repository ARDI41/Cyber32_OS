#pragma once

#include <stdint.h>

#include "../../drivers/sensors/sim_temperature_driver.h"
#include "../../registry/registry_records.h"

namespace Cyber32 {

class SimTemperatureDevice {
public:
    static const char* const DEVICE_ID;
    static const char* const DEVICE_TYPE;

    SimTemperatureDevice();

    bool begin(SimTemperatureDriver* driver);
    bool readPayload(uint32_t now_ms, CapabilityPayload& out_payload);
    const char* id() const;
    const char* type() const;

private:
    SimTemperatureDriver* driver_;

    void fillBasePayload(uint32_t now_ms, CapabilityPayload& out_payload) const;
};

}  // namespace Cyber32
