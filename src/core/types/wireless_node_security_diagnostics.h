#pragma once

#include <stdint.h>

#include "wireless_node_allowlist_records.h"

namespace Cyber32 {

enum class WirelessSecurityRejectReason : unsigned char {
    NONE,
    CHECKSUM_INVALID,
    MAC_NOT_ALLOWED,
    MAC_NODE_MISMATCH,
    NODE_BLOCKED,
    NODE_NOT_ALLOWED,
    UNTRUSTED,
    DUPLICATE_SEQUENCE,
    INVALID_PACKET,
    DEVICE_UPDATE_FAILED,
    PROVIDER_UPDATE_FAILED
};

struct WirelessNodeSecurityDiagnosticRecord {
    uint32_t node_id;
    uint8_t mac_address[WIRELESS_MAC_ADDRESS_SIZE];
    bool has_mac_address;
    WirelessNodeAllowState allow_state;
    WirelessTrustState trust_state;
    uint32_t last_seen_ms;
    uint32_t last_accepted_sequence_id;
    uint32_t last_rejected_sequence_id;
    const char* last_error_code;
    uint16_t checksum_reject_count;
    uint16_t mac_not_allowed_reject_count;
    uint16_t mac_node_mismatch_reject_count;
    uint16_t blocked_reject_count;
    uint16_t not_allowed_reject_count;
    uint16_t untrusted_reject_count;
    uint16_t duplicate_sequence_reject_count;
    uint16_t invalid_packet_reject_count;
    uint16_t accepted_packet_count;
};

inline uint16_t incrementSaturatingUint16(uint16_t value) {
    if (value == 65535U) {
        return 65535U;
    }

    return static_cast<uint16_t>(value + 1U);
}

}  // namespace Cyber32
