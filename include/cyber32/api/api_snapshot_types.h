#pragma once

#include <stdint.h>

#include "../public/capability_directory.h"
#include "../public/node_capability_map.h"
#include "../public/node_directory.h"
#include "../public/public_owner_types.h"

namespace Cyber32 {

static const uint16_t API_SNAPSHOT_CONTRACT_VERSION = 1;
static const uint8_t API_SNAPSHOT_MAX_NODES = NODE_DIRECTORY_MAX_PUBLIC_NODES;
static const uint8_t API_SNAPSHOT_MAX_CAPABILITIES =
    CAPABILITY_DIRECTORY_MAX_PUBLIC_CAPABILITIES;
static const uint8_t API_SNAPSHOT_MAX_LINKS = NODE_CAPABILITY_MAP_MAX_PUBLIC_LINKS;

enum class ApiSnapshotStatus : unsigned char {
    OK,
    TRUNCATED,
    ERROR
};

struct ApiSnapshotNodeEntry {
    PublicNodeId node_id;
    PublicLifecycleState lifecycle_state;
    PublicVisibilityState visibility_state;
    PublicTrustState trust_state;
    PublicFreshnessState freshness_state;
    uint8_t capability_count;
    bool diagnostics_available;
    bool valid;

    ApiSnapshotNodeEntry() {
        reset();
    }

    void reset() {
        node_id = 0;
        lifecycle_state = PublicLifecycleState::NONE;
        visibility_state = PublicVisibilityState::NONE;
        trust_state = PublicTrustState::UNKNOWN;
        freshness_state = PublicFreshnessState::UNKNOWN;
        capability_count = 0;
        diagnostics_available = false;
        valid = false;
    }
};

struct ApiSnapshotCapabilityEntry {
    uint32_t capability_id;
    PublicCapabilityInstanceId capability_instance_id;
    PublicLifecycleState lifecycle_state;
    PublicVisibilityState visibility_state;
    PublicAvailabilityState value_availability_state;
    PublicFreshnessState freshness_state;
    bool provider_available;
    bool diagnostics_available;
    bool valid;

    ApiSnapshotCapabilityEntry() {
        reset();
    }

    void reset() {
        capability_id = 0;
        capability_instance_id = 0;
        lifecycle_state = PublicLifecycleState::NONE;
        visibility_state = PublicVisibilityState::NONE;
        value_availability_state = PublicAvailabilityState::UNKNOWN;
        freshness_state = PublicFreshnessState::UNKNOWN;
        provider_available = false;
        diagnostics_available = false;
        valid = false;
    }
};

struct ApiSnapshotLinkEntry {
    PublicMappingLinkId link_id;
    PublicNodeId node_id;
    PublicCapabilityInstanceId capability_instance_id;
    PublicVisibilityState link_visibility_state;
    PublicFreshnessState link_freshness_state;
    bool diagnostics_available;
    uint8_t display_order;
    bool active;

    ApiSnapshotLinkEntry() {
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

struct ApiSnapshot {
    bool ok;
    const char* error_code;
    uint16_t contract_version;
    ApiSnapshotStatus status;
    uint8_t node_count;
    uint8_t capability_count;
    uint8_t mapping_count;
    bool nodes_truncated;
    bool capabilities_truncated;
    bool mappings_truncated;
    uint8_t broken_link_count;
    ApiSnapshotNodeEntry nodes[API_SNAPSHOT_MAX_NODES];
    ApiSnapshotCapabilityEntry capabilities[API_SNAPSHOT_MAX_CAPABILITIES];
    ApiSnapshotLinkEntry links[API_SNAPSHOT_MAX_LINKS];

    ApiSnapshot() {
        reset();
    }

    void reset() {
        ok = true;
        error_code = "none";
        contract_version = API_SNAPSHOT_CONTRACT_VERSION;
        status = ApiSnapshotStatus::OK;
        node_count = 0;
        capability_count = 0;
        mapping_count = 0;
        nodes_truncated = false;
        capabilities_truncated = false;
        mappings_truncated = false;
        broken_link_count = 0;

        for (uint8_t index = 0; index < API_SNAPSHOT_MAX_NODES; ++index) {
            nodes[index].reset();
        }
        for (uint8_t index = 0; index < API_SNAPSHOT_MAX_CAPABILITIES; ++index) {
            capabilities[index].reset();
        }
        for (uint8_t index = 0; index < API_SNAPSHOT_MAX_LINKS; ++index) {
            links[index].reset();
        }
    }
};

}  // namespace Cyber32
