# Milestone 10.0 - Capability API Method Implementation Plan

## Purpose

Milestone 10.0 plans future read-only Capability API methods for Cyber32.

These methods will expose capability information through `Cyber32Api` for future UI clients:

- Dev Panel
- Minimal App
- Mission Control
- Dashboard
- Cloud Bridge
- Marketplace
- AI Assistant

All future UI clients must use the same Cyber32 API. Capability API methods must remain internal C++ contracts first and must preserve Cyber32's capability-first, API-first, safety-first architecture.

Planned Capability API structs already exist from earlier API contract phases:

- `ApiCapabilityIdentity`
- `ApiCapabilityValue`
- `ApiCapabilityAvailability`
- `ApiCapabilityProviderInfo`
- `ApiCapabilityQuality`
- `ApiCapabilitySummary`
- `ApiCapabilityList`

Planned future method names:

```cpp
bool getCapabilityList(ApiCapabilityList& out_response);
bool getCapabilitySummary(uint8_t capability_index, ApiCapabilitySummary& out_response);
bool getCapabilityIdentity(uint8_t capability_index, ApiCapabilityIdentity& out_response);
bool getCapabilityValue(uint8_t capability_index, ApiCapabilityValue& out_response);
bool getCapabilityAvailability(uint8_t capability_index, ApiCapabilityAvailability& out_response);
bool getCapabilityProviderInfo(uint8_t capability_index, ApiCapabilityProviderInfo& out_response);
bool getCapabilityQuality(uint8_t capability_index, ApiCapabilityQuality& out_response);
```

This plan does not implement methods.

## 1. Scope

Milestone 10.0 plans read-only Capability API methods only.

The plan covers method purpose, ownership, allowed data sources, failure behavior, placeholder policy, validation expectations, and implementation order.

Capability API methods should eventually support:

- capability list screens
- capability value cards
- sensor detail screens
- stale/offline indicators
- provider diagnostic metadata
- quality/freshness display
- future project pages and templates that bind to `CAP_*` IDs

## 2. Non-Goals

Milestone 10.0 does not implement behavior.

Non-goals:

- No Capability API method implementation
- No behavior changes
- No provider selection
- No command execution
- No packet parsing
- No hardware access
- No WebServer
- No HTTP
- No JSON
- No UI/App code
- No Dashboard
- No Cloud
- No Marketplace
- No AI implementation
- No Registry ownership changes
- No Runtime architecture changes
- No `src/main.cpp` changes

## 3. Read-Only Philosophy

Capability API methods are observation methods only.

They may expose already-known capability state through existing public API-safe owners.

They may read:

- public capability payload state
- public Registry summaries where already supported
- public provider diagnostics where already supported
- stable placeholders where no owner exists yet

They must not:

- mutate Registry
- mutate Runtime
- mutate providers
- mutate diagnostics
- mutate capability payloads
- select providers
- call Services for command behavior
- call Drivers
- call Devices
- call HAL
- parse packets
- execute commands

Capability API methods must never become a hidden provider-selection, packet-ingestion, or hardware-control path.

## 4. Ownership

The API layer is not the owner of capability state. It only formats bounded read-only responses from public owners.

| Data | Owner |
| --- | --- |
| capability identity | capability catalog / Registry capability record / future capability metadata owner |
| capability ID | capability record or catalog |
| capability type | capability payload schema or capability metadata owner |
| capability value | Registry canonical capability payload |
| value unit | capability payload and capability metadata owner |
| value timestamp | capability payload owner |
| availability | capability payload and provider health owner |
| provider info | Registry provider records and active provider mapping |
| provider diagnostics | Registry provider diagnostics API-safe records |
| quality/freshness/staleness | capability payload, provider diagnostics, and future quality owner |
| last update timestamp | capability payload or provider record owner |

Future methods must preserve this ownership split. API may format data; it must not own, derive by packet parsing, or mutate the underlying state.

## 5. Allowed Data Sources

Future Capability API methods may read:

- existing public Registry summaries, if available
- existing public capability state APIs, if available
- existing provider diagnostics, if available
- existing runtime/API-safe read contracts, if available
- stable placeholders where no owner exists yet

Allowed reads must:

- remain bounded
- use public APIs only
- avoid private Registry arrays
- avoid provider selection
- avoid mutation
- avoid hardware access

## 6. Forbidden Data Sources

Future Capability API methods must not read or call:

- private Registry arrays
- Drivers
- Devices
- HAL
- raw packet buffers
- ESP-NOW callbacks
- packet parser internals
- provider selection logic
- Services for command behavior
- UI transport layers

The API must not infer capability state by parsing packets, inspecting transport internals, or calling hardware.

## 7. Method-By-Method Plan

### getCapabilityList(...)

Purpose:

Return a bounded list of capability summaries for UI capability-list screens.

Output struct:

- `ApiCapabilityList`

Required attachment or owner:

- Registry attachment and public capability enumeration owner once capability list data is backed by Registry.

Success behavior:

- return `true`
- `out_response.ok = true`
- `out_response.error_code = "none"`
- fill up to `API_MAX_CAPABILITY_SUMMARY_COUNT` entries
- set `count` to the number of returned summaries

Failure behavior:

- missing Registry, if required: return `false`, `error_code = "registry_not_attached"`
- unavailable owner: return `false`, `error_code = "capability_api_not_available"`

Placeholder policy:

- may return count `0` only if explicitly defined as available with no enumerated capabilities
- must not invent fake capability summaries
- must not fabricate values, units, providers, or quality data

Validation strategy:

- empty list
- bounded count
- repeated reads do not mutate Registry, providers, diagnostics, command state, or capability payloads
- System and Node API validation still passes

Future expansion notes:

- may later enumerate canonical capabilities through public Registry APIs
- should remain bounded by `API_MAX_CAPABILITY_SUMMARY_COUNT`
- should eventually include known wired and wireless capabilities through the same capability/provider model

### getCapabilitySummary(...)

Purpose:

Return one aggregate capability view for a bounded capability index.

Output struct:

- `ApiCapabilitySummary`

Required attachment or owner:

- capability list/index owner and public child data owners for identity, value, availability, provider info, and quality.

Success behavior:

- return `true`
- `out_response.ok = true`
- `out_response.error_code = "none"`
- fill identity, value, availability, provider, and quality fields

Failure behavior:

- invalid index: `invalid_capability_index`
- missing capability: `capability_not_found`
- missing Registry where required: `registry_not_attached`
- unavailable owner: `capability_api_not_available`

Placeholder policy:

- substructures may use placeholders only for fields without current owners
- placeholders must not imply a real provider or fresh value exists

Validation strategy:

- invalid index
- valid summary once a public owner exists
- repeated reads do not change canonical payloads or provider diagnostics

Future expansion notes:

- should compose the smaller Capability API methods once those methods exist
- provider metadata inside the summary remains diagnostic metadata and must not become a Logic dependency

### getCapabilityIdentity(...)

Purpose:

Return capability identity fields such as capability ID, friendly name, category, value type, and unit.

Output struct:

- `ApiCapabilityIdentity`

Required attachment or owner:

- capability catalog, Registry capability record, or future capability metadata owner.

Success behavior:

- return `true`
- `ok = true`
- `error_code = "none"`
- fill `capability_id`
- fill stable friendly name/category where available
- fill value type and unit from payload metadata or catalog metadata

Failure behavior:

- invalid index: `invalid_capability_index`
- missing capability: `capability_not_found`
- unavailable owner: `capability_api_not_available`

Placeholder policy:

- friendly name and category may use stable placeholders
- capability ID must not be invented
- unit should come from real payload/catalog state when available

Validation strategy:

- invalid index
- known capability identity once public enumeration exists
- identity read does not mutate state

Future expansion notes:

- future catalog metadata may provide richer display names and categories without changing Logic's `CAP_*` contract

### getCapabilityValue(...)

Purpose:

Return current capability value fields for value cards and sensor detail screens.

Output struct:

- `ApiCapabilityValue`

Required attachment or owner:

- Registry canonical capability payload or existing public capability-state API.

Success behavior:

- return `true`
- `ok = true`
- `error_code = "none"`
- copy value type, float/int/bool placeholders as appropriate
- copy unit and timestamp

Failure behavior:

- invalid index: `invalid_capability_index`
- missing capability: `capability_not_found`
- missing Registry where required: `registry_not_attached`

Placeholder policy:

- must not invent sensor values
- value fields without owners should remain deterministic defaults
- unavailable values should be represented through availability and error fields rather than fake readings

Validation strategy:

- valid read for existing canonical payloads when owner exists
- invalid index failure
- value reads do not mutate canonical payloads
- command state remains unchanged

Future expansion notes:

- should support wired and wireless sensors identically at the capability level
- should remain independent of hardware names and provider IDs for normal UI use

### getCapabilityAvailability(...)

Purpose:

Return capability availability, stale state, last update time, and provider existence information.

Output struct:

- `ApiCapabilityAvailability`

Required attachment or owner:

- capability payload owner and provider health owner.

Success behavior:

- return `true`
- `ok = true`
- `error_code = "none"`
- fill available/stale flags
- fill last update timestamp
- indicate whether a provider exists

Failure behavior:

- invalid index: `invalid_capability_index`
- missing capability: `capability_not_found`
- unavailable owner: `capability_api_not_available`

Placeholder policy:

- absent owner should not imply availability
- no provider should not be hidden behind fake availability

Validation strategy:

- available path
- unavailable/stale path
- read does not trigger timeout checks or provider selection
- repeated reads remain stable

Future expansion notes:

- should align with provider stale/lost behavior but must not run provider health updates itself

### getCapabilityProviderInfo(...)

Purpose:

Return read-only provider diagnostic metadata for a capability.

Output struct:

- `ApiCapabilityProviderInfo`

Required attachment or owner:

- Registry active provider mapping and public provider diagnostic owner.

Success behavior:

- return `true`
- `ok = true`
- `error_code = "none"`
- fill active provider ID where available
- fill provider type and status
- fill owner node placeholder if available
- indicate selected/active state

Failure behavior:

- invalid index: `invalid_capability_index`
- missing capability: `capability_not_found`
- no provider: `provider_not_available`
- missing Registry where required: `registry_not_attached`

Placeholder policy:

- provider ID must not be invented
- owner node ID must not be fabricated
- provider metadata is diagnostic only and must not become the normal Logic contract

Validation strategy:

- no provider path
- active provider diagnostic path once owner exists
- read does not trigger provider selection
- provider diagnostics are not reset

Future expansion notes:

- should support wired, wireless, virtual, cloud, dashboard, and future providers through the same metadata shape

### getCapabilityQuality(...)

Purpose:

Return read-only quality, freshness, and error metadata for capability cards.

Output struct:

- `ApiCapabilityQuality`

Required attachment or owner:

- capability payload quality fields, provider diagnostics, and future quality/freshness owner.

Success behavior:

- return `true`
- `ok = true`
- `error_code = "none"`
- fill quality string or stable quality placeholder
- fill error payload if present
- set `has_error`

Failure behavior:

- invalid index: `invalid_capability_index`
- missing capability: `capability_not_found`
- unavailable owner: `capability_api_not_available`

Placeholder policy:

- no fake quality claims
- no fake error history
- if no quality owner exists, use conservative stable placeholders

Validation strategy:

- normal quality read
- error quality read once owner exists
- repeated reads do not mutate diagnostics or payloads

Future expansion notes:

- may later integrate calibrated quality scoring, stale/lost reasons, and node diagnostics while remaining read-only

## 8. Error Handling Rules

Future Capability API methods should use compact stable error literals:

- `"none"`
- `"capability_not_found"`
- `"registry_not_attached"`
- `"capability_api_not_available"`
- `"invalid_capability_index"`
- `"provider_not_available"`

Rules:

- success responses set `ok = true`
- success responses set `error_code = "none"`
- failure responses set `ok = false` where the output struct has an `ok` field
- methods return `false` on failure
- failures must not mutate state
- avoid long or dynamic error messages

## 9. ESP32 Safety Rules

Capability API implementation must remain ESP32-safe:

- no dynamic allocation
- no Arduino `String`
- no STL containers
- no heap allocation
- use bounded arrays only
- respect `API_MAX_CAPABILITY_SUMMARY_COUNT`
- respect related API-level constants
- use `uint8_t` counts where practical
- use `const char*` only for stable literals/placeholders
- no unbounded capability, provider, value, or diagnostic lists

## 10. Minimal App Readiness

Future Capability API methods should support the first Minimal App sensor and project screens without exposing internals.

Supported future UI surfaces:

- sensor value card
- unit display
- capability availability state
- stale/offline indicator
- provider diagnostic metadata
- quality/freshness indicator
- future sensor detail screen
- custom project page capability widgets

Minimal App rules:

- consume Cyber32 API only
- do not parse packets
- do not access Registry arrays
- do not call Drivers
- do not call Devices
- do not call HAL
- do not call Services directly
- bind sensor UI to `CAP_*` IDs, not hardware names

Capability values may come from wired or wireless providers, but UI clients must see the stable capability contract first.

## 11. Recommended Implementation Order

Recommended future implementation order:

1. `getCapabilityList(...)`
2. `getCapabilitySummary(...)`
3. `getCapabilityIdentity(...)`
4. `getCapabilityValue(...)`
5. `getCapabilityAvailability(...)`
6. `getCapabilityProviderInfo(...)`
7. `getCapabilityQuality(...)`

Rationale:

`getCapabilityList(...)` should come first because it establishes bounded enumeration and empty-list behavior before detail methods depend on an index contract.

`getCapabilitySummary(...)` should follow because it can become the aggregate view used by Minimal App cards and dashboards.

The detailed methods should then be implemented one at a time so ownership, placeholder behavior, and read-only validation remain easy to audit.

## 12. Validation Expectations

Future Capability API validation should prove:

- empty capability list behavior
- bounded capability list count
- invalid capability index failure
- `capability_not_found` failure where appropriate
- valid capability read when a current public owner exists
- value reads do not mutate canonical payloads
- provider diagnostics are not reset
- provider selection is not triggered
- command state remains unchanged
- System API validation still passes
- Node API validation still passes
- existing provider, diagnostics, wireless, command-state, and capability-specific API validation remains unchanged

Validation must not require hardware.

Build rule:

- `pio run` should pass before implementation milestones are considered complete.
- If `pio` is unavailable in the current shell, report that explicitly.

## 13. Stop Conditions

Stop implementation and return to architecture review if future Capability API method work would require:

- packet parsing
- private Registry array access
- provider selection logic inside API
- command behavior
- hardware access
- Driver calls
- Device calls
- HAL calls
- Runtime architecture changes
- Registry ownership changes
- WebServer
- HTTP
- JSON
- UI/App code
- Dashboard
- Cloud
- Marketplace
- AI implementation
- dynamic memory
- Arduino `String`
- STL containers
- `src/main.cpp` changes

## 14. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.0 Phase 1 - Capability API List Method
```

Recommended scope:

- implement only `getCapabilityList(ApiCapabilityList& out_response)`
- prefer safe empty-list behavior or public-owner-backed bounded list behavior
- no detail methods
- no provider selection
- no packet parsing
- no WebServer
- no HTTP
- no JSON
- no app code
- no `src/main.cpp` changes

Reason:

System API and Node API now have safe read-only contracts. Capability API is the next UI-critical API area because Minimal App sensor cards need capability values through the Cyber32 API only.
