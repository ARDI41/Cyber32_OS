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

private:
    bool initialized_;
    bool pending_received_packet_;
    WirelessPacketHeader pending_header_;
    WirelessCapabilityValue pending_value_;
    WirelessNodeDiagnostics pending_diagnostics_;
    uint8_t pending_source_mac_[WIRELESS_MAC_ADDRESS_SIZE];
    bool pending_has_source_mac_;
};

}  // namespace Cyber32
