#pragma once

#include <stdint.h>

#include "../../core/types/wireless_node_records.h"
#include "../../core/types/wireless_packet_types.h"
#include "../../registry/registry_records.h"

namespace Cyber32 {

class WirelessTemperatureDevice {
public:
    static const char* const DEVICE_ID;
    static const char* const DEVICE_TYPE;

    WirelessTemperatureDevice();

    bool begin(uint32_t node_id);
    bool updateFromPacket(
        uint32_t now_ms,
        const WirelessPacketHeader& header,
        const WirelessCapabilityValue& value,
        const WirelessNodeDiagnostics& diagnostics);
    bool readPayload(CapabilityPayload& out_payload) const;
    bool nodeRecord(WirelessNodeRecord& out_record) const;
    void setTrustState(WirelessTrustState state);
    WirelessTrustState trustState() const;
    bool sequenceAccepted(uint32_t sequence_id) const;
    void markSequenceAccepted(uint32_t sequence_id);
    const char* id() const;
    const char* type() const;

private:
    bool initialized_;
    bool has_sequence_id_;
    CapabilityPayload latest_payload_;
    WirelessNodeRecord node_record_;

    void fillUnavailablePayload(uint32_t now_ms, const char* error_code);
    bool isValidTemperaturePacket(
        const WirelessPacketHeader& header,
        const WirelessCapabilityValue& value) const;
    bool isSameText(const char* left, const char* right) const;
    uint8_t signalQualityToUint8(float signal_quality_percent) const;
};

}  // namespace Cyber32
