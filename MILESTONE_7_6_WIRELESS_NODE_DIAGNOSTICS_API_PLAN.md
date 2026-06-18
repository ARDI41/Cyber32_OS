# Milestone 7.6 Wireless Node Diagnostics API Plan

## Goal

Expose wireless node diagnostics through read-only API methods.

This document is documentation only. It does not change source code, does not modify `src/main.cpp`, and does not change current Registry, WirelessService, API, Runtime, Logic, or validation behavior.

## Reviewed Inputs

- `CYBER32_BIBLE.md`
- `MILESTONE_7_5_WIRELESS_SECURITY_AUDIT.md`

## Problem

Cyber32 now has simulated wireless security layers:

- allowlist enforcement
- trust enforcement
- checksum validation
- duplicate sequence protection
- provider payload protection
- canonical payload protection

However, node-level wireless diagnostics are not yet exposed through API. Provider diagnostics show provider state, but operators also need to understand node admission, trust, freshness, signal, battery, and rejection state.

## 1. Diagnostics Purpose

Wireless node diagnostics should answer:

- Is this node known?
- Is this node allowed or blocked?
- Is this node trusted?
- When was it last seen?
- What was the last accepted sequence ID?
- What battery state was last reported?
- What signal state was last reported?
- Why are packets being rejected?

Diagnostics must not change normal capability reads.

Normal state remains:

```text
API getTemperatureState()
-> Registry canonical CAP_TEMPERATURE payload
```

Diagnostics are separate:

```text
API getWirelessNodeDiagnostic()
-> Registry wireless node diagnostic state
```

## 2. Required Diagnostic Fields

Each wireless node diagnostic response should expose:

- `node_id`
- `allow_state`
- `trust_state`
- `last_seen_ms`
- `last_sequence_id`
- `battery_present`
- `battery_level_percent`
- `battery_voltage`
- `signal_quality_percent`
- rejection counters, future-ready

Future rejection counters should be bounded fields, for example:

- `checksum_reject_count`
- `duplicate_sequence_reject_count`
- `not_allowed_reject_count`
- `blocked_reject_count`
- `untrusted_reject_count`
- `invalid_packet_reject_count`

Counters should be optional in the first implementation if storage is not ready yet, but the API shape should leave room for them.

## 3. Proposed API Structs

### ApiWirelessNodeDiagnostic

```cpp
struct ApiWirelessNodeDiagnostic {
    bool ok;
    RegistryResult registry_result;
    uint32_t node_id;
    WirelessNodeAllowState allow_state;
    WirelessTrustState trust_state;
    uint32_t last_seen_ms;
    uint32_t last_sequence_id;
    bool battery_present;
    float battery_level_percent;
    float battery_voltage;
    uint8_t signal_quality_percent;
    uint16_t checksum_reject_count;
    uint16_t duplicate_sequence_reject_count;
    uint16_t not_allowed_reject_count;
    uint16_t blocked_reject_count;
    uint16_t untrusted_reject_count;
    uint16_t invalid_packet_reject_count;
    const char* error_code;
};
```

### ApiWirelessNodeSummary

```cpp
struct ApiWirelessNodeSummary {
    bool ok;
    RegistryResult registry_result;
    uint8_t node_count;
    uint8_t allowed_count;
    uint8_t blocked_count;
    uint8_t unknown_count;
    const char* error_code;
};
```

## 4. Proposed API Methods

### getWirelessNodeDiagnostic

```cpp
bool getWirelessNodeDiagnostic(
    uint32_t node_id,
    ApiWirelessNodeDiagnostic& out_response);
```

Behavior:

- `node_id == 0` -> `ok = false`, `registry_result = INVALID_ID`
- Registry missing -> `ok = false`, `registry_result = NOT_ATTACHED`
- node missing -> `ok = false`, `registry_result = NOT_FOUND`
- success:
  - `ok = true`
  - `registry_result = OK`
  - copy node diagnostics from Registry-owned state
  - `error_code = "none"`

### getWirelessNodeDiagnosticByIndex

```cpp
bool getWirelessNodeDiagnosticByIndex(
    uint8_t index,
    ApiWirelessNodeDiagnostic& out_response);
```

Behavior:

- index outside registered node count -> `NOT_FOUND`
- success copies bounded node diagnostic record

### getWirelessNodeSummary

```cpp
bool getWirelessNodeSummary(ApiWirelessNodeSummary& out_response);
```

Behavior:

- Registry missing -> `NOT_ATTACHED`
- success:
  - `node_count = registry_.wirelessNodeAllowlistCount()`
  - count allowed, blocked, and unknown records by bounded index reads
  - no mutation

## 5. Registry Expectations

Registry remains the owner of stored state.

Current Registry allowlist state already includes:

- node ID
- allow state
- trust state
- added time
- last seen time

Future Registry node diagnostics may add:

- last accepted sequence ID
- battery fields
- signal fields
- rejection counters

Registry must provide public bounded read APIs. API must not read Registry arrays directly.

Registry must not:

- parse packets
- enforce trust policy
- enforce allowlist policy
- compute checksums
- expose raw packets

## 6. WirelessService Expectations

WirelessService remains policy owner.

WirelessService may update Registry-owned node diagnostic facts in future phases:

- last seen time
- last accepted sequence ID
- battery diagnostics
- signal diagnostics
- rejection counters

WirelessService must not:

- expose API
- run Logic
- change normal capability read contract
- write Registry arrays directly
- allocate dynamic diagnostics storage

## 7. API Rules

The API is read-only for Milestone 7.6.

API must:

- read Registry state only
- expose node diagnostics compactly
- return `RegistryResult` for machine-readable failure
- keep normal capability reads unchanged
- avoid raw wireless packet exposure

API must not:

- allow or block nodes
- change trust state
- select providers
- update provider payloads
- update canonical payloads
- call WirelessService for diagnostic state
- call Devices, Drivers, HAL, Logic, or Runtime update

## 8. Logic Rules

Logic remains provider-blind and wireless-blind.

Logic must not depend on:

- node ID
- allow state
- trust state
- signal state
- battery state
- sequence ID
- rejection counters
- provider ID

Logic continues to query capability IDs only.

## 9. Validation Plan

### Diagnostic By Node ID

Setup:

- register node `1001`
- allow state `ALLOWED`
- trust state `TRUSTED`

Expected:

- `api.getWirelessNodeDiagnostic(1001, out)` returns true
- `out.ok == true`
- `out.node_id == 1001`
- `out.allow_state == ALLOWED`
- `out.trust_state == TRUSTED`

### Diagnostic By Index

Expected:

- index `0` returns node `1001`
- out-of-range index returns `NOT_FOUND`

### Summary

Expected:

- node count equals Registry allowlist count
- allowed count includes node `1001`
- blocked count includes blocked validation node if registered

### Missing Node

Expected:

- `getWirelessNodeDiagnostic(9999, out)` returns false
- `registry_result == NOT_FOUND`

### Invalid Node

Expected:

- `getWirelessNodeDiagnostic(0, out)` returns false
- `registry_result == INVALID_ID`

### Normal Capability Reads Unchanged

Expected:

- `getTemperatureState()` still returns canonical `CAP_TEMPERATURE`
- diagnostics do not update provider payload
- diagnostics do not update active provider
- diagnostics do not update canonical payload

## 10. Future Dashboard And Mobile Studio Use

Dashboard and Mobile Studio may show:

- known wireless nodes
- allowed/blocked state
- trust state
- last seen status
- battery level
- signal quality
- rejection counts

They must not require Logic to understand wireless origin.

Pairing or allow/block control should be a separate explicit management milestone, not part of read-only diagnostics.

## 11. Stop Conditions

Stop implementation if any of these occur:

- normal capability reads change
- Logic depends on wireless diagnostics
- API mutates allowlist or trust state
- API calls WirelessService for diagnostic facts
- API exposes raw packet bytes
- Registry parses packets
- WirelessService writes Registry arrays directly
- diagnostics require dynamic allocation
- diagnostics require Arduino `String`
- diagnostics require STL containers
- diagnostics storage becomes unbounded

## Final Assessment

Milestone 7.6 should make wireless node state visible without changing the capability-first contract.

Correct model:

```text
normal read:
API -> Registry canonical CAP_* payload

diagnostic read:
API -> Registry wireless node diagnostic state
```

This keeps Registry as state owner, WirelessService as policy owner, Runtime scheduler-only, and Logic provider-blind.
