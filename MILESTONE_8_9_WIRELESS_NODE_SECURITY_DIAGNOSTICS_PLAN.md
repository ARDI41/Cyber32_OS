# Milestone 8.9 Wireless Node Security Diagnostics Plan

## Goal

Design read-only diagnostics for wireless node security state and packet rejection visibility.

## Problem

Cyber32 now enforces the major simulated wireless security gates:

- checksum validation
- MAC-to-node identity verification
- node allowlist validation
- trust validation
- duplicate sequence protection

Operators need a bounded way to understand why packets are accepted or rejected without changing normal capability reads and without exposing raw packets.

## Design Principles

1. Diagnostics are read-only from API.
2. Normal capability reads remain capability-first and provider-blind.
3. Logic must not depend on diagnostics.
4. Registry owns diagnostic state.
5. WirelessService updates diagnostic state through Registry public APIs.
6. API reads diagnostics only.
7. No raw packet data is exposed.
8. No packet history is stored.
9. All storage is bounded.

## Diagnostic Record Concept

Wireless node diagnostics should be stored as compact per-node state.

Recommended future record:

```cpp
struct WirelessNodeSecurityDiagnosticRecord
{
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
```

This record is a proposed implementation shape only. The implementation may merge these fields into existing bounded wireless node allowlist or diagnostic records if that keeps the Registry simpler.

## Required Diagnostic Fields

### `node_id`

Logical Cyber32 wireless node ID.

Source:

- packet header when available
- allowlist record when known

### `MAC address`

Transport identity observed from ESP-NOW source MAC.

Source:

- adapter-provided source MAC
- allowlist record MAC

Rules:

- expose only as bounded bytes or a fixed diagnostic representation
- no Arduino `String`
- no dynamic formatting allocation

### `allow_state`

Current allowlist state:

- `UNKNOWN`
- `ALLOWED`
- `BLOCKED`

Source:

- Registry allowlist record

### `trust_state`

Current trust state:

- `UNKNOWN`
- `TRUSTED`
- `UNTRUSTED`
- `BLOCKED`

Source:

- Registry allowlist record or wireless node record

### `last_seen_ms`

Most recent time a packet or known node event was observed.

Policy:

- accepted packets update `last_seen_ms`
- rejected packets may update rejection-specific observed time if useful
- no packet history

### `last_accepted_sequence_id`

Most recent accepted sequence ID for the node.

Policy:

- update only after successful provider update
- do not update for rejected packets

### `last_rejected_sequence_id`

Most recent rejected sequence ID for the node when available.

Policy:

- update for checksum, MAC, allowlist, trust, duplicate, and invalid packet rejections when packet header was readable

### `last_error_code`

Compact pointer to canonical error text.

Expected values include:

- `none`
- `wireless_checksum_invalid`
- `wireless_mac_not_allowed`
- `wireless_mac_node_mismatch`
- `wireless_node_blocked`
- `wireless_node_not_allowed`
- `wireless_untrusted`
- `wireless_duplicate_sequence`
- `device_update_failed`
- `provider_update_failed`

No dynamic allocation is allowed.

## Counters

All counters are bounded integer counters. V1 may saturate at max value rather than wrap.

### `checksum_reject_count`

Increment when checksum validation fails.

### `mac_not_allowed_reject_count`

Increment when source MAC is available but no allowlist record exists or the MAC record is not allowed.

### `mac_node_mismatch_reject_count`

Increment when source MAC maps to a different node ID than the packet claims.

### `blocked_reject_count`

Increment when MAC or node allowlist state is blocked.

### `not_allowed_reject_count`

Increment when node ID allowlist validation fails without a more specific MAC failure.

### `untrusted_reject_count`

Increment when trust state blocks payload update.

### `duplicate_sequence_reject_count`

Increment when duplicate sequence validation rejects a packet.

### `invalid_packet_reject_count`

Increment when packet shape, capability semantics, or Device packet validation fails.

### `accepted_packet_count`

Increment only after:

- checksum passes
- MAC-to-node check passes when source MAC exists
- node allowlist passes
- trust passes
- sequence passes
- Device update succeeds
- provider update succeeds

## Ownership

### Registry

Registry owns diagnostic state.

Registry may provide future public APIs such as:

```cpp
RegistryResult updateWirelessNodeSecurityAccepted(
    uint32_t node_id,
    const uint8_t mac_address[WIRELESS_MAC_ADDRESS_SIZE],
    bool has_mac_address,
    uint32_t sequence_id,
    uint32_t now_ms);

RegistryResult updateWirelessNodeSecurityRejected(
    uint32_t node_id,
    const uint8_t mac_address[WIRELESS_MAC_ADDRESS_SIZE],
    bool has_mac_address,
    uint32_t sequence_id,
    const char* error_code,
    WirelessSecurityRejectReason reason,
    uint32_t now_ms);

RegistryResult getWirelessNodeSecurityDiagnostic(
    uint32_t node_id,
    WirelessNodeSecurityDiagnosticRecord& out_record) const;
```

These names are conceptual. Final implementation should follow local Registry naming style.

Registry must not:

- parse packets
- call WirelessService
- call Devices or Drivers
- update capability payloads as a side effect of diagnostic reads

### WirelessService

WirelessService owns security policy and rejection classification.

WirelessService may:

- call Registry public diagnostic update methods after accept/reject decisions
- pass compact reason IDs or error codes

WirelessService must not:

- write Registry arrays directly
- store unbounded packet history
- expose raw packet data
- mutate API-facing diagnostics directly

### API

API reads diagnostics only.

API may expose:

- node diagnostic by node ID
- node diagnostic by index
- wireless security summary

API must not:

- mutate diagnostics
- reset counters in v1
- select providers
- expose raw packets
- change normal capability reads

### Logic

Logic remains provider-blind and diagnostics-blind by default.

Logic must not:

- depend on MAC address
- depend on rejection counters
- depend on provider ID
- depend on wireless origin

## API Visibility

Future API structs should extend or complement `ApiWirelessNodeDiagnostic`.

Required read-only fields:

- `node_id`
- MAC address bytes
- `has_mac_address`
- `allow_state`
- `trust_state`
- `last_seen_ms`
- `last_accepted_sequence_id`
- `last_rejected_sequence_id`
- `last_error_code`
- `checksum_reject_count`
- `mac_not_allowed_reject_count`
- `mac_node_mismatch_reject_count`
- `blocked_reject_count`
- `not_allowed_reject_count`
- `untrusted_reject_count`
- `duplicate_sequence_reject_count`
- `invalid_packet_reject_count`
- `accepted_packet_count`

No raw packet payload bytes should be exposed.

## Validation Plan

### Valid Packet

Setup:

- allowed MAC
- matching node ID
- trusted node
- valid checksum
- new sequence

Expected:

- packet accepted
- provider updates
- `accepted_packet_count` increments
- `last_accepted_sequence_id` updates
- `last_error_code == "none"`

### Invalid Checksum

Setup:

- packet has corrupted checksum

Expected:

- packet rejected
- `checksum_reject_count` increments
- `last_error_code == "wireless_checksum_invalid"`
- provider unchanged
- canonical payload unchanged

### Unknown MAC

Setup:

- source MAC not registered
- packet checksum valid

Expected:

- packet rejected
- `mac_not_allowed_reject_count` increments
- `last_error_code == "wireless_mac_not_allowed"`
- provider unchanged

### MAC/Node Mismatch

Setup:

- source MAC maps to node A
- packet claims node B

Expected:

- packet rejected
- `mac_node_mismatch_reject_count` increments
- `last_error_code == "wireless_mac_node_mismatch"`
- provider unchanged

### Blocked Node

Setup:

- MAC or node allowlist record is blocked

Expected:

- packet rejected
- `blocked_reject_count` increments
- `last_error_code == "wireless_node_blocked"`
- provider unchanged

### Untrusted Node

Setup:

- allowlist passes
- trust state blocks payload update

Expected:

- packet rejected
- `untrusted_reject_count` increments
- `last_error_code == "wireless_untrusted"`
- provider unchanged

### Duplicate Sequence

Setup:

- first packet accepted
- second packet repeats same sequence ID

Expected:

- duplicate rejected
- `duplicate_sequence_reject_count` increments
- `last_rejected_sequence_id` updates
- provider unchanged

### Diagnostic Reads

Expected:

- diagnostic reads do not mutate counters
- diagnostic reads do not update provider payload
- diagnostic reads do not update active provider
- diagnostic reads do not update canonical payload
- normal `getTemperatureState()` remains capability-first

## Canonical Payload Policy

Diagnostics must not change canonical payloads.

Canonical `CAP_TEMPERATURE` may change only through explicit provider selection/update flows already defined by the provider architecture.

## Stop Conditions

Stop implementation if any of the following are required:

- Logic sees diagnostics
- Logic sees MAC address
- API mutates diagnostics
- Registry parses packets
- Registry calls WirelessService
- WirelessService writes Registry arrays directly
- diagnostics expose raw packet bytes
- diagnostics require unbounded history
- dynamic allocation is introduced
- Arduino `String` is introduced
- STL containers are introduced
- `src/main.cpp` must change

## Final Assessment

Milestone 8.9 should add operator visibility without changing capability-first behavior.

PASS: The current security pipeline has clear rejection points that can map to bounded counters.

WARNING: Counter persistence is not defined for v1. Runtime-only counters are acceptable initially, but production diagnostics will eventually need a bounded persistence policy.
