# Milestone 10.1 - Capability API Readiness Audit

## 1. Scope

Milestone 10.1 audits the completed Milestone 10.0 read-only Capability API methods.

This audit covers the current placeholder behavior for:

- `getCapabilityList(...)`
- `getCapabilitySummary(...)`
- `getCapabilityIdentity(...)`
- `getCapabilityValue(...)`
- `getCapabilityAvailability(...)`
- `getCapabilityProviderInfo(...)`
- `getCapabilityQuality(...)`

The audited implementation is internal C++ API only. It does not expose WebServer, HTTP, JSON, UI, Dashboard, Cloud, Marketplace, or AI behavior.

The current Capability API deliberately uses an empty capability-list contract because Cyber32 does not yet have a public capability enumeration owner, Registry-backed safe capability list method, node-to-capability mapping owner, or owner-backed capability metadata path for this API layer.

## 2. Method-By-Method Status

### getCapabilityList(...)

Current behavior:

- returns `true`
- sets `out_response.ok = true`
- sets `out_response.error_code = "none"`
- sets `out_response.count = 0`
- does not invent fake capability summaries

Expected empty-capability-owner behavior:

- empty list is a successful read
- UI clients can safely display an empty capability state

Success/failure contract:

- current success: empty list
- current failure: none in placeholder mode
- future failure may include missing Registry or missing capability owner once a public owner exists

Current error handling:

- success uses `"none"`

Validation coverage:

- empty-list success
- `ok == true`
- `error_code == "none"`
- `count == 0`
- count bounded by `API_MAX_CAPABILITY_SUMMARY_COUNT`
- repeated reads remain stable

Future owner needed:

- public capability enumeration owner
- Registry-backed safe capability list method
- future capability catalog or capability metadata owner

### getCapabilitySummary(...)

Current behavior:

- aligns with `getCapabilityList(...)`
- returns `false` while the capability list is empty
- sets `out_response.ok = false`
- sets `out_response.error_code = "capability_not_found"`
- does not fill fake aggregate capability data

Expected empty-capability-owner behavior:

- any detail request is out of range while count is zero
- no fake summary is created

Success/failure contract:

- future success returns a populated `ApiCapabilitySummary`
- current failure returns `false` with `capability_not_found`

Current error handling:

- uses compact stable error literal `"capability_not_found"`

Validation coverage:

- index `0` fails when list count is zero
- `ok == false`
- `error_code == "capability_not_found"`
- repeated reads remain stable

Future owner needed:

- public indexed capability owner
- child field owners for identity, value, availability, provider info, and quality

### getCapabilityIdentity(...)

Current behavior:

- aligns with `getCapabilityList(...)`
- returns `false` while the capability list is empty
- sets `ok = false`
- sets `error_code = "capability_not_found"`
- clears deterministic identity defaults
- does not invent capability ID, friendly name, category, value type, or unit

Expected empty-capability-owner behavior:

- no capability identity is exposed until a public capability owner exists

Success/failure contract:

- future success returns capability identity from a public capability catalog, Registry capability record, or metadata owner
- current failure returns `false` with deterministic defaults

Current error handling:

- `"capability_not_found"`

Validation coverage:

- failure while capability list count is zero
- identity text fields remain default/null
- `value_type == PayloadValueType::NONE`
- repeated reads remain stable

Future owner needed:

- capability catalog
- Registry capability record
- unit and display metadata owner

### getCapabilityValue(...)

Current behavior:

- aligns with `getCapabilityList(...)`
- returns `false` while the capability list is empty
- sets `ok = false`
- sets `error_code = "capability_not_found"`
- sets `value_type = PayloadValueType::NONE`
- sets numeric values to zero
- sets bool value to `false`
- sets `timestamp_ms = 0`
- leaves unit default/null
- does not invent sensor values

Expected empty-capability-owner behavior:

- no value is exposed for a nonexistent capability
- zero values are deterministic defaults, not fake readings

Success/failure contract:

- future success returns value data from Registry canonical capability payload or another public API-safe owner
- current failure returns deterministic empty value state

Current error handling:

- `"capability_not_found"`

Validation coverage:

- failure while list count is zero
- `PayloadValueType::NONE`
- numeric defaults remain zero
- bool default remains false
- timestamp remains zero
- repeated reads remain stable

Future owner needed:

- Registry canonical capability payload owner exposed through this API layer
- value timestamp owner
- unit/catalog metadata owner

### getCapabilityAvailability(...)

Current behavior:

- aligns with `getCapabilityList(...)`
- returns `false` while the capability list is empty
- sets `ok = false`
- sets `error_code = "capability_not_found"`
- sets availability to unavailable/default state
- sets stale state to stale/default state
- sets `last_update_ms = 0`
- sets no provider
- does not imply a real capability is available

Expected empty-capability-owner behavior:

- no capability availability is reported for nonexistent capabilities
- no provider existence is implied
- no freshness or last-update state is invented

Success/failure contract:

- future success returns availability from capability payload and provider health owners
- current failure returns deterministic unavailable/no-provider state

Current error handling:

- `"capability_not_found"`

Validation coverage:

- failure while list count is zero
- unavailable/default availability
- stale/default stale state
- `last_update_ms == 0`
- no provider
- repeated reads remain stable

Future owner needed:

- capability payload owner
- provider health owner
- owner-backed stale/offline state

### getCapabilityProviderInfo(...)

Current behavior:

- aligns with `getCapabilityList(...)`
- returns `false` while the capability list is empty
- sets `ok = false`
- sets `error_code = "capability_not_found"`
- sets no provider ID
- sets provider type to `UNKNOWN`
- sets provider status to `UNKNOWN`
- sets `owner_node_id = 0`
- sets `has_owner_node = false`
- sets `selected = false`
- does not trigger provider selection

Expected empty-capability-owner behavior:

- no provider metadata is exposed for nonexistent capabilities
- no selected or active provider state is invented

Success/failure contract:

- future success returns read-only provider diagnostic metadata from public Registry/provider owners
- current failure returns deterministic empty provider metadata

Current error handling:

- `"capability_not_found"`

Validation coverage:

- failure while list count is zero
- no provider ID
- provider type/status defaults remain `UNKNOWN`
- owner node defaults remain empty
- selected flag remains false
- repeated reads remain stable

Future owner needed:

- Registry active provider mapping exposed through public APIs
- public provider diagnostic owner
- node-to-capability mapping owner

### getCapabilityQuality(...)

Current behavior:

- aligns with `getCapabilityList(...)`
- returns `false` while the capability list is empty
- sets `ok = false`
- sets `error_code = "capability_not_found"`
- sets `quality = 0`
- sets `error_code_payload = "none"`
- sets `has_error = false`
- does not invent quality, freshness, or error metadata

Expected empty-capability-owner behavior:

- no quality state is reported for nonexistent capabilities
- no error history is invented

Success/failure contract:

- future success returns quality and error metadata from capability payload, provider diagnostics, or future quality/freshness owners
- current failure returns deterministic no-quality/no-error state

Current error handling:

- `"capability_not_found"`

Validation coverage:

- failure while list count is zero
- quality default remains empty/null
- error payload remains `"none"`
- `has_error == false`
- repeated reads remain stable

Future owner needed:

- real quality/freshness owner
- provider diagnostic quality owner
- payload error metadata owner

## 3. Empty Capability-List Contract

The current `getCapabilityList(...)` contract is:

- returns `true`
- `ok = true`
- `error_code = "none"`
- `count = 0`

This means the Capability API is currently available as a safe read contract, but there are no known capabilities exposed through a public capability owner.

All detail methods must not invent fake capabilities.

Current detail methods fail with:

- return `false`
- `ok = false`
- `error_code = "capability_not_found"`

The detail failure behavior is intentional. It prevents UI clients from seeing placeholder capabilities as real sensor values, fresh readings, provider-backed metadata, or quality state.

## 4. Value Contract

`getCapabilityValue(...)` must not invent sensor values.

Current empty-owner behavior:

- returns `false`
- `ok = false`
- `error_code = "capability_not_found"`
- `value_type = PayloadValueType::NONE`
- numeric values are `0`
- bool value is `false`
- `timestamp_ms = 0`
- unit is default/null

Minimal App and future UI clients must treat this as no value, not as a valid zero reading.

## 5. Availability Contract

`getCapabilityAvailability(...)` must not imply availability when no capability exists.

Current empty-owner behavior:

- returns `false`
- `ok = false`
- `error_code = "capability_not_found"`
- unavailable/default availability
- stale/default stale state
- `last_update_ms = 0`
- no provider

The method must not trigger provider health updates, stale checks, timeout checks, or provider selection.

## 6. Provider Info Contract

`getCapabilityProviderInfo(...)` is diagnostic metadata only.

It must not trigger provider selection, provider health updates, provider diagnostics resets, command behavior, or provider policy.

Current empty-owner behavior:

- returns `false`
- `ok = false`
- `error_code = "capability_not_found"`
- no provider ID
- provider type `UNKNOWN`
- provider status `UNKNOWN`
- `owner_node_id = 0`
- `has_owner_node = false`
- `selected = false`

Provider metadata must remain diagnostic metadata and must not become the normal Logic contract.

## 7. Quality Contract

`getCapabilityQuality(...)` must not invent quality, freshness, or error metadata.

Current empty-owner behavior:

- returns `false`
- `ok = false`
- `error_code = "capability_not_found"`
- `quality = 0`
- `error_code_payload = "none"`
- `has_error = false`

Future quality data must come from a public owner and must remain read-only.

## 8. Safety Audit

Safety result: PASS.

Confirmed for Milestone 10.0 Capability API methods:

- no packet parsing
- no private Registry array access
- no provider selection
- no Services calls
- no Drivers calls
- no Devices calls
- no HAL calls
- no WebServer
- no HTTP
- no JSON
- no UI/App code
- no Runtime architecture changes
- no Registry ownership changes
- no `src/main.cpp` changes
- no Arduino `String`
- no STL containers
- no heap allocation

The Capability API methods remain read-only placeholder contracts and do not mutate Registry, Runtime, providers, diagnostics, command state, or capability payloads.

## 9. Minimal App Readiness

Ready now:

- Minimal App can safely call `getCapabilityList(...)`.
- Minimal App can handle an empty capability-list state.
- Minimal App can attempt detail reads and receive `capability_not_found`.
- Minimal App can distinguish no value from a fake zero value.
- Minimal App can safely rely on API-only empty-state behavior.
- Minimal App can keep UI empty states capability-first without touching Registry arrays, packet parsers, providers, Drivers, Devices, or HAL.

Not ready yet:

- real capability enumeration
- real sensor values
- real units
- live availability
- provider-backed metadata
- quality/freshness from a real owner
- node-to-capability mapping
- capability list backed by Registry or another public owner

The Minimal App still cannot show real capability data until a public capability owner and related mappings exist behind API-safe read contracts.

## 10. Validation Audit

Current validation coverage includes:

- capability list empty success
- detail method `capability_not_found` failures
- deterministic defaults for identity, value, availability, provider info, and quality
- repeated read stability
- System API validation still passes in the validation flow
- Node API validation still passes in the validation flow
- provider, diagnostics, command-state, wireless, and legacy capability validation remains preserved by the same validation path

PlatformIO note:

- The Codex shell has previously reported that `pio` is not on PATH.
- Compile validation requires a local PlatformIO environment when Codex cannot run `pio`.

## 11. Known Gaps

Known gaps before real Capability API data can be exposed:

- no public capability enumeration owner yet
- no Registry-backed safe capability list method yet
- no node-to-capability mapping owner
- no real value owner exposed through this API layer yet
- no real unit/catalog metadata owner exposed through this API layer yet
- no real provider-info mapping exposed through this API layer yet
- no real quality/freshness owner yet
- no owner-backed stale/offline state yet
- no Minimal App data model consuming these APIs yet

These gaps are expected. They should be resolved through documented milestones before UI clients rely on real capability data.

## 12. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.2 - Minimal App API Contract and Empty-State UI Plan
```

Rationale:

System API, Node API, and Capability API now have safe read-only empty-state contracts. The next step should define how the Minimal App consumes these APIs without accessing Registry, packet parsing, Services, Drivers, Devices, HAL, or transport internals.

## 13. Stop Conditions

Stop future implementation and return to architecture review if next work requires:

- packet parsing inside API
- private Registry array access
- provider selection inside API
- direct Driver calls
- direct Device calls
- direct HAL calls
- command execution
- WebServer/HTTP/JSON before contract approval
- UI/App code inside Core OS
- dynamic memory
- Arduino `String`
- STL containers
- `src/main.cpp` changes

Capability API must remain read-only until documented owner-backed data sources are introduced through public, bounded contracts.
