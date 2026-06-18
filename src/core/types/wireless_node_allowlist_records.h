#pragma once

#include <stdint.h>

#include "wireless_node_records.h"

namespace Cyber32 {

static const uint8_t WIRELESS_MAC_ADDRESS_SIZE = 6;

enum class WirelessNodeAllowState : unsigned char {
    UNKNOWN,
    ALLOWED,
    BLOCKED
};

struct WirelessNodeAllowlistRecord {
    uint32_t node_id;
    uint8_t mac_address[WIRELESS_MAC_ADDRESS_SIZE];
    bool has_mac_address;
    WirelessNodeAllowState allow_state;
    WirelessTrustState trust_state;
    uint32_t added_at_ms;
    uint32_t last_seen_ms;
};

inline bool wirelessMacAddressEquals(
    const uint8_t lhs[WIRELESS_MAC_ADDRESS_SIZE],
    const uint8_t rhs[WIRELESS_MAC_ADDRESS_SIZE]) {
    for (uint8_t i = 0; i < WIRELESS_MAC_ADDRESS_SIZE; ++i) {
        if (lhs[i] != rhs[i]) {
            return false;
        }
    }

    return true;
}

inline void clearWirelessMacAddress(uint8_t mac[WIRELESS_MAC_ADDRESS_SIZE]) {
    for (uint8_t i = 0; i < WIRELESS_MAC_ADDRESS_SIZE; ++i) {
        mac[i] = 0;
    }
}

}  // namespace Cyber32
