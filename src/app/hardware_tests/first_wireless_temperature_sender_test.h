#pragma once

#include <stdint.h>

#include "../../core/types/wireless_node_allowlist_records.h"
#include "../../core/types/wireless_packet_types.h"

namespace Cyber32 {

class FirstWirelessTemperatureSenderTest {
public:
    FirstWirelessTemperatureSenderTest();

    bool begin();
    bool sendTemperature(float temperature, uint32_t sequence_id);

private:
    uint8_t base_mac_[WIRELESS_MAC_ADDRESS_SIZE];
    bool initialized_;

    void fillKnownBaseMac();
    void copyBoundedText(char* destination, uint8_t destination_size, const char* source) const;
    bool packTemperaturePacket(
        float temperature,
        uint32_t sequence_id,
        uint8_t out_buffer[WIRELESS_MAX_PACKET_SIZE],
        uint16_t& out_length) const;
};

}  // namespace Cyber32
