# Milestone 6.1 Provider Selection Plan

## Goal

Design active provider selection for multi-provider capabilities.

This document is architecture only. It does not change source code, Registry behavior, PNP behavior, API behavior, Logic behavior, or validation behavior.

## Reviewed Inputs

- `MILESTONE_6_1_MULTI_PROVIDER_CAPABILITY_PLAN.md`
- `src/registry/capability_provider_record.h`
- `src/registry/registry.h`
- `src/registry/registry.cpp`

## 1. Active Provider Concept

An active provider is the provider selected by Registry to satisfy reads for a canonical capability ID.

Example:

```text
CAP_TEMPERATURE
  provider-sim-temperature-001      SIMULATED  AVAILABLE  priority 10
  provider-wireless-temperature-001 WIRELESS   AVAILABLE  priority 20

active provider -> provider-wireless-temperature-001
```

Logic still queries only:

```text
CAP_TEMPERATURE
```

Registry resolves that query to the active provider payload internally.

Logic must never see or depend on:

- provider ID
- module ID
- device ID
- node ID
- transport
- wired or wireless origin

## 2. Provider Priority Rules

Each `CapabilityProviderRecord` already contains:

```text
priority
```

Selection rule:

```text
higher priority wins among eligible providers
```

Suggested default priority bands:

| Provider Type | Suggested Priority |
|---|---:|
| real wired | 200 |
| wireless | 150 |
| simulated | 50 |
| unknown | 0 |

If two eligible providers have the same priority, Registry must use a deterministic tie-breaker:

```text
lowest provider table index wins
```

No random selection is allowed.

## 3. Freshness Rules

Each provider has:

```text
last_update_ms
status
latest_payload.stale
latest_payload.available
```

A provider is fresh only when:

```text
status == AVAILABLE
latest_payload.available == Availability::AVAILABLE
latest_payload.stale == StaleState::FRESH
```

Provider selection should prefer fresh providers over stale providers.

Recommended v1 eligibility order:

1. `AVAILABLE` and `FRESH`
2. `AVAILABLE` and `STALE`
3. `STALE`

Providers with these statuses are not eligible for normal active selection:

```text
UNAVAILABLE
LOST
DISABLED
UNKNOWN
```

Freshness must be evaluated with bounded table scans only:

```text
for each provider in MAX_CAPABILITY_PROVIDERS
```

No history is stored.

## 4. Failover Rules

Failover occurs when the current active provider becomes ineligible.

Ineligible examples:

- provider status becomes `UNAVAILABLE`
- provider status becomes `LOST`
- provider status becomes `DISABLED`
- provider payload becomes unavailable
- provider becomes stale beyond the documented timeout window

Failover behavior:

1. Registry keeps canonical `CAP_*` identity unchanged.
2. Registry scans bounded provider records for the same `capability_id`.
3. Registry selects the highest-priority eligible provider.
4. Registry updates active provider metadata.
5. Registry exposes the selected provider payload through normal capability reads.

Example:

```text
wired temperature available
wireless temperature available
-> select highest priority

wired lost
wireless available
-> automatic failover to wireless
```

If no provider is eligible:

```text
getCapabilityPayload(CAP_TEMPERATURE)
-> unavailable payload for CAP_TEMPERATURE
```

The error code should remain compact and canonical, such as:

```text
ERR_CAPABILITY_UNAVAILABLE
```

## 5. Recovery Rules

Recovery occurs when a previously ineligible higher-priority provider becomes eligible again.

Example:

```text
wired lost
wireless active
wired returns
```

Recommended v1 behavior:

```text
automatic recovery to highest-priority eligible provider
```

This keeps selection deterministic and simple.

Recovery must not notify or require Logic changes. Logic continues to query:

```text
CAP_TEMPERATURE
```

Optional future behavior:

```text
hysteresis or hold-down window
```

That is deferred until real wireless loss/recovery timing is measured.

## 6. Registry Storage Requirements

Current provider storage:

```text
MAX_CAPABILITY_PROVIDERS = 16
CapabilityProviderRecord capability_providers_[MAX_CAPABILITY_PROVIDERS]
uint8_t capability_provider_count_
```

Additional future storage required for active selection:

```text
active_provider_index per canonical capability
active_provider_valid flag
selected_payload per canonical capability or selected-provider read path
selection_policy per canonical capability, optional
```

The selected provider can be stored in one of two bounded ways.

### Option A: Extend CapabilityRecord

Add compact fields:

```text
bool active_provider_valid
uint8_t active_provider_index
```

Then existing `CapabilityRecord.latest_payload` remains the aggregate selected payload.

This is recommended for v1 because existing Logic and API reads already use `CapabilityRecord.latest_payload`.

### Option B: Separate Selection Table

Create a fixed table:

```text
CapabilityProviderSelection selections[MAX_CAPABILITIES]
```

This is more explicit but consumes additional memory.

### Registry Ownership

Registry owns provider selection state because Registry owns capability state.

Registry must not:

- poll Devices
- call Drivers
- execute Services
- run Logic
- expose API
- allocate memory dynamically

Provider updates come from Services or PNP/transport services through public Registry APIs.

## 7. API Visibility

Default API capability reads remain provider-blind:

```text
GET /capabilities/CAP_TEMPERATURE/state
-> selected payload only
```

Future diagnostics may expose:

```text
active_provider_id
active_provider_type
active_provider_status
provider_count
provider priority
provider last_update_ms
```

Possible future internal API methods:

```text
getCapabilityProviderSummary(capability_id, out_summary)
getActiveProvider(capability_id, out_provider)
getCapabilityProviderByIndex(index, out_provider)
```

Dashboard and Mobile Studio may show friendly provider names, but saved rules and Logic must continue to use `CAP_*` IDs.

## 8. Logic Behavior

Logic behavior does not change.

Logic query:

```text
registry.getCapabilityPayload(CAP_TEMPERATURE, payload)
```

Registry returns:

```text
active provider selected payload
```

Logic must not:

- inspect provider records
- prefer wired over wireless
- use node IDs
- use module names
- use device IDs
- know whether failover occurred

If Logic needs diagnostics, it must query diagnostic capability IDs such as:

```text
CAP_BATTERY_LEVEL
CAP_SIGNAL_STRENGTH
```

## 9. Validation Requirements

Provider selection validation must be added only after active selection APIs exist.

Required validation cases:

### Priority Selection

```text
wired temperature available, priority 200
wireless temperature available, priority 150
-> active provider is wired
```

```text
simulated temperature available, priority 10
wireless temperature available, priority 20
-> active provider is wireless
```

### Failover

```text
wired active
wired becomes LOST
wireless available
-> active provider becomes wireless
-> Logic still queries CAP_TEMPERATURE
```

### Recovery

```text
wireless active after wired loss
wired returns AVAILABLE and FRESH
-> active provider returns to wired if priority is higher
```

### Stale Handling

```text
provider payload stale
fresh alternate available
-> select fresh alternate
```

### No Eligible Provider

```text
all providers unavailable/lost/disabled
-> CAP_TEMPERATURE read returns unavailable payload
```

### Provider-Blind Logic

Validation must prove:

- Logic status is based on `CAP_TEMPERATURE`
- Logic does not read provider IDs
- Logic behavior survives active provider changes

### Bounded Storage

Validation must prove:

- provider table is bounded
- duplicate provider ID is rejected
- same capability from different providers is accepted only after future duplicate rules are implemented
- provider selection never requires dynamic allocation

## 10. Future Manual Override

Manual override is future work.

Manual override concept:

```text
capability_id = CAP_TEMPERATURE
manual_provider_id = provider-wireless-temperature-001
manual_override_enabled = true
```

Rules:

- override is stored as bounded Registry state
- override is visible through API diagnostics
- override must fail safely if selected provider becomes unavailable
- automatic failover may still be allowed for safety unless explicitly disabled by future policy
- Logic still queries only `CAP_TEMPERATURE`

Manual override must not be implemented until:

- active provider selection is validated
- provider diagnostics are exposed
- failover and recovery behavior are tested

## Example Selection Scenarios

### Wired And Wireless Available

```text
provider-wired-temperature-001:
  capability_id = CAP_TEMPERATURE
  provider_type = WIRED
  status = AVAILABLE
  priority = 200

provider-wireless-temperature-001:
  capability_id = CAP_TEMPERATURE
  provider_type = WIRELESS
  status = AVAILABLE
  priority = 150

selected -> provider-wired-temperature-001
```

### Wired Lost, Wireless Available

```text
provider-wired-temperature-001:
  status = LOST

provider-wireless-temperature-001:
  status = AVAILABLE

selected -> provider-wireless-temperature-001
```

### Wired Returns

```text
provider-wired-temperature-001:
  status = AVAILABLE
  latest_payload.stale = FRESH
  priority = 200

provider-wireless-temperature-001:
  status = AVAILABLE
  priority = 150

selected -> provider-wired-temperature-001
```

## Stop Conditions

Stop implementation and review architecture if:

- Logic depends on provider IDs
- Logic depends on module/device/node identity
- Provider storage becomes unbounded
- Provider selection requires dynamic allocation
- Provider selection requires Arduino `String`
- Provider selection requires STL containers
- Registry starts polling Devices or Drivers
- Registry executes Service policy
- Runtime owns provider selection policy
- API bypasses Registry for state
- duplicate protection is removed instead of moved to provider identity
- failover changes `CAP_*` IDs visible to Logic

## Final Assessment

Active provider selection belongs in Registry because Registry owns canonical capability state.

The intended model is:

```text
many providers
one canonical capability ID
one active provider
one selected payload returned to Logic
```

This preserves Cyber32 capability-first behavior while allowing wired, simulated, and wireless providers to coexist behind the same `CAP_*` contract.
