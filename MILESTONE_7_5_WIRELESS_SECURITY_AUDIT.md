# Milestone 7.5 Wireless Security Audit

## Goal

Audit the completed simulated wireless security architecture before real ESP-NOW transport integration.

This document is documentation only. It does not change source code, does not modify `src/main.cpp`, and does not change current wireless behavior.

## Reviewed Inputs

- `CYBER32_BIBLE.md`
- `MILESTONE_7_1_WIRELESS_TRUST_MODEL_PLAN.md`
- `MILESTONE_7_2_WIRELESS_PACKET_INTEGRITY_PLAN.md`
- `MILESTONE_7_3_SEQUENCE_PROTECTION_PLAN.md`
- `MILESTONE_7_4_NODE_ALLOWLIST_PLAN.md`

## Overall Assessment

**PASS with warnings**

The simulated wireless security architecture now has the required pre-radio safety layers:

- allowlist gate
- checksum validation
- trust gate
- duplicate sequence rejection
- provider payload protection
- canonical payload protection
- bounded Registry storage

The design is ready for simulated ESP-NOW flow validation, but real ESP-NOW transport must not begin until cryptographic/security gaps are explicitly planned.

## 1. Layer Compliance

Assessment: **PASS**

Wireless responsibilities remain separated:

- Transport driver stores and returns simulated packets.
- WirelessService owns packet policy.
- WirelessTemperatureDevice owns capability conversion and node-local state.
- Registry stores provider, allowlist, active provider, and canonical payload state.
- Runtime only schedules service callbacks.
- Logic reads capability IDs only.
- API exposes state and diagnostics without parsing packets.

No layer currently requires Logic or Runtime to know whether `CAP_TEMPERATURE` is wired, simulated, or wireless.

## 2. Runtime Scheduler-Only Compliance

Assessment: **PASS**

Runtime remains scheduler-only.

Runtime task callbacks may invoke:

- `WirelessService::processPackets(now_ms)`
- `WirelessService::checkTimeouts(now_ms)`

Runtime does not:

- parse packets
- validate checksum
- inspect allowlist records
- inspect trust state
- inspect sequence IDs
- choose providers
- update canonical payload directly

## 3. Registry Ownership Boundaries

Assessment: **PASS**

Registry owns bounded state:

- provider records
- active provider mapping
- canonical capability payload
- provider health
- wireless node allowlist records

Registry remains packet-agnostic.

Registry does not:

- parse raw packets
- compute packet checksums
- enforce trust policy
- enforce sequence policy
- call WirelessService
- call Devices or Drivers
- call Runtime or Logic

## 4. WirelessService Responsibilities

Assessment: **PASS**

WirelessService owns wireless packet policy and coordinates through public APIs.

Current intended processing order:

```text
read packet
checksum validation
allowlist validation
trust validation
sequence duplicate validation
device updateFromPacket()
provider update
mark sequence accepted
```

This order is architecture-correct because rejected packets fail before device/provider/canonical state changes.

## 5. Checksum Enforcement

Assessment: **PASS with warning**

Current checksum behavior:

- invalid checksum is rejected
- packet is consumed
- provider payload remains unchanged
- canonical payload remains unchanged
- compact error code: `wireless_checksum_invalid`

Warning:

- The checksum is a simple bounded additive checksum, not cryptographic integrity.
- It is suitable for simulated corruption detection only.
- Real ESP-NOW should use stronger integrity, such as CRC16 plus a separate security/authentication design.

## 6. Allowlist Enforcement

Assessment: **PASS**

Current allowlist behavior:

- allowed nodes continue through validation
- unknown nodes are rejected with `wireless_node_not_allowed`
- blocked nodes are rejected with `wireless_node_blocked`
- rejected nodes do not update provider payload
- rejected nodes do not update canonical payload

The allowlist model is bounded and stored in Registry.

## 7. Trust Enforcement

Assessment: **PASS**

Trust behavior:

- `TRUSTED` nodes may update provider state after all other checks pass
- `UNTRUSTED`, `BLOCKED`, and `UNKNOWN` trust states are rejected
- rejected packets do not update device/provider/canonical state

Warning:

- Trust state is currently controlled locally in the simulated device.
- Future real ESP-NOW integration should move node trust state toward Registry-owned node diagnostics/storage so trust decisions survive device object reset and can be inspected through diagnostics.

## 8. Sequence Protection

Assessment: **PASS with warning**

Current sequence behavior:

- first packet is accepted
- same sequence ID is rejected
- next sequence ID is accepted
- duplicate rejection consumes packet
- provider/canonical payload remain unchanged after duplicate rejection

Warning:

- V1 rejects only the immediately duplicated last accepted sequence ID.
- There is no replay window.
- There is no wraparound handling beyond accepting different IDs.

This is acceptable for simulated v1, but real ESP-NOW should define bounded replay protection.

## 9. Provider Protection

Assessment: **PASS**

Provider state is protected from rejected packets:

- checksum failure does not update provider payload
- unknown node does not update provider payload
- blocked node does not update provider payload
- untrusted node does not update provider payload
- duplicate sequence does not update provider payload

WirelessService updates provider payload only through Registry public APIs.

## 10. Canonical Payload Protection

Assessment: **PASS**

Canonical capability payload is protected.

Rejected wireless packets do not:

- call selected provider payload update
- call best provider payload update
- change active provider mapping
- overwrite canonical `CAP_TEMPERATURE`

Canonical payload changes remain explicit and Registry-owned through:

- `updateSelectedCapabilityPayload(...)`
- `updateBestCapabilityPayload(...)`

## 11. Failure Behavior

Assessment: **PASS**

Failure behavior is compact and fail-closed:

- invalid checksum -> `wireless_checksum_invalid`
- unknown node -> `wireless_node_not_allowed`
- blocked node -> `wireless_node_blocked`
- untrusted node -> `wireless_untrusted`
- duplicate sequence -> `wireless_duplicate_sequence`

Rejected packets may be consumed, preventing a bad packet from blocking the one-slot simulated transport indefinitely.

## 12. Diagnostics Visibility

Assessment: **WARNING**

Provider diagnostics exist at the API planning and implementation level, but wireless node-specific diagnostics are not complete yet.

Current visibility is enough for provider-level state, but future ESP-NOW work needs diagnostics for:

- node ID
- allow state
- trust state
- last seen time
- last accepted sequence ID
- checksum failures
- duplicate rejection count
- unknown/blocked rejection count
- battery and signal diagnostics

These must remain read-only diagnostics and must not alter normal capability reads.

## 13. Memory Safety

Assessment: **PASS**

The simulated wireless security path remains bounded:

- fixed packet structs
- fixed transport slot
- fixed provider table
- fixed allowlist table
- no packet history arrays
- no dynamic allocation
- no Arduino `String`
- no STL containers

Warning:

- Future replay windows, diagnostics counters, and node tables must remain fixed-size.

## 14. ESP-NOW Readiness

Assessment: **WARNING**

The architecture is ready for simulated ESP-NOW transport integration, but not yet production real-radio security.

Before real ESP-NOW hardware:

- define MAC identity handling
- define peer pairing flow
- define stronger checksum/CRC strategy
- define authentication or message integrity strategy
- define bounded replay window or sequence reset policy
- define unknown node diagnostics
- define blocked node persistence
- define sensor vs actuator wireless admission rules

Wireless actuators require a separate stricter security and safety review.

## Strengths

- Capability-first contract is preserved.
- Logic remains provider-blind.
- Runtime remains scheduler-only.
- Registry stores state only.
- WirelessService owns packet policy.
- Provider/canonical payloads fail closed on rejected packets.
- Allowlist, trust, checksum, and duplicate checks are bounded.
- No real ESP-NOW/WiFi code has been introduced.
- No dynamic allocation, Arduino `String`, or STL containers are required.

## Risks

- Current checksum is not cryptographic.
- Duplicate protection only rejects the last accepted sequence.
- Trust state is still primarily simulated-device-local.
- No persistent pairing model exists yet.
- Node diagnostics are incomplete.
- Real ESP-NOW MAC identity is not represented yet.
- Wireless actuator readiness is not sufficient yet.

## Architecture Violations

Assessment: **PASS**

No architecture violations identified in the simulated wireless security design.

Specifically:

- Runtime does not parse packets.
- Registry does not parse packets.
- Logic does not see provider or wireless node details.
- API does not mutate wireless provider state through diagnostics.
- WirelessService does not write Registry arrays directly.
- Device does not write Registry.

## Future ESP-NOW Requirements

Before real ESP-NOW integration, Cyber32 should define:

1. MAC-address based node identity.
2. Bounded pairing and allowlist persistence.
3. Stronger packet integrity, likely CRC16 at minimum.
4. Authentication or keyed message integrity for production.
5. Bounded replay protection.
6. Node diagnostics API.
7. Rejection counters with fixed-size storage.
8. Safe behavior for node reboot and sequence reset.
9. Separate policy for wireless actuator admission.
10. Failure behavior when wireless transport is noisy or unavailable.

## Recommended Next Milestone

Recommended next milestone:

**Milestone 7.6: Wireless Node Diagnostics API**

Goal:

- expose wireless node allowlist/trust/sequence/security diagnostics through read-only API methods
- keep normal capability reads unchanged
- keep Logic provider-blind
- keep Registry as state owner
- keep WirelessService as policy owner

Suggested follow-up:

**Milestone 7.7: ESP-NOW Identity And Pairing Plan**

This should define MAC identity, pairing workflow, persistence expectations, and security requirements before real radio implementation.

## Final Assessment

**PASS with warnings**

The simulated wireless security architecture is correctly layered and bounded. It is ready for continued simulated validation and diagnostics work.

It is not yet ready for real ESP-NOW production use until identity, pairing, stronger integrity, bounded replay, and diagnostics are completed.
