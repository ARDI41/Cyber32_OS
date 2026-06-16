#pragma once

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
    bool hasReceivedPacket() const;
    bool readReceivedPacket(
        WirelessPacketHeader& out_header,
        WirelessCapabilityValue& out_value,
        WirelessNodeDiagnostics& out_diagnostics);
    void clearReceivedPacket();

private:
    bool initialized_;
    bool failure_mode_;
    bool has_received_packet_;
    WirelessPacketHeader pending_header_;
    WirelessCapabilityValue pending_value_;
    WirelessNodeDiagnostics pending_diagnostics_;
};

}  // namespace Cyber32
