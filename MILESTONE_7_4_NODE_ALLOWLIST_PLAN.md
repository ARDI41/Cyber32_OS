# Milestone 7.4 Node Allowlist Plan

## Goal

Design wireless node allowlist and pairing behavior before real ESP-NOW integration.

This document is documentation only. It does not change source code, does not modify `src/main.cpp`, and does not change current wireless behavior.

## Reviewed Inputs

- `CYBER32_BIBLE.md`
- `MILESTONE_7_1_WIRELESS_TRUST_MODEL_PLAN.md`
- `MILESTONE_7_2_WIRELESS_PACKET_INTEGRITY_PLAN.md`
- `MILESTONE_7_3_SEQUENCE_PROTECTION_PLAN.md`

## Problem

Cyber32 now has a wireless trust model, checksum validation, and duplicate sequence protection. However, trusted nodes are still not explicitly paired or allowlisted.

Before real ESP-NOW integration, Cyber32 needs a bounded allowlist model so unknown nodes cannot update provider state merely by sending packets that pass checksum and sequence validation.

## 1. Allowlist Concept

The allowlist is a bounded list of wireless node identities that Cyber32 is willing to consider.

Node allowlist states:

- allowed node
- unknown node
- blocked node

### Allowed Node

Meaning:

- The node ID is known and permitted.
- Packets may continue to checksum, trust, sequence, and capability validation.

Allowed does not automatically mean payload updates always happen. The node must still pass:

- trust check
- checksum validation
- duplicate sequence validation
- capability semantic validation

### Unknown Node

Meaning:

- The node ID is not present in the bounded allowlist.
- The node has not been paired or preconfigured.

Expected behavior:

- WirelessService rejects packets from the node.
- Provider payload remains unchanged.
- Canonical payload remains unchanged.
- Diagnostics may record a compact rejected/unknown-node event in a future phase.

### Blocked Node

Meaning:

- The node ID is explicitly denied.
- Packets are rejected even if checksum, sequence, and trust-like data appear valid.

Expected behavior:

- WirelessService rejects packets before trust and provider update.
- Provider payload remains unchanged.
- Canonical payload remains unchanged.
- Diagnostics show blocked status when exposed.

## 2. Node Identity

V1 identity:

- `node_id`

Future identity:

- ESP-NOW peer MAC address
- bounded node label
- optional pairing token or short code

Identity rules:

- Logic must never depend on node ID.
- Capability reads must remain provider-blind.
- API diagnostics may expose node identity for operators.
- Raw packet details should not be exposed as normal state.

## 3. Pairing Ownership

Pairing is not a Runtime responsibility.

Ownership:

- Registry stores allowlist state.
- WirelessService enforces allowlist policy.
- API may expose read-only diagnostics first.
- Future explicit management API may request allow/block changes.
- Dashboard/Mobile Studio may provide user pairing UI later.

Runtime remains scheduler only.

## 4. Registry Ownership

Registry should own allowlist storage as bounded state.

Future record shape:

```cpp
enum class WirelessNodeAllowState : unsigned char {
    UNKNOWN,
    ALLOWED,
    BLOCKED
};

struct WirelessNodeAllowlistRecord {
    uint32_t node_id;
    WirelessNodeAllowState allow_state;
    WirelessTrustState trust_state;
    uint32_t paired_at_ms;
    uint32_t last_seen_ms;
};
```

Future Registry APIs:

```cpp
RegistryResult registerWirelessNodeAllowlistRecord(
    const WirelessNodeAllowlistRecord& record);

RegistryResult getWirelessNodeAllowState(
    uint32_t node_id,
    WirelessNodeAllowState& out_state) const;

RegistryResult setWirelessNodeAllowState(
    uint32_t node_id,
    WirelessNodeAllowState state);
```

Rules:

- fixed table only
- no heap allocation
- no Arduino `String`
- no STL containers
- Registry stores state only
- Registry does not parse packets
- Registry does not execute pairing workflow

## 5. WirelessService Behavior

WirelessService should enforce allowlist policy before trust, checksum-dependent provider updates, and capability conversion.

Recommended order:

```text
read packet
allowlist check
checksum validation
trust validation
sequence duplicate validation
device updateFromPacket()
provider update
```

Alternative order:

- checksum may run before allowlist to cheaply reject corrupted packets
- allowlist must still occur before any provider update

Required behavior:

- allowlisted node may continue through the existing validation chain
- unknown node is rejected
- blocked node is rejected
- rejected packet may be consumed
- provider payload remains unchanged
- canonical payload remains unchanged

Suggested compact error codes:

- `"wireless_node_unknown"`
- `"wireless_node_blocked"`

## 6. Relationship With Trust State

Allowlist and trust state are related but distinct.

Recommended policy:

1. Allowlist check first:
   - unknown rejected
   - blocked rejected
   - allowed continues

2. Trust state second:
   - `TRUSTED` continues
   - `UNTRUSTED` rejected
   - `BLOCKED` rejected
   - `UNKNOWN` rejected

Why both exist:

- allowlist answers: is this node identity permitted here?
- trust state answers: is this node currently trusted to update payloads?

Examples:

- allowed + trusted -> packets may update providers
- allowed + untrusted -> packets rejected until trust changes
- blocked + trusted-looking packet -> rejected by allowlist
- unknown + valid checksum -> rejected by allowlist

## 7. Diagnostics Visibility

Diagnostics should make allowlist decisions visible without changing normal capability reads.

Future diagnostics may expose:

- node ID
- allow state
- trust state
- last seen time
- last accepted sequence ID
- rejected packet counters if bounded

Provider diagnostics remain focused on provider state:

- provider ID
- provider type
- provider health
- priority
- latest payload
- active provider status

Normal capability APIs remain unchanged:

- `getTemperatureState()` returns canonical `CAP_TEMPERATURE`
- Logic remains provider-blind
- Logic does not see allowlist state

## 8. Validation Cases

### Allowed Node Accepted

Setup:

- node ID is allowlisted
- node trust state is `TRUSTED`
- packet checksum is valid
- sequence ID is new

Expected:

- `WirelessService::processPackets()` succeeds
- provider latest payload updates
- canonical payload updates only through explicit provider selection flow

### Unknown Node Rejected

Setup:

- packet node ID is not in allowlist
- packet otherwise valid

Expected:

- `processPackets()` returns false
- compact error code indicates unknown node
- provider latest payload unchanged
- canonical payload unchanged
- packet consumed

### Blocked Node Rejected

Setup:

- node ID exists in allowlist with blocked state
- packet otherwise valid

Expected:

- `processPackets()` returns false
- compact error code indicates blocked node
- provider latest payload unchanged
- canonical payload unchanged

### Provider Unchanged After Rejection

Setup:

- provider payload is known, for example `33.0F`
- rejected packet carries `99.0F`

Expected:

- provider payload remains `33.0F`
- canonical payload remains unchanged

## 9. Future Pairing Workflow

Pairing is documentation-only for this milestone.

Potential future pairing flow:

```text
node announces
-> API diagnostics reports unknown node
-> Dashboard/Mobile Studio user approves pairing
-> API requests allowlist update
-> Registry stores allowed node
-> WirelessService accepts future trusted packets
```

Future pairing rules:

- pairing must be explicit
- pairing storage must be bounded
- pairing must not require cloud services
- pairing must not require dynamic allocation
- actuator pairing must require stricter safety review than sensor pairing

## 10. Stop Conditions

Stop implementation if any of these occur:

- dynamic allocation is introduced
- Arduino `String` is introduced
- STL containers are introduced
- Runtime owns pairing
- Runtime parses packets
- Logic sees allowlist state
- Logic sees node ID
- Registry parses packets
- Registry calls WirelessService
- WirelessService writes Registry arrays directly
- unknown node updates provider payload
- blocked node updates provider payload
- rejected node changes canonical capability payload
- allowlist storage is unbounded

## Final Assessment

Milestone 7.4 should establish explicit node admission before real ESP-NOW:

```text
node_id allowed
+ trust state trusted
+ checksum valid
+ sequence not duplicate
-> provider payload may update
```

This preserves Cyber32's capability-first contract while adding a clear, bounded path toward safe real wireless sensors and future wireless actuators.
