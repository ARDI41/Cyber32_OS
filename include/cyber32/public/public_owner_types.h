#pragma once

#include <stdint.h>

// Reuse the existing small payload type enum without pulling Arduino,
// Registry, transport, driver, device, service, or HAL dependencies.
#include "../../../src/core/types/payload_types.h"

namespace Cyber32 {

typedef uint32_t PublicNodeId;
typedef uint32_t PublicCapabilityInstanceId;
typedef uint8_t PublicNodeIndex;
typedef uint8_t PublicCapabilityIndex;
typedef uint32_t PublicMappingLinkId;
typedef uint32_t PublicProviderId;

enum class PublicLifecycleState : unsigned char {
    NONE,
    PENDING,
    AVAILABLE,
    UNAVAILABLE,
    STALE,
    DISABLED,
    BLOCKED,
    REJECTED,
    ERROR
};

enum class PublicVisibilityState : unsigned char {
    NONE,
    HIDDEN,
    PENDING,
    PUBLIC,
    DIAGNOSTIC_ONLY
};

enum class PublicTrustState : unsigned char {
    UNKNOWN,
    UNPAIRED,
    PAIRED,
    TRUSTED,
    BLOCKED,
    REJECTED
};

enum class PublicAvailabilityState : unsigned char {
    UNKNOWN,
    AVAILABLE,
    UNAVAILABLE,
    UNSUPPORTED
};

enum class PublicFreshnessState : unsigned char {
    UNKNOWN,
    FRESH,
    STALE
};

enum class PublicHealthState : unsigned char {
    UNKNOWN,
    HEALTHY,
    UNHEALTHY,
    UNSUPPORTED
};

enum class PublicUnavailableReason : unsigned char {
    NONE,
    UNKNOWN,
    NO_OWNER,
    NOT_UPDATED_YET,
    STALE,
    UNSUPPORTED,
    BLOCKED,
    INTERNAL_ERROR
};

struct PublicNodeRecord {
    PublicNodeId node_id;
    uint8_t source_type;
    PublicLifecycleState lifecycle_state;
    PublicVisibilityState visibility_state;
    PublicTrustState trust_state;
    uint8_t capability_count;
    PublicFreshnessState freshness_state;
    bool diagnostics_available;
    uint32_t last_seen_ms;
    bool valid;

    PublicNodeRecord() {
        reset();
    }

    void reset() {
        node_id = 0;
        source_type = 0;
        lifecycle_state = PublicLifecycleState::NONE;
        visibility_state = PublicVisibilityState::NONE;
        trust_state = PublicTrustState::UNKNOWN;
        capability_count = 0;
        freshness_state = PublicFreshnessState::UNKNOWN;
        diagnostics_available = false;
        last_seen_ms = 0;
        valid = false;
    }
};

struct PublicCapabilityRecord {
    uint32_t capability_id;
    PublicCapabilityInstanceId capability_instance_id;
    PublicNodeId owner_node_id;
    uint8_t category;
    PublicLifecycleState lifecycle_state;
    PublicVisibilityState visibility_state;
    PublicAvailabilityState value_availability_state;
    PublicFreshnessState freshness_state;
    bool provider_available;
    bool diagnostics_available;
    bool valid;

    PublicCapabilityRecord() {
        reset();
    }

    void reset() {
        capability_id = 0;
        capability_instance_id = 0;
        owner_node_id = 0;
        category = 0;
        lifecycle_state = PublicLifecycleState::NONE;
        visibility_state = PublicVisibilityState::NONE;
        value_availability_state = PublicAvailabilityState::UNKNOWN;
        freshness_state = PublicFreshnessState::UNKNOWN;
        provider_available = false;
        diagnostics_available = false;
        valid = false;
    }
};

struct PublicNodeCapabilityLink {
    PublicMappingLinkId link_id;
    PublicNodeId node_id;
    PublicCapabilityInstanceId capability_instance_id;
    PublicVisibilityState link_visibility_state;
    PublicFreshnessState link_freshness_state;
    bool diagnostics_available;
    uint8_t display_order;
    bool active;

    PublicNodeCapabilityLink() {
        reset();
    }

    void reset() {
        link_id = 0;
        node_id = 0;
        capability_instance_id = 0;
        link_visibility_state = PublicVisibilityState::NONE;
        link_freshness_state = PublicFreshnessState::UNKNOWN;
        diagnostics_available = false;
        display_order = 0;
        active = false;
    }
};

struct PublicCapabilityValueSnapshot {
    PublicCapabilityInstanceId capability_instance_id;
    PayloadValueType value_type;
    bool value_valid;
    bool value_available;
    bool value_stale;
    PublicUnavailableReason unavailable_reason;
    float numeric_value;
    bool bool_value;
    uint32_t last_update_ms;
    uint8_t unit_metadata_ref;
    uint8_t quality_ref;

    PublicCapabilityValueSnapshot() {
        reset();
    }

    void reset() {
        capability_instance_id = 0;
        value_type = PayloadValueType::NONE;
        value_valid = false;
        value_available = false;
        value_stale = false;
        unavailable_reason = PublicUnavailableReason::NO_OWNER;
        numeric_value = 0.0F;
        bool_value = false;
        last_update_ms = 0;
        unit_metadata_ref = 0;
        quality_ref = 0;
    }
};

struct PublicNodeDiagnosticsSnapshot {
    PublicNodeId node_id;
    bool diagnostics_available;
    bool health_known;
    bool health_ok;
    bool stale;
    uint32_t last_update_ms;
    uint16_t accepted_count;
    uint16_t rejected_count;
    PublicUnavailableReason unavailable_reason;

    PublicNodeDiagnosticsSnapshot() {
        reset();
    }

    void reset() {
        node_id = 0;
        diagnostics_available = false;
        health_known = false;
        health_ok = false;
        stale = false;
        last_update_ms = 0;
        accepted_count = 0;
        rejected_count = 0;
        unavailable_reason = PublicUnavailableReason::NO_OWNER;
    }
};

struct PublicCapabilityDiagnosticsSnapshot {
    PublicCapabilityInstanceId capability_instance_id;
    bool diagnostics_available;
    bool health_known;
    bool health_ok;
    bool stale;
    uint32_t last_update_ms;
    uint16_t accepted_count;
    uint16_t rejected_count;
    PublicUnavailableReason unavailable_reason;

    PublicCapabilityDiagnosticsSnapshot() {
        reset();
    }

    void reset() {
        capability_instance_id = 0;
        diagnostics_available = false;
        health_known = false;
        health_ok = false;
        stale = false;
        last_update_ms = 0;
        accepted_count = 0;
        rejected_count = 0;
        unavailable_reason = PublicUnavailableReason::NO_OWNER;
    }
};

struct PublicProviderDiagnosticsSnapshot {
    PublicProviderId provider_id;
    bool diagnostics_available;
    bool health_known;
    bool health_ok;
    bool stale;
    uint32_t last_update_ms;
    uint16_t accepted_count;
    uint16_t rejected_count;
    PublicUnavailableReason unavailable_reason;

    PublicProviderDiagnosticsSnapshot() {
        reset();
    }

    void reset() {
        provider_id = 0;
        diagnostics_available = false;
        health_known = false;
        health_ok = false;
        stale = false;
        last_update_ms = 0;
        accepted_count = 0;
        rejected_count = 0;
        unavailable_reason = PublicUnavailableReason::NO_OWNER;
    }
};

}  // namespace Cyber32
