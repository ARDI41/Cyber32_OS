#pragma once

#include "../../core/types/wireless_node_allowlist_records.h"
#include "../../core/types/wireless_packet_types.h"

namespace Cyber32 {

class EspNowTransportDriver {
public:
    EspNowTransportDriver();

    bool begin();
    bool initialized() const;
    bool hasReceivedPacket() const;
    void clearReceivedPacket();
    bool readReceivedPacket(
        WirelessPacketHeader& out_header,
        WirelessCapabilityValue& out_value,
        WirelessNodeDiagnostics& out_diagnostics);
    bool readReceivedPacket(
        WirelessPacketHeader& out_header,
        WirelessCapabilityValue& out_value,
        WirelessNodeDiagnostics& out_diagnostics,
        uint8_t out_source_mac[WIRELESS_MAC_ADDRESS_SIZE],
        bool& out_has_source_mac);
    bool callbackReceived() const;
    uint16_t lastReceivedLength() const;

private:
    static void receiveCallback(const uint8_t* source_mac, const uint8_t* data, int data_len);

    void clearCallbackState();
    void recordReceiveCallback(const uint8_t* source_mac, int data_len);

    bool initialized_;
    bool pending_received_packet_;
    WirelessPacketHeader pending_header_;
    WirelessCapabilityValue pending_value_;
    WirelessNodeDiagnostics pending_diagnostics_;
    uint8_t pending_source_mac_[WIRELESS_MAC_ADDRESS_SIZE];
    bool pending_has_source_mac_;
    bool callback_received_;
    uint16_t last_received_length_;
    uint8_t last_source_mac_[WIRELESS_MAC_ADDRESS_SIZE];
};

}  // namespace Cyber32
