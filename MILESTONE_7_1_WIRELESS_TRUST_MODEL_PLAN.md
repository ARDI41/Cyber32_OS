# Milestone 7.1 Wireless Trust Model Plan

## Goal

Design trusted and untrusted wireless node handling before real ESP-NOW integration.

This document is documentation only. It does not change source code, does not modify `src/main.cpp`, and does not change current Registry, WirelessService, Runtime, Logic, API, or validation behavior.

## Reviewed Inputs

- `CYBER32_BIBLE.md`
- `MILESTONE_6_9_WIRELESS_ARCHITECTURE_AUDIT.md`
- `MILESTONE_7_0_PROVIDER_DIAGNOSTICS_API_PLAN.md`

## Problem

The current simulated wireless path treats packets as implicitly trusted. That is acceptable for the simulated v1 packet flow, but it is not safe enough for real ESP-NOW nodes.

Before real radio support, Cyber32 needs a bounded trust model that can reject untrusted or blocked nodes without changing normal capability reads and without making Logic aware of wireless origin.

## 1. Trust States

Cyber32 already has the compact trust-state concept in `WirelessTrustState`:

- `TRUSTED`
- `UNTRUSTED`
- `BLOCKED`

The v1 trust model should also preserve `UNKNOWN` as an internal default for records that have not been explicitly initialized.

### TRUSTED

Meaning:

- The node is allowed to contribute provider payload updates.
- Packets from the node may update the corresponding virtual device.
- Packets from the node may update the provider latest payload through WirelessService and Registry public APIs.

Allowed behavior:

- `WirelessService::processPackets()` may accept packets from the node.
- Provider status may become `AVAILABLE`.
- Provider latest payload may be updated.
- Canonical capability payload may later change through explicit provider-selection flow.

### UNTRUSTED

Meaning:

- The node is known or newly observed but not yet allowed to affect capability state.
- Packets from the node are ignored for payload updates.

Required behavior:

- Packet may be parsed enough to identify node ID and trust state.
- Provider payload must not be updated.
- Active provider mapping must not be changed.
- Canonical capability payload must not be changed.
- Diagnostic state may record that the node exists and is untrusted.

### BLOCKED

Meaning:

- The node is explicitly denied.
- Packets from the node must not update device, provider, or canonical capability state.

Required behavior:

- Rejected before provider payload update.
- Provider payload remains unchanged.
- Canonical capability payload remains unchanged.
- Diagnostics may show blocked state for audit visibility.

## 2. Node Lifecycle

### New Node

Initial v1 policy:

- A newly observed real wireless node starts as `UNTRUSTED` unless preconfigured as trusted.
- Simulated validation nodes may be explicitly initialized as `TRUSTED`.
- New-node discovery must be bounded and must not allocate memory dynamically.

Expected state:

- `WirelessNodeRecord.node_id` is stored if a bounded slot is available.
- `WirelessNodeRecord.trust_state = WirelessTrustState::UNTRUSTED`.
- Provider payloads are not updated until the node becomes trusted.

### Trusted Node

A trusted node is a node whose packets can contribute to capability provider state.

Expected state:

- `WirelessNodeRecord.trust_state = WirelessTrustState::TRUSTED`.
- `WirelessNodeRecord.status` follows health updates such as `AVAILABLE`, `STALE`, `UNAVAILABLE`, or `LOST`.
- Provider payload updates are allowed when packet validation succeeds.

### Blocked Node

A blocked node is explicitly denied.

Expected state:

- `WirelessNodeRecord.trust_state = WirelessTrustState::BLOCKED`.
- Incoming packets are rejected.
- Provider payloads are not updated from blocked packets.
- Existing canonical capability payload remains last known or selected fallback; it is not overwritten by blocked data.

## 3. Registry Ownership

Registry owns stored state.

Trust state should be stored in `WirelessNodeRecord`, not in Logic, Runtime, API, or raw packet code.

Registry responsibilities:

- Store bounded wireless node records.
- Store each node trust state.
- Provide read/update APIs for node trust state in a future implementation phase.
- Store provider records and provider health.
- Store active provider mapping.

Registry must not:

- Parse raw packets.
- Execute trust policy beyond state storage and bounded read/write helpers.
- Call WirelessService, Runtime, Logic, API, Devices, Drivers, or HAL.
- Allocate dynamic memory.

## 4. WirelessService Behavior

WirelessService owns wireless packet policy.

Before a packet updates any provider payload, WirelessService must confirm the node is trusted.

### TRUSTED Node Packet

Flow:

```text
Transport Driver
-> WirelessService
-> validate packet identity and capability
-> read node trust state
-> TRUSTED accepted
-> WirelessTemperatureDevice update
-> Registry updateCapabilityProviderPayload()
```

Expected result:

- Process returns true if packet validation, device update, and provider update all succeed.
- Provider latest payload is updated.
- Provider status may become `AVAILABLE`.

### UNTRUSTED Node Packet

Flow:

```text
Transport Driver
-> WirelessService
-> identify node
-> trust state UNTRUSTED
-> reject before provider payload update
```

Expected result:

- Process returns false or a compact rejected status in a later result model.
- Provider latest payload remains unchanged.
- Canonical capability payload remains unchanged.
- Packet slot is cleared after the rejection decision if the packet was consumed.

### BLOCKED Node Packet

Flow:

```text
Transport Driver
-> WirelessService
-> identify node
-> trust state BLOCKED
-> reject before provider payload update
```

Expected result:

- Provider latest payload remains unchanged.
- Canonical capability payload remains unchanged.
- Diagnostic state records blocked node status.

## 5. Diagnostics Visibility

Provider diagnostics should expose enough trust-related state for validation, Dashboard, Mobile Studio, and future AI tools to explain why a wireless provider is not updating.

Future diagnostics may expose:

- `node_id`
- `trust_state`
- `node_status`
- `last_seen_ms`
- `last_sequence_id`
- `missed_heartbeat_count`
- `battery_present`
- `battery_level_percent`
- `battery_voltage`
- `signal_quality_percent`

Rules:

- Normal capability reads remain unchanged.
- `getTemperatureState()` still returns the canonical `CAP_TEMPERATURE` payload only.
- Logic remains provider-blind and wireless-blind.
- Diagnostics must not expose raw ESP-NOW packets.
- Diagnostics must not mutate trust state.

## 6. Capability Read Behavior

Normal capability reads do not change.

Example:

```text
wired or simulated temperature provider
wireless trusted temperature provider
wireless untrusted temperature provider

Logic:
get CAP_TEMPERATURE
```

Logic must not know:

- provider ID
- wireless node ID
- trust state
- transport type
- packet source

If a wireless node is untrusted or blocked, its packets simply do not update provider state. Logic continues to read the selected canonical `CAP_TEMPERATURE` payload.

## 7. Validation Cases

### Trusted Packet Accepted

Setup:

- Wireless node record exists.
- `trust_state = TRUSTED`.
- Provider record exists for `CAP_TEMPERATURE`.
- Inject valid wireless `CAP_TEMPERATURE` packet.

Expected:

- `WirelessService::processPackets(now_ms)` succeeds.
- Wireless provider latest payload updates.
- Provider status becomes `AVAILABLE`.
- Canonical payload changes only after explicit provider-selection/update flow.

### Untrusted Packet Rejected

Setup:

- Wireless node record exists.
- `trust_state = UNTRUSTED`.
- Provider record has previous valid payload, for example `23.5F`.
- Inject valid packet with different value, for example `99.0F`.

Expected:

- Packet does not update provider latest payload.
- Provider latest payload remains `23.5F`.
- Canonical payload remains unchanged.
- Diagnostics report `UNTRUSTED`.

### Blocked Packet Rejected

Setup:

- Wireless node record exists.
- `trust_state = BLOCKED`.
- Provider record has previous valid payload.
- Inject valid packet with different value.

Expected:

- Packet does not update provider latest payload.
- Provider latest payload remains unchanged.
- Canonical payload remains unchanged.
- Diagnostics report `BLOCKED`.

### Canonical Payload Unchanged After Rejected Packet

Setup:

- Canonical `CAP_TEMPERATURE` has selected value, for example `24.0F`.
- Inject packet from `UNTRUSTED` or `BLOCKED` node with a different value.

Expected:

- `getTemperatureState()` still returns `24.0F`.
- Active provider mapping is unchanged.
- Provider diagnostics explain rejection through node trust state.

## 8. Future Registry API Plan

Future implementation should add bounded Registry node-state helpers, for example:

```cpp
RegistryResult registerWirelessNodeWithResult(const WirelessNodeRecord& record);
RegistryResult getWirelessNode(uint32_t node_id, WirelessNodeRecord& out_record) const;
RegistryResult updateWirelessNodeTrustState(uint32_t node_id, WirelessTrustState trust_state);
RegistryResult updateWirelessNodeDiagnostics(uint32_t node_id, const WirelessNodeRecord& record);
uint8_t wirelessNodeCount() const;
```

Implementation rules:

- Fixed node table only.
- No heap allocation.
- No Arduino `String`.
- No STL containers.
- Registry stores state only.
- Registry does not parse packets.

## 9. Future API Plan

Provider diagnostics may be extended with node trust state, or a separate node diagnostics response may be added.

Candidate structs:

```cpp
struct ApiWirelessNodeDiagnostic {
    bool ok;
    RegistryResult registry_result;
    uint32_t node_id;
    WirelessNodeStatus status;
    WirelessTrustState trust_state;
    uint32_t last_seen_ms;
    uint32_t last_sequence_id;
    uint8_t missed_heartbeat_count;
    bool battery_present;
    float battery_level_percent;
    float battery_voltage;
    uint8_t signal_quality_percent;
    const char* error_code;
};
```

API rules:

- API reads Registry only.
- API does not trust, block, or unblock nodes unless a later explicit management API is designed.
- API does not expose raw packets.
- API does not update provider payloads.
- API does not select providers.

## 10. Security V1 Scope

The trust model is not full cryptographic security.

V1 provides:

- bounded known-node tracking
- trusted/untrusted/blocked policy
- rejection of untrusted packet updates
- diagnostics for why packets do not affect state

V1 does not yet provide:

- encryption
- authenticated pairing
- key rotation
- anti-replay beyond sequence tracking
- signed packets

Those features should be designed before production wireless actuator support.

## 11. Stop Conditions

Stop implementation if any of these occur:

- Logic sees trust state.
- Logic depends on wireless node ID.
- Runtime owns trust policy.
- Registry parses raw packets.
- Registry calls WirelessService.
- WirelessService writes Registry arrays directly.
- Provider payload updates happen for `UNTRUSTED` nodes.
- Provider payload updates happen for `BLOCKED` nodes.
- Rejected packets overwrite canonical capability payloads.
- Trust storage becomes unbounded.
- Dynamic allocation is introduced.
- Arduino `String` is introduced in core paths.
- STL containers are introduced in core paths.

## Final Assessment

Milestone 7.1 should make wireless trust explicit before real ESP-NOW integration.

Correct model:

```text
packet source
-> WirelessService trust policy
-> trusted packets update provider state
-> Registry provider selection updates canonical payload explicitly
-> Logic reads CAP_* only
```

This keeps Cyber32 capability-first, local-first, bounded, and safe while creating a clear path toward real wireless sensors and future wireless actuators.
