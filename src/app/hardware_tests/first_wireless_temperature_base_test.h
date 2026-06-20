#pragma once

#include <stdint.h>

#include "../../api/cyber32_api.h"
#include "../../devices/communication/wireless_temperature_device.h"
#include "../../drivers/communication/espnow_transport_driver.h"
#include "../../registry/registry.h"
#include "../../services/wireless/wireless_service.h"

namespace Cyber32 {

class FirstWirelessTemperatureBaseTest {
public:
    FirstWirelessTemperatureBaseTest();

    bool begin();
    bool run(uint32_t now_ms);

private:
    Registry registry_;
    EspNowTransportDriver espnow_transport_;
    WirelessTemperatureDevice wireless_temperature_device_;
    WirelessService wireless_service_;
    Cyber32Api api_;
    bool initialized_;

    bool registerAllowlistRecord();
    bool registerWirelessProvider();
    bool registerSecurityDiagnostic();
};

}  // namespace Cyber32
