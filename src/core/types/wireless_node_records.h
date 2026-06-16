#pragma once

#include <stdint.h>

#include "wireless_packet_types.h"

namespace Cyber32 {

enum class WirelessNodeStatus : unsigned char {
    UNKNOWN,
    AVAILABLE,
    STALE,
    UNAVAILABLE,
    LOST
};

enum class WirelessTrustState : unsigned char {
    UNKNOWN,
    TRUSTED,
    UNTRUSTED,
    BLOCKED
};

struct WirelessNodeRecord {
    uint32_t node_id;
    WirelessNodeStatus status;
    WirelessTrustState trust_state;
    uint32_t last_seen_ms;
    uint32_t last_sequence_id;
    uint8_t missed_heartbeat_count;
    bool battery_present;
    float battery_level_percent;
    float battery_voltage;
    uint8_t signal_quality_percent;
};

struct WirelessCapabilityRecord {
    uint32_t node_id;
    const char* capability_id;
    WirelessPayloadType payload_type;
    bool available;
};

}  // namespace Cyber32
