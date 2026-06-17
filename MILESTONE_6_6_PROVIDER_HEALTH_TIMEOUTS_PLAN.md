# Milestone 6.6 Provider Health Timeouts Plan

## Goal

Design automatic provider health transitions based on `last_update_ms` and bounded timeout policy.

This document is documentation only. It does not change source code, does not modify `src/main.cpp`, and does not change current Registry, WirelessService, Logic, Runtime, API, or validation behavior.

## Reviewed Inputs

- `CYBER32_BIBLE.md`
- `MILESTONE_6_5_PROVIDER_FAILOVER_RECOVERY_PLAN.md`
- `MILESTONE_6_3_WIRELESS_SERVICE_PLAN.md`

## Problem

Provider failover and recovery can now be designed around explicit provider status changes.

The next step is automatic provider health management:

```text
updates stop
-> provider becomes STALE
-> provider becomes LOST
-> failover can select another provider
-> provider recovers after a valid update
```

This must remain bounded, Registry-owned, and provider-blind to Logic.

## 1. Provider Health Concept

Provider health is the Registry-owned status of a capability provider.

Provider health is stored in:

```text
CapabilityProviderRecord.status
CapabilityProviderRecord.last_update_ms
```

Provider health describes the provider, not the canonical capability itself.

Logic must not inspect provider health. Logic continues to query:

```text
getCapabilityPayload(CAP_TEMPERATURE)
```

API diagnostics may expose provider health in a future phase, but normal capability state reads remain capability-first.

### AVAILABLE

The provider has recently supplied a valid payload and may be selected by automatic provider selection.

Expected meaning:

```text
provider can supply canonical capability payload
latest_payload is current enough for normal selection
```

### STALE

The provider has a last known payload, but it has not updated within the stale timeout.

Expected meaning:

```text
provider data is old
provider should not beat fresh AVAILABLE providers
provider may be used only by an explicit stale fallback policy
```

Recommended v1 policy:

```text
selectBestProvider() ignores STALE providers
```

This keeps automatic selection conservative.

### UNAVAILABLE

The provider is known to be unavailable due to explicit Service or Device state.

Expected meaning:

```text
provider cannot currently supply valid payload
provider remains unavailable until a Service writes a valid update
```

Timeout scanning should not turn `UNAVAILABLE` into another state automatically.

### LOST

The provider has not updated within the lost timeout.

Expected meaning:

```text
provider is not eligible
provider remains LOST until a valid update arrives
```

Lost providers must be ignored by `selectBestProvider()`.

### DISABLED

The provider is intentionally disabled by configuration, safety policy, manual control, or future production management.

Expected meaning:

```text
provider must not be selected
provider remains DISABLED until explicitly re-enabled
```

Timeout scanning must not change `DISABLED`.

## 2. Timeout Constants

Add future Registry-level timeout constants:

```cpp
static constexpr uint32_t PROVIDER_STALE_TIMEOUT_MS = ...;
static constexpr uint32_t PROVIDER_LOST_TIMEOUT_MS = ...;
```

Required relationship:

```text
PROVIDER_LOST_TIMEOUT_MS > PROVIDER_STALE_TIMEOUT_MS
```

Recommended initial values should be conservative and ESP32-friendly.

Example development values:

```text
PROVIDER_STALE_TIMEOUT_MS = 5000
PROVIDER_LOST_TIMEOUT_MS  = 15000
```

Production values may later be capability-specific or provider-type-specific, but v1 should start with fixed global constants.

Rules:

- constants must be compile-time constants
- no dynamic configuration storage in this phase
- no heap allocation
- no STL containers
- no Arduino `String`

## 3. Registry Method Plan

Add a future Registry method:

```cpp
RegistryResult updateProviderHealth(uint32_t now_ms);
```

Purpose:

```text
scan provider table
update provider health from last_update_ms and timeout policy
return compact result
```

Registry owns this method because Registry owns:

- provider records
- provider status
- provider timestamps
- bounded provider table

## 4. Required Behavior

`updateProviderHealth(now_ms)` must scan:

```text
capability_providers_[0..capability_provider_count_)
```

Behavior:

```text
AVAILABLE provider older than stale timeout -> STALE
STALE provider older than lost timeout -> LOST
LOST provider stays LOST until valid update
DISABLED provider stays DISABLED
UNAVAILABLE provider stays UNAVAILABLE unless updated by Service
UNKNOWN may remain UNKNOWN or become LOST only if policy explicitly says so later
```

Age calculation must be overflow-safe:

```cpp
uint32_t age_ms = now_ms - provider.last_update_ms;
```

This is safe for unsigned wraparound when comparing elapsed time.

Recommended transition table:

| Current Status | Age | New Status |
| --- | --- | --- |
| `AVAILABLE` | `< PROVIDER_STALE_TIMEOUT_MS` | `AVAILABLE` |
| `AVAILABLE` | `>= PROVIDER_STALE_TIMEOUT_MS` | `STALE` |
| `STALE` | `< PROVIDER_LOST_TIMEOUT_MS` | `STALE` |
| `STALE` | `>= PROVIDER_LOST_TIMEOUT_MS` | `LOST` |
| `LOST` | any | `LOST` |
| `DISABLED` | any | `DISABLED` |
| `UNAVAILABLE` | any | `UNAVAILABLE` |

The method may return:

```text
RegistryResult::OK
```

when scan completes successfully.

No provider records is still a successful bounded scan:

```text
capability_provider_count_ == 0 -> OK
```

## 5. Canonical Payload Policy

Health update alone must not silently rewrite canonical payloads.

This method:

```text
updateProviderHealth(now_ms)
```

must not call:

```text
updateSelectedCapabilityPayload()
updateBestCapabilityPayload()
```

Canonical payload changes must remain explicit and testable.

Allowed future sequence:

```text
updateProviderHealth(now_ms)
updateBestCapabilityPayload(CAP_TEMPERATURE)
```

This keeps two separate concerns:

```text
health scan -> provider statuses
best update -> active provider and canonical payload
```

## 6. WirelessService Future Flow

Future `WirelessService::checkTimeouts(now_ms)` should call Registry public APIs only.

Recommended flow:

```text
WirelessService::checkTimeouts(now_ms)
  -> registry.updateProviderHealth(now_ms)
  -> registry.updateBestCapabilityPayload(CAP_TEMPERATURE)
```

For the first wireless slice, this applies to:

```text
provider-wireless-temperature-001
```

WirelessService may coordinate timeout checks, but it must not own provider health policy.

WirelessService must not:

- scan Registry arrays directly
- implement its own provider selection
- rewrite canonical payload directly
- expose API
- run Logic
- make Logic aware of wireless origin

## 7. Provider Update Recovery

Valid provider updates recover provider health through:

```text
updateCapabilityProviderPayload(provider_id, payload, AVAILABLE, now_ms)
```

Recovery behavior:

```text
LOST -> AVAILABLE after valid update
STALE -> AVAILABLE after valid update
UNAVAILABLE -> AVAILABLE after valid update if Service confirms availability
DISABLED -> remains DISABLED unless explicitly re-enabled by future policy
```

For v1, `updateCapabilityProviderPayload()` already receives the desired status from the caller. Services must pass `AVAILABLE` only after validating a payload.

## 8. Validation Plan

Validation should be added after `updateProviderHealth()` is implemented.

### Fresh Provider Remains Available

Setup:

```text
provider status = AVAILABLE
last_update_ms = now_ms
```

Call:

```text
updateProviderHealth(now_ms + small_delta)
```

Expect:

```text
provider status == AVAILABLE
```

### Available Becomes Stale

Setup:

```text
provider status = AVAILABLE
last_update_ms = now_ms
```

Call:

```text
updateProviderHealth(now_ms + PROVIDER_STALE_TIMEOUT_MS)
```

Expect:

```text
provider status == STALE
```

### Stale Becomes Lost

Setup:

```text
provider status = STALE
last_update_ms = now_ms
```

Call:

```text
updateProviderHealth(now_ms + PROVIDER_LOST_TIMEOUT_MS)
```

Expect:

```text
provider status == LOST
```

### Lost Provider Ignored By Selection

Setup:

```text
provider-wireless-temperature-001 -> LOST, priority 20
provider-sim-temperature-001      -> AVAILABLE, priority 10
```

Call:

```text
selectBestProvider(CAP_TEMPERATURE)
```

Expect:

```text
provider-sim-temperature-001 selected
```

### Failover After Health Update

Setup:

```text
wireless provider active and AVAILABLE
wireless last_update_ms older than lost timeout
sim provider AVAILABLE
```

Call:

```text
updateProviderHealth(now_ms)
updateBestCapabilityPayload(CAP_TEMPERATURE)
```

Expect:

```text
wireless status == LOST
active provider == provider-sim-temperature-001
canonical CAP_TEMPERATURE == 22.4F
```

### Recovery After Provider Payload Update

Setup:

```text
wireless provider LOST
sim provider active
```

Call:

```text
updateCapabilityProviderPayload(
  provider-wireless-temperature-001,
  recovered_payload_24_0,
  AVAILABLE,
  now_ms)
updateBestCapabilityPayload(CAP_TEMPERATURE)
```

Expect:

```text
wireless status == AVAILABLE
active provider == provider-wireless-temperature-001
canonical CAP_TEMPERATURE == 24.0F
```

### Canonical Payload Not Rewritten By Health Scan Alone

Setup:

```text
canonical CAP_TEMPERATURE == 23.5F
wireless provider becomes LOST during health scan
sim provider available at 22.4F
```

Call:

```text
updateProviderHealth(now_ms)
```

Expect:

```text
canonical CAP_TEMPERATURE still == 23.5F
```

Then call:

```text
updateBestCapabilityPayload(CAP_TEMPERATURE)
```

Expect:

```text
canonical CAP_TEMPERATURE == 22.4F
```

## 9. Runtime Integration

Runtime remains scheduler-only.

Future Runtime task:

```text
task.wireless_service.check_timeouts
```

Task callback:

```text
WirelessService::checkTimeouts(now_ms)
```

Runtime must not:

- own timeout thresholds
- scan providers
- choose providers
- update canonical payloads
- know `CAP_TEMPERATURE`

## 10. API Visibility

Normal API state methods remain unchanged:

```text
getTemperatureState()
```

Future provider diagnostics API may expose:

- provider ID
- provider type
- provider status
- priority
- last update timestamp
- active provider marker

Diagnostics API must remain read-only unless a future management milestone explicitly adds provider enable/disable controls.

## 11. Stop Conditions

Stop and review architecture if any of these occur:

- Logic sees provider health
- Logic sees provider ID
- Registry calls Services
- Registry calls Runtime
- Runtime owns provider health policy
- WirelessService directly scans Registry arrays
- health scanning becomes unbounded
- provider history is stored instead of latest compact state
- dynamic allocation is required
- Arduino `String` or STL containers are introduced in core paths
- canonical payload changes during health scan without an explicit selected/best payload update method
- lost providers remain eligible for automatic selection

## Final Assessment

Milestone 6.6 should add bounded Registry-owned provider health scanning:

```text
updateProviderHealth(now_ms)
```

The method should update provider statuses only.

Failover and recovery should remain explicit:

```text
updateProviderHealth(now_ms)
updateBestCapabilityPayload(capability_id)
```

This preserves Cyber32's core rules:

```text
Registry owns provider state
Runtime schedules only
WirelessService coordinates through Registry APIs
Logic remains provider-blind
canonical payload changes are explicit and testable
```

