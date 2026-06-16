# Milestone 6.1 Multi-Provider Capability Plan

## Goal

Design how Cyber32 supports multiple providers for the same capability ID.

This document is documentation only. It does not change source code, does not change Registry behavior, and does not implement multi-provider support.

## Problem

Both wired/simulated temperature and wireless temperature may expose:

```text
CAP_TEMPERATURE
```

Current Registry duplicate-ID rules block the second provider. That behavior is correct for the current single-provider Registry, but it does not support the Cyber32 hardware-agnostic goal once wired and wireless providers can expose equivalent capabilities.

Logic must still use:

```text
CAP_TEMPERATURE
```

Logic must not know whether the active provider is wired, simulated, or wireless.

## Reviewed Documents

- `REGISTRY_SCHEMA.md`
- `CAPABILITY_SCHEMA.md`
- `PRODUCTION_BOOTSTRAP_PLAN.md`
- `MILESTONE_6_ESPNOW_ARCHITECTURE_PLAN.md`
- `MILESTONE_6_2_WIRELESS_IMPLEMENTATION_PLAN.md`

## 1. Provider Concept

A provider is one concrete source of a canonical capability.

Example providers:

```text
simulated temperature device -> CAP_TEMPERATURE
wireless temperature node -> CAP_TEMPERATURE
future real I2C temperature sensor -> CAP_TEMPERATURE
```

Provider fields:

```text
capability_id
owner_device_index
owner_module_index
provider_id
provider_type
priority
status
latest_payload
last_update_ms
```

### capability_id

Canonical capability ID, such as:

```text
CAP_TEMPERATURE
```

Multiple providers may share the same `capability_id`.

### owner_device_index

Registry Device table index for the provider Device.

This remains internal Registry/provider metadata. Logic must not depend on it.

### owner_module_index

Registry Module table index for the provider Module.

This allows diagnostics and API provider visibility.

### provider_id

Stable provider identity.

Suggested v1 format:

```text
provider:<owner_device_index>:<capability_id>
```

For fixed-size ESP32 storage, implementation may use compact numeric IDs instead:

```text
uint8_t provider_index
```

Provider identity must be unique within the Registry provider table.

### provider_type

Allowed v1 provider types:

```text
wired
simulated
wireless
```

Future provider types may include:

```text
service
virtual
gateway
```

### priority

Provider selection priority.

Suggested range:

```text
0..255
```

Higher value means preferred provider when multiple providers are available.

### status

Provider availability state:

```text
AVAILABLE
STALE
UNAVAILABLE
LOST
DISABLED
ERROR
```

Provider status is separate from the canonical capability concept.

## 2. Registry Storage Model

Registry should separate:

```text
canonical capability
provider records
selected active provider
per-provider payload
selected aggregate payload
```

### Canonical Capability Record

There should be one canonical capability concept per `capability_id`.

Example:

```text
CAP_TEMPERATURE
category = sensors
kind = sensor
data_type = FLOAT
access = read
```

The canonical record answers:

```text
Does the system have CAP_TEMPERATURE as a concept?
What is the selected active payload?
```

### Provider Records

Provider records answer:

```text
Who can provide CAP_TEMPERATURE?
What is each provider's latest state?
Which provider is selected?
```

Suggested fixed table:

```text
MAX_CAPABILITY_PROVIDERS = bounded value
```

ProviderRecord fields:

```text
provider_id
capability_id
owner_module_index
owner_device_index
provider_type
priority
status
latest_payload
last_update_ms
```

### Selected Active Provider

Canonical capability state stores:

```text
active_provider_index
selected_payload
selection_policy
```

When Logic queries `CAP_TEMPERATURE`, Registry returns:

```text
selected_payload from active_provider_index
```

### Latest Payload Per Provider

Each provider stores its own latest payload.

Example:

```text
provider 0: simulated CAP_TEMPERATURE = 22.4 C
provider 1: wireless CAP_TEMPERATURE = 23.1 C
```

### Aggregate/Selected Payload

The canonical capability record stores or derives the selected payload.

For v1, the selected payload should be a copy for compact reads:

```text
CapabilityPayload selected_payload
```

Registry must not average, fuse, or transform sensor values unless a documented Service owns that policy.

## 3. Selection Policy

Provider selection must be deterministic and bounded.

Supported policies:

```text
first_available
priority_based
freshest_payload
manual_override_future
```

### Default: First Available

Initial v1 policy:

```text
select first provider with status AVAILABLE
```

This is simple, deterministic, and ESP32-friendly.

### Priority-Based

Provider priority may prefer a real wired sensor over simulated or wireless:

```text
wired priority 200
wireless priority 150
simulated priority 50
```

If priorities tie, use lower provider index or first registered.

### Freshest Payload

Freshest selection may choose the provider with the most recent valid payload.

This must be bounded:

```text
compare last_update_ms for providers of the same capability
```

Freshest policy should not be default until stale/lost behavior is fully validated.

### Manual Override Future

Future API/Dashboard may allow a user to select the active provider.

Rules:

- manual override is future work
- override must be stored as bounded Registry state
- Logic still queries `CAP_TEMPERATURE`
- Dashboard may show provider names, but must preserve capability IDs

## 4. Logic Read Model

Logic remains unchanged.

Logic query:

```text
getCapabilityPayload(CAP_TEMPERATURE)
```

Registry behavior:

```text
find canonical CAP_TEMPERATURE
find active_provider_index
return selected provider payload
```

Logic must not see:

- provider ID
- node ID
- module ID
- device ID
- transport
- priority

Logic may use diagnostics capabilities separately:

```text
CAP_BATTERY_LEVEL
CAP_SIGNAL_STRENGTH
```

But Logic still binds by capability ID only.

## 5. API Visibility

API may expose provider information for diagnostics.

Allowed API visibility:

```text
active_provider
provider_count
provider_type
provider_status
last_update_ms
owner module/device display metadata
```

Potential future endpoints:

```text
GET /capabilities/CAP_TEMPERATURE
GET /capabilities/CAP_TEMPERATURE/providers
GET /capabilities/CAP_TEMPERATURE/providers/active
```

Rules:

- API state reads still return selected capability payload by default
- API commands must still go through Services
- provider list is diagnostics, not Logic binding
- Dashboard may display wired vs wireless, but saved rules still use `CAP_*`

## 6. PNP Registration Changes

Current behavior:

```text
duplicate capability_id -> registration fails
```

Future multi-provider behavior:

```text
same capability_id + different provider -> accepted
same provider identity -> rejected
```

PNP Registration should:

1. register ModuleRecord
2. register DeviceRecord
3. register or find canonical CapabilityRecord
4. register ProviderRecord
5. update provider selection if needed

Duplicate rules:

| Case | Future Result |
|---|---|
| same module/device/capability provider repeats | reject duplicate provider |
| different module/device provides same capability | accept as new provider |
| capability ID unknown or invalid | reject |
| provider table full | return `ERR_REGISTRY_FULL` or `TABLE_FULL` |

Current Registry duplicate protection must not be removed entirely. It must move to the correct identity boundary:

```text
canonical capability ID is unique
provider identity is unique
```

## 7. Validation Plan

Required validation cases:

1. Register wired/simulated `CAP_TEMPERATURE`.
2. Register wireless `CAP_TEMPERATURE`.
3. Registry has one canonical `CAP_TEMPERATURE` concept.
4. Registry has two providers for `CAP_TEMPERATURE`.
5. Logic query for `CAP_TEMPERATURE` returns active provider payload.
6. Logic does not know provider ID.
7. API can expose active provider diagnostics.
8. Provider failover does not require Logic changes.
9. Duplicate registration of the same provider fails.
10. Provider table full returns bounded error.

### Example

Initial providers:

```text
provider 0: simulated temperature, AVAILABLE, value 22.4
provider 1: wireless temperature, AVAILABLE, value 23.1
```

Registry canonical query:

```text
getCapabilityPayload(CAP_TEMPERATURE)
-> selected provider payload
```

If provider 0 becomes unavailable:

```text
Registry selection updates to provider 1
Logic query stays CAP_TEMPERATURE
Logic receives wireless provider payload
Logic does not know transport changed
```

## 8. Stop Conditions

Stop implementation and review architecture if:

- Logic depends on provider ID
- Logic depends on module ID, device ID, node ID, or transport
- Registry loses duplicate protection entirely
- provider storage becomes unbounded
- dynamic allocation is required
- Arduino `String` is required
- STL containers are required
- provider selection requires scanning unbounded lists
- API command path bypasses Services
- Registry executes command or provider policy
- Runtime owns provider selection policy

## Required Registry Concepts Before Implementation

Before source changes, define:

- `ProviderRecord`
- provider table size
- provider registration result codes
- active provider selection method
- provider payload update method
- provider status update method
- canonical capability lookup behavior
- API diagnostics response shape

## Final Assessment

Cyber32 should support multiple providers by keeping capability IDs canonical and introducing bounded provider records beneath each capability.

The correct model is:

```text
one capability ID
many bounded providers
one selected active provider
one payload returned to Logic
```

This preserves the capability-first architecture while allowing wired, simulated, and wireless providers to coexist.
