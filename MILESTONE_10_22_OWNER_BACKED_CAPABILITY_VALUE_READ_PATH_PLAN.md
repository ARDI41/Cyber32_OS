# Milestone 10.22 - Owner-Backed Capability Value Read Path Plan

## 1. Scope

Milestone 10.22 plans the future read path for owner-backed capability values.

This milestone is documentation only.

It defines how real Cyber32 capability values should eventually move from an approved internal Core-owned source into public API-safe read snapshots.

This milestone does not:

- implement value reads
- implement value owner storage
- implement real `getCapabilityValue(...)`
- implement node-to-capability mapping
- implement public arrays
- implement provider selection
- parse packets
- change API behavior
- change source code
- modify `src/main.cpp`

## 2. Background

Recent contracts define the future public read model:

- public node records
- public capability records
- node-to-capability mapping

Those contracts establish that UI-visible objects must be owner-backed, bounded, and API-safe.

Important current constraints:

- `getCapabilityValue(...)` must not expose fake values.
- Minimal App must not show default zero as a real sensor value.
- capability existence is separate from value availability.
- node-to-capability mapping does not prove a fresh value exists.
- public capability records may exist before any valid reading has arrived.
- API placeholder behavior must remain honest until real owner-backed value state exists.

## 3. Value Read Path Definition

Approved future read path:

```text
Approved internal source
-> canonical / owned value state
-> public API-safe value snapshot
-> Cyber32Api read method
-> UI / Transport
```

The read path must be one-way for UI clients.

Forbidden paths:

```text
UI / App / Transport -> packets
UI / App / Transport -> providers
UI / App / Transport -> drivers
UI / App / Transport -> devices
UI / App / Transport -> HAL
```

Forbidden API paths:

```text
Cyber32Api -> packet parser
Cyber32Api -> provider selection
Cyber32Api -> raw Registry private arrays
Cyber32Api -> Drivers / Devices / HAL
```

Cyber32Api should read only from approved public owner contracts.

## 4. Future Value Owner Concept

Future Core OS should introduce a bounded value owner concept.

Possible names:

- `CapabilityValueStore`
- `PublicCapabilityValueStore`
- `CanonicalCapabilityState`
- `CapabilityStateStore`

Recommended conceptual name:

```text
CapabilityValueStore
```

This name is conceptual only. Do not implement it yet.

### Responsibilities

The future value owner should:

- store current value snapshots for public capability records
- store `PayloadValueType`
- store numeric and boolean payload state safely
- store an explicit validity flag
- store freshness and stale state
- store timestamp / last update placeholders
- store unit metadata references or placeholders
- expose read-only snapshots to Cyber32Api
- prevent default zero from being treated as a real value
- provide deterministic unavailable state
- fail deterministically when full or unavailable

The value owner must not expose private provider pointers, private Registry arrays, packet buffers, driver internals, or HAL state.

## 5. Value Snapshot Fields

Conceptual value snapshot fields:

- capability instance ID or public capability reference
- `PayloadValueType`
- `value_valid`
- `value_fresh`
- `value_stale`
- unavailable reason
- numeric value placeholder
- boolean value placeholder
- timestamp / last update in milliseconds
- unit metadata placeholder
- quality / diagnostics linkage placeholder

Rules:

- `PayloadValueType::NONE` means no value.
- numeric zero is not a real value unless `value_valid == true` and the payload type supports numeric data.
- boolean false is not a real value unless `value_valid == true` and the payload type supports boolean data.
- stale state must be explicit.
- unavailable state must be explicit.
- default struct values must represent no valid reading.

## 6. Value Lifecycle

Conceptual lifecycle:

1. A public capability record exists.
2. A value owner may or may not have a snapshot for that capability.
3. An approved provider / canonical Core path updates the value owner.
4. Value becomes valid only after an approved update.
5. Value becomes stale when a future timeout policy marks it stale.
6. Value becomes unavailable when the owner disappears, update fails, provider becomes unavailable, or policy blocks it.
7. The public capability may remain visible while its value is unavailable.
8. API reads do not mutate value state.
9. UI clients display unavailable or stale values honestly.

Capability visibility and value validity are separate contracts.

## 7. API Mapping

Future `getCapabilityValue(...)` behavior should follow this shape:

- validate the capability index through public capability ownership
- if capability does not exist, return `capability_not_found`
- if capability exists but no value owner exists, return an unavailable / no-value response
- if value exists and is valid / fresh, copy the value and payload type
- if value exists but is stale, return stale state according to the approved contract
- never invent a reading
- never read packet buffers
- never call providers directly
- never trigger provider selection
- never call HAL, Drivers, Devices, or Services

Future `getCapabilityAvailability(...)` should reflect owner-backed value and provider availability state.

Important rule:

```text
Capability existence != value availability
```

## 8. Payload Type Rules

Payload type rules:

- `PayloadValueType::NONE` means no value.
- numeric payloads require explicit valid state.
- boolean payloads require explicit valid state.
- future payload types must remain bounded.
- UI clients should interpret payloads only when the API response indicates ok, valid, and not unavailable.
- stale values must be displayed as stale if returned.
- unit metadata must be owner-backed or explicitly unavailable.
- unit strings must not be guessed from capability names.

Default values are deterministic placeholders, not real readings.

## 9. Freshness and Stale Policy

Freshness policy is not implemented in this milestone.

Future policy should define:

- timestamp owner
- update interval / report interval handling
- stale timeout
- lost / unavailable timeout
- whether stale values remain readable with a stale flag
- whether expired values are hidden, unavailable, or visible as stale

Rules:

- stale is not necessarily an error.
- stale is not current.
- stale state must be visible to API consumers.
- read operations must not refresh hardware or provider state.
- timeout checks must not happen inside UI clients.

## 10. Error and Unavailable Reasons

Conceptual unavailable / error reasons:

- `no_value_owner`
- `not_updated_yet`
- `stale`
- `provider_unavailable`
- `capability_disabled`
- `capability_blocked`
- `value_type_unknown`
- `diagnostics_error`
- `owner_error`

Do not implement a new enum in this milestone unless a later implementation milestone explicitly approves it.

For now, these names document future stable reason concepts only.

## 11. Provider and Canonical Source Boundary

Providers may update internal canonical state only through approved Core paths.

Cyber32Api reads public value snapshots only.

Cyber32Api must not:

- call provider reads
- select providers
- refresh providers
- poll hardware
- parse wireless packets
- read transport buffers
- expose provider pointers or private indexes

Provider metadata remains diagnostics metadata. It must not become the normal Logic contract.

Capability value reads must be passive and read-only.

## 12. Diagnostics and Quality

Value validity is separate from diagnostics health.

Future diagnostics may explain why a value is unavailable, stale, rejected, or blocked.

Future quality state may include:

- freshness
- signal-derived quality
- provider health
- packet rejection context
- last valid update context

Rules:

- `getCapabilityQuality(...)` is not required for basic value existence.
- zero diagnostics counters do not prove a healthy value.
- diagnostics reads must not mutate values.
- value reads must not reset diagnostics.
- unavailable value state must not be hidden behind successful diagnostics.

## 13. Minimal App Implications

Minimal App remains mock-only until owner-backed values are available through approved API methods.

Minimal App must:

- use Cyber32 API only
- avoid packet parsing
- avoid provider polling
- avoid driver or HAL access
- avoid showing default zero as real data
- display unavailable values honestly
- display stale values honestly
- show a real reading only when API reports valid / available state
- avoid inferring unit or freshness from names

Initial UI should be prepared to show:

- no value yet
- unavailable
- stale
- valid fresh reading
- unsupported payload type
- diagnostics reason placeholder

## 14. Bounds and Safety

Future implementation constraints:

- fixed-size snapshots
- no heap allocation
- no dynamic strings
- no Arduino `String`
- no STL containers
- bounded unit metadata
- deterministic unavailable state
- deterministic failure when storage is full
- compact result codes or stable error literals
- no final storage limits chosen until a future implementation milestone

Safety rules:

- no fake nodes
- no fake capabilities
- no fake values
- no default zero as real data
- no packet parsing in API
- no private Registry array exposure
- no provider selection during reads
- no command execution during reads
- no app or transport direct access to Drivers, Devices, HAL, Services, or Logic

## 15. Open Questions

Open decisions:

- final value owner name
- freshness timeout policy
- timestamp owner
- unit metadata ownership
- numeric precision and representation
- supported payload types beyond current scalar values
- whether stale values are returned with stale flag or hidden as unavailable
- unavailable reason enum or stable string contract
- relationship between Runtime selected payload and public value store
- whether actuator command state is represented as a readable value
- whether cached values survive reboot later
- whether value owner is separate from public capability owner
- how local wired values and wireless values share the same store
- whether quality and diagnostics references are stored directly or joined at read time

## 16. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.23 - Owner-Backed Diagnostics Read Path Plan
```

Purpose:

Plan how public diagnostics snapshots should be read safely without exposing private Registry arrays, packet internals, provider internals, or fake health state.

## 17. Stop Conditions

Stop future work and return to architecture review if it tries to:

- implement real `getCapabilityValue(...)` before this plan is approved
- create fake readings
- treat default zero as real sensor data
- parse packets in Cyber32Api
- call providers directly from Cyber32Api reads
- trigger provider selection from a value read
- expose Registry internals
- expose private Registry arrays
- add live transport
- let app or transport call HAL, Drivers, Devices, Services, or Logic
- modify `src/main.cpp`
- add WebServer before transport approval
- add HTTP before transport approval
- add JSON before transport approval
- add BLE before transport approval
- add WiFi app transport before transport approval
