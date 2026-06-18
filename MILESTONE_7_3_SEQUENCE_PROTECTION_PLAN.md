# Milestone 7.3 Sequence Protection Plan

## Goal

Design duplicate wireless packet protection using sequence IDs.

This document is documentation only. It does not change source code, does not modify `src/main.cpp`, and does not change current wireless packet behavior.

## Reviewed Inputs

- `CYBER32_BIBLE.md`
- `MILESTONE_7_2_WIRELESS_PACKET_INTEGRITY_PLAN.md`
- `MILESTONE_7_1_WIRELESS_TRUST_MODEL_PLAN.md`

## Problem

Trusted packets with valid checksums may still be duplicated by the transport, replayed accidentally, or resent by a node. Without sequence protection, a duplicate packet can be processed repeatedly and update provider timestamps or payload state as if it were fresh data.

Cyber32 needs bounded duplicate protection before real ESP-NOW integration.

## 1. Sequence Ownership

Sequence ownership is per wireless node.

The receiver must track the last accepted sequence ID for each node. The sequence ID belongs to the wireless node identity, not to Logic, Runtime, or the canonical capability.

Storage location:

- `WirelessNodeRecord::last_sequence_id`

Reason:

- Node records already own wireless node facts.
- Duplicate protection is transport/node state.
- Logic must remain provider-blind and wireless-blind.

## 2. Node State

Required node state:

```cpp
uint32_t node_id;
uint32_t last_sequence_id;
```

Existing `WirelessPacketHeader::sequence_id` is currently `uint16_t`. The value may be widened in a future packet schema revision, but v1 sequence comparison can operate on the existing fixed-width field and store it in the wider `WirelessNodeRecord::last_sequence_id`.

Initialization:

- A newly initialized simulated node may start with `last_sequence_id = 0`.
- The first accepted packet needs a way to avoid rejecting sequence `0` solely because the default state is `0`.

Recommended v1 addition:

- Add a bounded boolean flag such as `has_last_sequence_id` only if needed.
- If no flag is added, validation should avoid sequence `0` for the first packet.

Preferred future state:

```cpp
bool has_last_sequence_id;
uint32_t last_sequence_id;
```

This keeps first-packet behavior explicit.

## 3. Acceptance Rules

V1 rules are intentionally simple and bounded.

### First Packet Accepted

If the node has no accepted sequence yet:

- accept the packet after trust and checksum validation
- update `last_sequence_id`
- update provider payload through existing flow

### Same Sequence ID Rejected

If incoming `sequence_id == last_sequence_id`:

- reject as duplicate
- do not update virtual device payload
- do not update provider payload
- do not update active provider mapping
- do not update canonical payload
- do not change `last_sequence_id`

### Newer Sequence ID Accepted

V1 accepts any sequence ID that differs from `last_sequence_id`.

Rationale:

- This catches immediate duplicate delivery.
- It avoids packet history arrays.
- It avoids complex wraparound logic in the first implementation.

Future policy may make this stricter.

## 4. Rejection Behavior

Duplicate packet rejection must fail closed.

Expected behavior:

- `WirelessService::processPackets()` returns false.
- Packet is consumed by the transport read path.
- `lastErrorCode()` returns a compact error code such as:
  - `"wireless_duplicate_sequence"`
  - future canonical `ERR_WIRELESS_DUPLICATE_SEQUENCE`
- Provider latest payload remains unchanged.
- Provider status remains unchanged unless a later health scan changes it.
- Canonical capability payload remains unchanged.
- Active provider mapping remains unchanged.

Rejected duplicate packets must not refresh provider health timestamps.

## 5. Ownership

### WirelessService

WirelessService enforces sequence policy.

Responsibilities:

- read packet from transport
- verify checksum
- verify trust state
- check duplicate sequence before device/provider updates
- reject duplicate sequence fail-closed
- allow non-duplicate sequence through existing device/provider update flow

WirelessService must not:

- allocate packet history
- write Registry arrays directly
- expose API
- run Logic
- change Runtime state

### WirelessTemperatureDevice

WirelessTemperatureDevice stores node state and exposes accessors needed by WirelessService.

Responsibilities:

- hold `WirelessNodeRecord`
- expose current `last_sequence_id`
- update node record after accepted packet
- keep payload conversion capability-specific

Important design note:

- If `updateFromPacket()` updates `last_sequence_id`, WirelessService must check duplicates before calling it.
- That may require a read-only accessor for current node record or current `last_sequence_id`.

### Registry

Registry remains packet-agnostic.

Registry responsibilities:

- store provider records
- store active provider mapping
- store canonical selected payload
- store node/provider facts if future phases add node record storage

Registry must not:

- parse packets
- compute checksums
- compare sequence IDs
- call WirelessService or Devices

### Runtime

Runtime remains scheduler only.

Runtime may call:

- `WirelessService::processPackets(now_ms)`
- `WirelessService::checkTimeouts(now_ms)`

Runtime must not:

- parse packets
- validate checksum
- inspect sequence IDs
- choose providers
- own trust policy

### Logic

Logic remains provider-blind.

Logic must not see:

- node ID
- sequence ID
- checksum
- transport origin
- provider ID

Logic continues to query `CAP_*` payloads only.

## 6. Validation

### First Packet Accepted

Setup:

- node trust state is `TRUSTED`
- checksum is valid
- sequence ID is new

Expected:

- `processPackets()` returns true
- provider latest payload updates
- node `last_sequence_id` becomes the accepted sequence ID

### Duplicate Rejected

Setup:

- previous accepted sequence ID is `50`
- provider latest payload is `30.0F`
- inject valid checksum packet with sequence ID `50` and value `31.0F`

Expected:

- `processPackets()` returns false
- `lastErrorCode() == "wireless_duplicate_sequence"`
- provider latest payload remains `30.0F`
- canonical payload remains unchanged
- packet slot is cleared

### Next Sequence Accepted

Setup:

- previous accepted sequence ID is `50`
- inject valid checksum packet with sequence ID `51` and value `32.0F`

Expected:

- `processPackets()` returns true
- provider latest payload becomes `32.0F`
- node `last_sequence_id == 51`

## 7. Future Wraparound Handling

V1 should document wraparound but avoid complex logic.

Simple v1:

- reject only exact duplicate of last accepted sequence ID
- accept different sequence IDs

Future options:

- modulo-aware newer-than comparison
- bounded replay window
- per-node dropped packet counters
- sequence reset marker after node reboot
- explicit `NODE_ANNOUNCE` sequence reset handling

Any future replay window must remain fixed-size and bounded.

## 8. Stop Conditions

Stop implementation if any of these occur:

- packet history arrays are introduced without fixed bounds
- dynamic allocation is required
- Arduino `String` is introduced
- STL containers are introduced
- Registry parses packets
- Runtime parses packets
- Logic sees sequence IDs
- duplicate rejection updates provider payload
- duplicate rejection updates canonical payload
- duplicate rejection refreshes provider health timestamp

## Final Assessment

Milestone 7.3 should add simple, bounded duplicate protection:

```text
trusted node
+ valid checksum
+ sequence_id != last accepted sequence
-> accepted
```

This protects the simulated wireless provider path from immediate duplicate packets while keeping Registry packet-agnostic, Runtime scheduler-only, and Logic capability-first.
