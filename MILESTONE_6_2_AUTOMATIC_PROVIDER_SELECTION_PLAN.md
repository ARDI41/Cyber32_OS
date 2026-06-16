# Milestone 6.2 Automatic Provider Selection Plan

## Goal

Define automatic provider selection and failover for multi-provider capabilities.

This document is documentation only. It does not change source code, Registry behavior, Services, API, Logic, PNP, validation, or `src/main.cpp`.

## Reviewed Inputs

- `MILESTONE_6_1_PROVIDER_SELECTION_PLAN.md`
- `MILESTONE_6_1_MULTI_PROVIDER_CAPABILITY_PLAN.md`

## Core Rule

Automatic provider selection must preserve capability-first behavior.

Logic reads:

```text
CAP_TEMPERATURE
```

Registry may select among multiple providers internally, but Logic must never depend on:

- provider ID
- module ID
- device ID
- node ID
- transport
- wired or wireless origin

## 1. Selection Scope

Automatic selection applies only within providers that share the same canonical `capability_id`.

Example:

```text
CAP_TEMPERATURE
  provider-sim-temperature-001
  provider-wireless-temperature-001
```

Registry must not select a provider whose `capability_id` differs from the requested capability.

## 2. First Available Policy

The simplest automatic policy is:

```text
select the first eligible provider in provider table order
```

A provider is eligible when:

```text
provider.capability_id == requested capability_id
provider.status == AVAILABLE
provider.latest_payload.available == AVAILABLE
```

Tie-breaking is inherent:

```text
lowest provider table index wins
```

Use cases:

- initial bring-up
- minimal deterministic selection
- fallback when priority configuration is not trusted yet

Limitations:

- does not prefer real hardware over simulated providers
- does not prefer fresher payloads
- depends on registration order

## 3. Priority Policy

Priority-based selection chooses the eligible provider with the highest `priority`.

Eligibility:

```text
same capability_id
status == AVAILABLE
latest_payload.available == AVAILABLE
```

Selection:

```text
highest priority wins
```

Tie-breaker:

```text
lowest provider table index wins
```

Suggested default priority bands:

| Provider Type | Priority |
|---|---:|
| real wired | 200 |
| wireless | 150 |
| simulated | 50 |
| unknown | 0 |

Example:

```text
wired temperature available, priority 200
wireless temperature available, priority 150
-> select wired
```

This should become the preferred v1 policy once provider registration can assign stable priorities.

## 4. Freshness Policy

Freshness prevents stale data from winning only because it has higher priority.

A provider is fresh when:

```text
provider.status == AVAILABLE
provider.latest_payload.available == AVAILABLE
provider.latest_payload.stale == FRESH
```

Selection order:

1. fresh available providers
2. stale available providers only if no fresh provider exists
3. no provider selected if all providers are unavailable, lost, disabled, or unknown

Within the same freshness class:

```text
highest priority wins
```

If priority ties:

```text
newer last_update_ms wins
```

If timestamps tie:

```text
lowest provider table index wins
```

Freshness must use bounded scans only:

```text
for i in 0..MAX_CAPABILITY_PROVIDERS - 1
```

No packet history, payload history, heap allocation, or STL containers are allowed.

## 5. Stale, Unavailable, And Lost Handling

Provider status meanings for selection:

| Status | Selection Behavior |
|---|---|
| `AVAILABLE` | eligible if payload is available |
| `STALE` | eligible only if no fresh available provider exists |
| `UNAVAILABLE` | not eligible |
| `LOST` | not eligible |
| `DISABLED` | not eligible |
| `UNKNOWN` | not eligible |

Payload state also matters:

| Payload State | Selection Behavior |
|---|---|
| `available == AVAILABLE`, `stale == FRESH` | preferred eligible |
| `available == AVAILABLE`, `stale == STALE` | fallback eligible |
| `available != AVAILABLE` | not eligible |

If the active provider becomes stale and a fresh alternate exists:

```text
fail over to fresh alternate
```

If the active provider becomes unavailable or lost:

```text
fail over to the best eligible provider
```

If no eligible provider exists:

```text
active provider mapping may be cleared or marked invalid
canonical payload becomes unavailable
```

## 6. Failover Behavior

Failover is triggered when the active provider is no longer the best eligible provider.

Triggers:

- active provider becomes `UNAVAILABLE`
- active provider becomes `LOST`
- active provider becomes `DISABLED`
- active provider payload becomes unavailable
- active provider becomes stale while a fresh alternate exists
- higher-priority eligible provider appears and recovery policy allows switching

Failover steps:

1. Registry scans bounded provider storage for the same `capability_id`.
2. Registry evaluates eligibility.
3. Registry selects the best provider by policy.
4. Registry updates active provider mapping.
5. Registry updates selected canonical payload from provider `latest_payload`.
6. Logic continues reading the same `CAP_*` ID.

Example:

```text
wired CAP_TEMPERATURE active
wired provider becomes LOST
wireless CAP_TEMPERATURE is AVAILABLE and FRESH
-> active provider becomes wireless
-> selected payload becomes wireless latest_payload
```

## 7. Recovery Behavior

Recovery occurs when a previously failed or stale provider becomes eligible again.

Default recovery rule:

```text
select best eligible provider every time selection is evaluated
```

That means a recovered higher-priority provider can become active again automatically.

Example:

```text
wired priority 200, LOST
wireless priority 150, active
wired returns AVAILABLE and FRESH
-> active provider returns to wired
```

Future optional recovery controls:

- hold-down timer before switching back
- manual override
- sticky active provider until failure
- minimum freshness age

These controls are deferred until the simple bounded policy is validated.

## 8. Selected Payload Update

The selected payload is the payload returned by normal capability reads:

```text
getCapabilityPayload(CAP_TEMPERATURE)
```

Recommended v1 behavior:

1. Provider payload update occurs through a Registry provider API.
2. Registry stores the provider `latest_payload`.
3. Registry evaluates active provider selection for that capability.
4. Registry copies the active provider `latest_payload` into the canonical `CapabilityRecord.latest_payload`.
5. Existing Logic and API reads continue to use existing capability payload APIs.

Registry must not transform provider payload values.

Registry may copy:

```text
capability_id
schema_version
timestamp_ms
available
stale
value_type
value_float
value_int
unit
quality
error_code
```

If no provider is eligible, Registry should write a bounded unavailable payload for the canonical capability:

```text
available = UNAVAILABLE
stale = STALE
value_type = NONE
error_code = ERR_CAPABILITY_UNAVAILABLE
```

No averaging, fusion, smoothing, filtering, or unit conversion is part of automatic provider selection.

## 9. Registry Responsibilities

Registry owns:

- provider table storage
- active provider mapping
- bounded provider scan
- selection policy execution
- selected payload copy
- unavailable selected payload when no provider is eligible

Registry must not:

- poll Devices
- call Drivers
- call Services
- run Logic
- expose API
- allocate memory dynamically
- keep packet history
- infer wireless behavior from module names

Runtime does not own selection policy.

Services provide updates to Registry; Registry selects among stored provider state.

## 10. API Visibility

Default capability state API remains provider-blind:

```text
GET /capabilities/CAP_TEMPERATURE/state
-> selected payload
```

Future diagnostics may expose:

```text
active_provider_id
active_provider_type
active_provider_status
provider_count
provider priority
provider freshness
provider last_update_ms
```

Diagnostics must not become the control path for Logic.

## 11. Validation Cases

### First Available

```text
provider A: CAP_TEMPERATURE, AVAILABLE, priority 10
provider B: CAP_TEMPERATURE, AVAILABLE, priority 10
policy = first_available
expect active provider A
```

### Priority

```text
simulated: priority 10, AVAILABLE
wireless: priority 20, AVAILABLE
expect active provider wireless
```

```text
wired: priority 200, AVAILABLE
wireless: priority 150, AVAILABLE
expect active provider wired
```

### Freshness

```text
wired: priority 200, AVAILABLE, STALE
wireless: priority 150, AVAILABLE, FRESH
expect active provider wireless
```

### Failover

```text
wired active
wired becomes LOST
wireless AVAILABLE and FRESH
expect active provider wireless
expect getCapabilityPayload(CAP_TEMPERATURE) returns wireless payload
```

### Recovery

```text
wireless active after wired loss
wired returns AVAILABLE and FRESH with higher priority
expect active provider wired
expect Logic still reads CAP_TEMPERATURE only
```

### No Eligible Provider

```text
all CAP_TEMPERATURE providers UNAVAILABLE or LOST
expect active provider missing or invalid
expect canonical CAP_TEMPERATURE payload unavailable
```

### Capability Isolation

```text
CAP_DISTANCE provider available
CAP_TEMPERATURE provider lost
expect CAP_DISTANCE provider is never selected for CAP_TEMPERATURE
```

### Logic Provider Blindness

```text
Logic evaluates CAP_TEMPERATURE
active provider changes
Logic still has no provider ID dependency
```

### Bounded Behavior

Validate:

- selection scans no more than `MAX_CAPABILITY_PROVIDERS`
- active mapping count remains bounded
- duplicate providers are still rejected
- no dynamic allocation is required
- no Arduino `String`
- no STL containers

## 12. Stop Conditions

Stop implementation and review architecture if:

- Logic reads provider IDs
- provider selection changes `CAP_*` IDs
- Registry needs unbounded provider lists
- Registry stores payload history
- Registry calls Devices or Drivers
- Runtime owns provider selection
- Services bypass Registry for active payload
- API bypasses Registry state
- automatic selection requires dynamic allocation
- automatic selection requires Arduino `String`
- automatic selection requires STL containers

## Final Assessment

Automatic provider selection should be deterministic, bounded, and Registry-owned.

The v1 target policy is:

```text
fresh available providers first
highest priority wins
newest update breaks priority ties
lowest provider index breaks final ties
```

Logic remains provider-blind, and normal capability reads return the selected payload for the canonical `CAP_*` ID.
