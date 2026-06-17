# Milestone 7.2 Wireless Packet Integrity Plan

## Goal

Design checksum or CRC validation and duplicate sequence protection for simulated wireless packets before real ESP-NOW.

This document is documentation only. It does not change source code, does not modify `src/main.cpp`, and does not change current wireless packet behavior.

## Reviewed Inputs

- `CYBER32_BIBLE.md`
- `MILESTONE_7_1_WIRELESS_TRUST_MODEL_PLAN.md`
- `MILESTONE_6_1_WIRELESS_PACKET_SCHEMA.md`
- `MILESTONE_6_9_WIRELESS_ARCHITECTURE_AUDIT.md`

## Problem

Wireless packets are now trust-gated, but packet integrity is not yet validated. A trusted node can still provide corrupted, malformed, replayed, or duplicated data unless the packet path validates integrity before provider payload updates.

Before real ESP-NOW support, Cyber32 must reject corrupted packets and duplicate sequence IDs while preserving capability-first behavior.

## 1. Packet Integrity Concept

Packet integrity has three responsibilities:

- detect corrupted packet contents
- reject duplicate or replayed sequence IDs
- fail closed before provider payload updates

Integrity validation must happen after a packet is read from the transport slot and before the packet updates a virtual device or provider record.

Correct flow:

```text
Transport Driver
-> WirelessService
-> trust check
-> integrity check
-> duplicate sequence check
-> WirelessTemperatureDevice capability semantics
-> Registry provider update
```

Failure flow:

```text
Transport Driver
-> WirelessService
-> integrity or duplicate failure
-> reject
-> provider payload unchanged
-> canonical payload unchanged
```

## 2. Packet Fields

The packet schema should include a bounded checksum field.

Recommended v1 field:

```cpp
uint16_t checksum;
```

Placement:

- `WirelessPacketHeader` may include `checksum`.
- Alternatively, a small packet envelope may contain `WirelessPacketHeader`, payload, diagnostics, and checksum.
- The selected implementation should keep packet structs fixed-size and bounded.

Fields included in the checksum:

- `magic`
- `protocol_version`
- `packet_type`
- `flags`
- `sequence_id`
- `node_id`
- `payload_length`
- `capability_id`
- `payload_type`
- `value_float` byte representation
- `value_int`
- `error_code`
- diagnostics fields when included in the packet:
  - `battery_present`
  - `battery_level_percent`
  - `battery_voltage`
  - `signal_quality_percent`

Fields not included:

- locally computed `last_seen_ms`
- local provider status
- local trust state
- canonical payload state

Reason:

- `last_seen_ms`, provider status, trust state, and canonical state are receiver-owned facts, not packet-owned facts.

## 3. V1 Checksum Strategy

Milestone 7.2 should start with a simple bounded checksum, then leave room for CRC8 or CRC16 later.

Recommended v1:

- fixed-width `uint16_t`
- additive checksum over deterministic packet bytes
- no dynamic allocation
- no variable-length buffers
- no heap-owned packet views
- no packet history

Future:

- replace or supplement with CRC8/CRC16
- add authenticated message code if real security requirements demand it
- add keying only after a separate security architecture plan

V1 checksum is for corruption detection, not cryptographic authenticity.

## 4. Duplicate Sequence Policy

Each wireless node stores the most recent accepted sequence ID.

Storage:

- `WirelessNodeRecord::last_sequence_id`

Policy:

- If incoming `sequence_id` equals `last_sequence_id`, reject as duplicate.
- If incoming `sequence_id` differs from `last_sequence_id`, accept for v1 after checksum and trust checks pass.
- On accepted packet, update `last_sequence_id`.
- On rejected packet, do not update `last_sequence_id`.

Simple v1 wraparound:

- Wraparound is allowed.
- V1 does not require monotonic greater-than comparison.
- V1 only rejects exact duplicate of the last accepted sequence ID.

Reason:

- This is bounded and avoids packet history.
- It catches immediate duplicate delivery without requiring an unbounded replay window.

Future policy:

- add sliding window replay protection only if it can remain bounded
- add per-node sequence validation modes
- add timestamp or nonce only if architecture remains fixed-size

## 5. Ownership

### WirelessService

WirelessService owns packet policy:

- trust gate
- checksum validation
- duplicate sequence rejection
- fail-closed decision
- compact error reporting

WirelessService must not:

- write Registry arrays directly
- expose API
- run Logic
- call Runtime update
- allocate memory

### WirelessTemperatureDevice

WirelessTemperatureDevice owns capability semantics:

- packet type must be suitable for temperature update
- `capability_id == CAP_TEMPERATURE`
- payload type must be `FLOAT`
- valid packet becomes `CapabilityPayload`

WirelessTemperatureDevice should not own:

- provider selection
- active provider mapping
- canonical payload selection
- Runtime scheduling

### Registry

Registry stores latest facts:

- node record
- provider record
- active provider mapping
- canonical selected payload

Registry must not:

- parse packets
- compute packet checksums
- reject packet duplicates directly
- call Services, Devices, Runtime, Logic, or API

### Runtime

Runtime remains scheduler only.

Runtime may invoke:

- `task.wireless_service.process_packets`
- `task.wireless_service.check_timeouts`

Runtime must not:

- parse packets
- compute checksums
- choose providers
- own trust policy
- own duplicate policy

### Logic

Logic remains provider-blind and wireless-blind.

Logic reads:

- `CAP_TEMPERATURE`
- other capability IDs as needed

Logic must not read:

- node ID
- sequence ID
- checksum
- packet type
- provider ID
- transport origin

## 6. Failure Behavior

### Invalid Checksum

Expected behavior:

- `WirelessService::processPackets()` returns false.
- `lastErrorCode()` becomes a compact integrity error, for example `"wireless_checksum_invalid"` or a future `ERR_WIRELESS_CHECKSUM_INVALID`.
- Transport slot may be consumed.
- Provider latest payload remains unchanged.
- Canonical capability payload remains unchanged.
- Active provider mapping remains unchanged.

### Duplicate Sequence

Expected behavior:

- `WirelessService::processPackets()` returns false.
- `lastErrorCode()` becomes `"wireless_duplicate_sequence"` or a future `ERR_WIRELESS_DUPLICATE_SEQUENCE`.
- Provider latest payload remains unchanged.
- Canonical capability payload remains unchanged.
- `last_sequence_id` remains the last accepted sequence.

### Valid Next Sequence

Expected behavior:

- Packet is accepted after trust and checksum validation.
- Device updates payload.
- Provider latest payload updates.
- Node record `last_sequence_id` updates.
- Canonical payload updates only through explicit best-provider selected-payload flow.

## 7. Validation Plan

### Valid Checksum Accepted

Setup:

- Node is `TRUSTED`.
- Provider exists.
- Packet checksum is valid.
- Sequence ID is new.

Expected:

- `processPackets()` returns true.
- Provider latest payload updates.
- Provider status remains or becomes `AVAILABLE`.

### Invalid Checksum Rejected

Setup:

- Node is `TRUSTED`.
- Provider has previous valid payload, for example `26.0F`.
- Inject packet with invalid checksum and value `27.0F`.

Expected:

- `processPackets()` returns false.
- Error code indicates checksum failure.
- Provider latest payload remains `26.0F`.
- Canonical payload remains unchanged.

### Duplicate Sequence Rejected

Setup:

- Node is `TRUSTED`.
- Sequence ID `10` was already accepted.
- Inject another valid-checksum packet with sequence ID `10` and different value.

Expected:

- `processPackets()` returns false.
- Error code indicates duplicate sequence.
- Provider latest payload remains unchanged.
- Canonical payload remains unchanged.

### Next Valid Sequence Accepted

Setup:

- Previous accepted sequence ID is `10`.
- Inject valid-checksum packet with sequence ID `11`.

Expected:

- `processPackets()` returns true.
- Provider latest payload updates.
- Node record `last_sequence_id == 11`.

## 8. Error Model

V1 may start with compact local string constants inside WirelessService.

Recommended future canonical error IDs:

- `ERR_WIRELESS_CHECKSUM_INVALID`
- `ERR_WIRELESS_DUPLICATE_SEQUENCE`
- `ERR_WIRELESS_PACKET_INVALID`
- `ERR_WIRELESS_UNTRUSTED`

Rules:

- Do not add new IDs until an implementation phase requests them.
- Keep errors compact.
- Do not allocate dynamic messages.

## 9. Stop Conditions

Stop implementation if any of these occur:

- Registry parses packets.
- Runtime parses packets.
- Logic sees checksum, packet type, sequence ID, node ID, or provider ID.
- Packet history becomes unbounded.
- Duplicate detection requires an unbounded replay window.
- Checksum requires heap allocation.
- Packet validation requires JSON.
- Arduino `String` is introduced.
- STL containers are introduced.
- Provider payload changes after checksum rejection.
- Canonical payload changes after duplicate rejection.

## Final Assessment

Milestone 7.2 should make packet acceptance fail closed:

```text
trusted node
+ valid checksum
+ non-duplicate sequence
-> provider payload may update
```

Any failed integrity check must leave provider and canonical capability state unchanged. This preserves the Cyber32 rule that Logic reads clean capability state and never needs to understand wireless transport details.
