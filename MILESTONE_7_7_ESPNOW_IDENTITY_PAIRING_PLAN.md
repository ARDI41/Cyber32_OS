# Milestone 7.7 ESP-NOW Identity Pairing Plan

## Goal

Design real ESP-NOW node identity and pairing before implementing real ESP-NOW transport.

This document is documentation only. It does not change source code, does not modify `src/main.cpp`, and does not change current simulated wireless behavior.

## Reviewed Inputs

- `CYBER32_BIBLE.md`
- `MILESTONE_7_4_NODE_ALLOWLIST_PLAN.md`
- `MILESTONE_7_5_WIRELESS_SECURITY_AUDIT.md`
- `MILESTONE_7_6_WIRELESS_NODE_DIAGNOSTICS_API_PLAN.md`

## Problem

Cyber32 currently uses a simulated `node_id`. Real ESP-NOW packets will arrive with a source MAC address. Before real ESP-NOW transport is implemented, Cyber32 needs a bounded model for mapping MAC addresses to nodes, pairing unknown devices, and keeping unknown or blocked nodes from updating providers.

## 1. Node Identity Model

### Node ID

`node_id` remains the compact internal identifier for a wireless node.

Rules:

- stable within Registry state
- used by provider and diagnostics systems
- not visible to Logic
- may be derived from configuration, pairing, or a node announce packet

### MAC Address

Real ESP-NOW nodes have a source MAC address.

Required storage:

```cpp
uint8_t mac_address[6];
```

Rules:

- MAC address identifies the radio peer at transport level.
- MAC address must be stored in bounded records.
- MAC address must not be represented using dynamic strings.
- MAC address must not be required by Logic.

### Provider ID Relationship

Provider ID identifies a capability provider.

Example:

```text
node_id: 1001
mac: AA:BB:CC:DD:EE:01
provider_id: provider-wireless-temperature-001
capability_id: CAP_TEMPERATURE
```

Rules:

- one node may expose one or more providers in future phases
- provider ID remains a Registry/provider diagnostic concept
- Logic reads only `CAP_*`

### Device ID Relationship

Virtual wireless devices represent node capabilities inside Cyber32.

Example:

```text
device_id: device-wireless-temperature-001
node_id: 1001
mac: AA:BB:CC:DD:EE:01
capability_id: CAP_TEMPERATURE
```

Rules:

- device ID is metadata and implementation identity
- Logic must not depend on device ID
- WirelessService maps valid packets to virtual device behavior

## 2. Pairing States

Pairing state should be explicit and bounded.

Recommended enum:

```cpp
enum class WirelessNodePairingState : unsigned char {
    UNKNOWN,
    PAIRING_REQUESTED,
    ALLOWED,
    BLOCKED
};
```

### UNKNOWN

Meaning:

- MAC/node is not paired or allowed.
- Packet may be visible in diagnostics.
- Packet must not update providers.

### PAIRING_REQUESTED

Meaning:

- Node has announced itself or requested pairing.
- User/admin has not approved yet.
- Packet must not update providers.

### ALLOWED

Meaning:

- Node is paired/allowlisted.
- Packet may continue through checksum, trust, sequence, and capability validation.

### BLOCKED

Meaning:

- Node is explicitly denied.
- Packet must not update providers.

## 3. Registry Storage Requirements

Registry owns bounded node identity and pairing state.

Future record shape:

```cpp
struct WirelessNodeIdentityRecord {
    uint32_t node_id;
    uint8_t mac_address[6];
    WirelessNodePairingState pairing_state;
    WirelessNodeAllowState allow_state;
    WirelessTrustState trust_state;
    uint32_t added_at_ms;
    uint32_t last_seen_ms;
};
```

Registry requirements:

- fixed table only
- no dynamic allocation
- no Arduino `String`
- no STL containers
- bounded MAC storage
- public read helpers by node ID, MAC, and index
- no packet parsing

Suggested constant:

```cpp
MAX_WIRELESS_NODE_IDENTITIES = 16
```

## 4. Pairing Flow V1

V1 pairing should be conservative.

Flow:

```text
node announces itself
-> ESP-NOW transport captures source MAC
-> WirelessService sees unknown MAC
-> Registry records or exposes unknown/pairing-requested diagnostic state
-> packet does not update provider
-> user/admin may allow or block later
-> allowed node may update providers on future valid packets
```

Important:

- unknown nodes do not update providers
- pairing is explicit
- pairing can be runtime-only in v1
- persistence is future work

## 5. WirelessService Behavior

WirelessService owns packet admission policy.

Real ESP-NOW processing should check source MAC before provider updates.

Recommended order:

```text
read packet + source MAC
lookup MAC in Registry identity/allowlist table
reject unknown or blocked MAC
map allowed MAC to node_id
checksum validation
trust validation
sequence duplicate validation
device updateFromPacket()
provider update
mark sequence accepted
```

Unknown MAC:

- return false
- compact error: `wireless_node_not_allowed`
- provider unchanged
- canonical unchanged

Blocked MAC:

- return false
- compact error: `wireless_node_blocked`
- provider unchanged
- canonical unchanged

Allowed MAC:

- continue through existing packet safety gates

## 6. Diagnostics API

Wireless diagnostics should show:

- MAC address
- node ID
- pairing state
- allow state
- trust state
- last seen time
- last accepted sequence ID
- battery and signal fields
- rejection counters when available

Diagnostics must remain read-only unless a separate explicit management API is designed.

Normal capability reads remain unchanged.

## 7. Persistence Future

Allowlist and pairing state should eventually persist across reboot.

V1 may be runtime-only:

- useful for simulated validation
- enough for architecture proof
- not enough for production real ESP-NOW deployment

Future persistence options:

- fixed-size flash-backed records
- explicit import/export of paired node table
- versioned pairing schema
- checksum or CRC on stored pairing records

Persistence must remain bounded.

## 8. Security Notes

MAC address alone is not strong authentication.

Risks:

- MAC spoofing
- replayed packets
- copied node IDs
- unauthenticated node announce packets

Required future security work:

- keyed message integrity
- bounded replay protection beyond last-sequence duplicate
- secure pairing confirmation
- optional per-node secrets
- stricter wireless actuator admission policy

Wireless actuator nodes require a separate safety/security milestone before implementation.

## 9. Validation Plan

### Known MAC Accepted

Setup:

- MAC is registered and allowed
- node maps to `node_id = 1001`
- trust state is `TRUSTED`
- packet checksum is valid
- sequence ID is new

Expected:

- packet accepted
- provider payload updates
- canonical payload updates only through explicit provider-selection flow

### Unknown MAC Rejected

Setup:

- packet source MAC is not registered
- packet otherwise valid

Expected:

- packet rejected
- error indicates node not allowed
- provider payload unchanged
- canonical payload unchanged
- diagnostics may show unknown/pairing-requested node

### Blocked MAC Rejected

Setup:

- MAC exists with `BLOCKED`
- packet otherwise valid

Expected:

- packet rejected
- error indicates blocked node
- provider payload unchanged
- canonical payload unchanged

### Diagnostics Shows Unknown Node

Setup:

- unknown MAC announces itself

Expected:

- diagnostics exposes MAC and pairing state if a bounded slot is available
- normal capability reads unchanged

## 10. Stop Conditions

Stop implementation if any of these occur:

- dynamic allocation is introduced
- Arduino `String` is introduced
- STL containers are introduced
- Runtime owns pairing
- Runtime parses packets
- Logic sees MAC addresses
- Logic sees pairing state
- Registry parses raw packets
- Registry calls WirelessService
- unknown nodes update providers
- blocked nodes update providers
- MAC storage is unbounded
- pairing requires cloud service

## Final Assessment

Milestone 7.7 should bridge simulated `node_id` security to real ESP-NOW identity:

```text
source MAC
-> bounded Registry identity lookup
-> node_id mapping
-> allow/trust/checksum/sequence gates
-> provider update only after all gates pass
```

This keeps Cyber32 local-first, capability-first, bounded, and ready for real ESP-NOW design without exposing wireless details to Logic.
