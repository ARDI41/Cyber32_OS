# Milestone 6.4 Selected Provider Payload Plan

## Goal

Design how Registry updates the canonical capability payload from the selected active provider.

This document is documentation only. It does not change source code, does not modify `src/main.cpp`, and does not change current Registry, WirelessService, Logic, or API behavior.

## Reviewed Inputs

- `MILESTONE_6_1_MULTI_PROVIDER_CAPABILITY_PLAN.md`
- `MILESTONE_6_2_AUTOMATIC_PROVIDER_SELECTION_PLAN.md`
- `MILESTONE_6_3_WIRELESS_SERVICE_PLAN.md`

## 1. Selected Provider Payload Concept

Cyber32 supports multiple providers for one canonical capability ID:

```text
CAP_TEMPERATURE
  provider-sim-temperature-001
  provider-wireless-temperature-001
```

Each provider stores its own latest payload:

```text
CapabilityProviderRecord.latest_payload
```

The canonical capability record stores the payload returned to normal readers:

```text
CapabilityRecord.latest_payload
```

The selected provider payload is the active provider's `latest_payload` copied into the canonical capability record.

This preserves the existing read contract:

```text
Logic -> getCapabilityPayload(CAP_TEMPERATURE)
API   -> getTemperatureState()
```

Neither Logic nor API needs to know which provider supplied the payload.

## 2. Registry Method

Add a future Registry method:

```cpp
RegistryResult updateSelectedCapabilityPayload(const char* capability_id);
```

Purpose:

```text
copy active provider latest_payload into canonical capability latest_payload
```

Registry owns this because Registry owns:

- canonical capability state
- provider records
- active provider mapping
- selected payload state

## 3. Required Behavior

`updateSelectedCapabilityPayload(capability_id)` must:

1. Validate `capability_id`.
2. Find the active provider mapping for `capability_id`.
3. Find the provider record by active `provider_id`.
4. Confirm provider capability matches `capability_id`.
5. Confirm provider payload capability matches `capability_id`.
6. Find canonical capability record.
7. Copy provider `latest_payload` into canonical `CapabilityRecord.latest_payload`.
8. Return `RegistryResult::OK`.

Logical flow:

```text
capability_id
-> getActiveProvider(capability_id)
-> getCapabilityProvider(active.provider_id)
-> validate provider.latest_payload.capability_id
-> findCapabilityIndex(capability_id)
-> capabilities_[index].latest_payload = provider.latest_payload
```

No payload transformation is allowed.

Allowed copy fields:

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

## 4. Provider-Blind Logic

Logic remains unchanged.

Logic continues to query:

```text
CAP_TEMPERATURE
```

Logic must not:

- read provider IDs
- read node IDs
- read module IDs
- read device IDs
- know whether provider is wired, simulated, or wireless

After selected payload update:

```text
getCapabilityPayload(CAP_TEMPERATURE)
-> canonical latest_payload copied from active provider
```

## 5. API Behavior

Normal API state reads remain unchanged.

Example:

```text
GET /capabilities/CAP_TEMPERATURE/state
```

or internal:

```cpp
api.getTemperatureState(out_state)
```

returns the canonical selected payload.

Provider diagnostics may be exposed later, but default capability state must remain provider-blind.

## 6. Failure Behavior

### Invalid Capability ID

Condition:

```text
capability_id is null or empty
```

Result:

```text
RegistryResult::INVALID_ID
```

No payload changes.

### No Active Provider

Condition:

```text
getActiveProvider(capability_id) returns NOT_FOUND
```

Result:

```text
RegistryResult::NOT_FOUND
```

No payload changes in the initial implementation.

Future behavior may optionally write an unavailable canonical payload, but that should be a separate phase.

### Provider Missing

Condition:

```text
active provider_id no longer exists in provider table
```

Result:

```text
RegistryResult::NOT_FOUND
```

No payload changes.

### Provider Unavailable

Condition:

```text
provider.status == UNAVAILABLE
provider.status == LOST
provider.status == DISABLED
```

Result:

```text
RegistryResult::UNAVAILABLE
```

No payload copy.

Future failover should run before this method if another provider is available.

### Provider Stale

Condition:

```text
provider.status == STALE
```

Initial behavior:

```text
allow copy only if provider.latest_payload.available == AVAILABLE
and provider.latest_payload.stale == STALE
```

This lets Logic see stale explicitly.

If stale should be blocked later, that must be documented as a policy change.

### Payload Capability Mismatch

Condition:

```text
provider.capability_id != capability_id
or provider.latest_payload.capability_id != capability_id
```

Result:

```text
RegistryResult::INVALID_RECORD
```

No payload changes.

### Canonical Capability Missing

Condition:

```text
findCapabilityIndex(capability_id) == NOT_FOUND
```

Result:

```text
RegistryResult::NOT_FOUND
```

No payload changes.

## 7. WirelessService Flow

Once selected payload support exists, WirelessService packet processing may use this flow:

```text
read simulated packet
-> WirelessTemperatureDevice::updateFromPacket(...)
-> WirelessTemperatureDevice::readPayload(...)
-> Registry::updateCapabilityProviderPayload(provider_id, payload, AVAILABLE, now_ms)
-> Registry::selectBestProvider(CAP_TEMPERATURE, selected)
-> Registry::setActiveProvider(CAP_TEMPERATURE, selected.provider_id)
-> Registry::updateSelectedCapabilityPayload(CAP_TEMPERATURE)
```

Responsibilities remain separated:

- WirelessService processes packet and triggers public Registry APIs.
- Registry stores provider facts.
- Registry selects provider.
- Registry copies selected payload.
- Logic reads canonical capability only.

WirelessService must not write Registry arrays directly.

WirelessService must not manually copy into `CapabilityRecord`.

## 8. Validation Requirements

Validation must prove the selected provider payload path without changing Logic.

Required cases:

### Existing Wired/Sim Provider Still Works

```text
provider-sim-temperature-001 has value 22.4F
canonical CAP_TEMPERATURE initially returns 22.4F
```

### Wireless Provider Receives Packet

```text
wireless packet value = 23.5F
WirelessService updates provider-wireless-temperature-001 latest_payload
provider latest_payload.value_float == 23.5F
```

### Best Provider Selection

```text
wireless provider priority > simulated provider priority
selectBestProvider(CAP_TEMPERATURE)
-> provider-wireless-temperature-001
```

### Selected Payload Update

```text
setActiveProvider(CAP_TEMPERATURE, provider-wireless-temperature-001)
updateSelectedCapabilityPayload(CAP_TEMPERATURE)
-> OK
```

### Canonical Read Changes

```text
getCapabilityPayload(CAP_TEMPERATURE)
-> value_float == 23.5F
```

### Logic Remains Provider-Blind

```text
TemperatureLogic::evaluate()
queries CAP_TEMPERATURE only
lastTemperature() == 23.5F
status == TEMPERATURE_SEEN
```

### Failure Cases

Validate:

- no active provider returns `NOT_FOUND`
- active provider missing returns `NOT_FOUND`
- unavailable provider returns `UNAVAILABLE`
- payload mismatch returns `INVALID_RECORD`
- canonical payload remains unchanged after failure

## 9. Stop Conditions

Stop implementation and review architecture if:

- Logic reads provider IDs
- API normal state reads require provider IDs
- WirelessService writes canonical payload directly
- Registry calls WirelessService
- Registry calls Devices or Drivers
- Runtime chooses providers
- selected payload update transforms sensor values
- provider storage becomes unbounded
- dynamic allocation is introduced
- Arduino `String` is introduced
- STL containers are introduced

## Final Assessment

`updateSelectedCapabilityPayload(capability_id)` is the final bridge between multi-provider storage and existing capability-first reads.

The intended flow is:

```text
provider payload updated
-> Registry selects active provider
-> Registry copies selected provider payload into canonical capability payload
-> Logic/API continue reading CAP_TEMPERATURE normally
```

This keeps Cyber32 provider-aware internally while preserving provider-blind Logic and API contracts.
