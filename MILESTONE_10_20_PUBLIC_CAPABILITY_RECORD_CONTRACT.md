# Milestone 10.20 - Public Capability Record Contract

## 1. Scope

Milestone 10.20 defines the public capability record contract used by Cyber32Api before implementing real capability list data.

This milestone is documentation only.

It defines:

- what a public capability record is
- why it is needed
- who should own it in a future implementation
- how it should be indexed safely
- which fields are required
- which fields are optional or future
- how capability identity, visibility, lifecycle, value state, provider metadata, and diagnostics should work
- how existing Capability API methods should map to public capability records later
- the relationship to public node records
- bounds, safety rules, Minimal App implications, open questions, and stop conditions

No firmware implementation is performed in this milestone.

## 2. Background

The public capability record contract is needed because Cyber32 is moving from safe empty-state API contracts toward real owner-backed capability data.

Current state:

- Capability API currently returns a safe empty list.
- Capability detail methods currently return `capability_not_found`.
- Minimal App is mock-only.
- Minimal App correctly treats empty capability lists as valid empty states.
- Real capabilities must not be invented.
- App, UI, and transport layers must not infer capabilities from packets.
- App, UI, and transport layers must not infer capabilities from Registry internals.
- App, UI, and transport layers must not infer capabilities from providers, drivers, or hardware names.
- Cyber32 is capability-first.

Before `getCapabilityList(...)` can return real data, Core OS needs a bounded public owner-backed capability record model that Cyber32Api can read safely.

## 3. Public Capability Record Definition

A public capability record is a bounded, owner-backed, API-safe representation of a real capability instance exposed by Cyber32 Core OS.

A public capability record is distinct from:

- raw packets
- ESP-NOW payloads
- Registry private arrays
- provider internals
- driver internals
- device internals
- module internals
- transport state
- temporary sensor readings
- UI cards
- debug serial output
- hardware-test decode output

Public capability records exist so API clients can ask:

```text
What capabilities does Cyber32 Core currently expose through its public API?
```

Public capability records must not be fabricated to satisfy UI needs. If no owner-backed public capability record exists, the correct API answer remains an empty list or `capability_not_found`.

## 4. Ownership Model

Future Core OS should introduce a dedicated public capability owner concept.

Possible component names:

- `PublicCapabilityStore`
- `CapabilityDirectory`
- `PublicCapabilityRegistry`

Recommended concept name for planning:

```text
CapabilityDirectory
```

This name is conceptual only. Do not implement it yet.

### Ownership Responsibilities

The future owner should be responsible for:

- storing public capability records
- assigning or accepting stable public capability identities through approved Core policy
- maintaining bounded indexed access
- tracking capability lifecycle state
- tracking public visibility state
- tracking value availability state
- tracking freshness placeholder state
- tracking provider availability placeholder state
- tracking diagnostics availability placeholder state
- exposing read-only public records to Cyber32Api
- preventing fake capabilities from being exposed
- enforcing deterministic failure when full

### Creation Responsibility

Public capability records should be created only through approved Core paths, such as:

- future discovery-to-public-record bridge
- future node-to-capability mapping bridge
- future local wired device registration bridge
- future trusted static capability registration path
- future provider/canonical capability ownership path

UI, app, and transport layers must not create public capability records directly.

### Update Responsibility

The future owner should update public capability records from approved Core inputs only.

Possible update sources:

- public node ownership state
- capability discovery state
- provider/canonical state
- owner-backed value freshness state
- owner-backed diagnostics state
- future wired device state
- future wireless node state

Updates must not come from UI guesses, app-side logs, debug output, packets parsed by API, provider internals exposed directly, driver names, or transport internals.

### Deletion / Expiry Responsibility

The future owner should define:

- stale timeout behavior
- expired record behavior
- disabled capability behavior
- blocked/rejected behavior
- whether records are removed, hidden, or retained for diagnostics
- whether user-approved records persist later

### Read-Only API Access Responsibility

Cyber32Api should read public capability records through public owner methods only.

Cyber32Api must not:

- read private Registry arrays
- parse packets
- call Drivers
- call Devices
- call HAL
- call Services directly for UI reads
- call provider selection
- expose provider pointers or private indexes

## 5. Required Fields

Required public capability record fields conceptually:

- `capability_id` / `CAP_*` ID
- `capability_instance_id` or public `capability_index`
- `owner_node_id` placeholder
- capability type/category
- lifecycle/status
- visibility state
- value availability state
- freshness placeholder
- provider availability placeholder
- diagnostics availability placeholder

Field expectations:

### `capability_id` / `CAP_*` ID

Identifies the stable capability type.

Examples:

- `CAP_TEMPERATURE`
- `CAP_DISTANCE`
- `CAP_SERVO_POSITION`
- `CAP_MOTOR_CONTROL`
- `CAP_RELAY_CONTROL`

### `capability_instance_id` / `capability_index`

Identifies one exposed capability instance.

The public `capability_index` may be used for bounded API reads, but a future `capability_instance_id` may be needed for identity stability beyond current list ordering.

### `owner_node_id` Placeholder

Identifies the public node that owns this capability when node ownership exists.

This may remain placeholder/unavailable until Milestone 10.21 defines node-to-capability mapping.

### Capability Type / Category

Describes broad capability category for UI grouping and diagnostics.

Examples:

- sensor
- actuator
- diagnostic
- system
- unknown

### Lifecycle / Status

Represents whether the capability is available, unavailable, stale, disabled, blocked, or in error.

### Visibility State

Controls whether the capability appears in public API lists or remains internal.

### Value Availability State

Represents whether a current owner-backed value exists.

### Freshness Placeholder

Represents whether freshness is known, fresh, stale, or unavailable.

### Provider Availability Placeholder

Represents whether provider metadata exists.

### Diagnostics Availability Placeholder

Represents whether owner-backed diagnostics exist.

All fields must be fixed-size or primitive firmware-safe values in a future implementation.

## 6. Optional / Future Fields

Optional or future public capability fields:

- `display_name`
- unit metadata
- value range metadata
- precision metadata
- source/provider ID
- source/provider type
- quality score
- last update timestamp
- calibration status
- icon/type hint
- user label

### Public API Safety

Generally safe for public API once bounded and owner-backed:

- `display_name`
- unit metadata
- value type
- value availability state
- freshness state
- lifecycle/status
- source/provider type as broad category
- quality score if owner-backed
- last update timestamp if owner-backed
- icon/type hint

May require trust, pairing, or explicit policy before public API exposure:

- source/provider ID
- precise provider metadata
- calibration status
- hardware-derived metadata
- user label if user-owned state is added later
- range/precision metadata that could imply unsafe actuator bounds

Rules:

- optional fields must remain bounded
- no dynamic strings
- no Arduino `String`
- no STL containers
- no unbounded metadata blobs
- no provider pointer or private array index exposed as public identity
- no hardware name becomes the primary contract

## 7. Stable Capability Identity Rules

Planned capability identity rules:

- `CAP_*` ID describes capability type
- `capability_instance_id` or public `capability_index` identifies one exposed instance
- identity is stable within runtime
- identity may later be stable across reboot if owner identity exists
- identity must be a bounded numeric or fixed-size type
- identity must be generated or accepted only through approved Core policy
- identity must not be generated by UI
- identity must not be generated by Minimal App
- identity must not be generated by transport adapter
- identity must not be parsed by API from raw packets
- no dynamic strings
- no Arduino `String`
- no STL containers
- no provider pointer exposed as public identity
- no provider private index exposed as public identity

The public capability identity must preserve capability-first behavior. UI clients bind to `CAP_*` semantics and public API records, not driver names, provider IDs, device names, module IDs, packet fields, or hardware pins.

## 8. Capability Visibility States

Possible capability visibility states:

- empty / none
- pending_owner
- available
- unavailable
- stale
- disabled
- blocked / rejected
- error

### Empty / None

No public capability record exists.

Current API behavior:

- `getCapabilityList(...)` returns count `0`
- detail reads return `capability_not_found`

### Pending Owner

A candidate capability may exist internally but does not yet have an approved public owner.

Future policy decision:

- pending owner state may remain internal
- or may be exposed through a dedicated discovery/pairing API

### Available

Capability exists and has valid owner-backed availability.

Available does not automatically mean it has a fresh value. Value availability is explicit.

### Unavailable

Capability is known but currently unavailable.

### Stale

Capability has not refreshed within a freshness timeout.

### Disabled

Capability is known but intentionally disabled by policy or configuration.

### Blocked / Rejected

Capability is known but blocked or rejected.

Future policy decision:

- blocked/rejected capabilities may remain internal
- or may appear only in diagnostics/security views

### Error

Capability record has an owner-backed error state.

## 9. Capability Lifecycle

Conceptual lifecycle:

1. Capability is discovered internally from an approved Core path.
2. Owner node or local owner is validated.
3. Public capability record is created by the future capability owner.
4. Provider/canonical value owner is attached later.
5. Freshness/status is updated through approved owner-backed state.
6. Capability becomes stale if owner/value freshness timeout expires.
7. Capability becomes unavailable if no valid owner-backed state remains.
8. Capability may be removed, hidden, disabled, blocked, or retained for diagnostics depending on future policy.

No implementation is performed in this milestone.

## 10. API Mapping

Future Capability API methods should map to public capability records only through approved owner-backed reads.

### `getCapabilityList(...)`

Reads bounded public capability records.

Current behavior must remain valid:

- empty list success when no public capability records exist

### `getCapabilitySummary(...)`

Reads aggregate summary for one public capability by safe index.

Current behavior must remain valid:

- returns `capability_not_found` when index is out of range

### `getCapabilityIdentity(...)`

Reads public identity fields:

- `capability_id`
- display/friendly name when available
- category/type
- value type if known
- unit metadata if known

### `getCapabilityValue(...)`

Reads owner-backed current value state only.

Must not:

- invent values
- show default zero as a reading
- read packets
- call providers directly
- trigger provider selection

### `getCapabilityAvailability(...)`

Reads owner-backed availability and freshness state:

- available/unavailable
- stale/not stale
- last update timestamp if owner-backed
- provider presence if owner-backed

### `getCapabilityProviderInfo(...)`

Reads owner-backed provider metadata only.

Provider info is diagnostic/read-only and must not trigger provider selection.

### `getCapabilityQuality(...)`

Reads owner-backed quality and error state only.

Unavailable quality remains valid until a public owner exists.

Current empty-list and `capability_not_found` behavior must remain valid for all detail methods.

## 11. Capability Value Rules

Capability value rules:

- public capability record may exist without current value
- value availability must be explicit
- `PayloadValueType::NONE` means no value
- default zero is not a real reading
- `false` bool defaults after failed reads are not real false readings
- stale value must be marked stale
- unavailable value must be explicit
- timestamps must be owner-backed
- freshness must be owner-backed
- unit metadata must not be guessed by UI
- no fake `CAP_TEMPERATURE` reading
- no fake sensor readings
- value reads must not mutate provider state
- value reads must not trigger provider selection

If a public capability exists but no value owner exists, `getCapabilityValue(...)` should report unavailable/no value rather than inventing a value.

## 12. Provider Metadata Rules

Provider metadata rules:

- provider metadata is diagnostic/read-only
- provider metadata must not trigger provider selection
- provider status must be owner-backed
- provider ID/type must not expose private pointers
- provider ID/type must not expose private Registry array indexes
- provider details may be unavailable even if capability exists
- provider metadata must not become the Logic contract
- UI may display provider metadata only as diagnostics

Provider metadata should help explain source and health, but normal UI and Logic remain capability-first.

## 13. Bounds and Limits

Required future bounds:

- maximum public capability count
- maximum capabilities per node
- maximum display name length
- maximum unit metadata length
- maximum provider metadata length
- maximum value range metadata length if added
- maximum precision/calibration metadata length if added
- deterministic failure when full

Implementation constraints for future firmware:

- fixed arrays if implemented later
- no heap allocation
- no dynamic strings
- no Arduino `String`
- no STL containers
- bounded copy behavior
- compact result codes

Open decisions:

- final maximum public capability count
- final maximum capabilities per node
- final display name length
- final unit metadata length
- final provider metadata length
- whether these bounds reuse existing Registry/provider limits or define public API-specific limits

Do not choose final numbers in this contract unless a later milestone confirms them.

## 14. Safety Rules

Safety rules:

- no fake capabilities
- no fake `CAP_TEMPERATURE`
- no fake sensor values
- no capability inferred by UI
- no capability inferred by Minimal App
- no capability inferred by transport
- no packet parsing in API
- no packet parsing in UI
- no packet parsing in transport
- no direct Registry array exposure
- no HAL calls from API consumers
- no Driver calls from API consumers
- no Device calls from API consumers
- no Service calls from API consumers
- no Logic calls from API consumers
- no provider selection
- no command execution
- no default values presented as real capability data
- no unit guessed by UI
- no provider pointer exposed as public data
- no disabled/blocked capability shown as a normal available capability

API consumers include:

- Minimal App
- Dev Panel
- Mission Control
- Dashboard
- Cloud Bridge
- Marketplace clients
- AI Assistant
- future transport adapters

## 15. Minimal App Implications

Minimal App implications:

- app remains mock-only for now
- empty capability list remains valid
- `capability_not_found` remains valid
- no real UI change is required yet
- future real capability cards require public capability records
- future available/unavailable/stale states must be shown honestly if API exposes them
- app must not construct capabilities from packet logs
- app must not construct capabilities from MAC addresses
- app must not construct capabilities from debug output
- app must not construct capabilities from Registry assumptions
- app must not construct capabilities from provider names
- app must not construct capabilities from driver names
- app must not show default zero as a real reading
- app must not hard-code `CAP_TEMPERATURE` as real unless returned by API

The Minimal App should continue to use mock data until owner-backed public capability records exist and a transport milestone is approved.

## 16. Relationship to Public Node Record

Public capabilities may later be attached to public nodes.

Relationship rules:

- node-to-capability mapping is not implemented here
- `owner_node_id` may remain placeholder until Milestone 10.21
- local/internal Core capabilities may need policy for node-less exposure later
- wired local devices may become local node-owned capabilities or Core-owned capabilities depending on future policy
- wireless node capabilities should attach to public node records after public node and capability owners exist
- no direct mapping to Registry arrays
- no app inference from node ID to capability list
- no provider inference from capability to node without an approved mapping owner

Milestone 10.21 should define the bounded public mapping between public nodes and public capabilities before `getNodeCapabilities(...)` returns real data.

## 17. Open Questions

Open decisions:

- final owner component name: `CapabilityDirectory`, `PublicCapabilityStore`, or `PublicCapabilityRegistry`
- maximum public capabilities
- maximum capabilities per node
- `capability_instance_id` width/type
- whether public `capability_index` is stable enough for UI
- how local Core capabilities differ from remote node capabilities
- whether node-less capabilities are allowed
- value freshness timeout
- diagnostics freshness timeout
- unit metadata ownership
- value type metadata ownership
- range/precision metadata ownership
- provider metadata visibility
- provider identity format
- disabled/blocked capability visibility policy
- whether actuator capabilities need stricter public exposure policy than sensor capabilities

## 18. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.21 - Node-to-Capability Mapping Contract
```

Purpose:

Define the bounded public mapping between public nodes and public capabilities before implementing `getNodeCapabilities(...)` with real data.

## 19. Stop Conditions

Stop future work and return to architecture review if it tries to:

- implement capability list data before contract approval
- implement node-to-capability mapping before its contract
- create fake capabilities
- create fake sensor values
- expose Registry internals
- expose private Registry arrays
- parse packets in API
- add live transport
- call HAL from app or transport
- call Drivers from app or transport
- call Devices from app or transport
- call Services from app or transport
- call Logic from app or transport
- modify `src/main.cpp`
- add WebServer before transport approval
- add HTTP before transport approval
- add JSON before transport approval
- add BLE before transport approval
- add WiFi app transport before transport approval
