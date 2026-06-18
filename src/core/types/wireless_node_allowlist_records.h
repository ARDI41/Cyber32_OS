#pragma once

#include <stdint.h>

#include "wireless_node_records.h"

namespace Cyber32 {

enum class WirelessNodeAllowState : unsigned char {
    UNKNOWN,
    ALLOWED,
    BLOCKED
};

struct WirelessNodeAllowlistRecord {
    uint32_t node_id;
    WirelessNodeAllowState allow_state;
    WirelessTrustState trust_state;
    uint32_t added_at_ms;
    uint32_t last_seen_ms;
};

}  // namespace Cyber32
