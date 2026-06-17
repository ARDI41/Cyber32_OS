# Milestone 7.0 Provider Diagnostics API Plan

## Goal

Design API visibility for provider diagnostics without changing capability-first normal reads.

This document is documentation only. It does not change source code, does not modify `src/main.cpp`, and does not change current API, Registry, Logic, Runtime, Services, or validation behavior.

## Reviewed Inputs

- `CYBER32_BIBLE.md`
- `MILESTONE_6_9_WIRELESS_ARCHITECTURE_AUDIT.md`
- `MILESTONE_7_CAPABILITY_PROVIDER_ROADMAP.md`

## Problem

Registry already stores:

- provider records
- active provider mapping
- provider health
- provider priority
- provider `last_update_ms`
- provider latest payload
- selected canonical payload

The internal API does not yet expose provider diagnostics.

Normal capability reads must remain unchanged:

```text
getTemperatureState()
getDistanceState()
getServoPositionState()
getMotorControlState()
getRelayControlState()
```

Provider diagnostics are an additional diagnostic surface, not a replacement for capability-first reads.

## 1. Diagnostics Purpose

Provider diagnostics exist to explain where capability data is coming from and whether providers are healthy.

Diagnostics should show:

- provider health
- active provider for a capability
- provider type
- provider priority
- `last_update_ms`
- latest provider payload status
- latest provider payload stale state
- latest provider payload error code
- future battery diagnostics
- future signal diagnostics

Diagnostics are useful for:

- validation
- production troubleshooting
- Dashboard display
- Mobile Studio visibility
- future AI explanation and recommendations

Diagnostics must not make Logic provider-aware.

## 2. Normal Capability Reads Remain Unchanged

Normal API capability reads must continue to return canonical capability state only.

Example:

```text
getTemperatureState()
-> CAP_TEMPERATURE canonical payload
```

It must not return:

- provider list
- provider ID
- provider type
- raw packet data
- node ID
- transport details

Provider diagnostics are separate methods.

## 3. API Structs

### ApiProviderDiagnostic

Purpose:

```text
one provider diagnostic record
```

Suggested fields:

```cpp
struct ApiProviderDiagnostic {
    bool ok;
    const char* provider_id;
    const char* capability_id;
    CapabilityProviderType provider_type;
    CapabilityProviderStatus status;
    bool active;
    uint8_t priority;
    uint32_t last_update_ms;
    Availability payload_available;
    StaleState payload_stale;
    PayloadValueType payload_value_type;
    float payload_value_float;
    int32_t payload_value_int;
    const char* payload_unit;
    const char* payload_quality;
    const char* payload_error_code;
    RegistryResult registry_result;
    const char* error_code;
};
```

Rules:

- fixed fields only
- no dynamic allocation
- no Arduino `String`
- no STL containers
- no raw wireless packet fields
- no packet history

### ApiProviderSummary

Purpose:

```text
bounded provider diagnostics summary
```

Suggested fields:

```cpp
struct ApiProviderSummary {
    bool ok;
    uint8_t provider_count;
    uint8_t active_provider_count;
    uint8_t available_count;
    uint8_t stale_count;
    uint8_t unavailable_count;
    uint8_t lost_count;
    uint8_t disabled_count;
    const char* error_code;
};
```

This avoids returning an unbounded provider list.

Future bounded list APIs may expose one record by index only if needed:

```text
getProviderDiagnosticByIndex(index, out_response)
```

but v1 should start with provider ID and active capability diagnostics.

## 4. API Methods

### getProviderDiagnostic(provider_id, out_response)

Signature plan:

```cpp
bool getProviderDiagnostic(
    const char* provider_id,
    ApiProviderDiagnostic& out_response);
```

Behavior:

```text
read Registry provider record by provider_id
fill diagnostic response
mark active = true if provider is active for its capability
return true on RegistryResult::OK
return false on missing or invalid provider
```

Failure:

```text
missing provider -> RegistryResult::NOT_FOUND
null/empty provider_id -> RegistryResult::INVALID_ID
error_code -> ERR_CAPABILITY_UNAVAILABLE or future diagnostics error
```

### getActiveProviderDiagnostic(capability_id, out_response)

Signature plan:

```cpp
bool getActiveProviderDiagnostic(
    const char* capability_id,
    ApiProviderDiagnostic& out_response);
```

Behavior:

```text
read active provider mapping from Registry
read provider record from Registry
fill diagnostic response
active = true
return true on success
```

Failure:

```text
null/empty capability_id -> INVALID_ID
no active provider -> NOT_FOUND
missing provider record -> NOT_FOUND
```

### getProviderSummary(out_response)

Signature plan:

```cpp
bool getProviderSummary(ApiProviderSummary& out_response);
```

Behavior:

```text
read provider count
read active provider count
compute bounded status counts through Registry-provided summary API or future bounded read API
return true on success
```

Important:

API must not read Registry arrays directly. If summary counts require scanning provider records, Registry should provide a public summary method in a prior or same milestone.

Recommended Registry support:

```cpp
RegistryResult getProviderSummary(ProviderSummaryRecord& out_summary) const;
```

or bounded index read:

```cpp
RegistryResult getCapabilityProviderByIndex(uint8_t index, CapabilityProviderRecord& out_record) const;
```

Registry owns storage access. API consumes public summaries.

## 5. Rules

Provider diagnostics API must follow these rules:

- normal capability reads remain unchanged
- Logic remains provider-blind
- API reads Registry only
- API does not select providers
- API does not update providers
- API does not update provider health
- API does not update canonical payloads
- API does not expose raw wireless packets
- API does not call WirelessService for provider state
- API does not call Devices or Drivers
- API does not call HAL
- provider list behavior must remain bounded
- no dynamic allocation
- no Arduino `String`
- no STL containers

## 6. Validation Plan

Validation should be added after API diagnostics methods are implemented.

### Provider Diagnostic By ID

Setup:

```text
provider-wireless-temperature-001 exists
capability_id = CAP_TEMPERATURE
provider_type = WIRELESS
status = AVAILABLE
priority = 20
latest_payload.value_float = 24.0F
```

Call:

```text
api.getProviderDiagnostic("provider-wireless-temperature-001", response)
```

Expect:

```text
response.ok == true
response.provider_id == "provider-wireless-temperature-001"
response.capability_id == CAP_TEMPERATURE
response.provider_type == WIRELESS
response.status == AVAILABLE
response.priority == 20
response.payload_value_float == 24.0F
```

### Active Provider Diagnostic

Setup:

```text
active provider for CAP_TEMPERATURE = provider-wireless-temperature-001
```

Call:

```text
api.getActiveProviderDiagnostic(CAP_TEMPERATURE, response)
```

Expect:

```text
response.ok == true
response.active == true
response.provider_id == "provider-wireless-temperature-001"
```

### Provider Summary

Call:

```text
api.getProviderSummary(summary)
```

Expect:

```text
summary.ok == true
summary.provider_count == registry.capabilityProviderCount()
summary.active_provider_count == registry.activeProviderCount()
status counts match provider records
```

### Missing Provider

Call:

```text
api.getProviderDiagnostic("provider-missing", response)
```

Expect:

```text
return false
response.ok == false
response.registry_result == RegistryResult::NOT_FOUND
response.error_code != "none"
```

### Normal Capability Read Unchanged

Call:

```text
api.getTemperatureState(state)
```

Expect:

```text
state.ok == true
state.payload.capability_id == CAP_TEMPERATURE
state.payload is canonical selected payload only
state does not expose provider_id
```

## 7. Future Battery And Signal Diagnostics

Wireless provider diagnostics should later include:

- battery present
- battery level percent
- battery voltage
- signal quality percent
- RSSI dBm if available
- last seen timestamp

These may come from future Registry node diagnostic records or provider diagnostic records.

They must not replace main capability payloads.

Example:

```text
CAP_TEMPERATURE remains temperature
provider diagnostics show battery/signal health
```

## 8. Dashboard And Mobile Studio Use

Dashboard and Mobile Studio may show:

- active provider
- provider health
- provider type
- priority
- freshness
- battery/signal diagnostics
- lost/stale warnings

They must not:

- bypass API
- call Devices or Drivers
- store Logic rules using provider IDs
- make core Logic provider-specific

Dashboard/Mobile Studio rules must compile to capability IDs:

```text
CAP_TEMPERATURE
CAP_BATTERY_LEVEL
CAP_SIGNAL_STRENGTH
```

not provider IDs.

## 9. Stop Conditions

Stop and review architecture if any of these occur:

- API changes normal capability read contract
- API exposes raw packet data
- API mutates provider selection
- API updates provider records
- API calls WirelessService for provider state
- API calls Devices or Drivers
- Logic depends on provider diagnostics
- Dashboard rules store provider IDs
- provider diagnostics require dynamic allocation
- provider diagnostics expose unbounded provider lists
- Arduino `String` or STL containers are introduced in core API paths

## Final Assessment

Milestone 7.0 should add read-only provider diagnostics to API without changing normal capability reads.

Correct model:

```text
normal state:
API -> Registry canonical capability payload

diagnostics:
API -> Registry provider diagnostic state
```

This keeps Cyber32 capability-first while making provider health visible to validation, Dashboard, Mobile Studio, and future AI tools.

