#pragma once

#include <stdint.h>

#include "../../devices/communication/wireless_temperature_device.h"
#include "../../drivers/communication/sim_espnow_transport_driver.h"
#include "../../registry/registry.h"
#include "wireless_packet_transport.h"

namespace Cyber32 {

class WirelessService {
public:
    static const char* const WIRELESS_TEMPERATURE_PROVIDER_ID;

    WirelessService();

    void begin();
    void attachRegistry(Registry* registry);
    void attachTransportDriver(SimEspNowTransportDriver* transport);
    bool attachTransportAdapter(const WirelessPacketTransportAdapter& adapter);
    void attachWirelessTemperatureDevice(WirelessTemperatureDevice* device);
    bool processPackets(uint32_t now_ms);
    bool checkTimeouts(uint32_t now_ms);
    bool lastProcessResult() const;
    const char* lastErrorCode() const;

private:
    Registry* registry_;
    SimEspNowTransportDriver* transport_;
    WirelessPacketTransportAdapter transport_adapter_;
    WirelessTemperatureDevice* temperature_device_;
    bool last_process_result_;
    const char* last_error_code_;

    bool hasRequiredAttachments() const;
};

}  // namespace Cyber32
