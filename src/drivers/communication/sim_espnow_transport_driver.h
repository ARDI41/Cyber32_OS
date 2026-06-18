#pragma once

#include "../../core/types/wireless_node_allowlist_records.h"
#include "../../core/types/wireless_packet_types.h"

namespace Cyber32 {

class SimEspNowTransportDriver {
public:
    SimEspNowTransportDriver();

    void begin();
    bool initialized() const;
    void setFailureMode(bool enabled);
    bool injectReceivedCapabilityValue(
        const WirelessPacketHeader& header,
        const WirelessCapabilityValue& value,
        const WirelessNodeDiagnostics& diagnostics);
    bool injectReceivedCapabilityValueWithMac(
        const uint8_t source_mac[WIRELESS_MAC_ADDRESS_SIZE],
        const WirelessPacketHeader& header,
        const WirelessCapabilityValue& value,
        const WirelessNodeDiagnostics& diagnostics);
    bool hasReceivedPacket() const;
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
    void clearReceivedPacket();

private:
    bool initialized_;
    bool failure_mode_;
    bool has_received_packet_;
    WirelessPacketHeader pending_header_;
    WirelessCapabilityValue pending_value_;
    WirelessNodeDiagnostics pending_diagnostics_;
    uint8_t pending_source_mac_[WIRELESS_MAC_ADDRESS_SIZE];
    bool pending_has_source_mac_;
};

}  // namespace Cyber32
