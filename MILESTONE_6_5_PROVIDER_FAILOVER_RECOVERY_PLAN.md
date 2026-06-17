# Milestone 6.5 Provider Failover Recovery Plan

## Goal

Design automatic failover and recovery between multiple providers for the same Cyber32 capability.

This document is documentation only. It does not change source code, does not modify `src/main.cpp`, and does not change current Registry, WirelessService, Logic, Runtime, API, or validation behavior.

## Reviewed Inputs

- `CYBER32_BIBLE.md`
- `MILESTONE_6_1_MULTI_PROVIDER_CAPABILITY_PLAN.md`
- `MILESTONE_6_2_AUTOMATIC_PROVIDER_SELECTION_PLAN.md`
- `MILESTONE_6_4_SELECTED_PROVIDER_PAYLOAD_PLAN.md`

## Problem

Cyber32 can now store multiple providers for a single canonical capability and explicitly copy the active provider payload into the canonical capability payload.

Example:

```text
CAP_TEMPERATURE
  provider-sim-temperature-001       -> 22.4F
  provider-wireless-temperature-001  -> 23.5F
```

The next required behavior is provider failover and recovery:

- if the active provider becomes unavailable, stale beyond policy, lost, or disabled, Registry must select the next eligible provider
- if a better provider recovers, Registry must select it again
- canonical `CAP_TEMPERATURE` must update from the selected provider
- Logic must remain provider-blind

## 1. Failover Concept

Failover occurs when the currently active provider is no longer the best eligible source for a capability.

Triggers:

- active provider status becomes `UNAVAILABLE`
- active provider status becomes `LOST`
- active provider status becomes `DISABLED`
- active provider becomes stale beyond the v1 fallback policy
- another eligible provider is selected by priority and freshness policy

Failover flow:

```text
provider status changes
-> Registry selects best eligible provider
-> Registry updates active provider mapping
-> Registry copies selected provider latest_payload into canonical capability payload
-> Logic continues reading CAP_TEMPERATURE
```

Example:

```text
wireless temperature active at 23.5F
wireless provider becomes LOST
simulated provider remains AVAILABLE at 22.4F
updateBestCapabilityPayload(CAP_TEMPERATURE)
canonical CAP_TEMPERATURE becomes 22.4F
```

Failover must not expose provider IDs to Logic.

## 2. Recovery Concept

Recovery occurs when a previously unavailable, lost, or stale provider becomes eligible again.

Recovery flow:

```text
provider payload updates
-> provider status becomes AVAILABLE
-> Registry selects best eligible provider
-> Registry updates active provider mapping
-> Registry copies selected provider latest_payload into canonical capability payload
```

Example:

```text
simulated temperature active at 22.4F
wireless provider recovers at 24.0F with higher priority
updateBestCapabilityPayload(CAP_TEMPERATURE)
canonical CAP_TEMPERATURE becomes 24.0F
```

Recovery should use the same selection policy as failover. There should not be a special recovery-only path in v1.

## 3. Provider Eligibility

Provider eligibility determines whether a provider can be selected as the source for a canonical capability payload.

### Eligible

`CapabilityProviderStatus::AVAILABLE`

An `AVAILABLE` provider is eligible if:

- `provider.capability_id` matches the requested capability
- `provider.latest_payload.capability_id` matches the requested capability
- `provider.latest_payload.available == Availability::AVAILABLE`
- `provider.latest_payload.value_type` is compatible with the canonical capability record

### Optional Fallback

`CapabilityProviderStatus::STALE`

`STALE` is optional fallback in v1.

Recommended v1 policy:

- `STALE` providers are not selected by `selectBestProvider()`
- `STALE` providers may be copied by `updateSelectedCapabilityPayload()` only when already active and when payload is explicitly marked `AVAILABLE` and `STALE`
- automatic best-provider selection should prefer fresh `AVAILABLE` providers

This keeps automatic failover conservative.

### Not Eligible

These provider states are not eligible:

- `UNAVAILABLE`
- `LOST`
- `DISABLED`
- `UNKNOWN`

These providers must not be selected by automatic best-provider selection.

## 4. Registry Method Plan

Add a future Registry method:

```cpp
RegistryResult updateBestCapabilityPayload(const char* capability_id);
```

Purpose:

```text
select best eligible provider
-> set active provider
-> copy selected provider payload into canonical capability payload
```

Expected implementation flow:

```text
updateBestCapabilityPayload(capability_id)
  validate capability_id
  selectBestProvider(capability_id, selected)
  setActiveProvider(capability_id, selected.provider_id)
  updateSelectedCapabilityPayload(capability_id)
  return final RegistryResult
```

Equivalent internal sequence:

```cpp
ActiveCapabilityProvider selected;
RegistryResult select_result = selectBestProvider(capability_id, selected);
if (select_result != RegistryResult::OK) {
    return select_result;
}

RegistryResult active_result = setActiveProvider(capability_id, selected.provider_id);
if (active_result != RegistryResult::OK) {
    return active_result;
}

return updateSelectedCapabilityPayload(capability_id);
```

Registry owns this method because Registry owns:

- provider records
- provider selection
- active provider mapping
- canonical capability payload state

## 5. Failure Behavior

### Invalid Capability ID

Input:

```text
capability_id == null
capability_id == ""
```

Expected result:

```text
RegistryResult::INVALID_ID
```

Canonical payload remains unchanged.

### No Eligible Provider

If no provider is eligible:

```text
selectBestProvider(capability_id) -> NOT_FOUND
```

Recommended v1 policy:

- return `RegistryResult::NOT_FOUND`
- leave canonical payload as last known value
- do not mark canonical payload unavailable automatically in this method

Reason:

Leaving the canonical payload unchanged avoids hiding the last known valid state. Stale/lost canonical marking should be handled by a future explicit timeout/staleness method so the state transition is visible and testable.

### Active Provider Missing

`updateBestCapabilityPayload()` should not depend on the existing active provider being valid.

If the previous active provider is missing but another eligible provider exists:

```text
selectBestProvider() selects fallback
setActiveProvider() replaces mapping
updateSelectedCapabilityPayload() copies fallback payload
```

If no eligible provider exists:

```text
RegistryResult::NOT_FOUND
```

Canonical payload remains unchanged under v1 policy.

### Selected Provider Unavailable

This should normally be prevented by `selectBestProvider()`.

If a selected provider becomes unavailable between selection and selected-payload copy:

```text
updateSelectedCapabilityPayload() -> UNAVAILABLE
```

Recommended behavior:

- return `RegistryResult::UNAVAILABLE`
- leave canonical payload unchanged
- future retry may select another provider

### Payload Capability Mismatch

If selected provider payload does not match the requested capability:

```text
RegistryResult::INVALID_RECORD
```

Canonical payload remains unchanged.

This indicates a provider record corruption or invalid caller behavior.

## 6. WirelessService Future Flow

Future WirelessService provider update flow:

```text
read packet from transport
-> update WirelessTemperatureDevice
-> read CAP_TEMPERATURE payload from virtual Device
-> update provider payload:
   updateCapabilityProviderPayload(
       "provider-wireless-temperature-001",
       payload,
       CapabilityProviderStatus::AVAILABLE,
       now_ms)
-> updateBestCapabilityPayload(CAP_TEMPERATURE)
```

WirelessService remains a Service layer component.

WirelessService may:

- read simulated wireless transport packets
- update virtual wireless Device
- update provider payload through Registry public APIs
- ask Registry to update the best canonical capability payload

WirelessService must not:

- write Registry arrays directly
- choose providers itself
- expose API
- run Logic
- make Logic aware of wireless origin

## 7. Validation Plan

Validation should be added in a future phase after `updateBestCapabilityPayload()` is implemented.

### Setup

Register providers:

```text
provider-sim-temperature-001
  capability_id = CAP_TEMPERATURE
  provider_type = SIMULATED
  status = AVAILABLE
  priority = 10
  latest_payload.value_float = 22.4F

provider-wireless-temperature-001
  capability_id = CAP_TEMPERATURE
  provider_type = WIRELESS
  status = AVAILABLE
  priority = 20
  latest_payload.value_float = 23.5F
```

### Higher Priority Wireless Wins

Call:

```text
updateBestCapabilityPayload(CAP_TEMPERATURE)
```

Expect:

```text
RegistryResult::OK
active provider = provider-wireless-temperature-001
canonical CAP_TEMPERATURE value_float == 23.5F
```

### Wireless Lost Failover

Update wireless provider:

```text
status = LOST
```

Call:

```text
updateBestCapabilityPayload(CAP_TEMPERATURE)
```

Expect:

```text
RegistryResult::OK
active provider = provider-sim-temperature-001
canonical CAP_TEMPERATURE value_float == 22.4F
```

### Wireless Recovery

Update wireless provider:

```text
status = AVAILABLE
latest_payload.value_float = 24.0F
last_update_ms = newer
```

Call:

```text
updateBestCapabilityPayload(CAP_TEMPERATURE)
```

Expect:

```text
RegistryResult::OK
active provider = provider-wireless-temperature-001
canonical CAP_TEMPERATURE value_float == 24.0F
```

### No Eligible Provider

Set all providers to unavailable states:

```text
provider-sim-temperature-001      -> UNAVAILABLE
provider-wireless-temperature-001 -> LOST
```

Call:

```text
updateBestCapabilityPayload(CAP_TEMPERATURE)
```

Expect:

```text
RegistryResult::NOT_FOUND
canonical CAP_TEMPERATURE remains last known value under v1 policy
```

### Provider-Blind Logic

After each provider transition:

```text
TemperatureLogic::evaluate()
```

Expect:

```text
Logic queries CAP_TEMPERATURE only
Logic never reads provider_id
Logic status follows canonical payload
```

## 8. Registry Expectations

Registry may:

- scan bounded provider records
- select the best eligible provider
- update active provider mapping
- copy selected provider payload into canonical capability payload

Registry must not:

- call Services
- call Devices
- call Drivers
- call Runtime
- call API
- parse wireless packets
- run Logic
- allocate memory dynamically

Provider selection must remain bounded by:

```text
MAX_CAPABILITY_PROVIDERS
MAX_ACTIVE_CAPABILITY_PROVIDERS
```

## 9. Canonical Payload Policy

Canonical payload updates must remain explicit.

Allowed canonical provider update methods:

```text
updateSelectedCapabilityPayload(capability_id)
updateBestCapabilityPayload(capability_id)
```

Canonical payload must not change merely because:

- a provider payload was registered
- a provider payload was updated
- `selectBestProvider()` was called
- `setActiveProvider()` was called

This keeps state transitions deliberate, visible, and testable.

## 10. Stop Conditions

Stop and review architecture if any of these occur:

- Logic sees provider ID
- Logic depends on provider type
- API bypasses Registry provider state
- WirelessService chooses active provider itself
- Registry calls Services, Devices, Drivers, Runtime, API, or Logic
- provider selection becomes unbounded
- dynamic allocation is required
- Arduino `String` or STL containers are introduced in core paths
- canonical payload changes without an explicit Registry method
- `selectBestProvider()` mutates active provider mapping
- failover hides unavailable/lost provider state from diagnostics

## Final Assessment

Milestone 6.5 should add a single Registry-owned orchestration method:

```text
updateBestCapabilityPayload(capability_id)
```

The method should combine existing bounded primitives:

```text
selectBestProvider()
setActiveProvider()
updateSelectedCapabilityPayload()
```

This preserves the Cyber32 architecture:

```text
providers may fail or recover
Registry owns selection and canonical state
Logic remains provider-blind
API remains capability-first
WirelessService remains transport-processing Service code
```

