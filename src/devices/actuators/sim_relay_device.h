#pragma once

#include <stdint.h>

#include "../../drivers/actuators/sim_relay_driver.h"
#include "../../registry/registry_records.h"

namespace Cyber32 {

class SimRelayDevice {
public:
    static const char* const DEVICE_ID;
    static const char* const DEVICE_TYPE;

    SimRelayDevice();

    bool begin(SimRelayDriver* driver);
    bool readPayload(uint32_t now_ms, CapabilityPayload& out_payload);
    bool setEnabled(uint32_t now_ms, bool enabled, CapabilityPayload& out_payload);
    bool disable(uint32_t now_ms, CapabilityPayload& out_payload);
    const char* id() const;
    const char* type() const;

private:
    SimRelayDriver* driver_;

    void fillUnavailablePayload(
        uint32_t now_ms,
        CapabilityPayload& out_payload,
        const char* error_code) const;
};

}  // namespace Cyber32
