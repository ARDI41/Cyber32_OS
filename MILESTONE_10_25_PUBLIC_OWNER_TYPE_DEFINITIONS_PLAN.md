# Milestone 10.25 - Public Owner Type Definitions Plan

## 1. Scope

Milestone 10.25 plans the shared public owner types needed before future owner-store implementation begins.

This milestone is documentation only.

It defines conceptual firmware-safe public types, enums, and structs for future:

- `NodeDirectory`
- `CapabilityDirectory`
- `NodeCapabilityMap`
- `CapabilityValueStore`
- `DiagnosticsStore`

This milestone does not:

- implement public types
- create header files
- implement owner stores
- implement real API data
- create public arrays in code
- change API behavior
- change source code
- modify `src/main.cpp`

## 2. Background

Shared public owner types are needed because recent plans define the future owner-backed Core data path:

- `NodeDirectory` is planned.
- `CapabilityDirectory` is planned.
- `NodeCapabilityMap` is planned.
- `CapabilityValueStore` is planned.
- `DiagnosticsStore` is planned.
- API transition planning requires empty, not found, unavailable, stale, and no-value compatibility.
- all future public owner types must be bounded and firmware-safe.

Without shared type definitions, future owners could drift into incompatible fields, ambiguous defaults, fake health state, or accidental exposure of private Registry/provider internals.

## 3. Type Design Principles

Future public owner types should follow these principles:

- fixed-size structs
- primitive fields where possible
- explicit validity flags
- explicit availability flags
- explicit stale flags
- no dynamic allocation
- no Arduino `String`
- no STL containers
- no unbounded metadata blobs
- no private pointers
- no Registry private indexes exposed as public identity
- no provider private indexes exposed as public identity
- deterministic defaults mean unavailable, unknown, or no value
- zero is not real unless a valid flag says so
- false is not real unless a valid flag says so
- counters are not meaningful unless diagnostics availability is explicit

The type system must make unavailable state easy and fake state hard.

## 4. Public Identity Types

Conceptual public identity types:

- `PublicNodeId`
- `PublicCapabilityInstanceId`
- `PublicCapabilityIndex`
- `PublicNodeIndex`
- `PublicProviderId` placeholder
- `PublicMappingLinkId` placeholder

Do not implement these types in this milestone.

### PublicNodeId

Represents a public node identity.

Expected properties:

- should become stable when an approved node owner exists
- may be runtime-only until persistence is approved
- must not be a raw private Registry index
- must not be a private pointer
- must not automatically be a MAC address without explicit approval

### PublicCapabilityInstanceId

Represents one public capability instance.

Expected properties:

- should become stable within the public capability owner
- may later become stable across reboot if persistence exists
- must remain capability-first and tied to `CAP_*` semantics
- must not expose provider pointers or private provider indexes

### PublicCapabilityIndex

Represents a bounded API list index into public capability records.

Expected properties:

- may be runtime-only
- may be used for safe indexed API reads
- must be validated before use
- must not be treated as a permanent public identity unless a later milestone approves it

### PublicNodeIndex

Represents a bounded API list index into public node records.

Expected properties:

- may be runtime-only
- may be used for safe indexed API reads
- must be validated before use
- must not expose private Registry storage order

### PublicProviderId Placeholder

Represents future public provider metadata identity if provider diagnostics become public.

Expected properties:

- open decision
- diagnostic only
- must not become the Logic contract
- must not expose private provider pointer/index state

### PublicMappingLinkId Placeholder

Represents future node-to-capability link identity if explicit link identity is needed.

Expected properties:

- open decision
- may be runtime-only
- must reference public node and public capability identities safely
- must not expose private storage layout

## 5. Public Lifecycle and Status Enums

Conceptual future enums:

- `PublicLifecycleState`
- `PublicVisibilityState`
- `PublicTrustState`
- `PublicAvailabilityState`
- `PublicFreshnessState`
- `PublicHealthState`
- `PublicUnavailableReason`

Do not implement enum values in this milestone.

Expected state vocabulary:

- none
- pending
- available
- unavailable
- stale
- disabled
- blocked
- rejected
- error
- unknown
- unsupported

### PublicLifecycleState

Represents broad object lifecycle, such as none, pending, available, unavailable, stale, disabled, blocked, rejected, or error.

### PublicVisibilityState

Represents whether an owner-backed record is public, hidden, pending, diagnostic-only, or unavailable.

### PublicTrustState

Represents future pairing/trust state for public node visibility and security display.

### PublicAvailabilityState

Represents whether a record, value, provider, or diagnostic category is available.

### PublicFreshnessState

Represents freshness separately from existence and validity.

### PublicHealthState

Represents health only when health is known. Unknown health must not be treated as healthy.

### PublicUnavailableReason

Represents why a field, record, value, or diagnostic category is unavailable.

Open decision:

- unavailable reasons may later be enum values, stable literals, or compact IDs.

## 6. Public Node Record Type Plan

Future struct concept:

```text
PublicNodeRecord
```

Conceptual fields:

- node ID
- source type
- lifecycle state
- visibility state
- trust state
- capability count
- freshness state
- diagnostics available
- last seen timestamp placeholder
- display name buffer placeholder later
- optional source identity placeholder later

Default state:

- unavailable / unknown
- diagnostics unavailable
- capability count zero but not meaningful unless node is valid
- no display name
- no source identity
- not a fake node

Rules:

- a default node record must not appear as a real node.
- source identity must be explicitly available before use.
- display strings must be bounded if added later.
- source MAC visibility remains an open policy decision.

## 7. Public Capability Record Type Plan

Future struct concept:

```text
PublicCapabilityRecord
```

Conceptual fields:

- capability ID / `CAP_*` ID
- capability instance ID
- owner node ID placeholder
- category / type
- lifecycle state
- visibility state
- value availability state
- freshness state
- provider available
- diagnostics available
- unit metadata placeholder later

Default state:

- unavailable / unknown
- no value
- no owner node
- provider unavailable
- diagnostics unavailable
- not a fake capability

Rules:

- a default capability record must not appear as a real capability.
- `CAP_*` ID is the stable semantic contract.
- unit metadata must be owner-backed or explicitly unavailable.
- provider metadata remains diagnostic metadata.

## 8. Node-Capability Link Type Plan

Future struct concept:

```text
PublicNodeCapabilityLink
```

Conceptual fields:

- node ID or public node reference
- capability instance ID or public capability reference
- link visibility state
- link freshness state
- diagnostics available
- order / group placeholder

Default state:

- no active link
- no public node reference
- no public capability reference
- diagnostics unavailable
- not a fake relationship

Rules:

- mapping must not imply a fresh value exists.
- mapping must not imply diagnostics are healthy.
- mapping must not be inferred by UI, app, transport, API packet parsing, or provider names.
- links must reference public records only.

## 9. Capability Value Snapshot Type Plan

Future struct concept:

```text
PublicCapabilityValueSnapshot
```

Conceptual fields:

- capability instance ID
- `PayloadValueType`
- `value_valid`
- `value_available`
- `value_stale`
- unavailable reason
- numeric value placeholder
- boolean value placeholder
- last update timestamp
- unit metadata reference placeholder
- quality reference placeholder

Rules:

- `PayloadValueType::NONE` means no value.
- numeric `0` is not real unless `value_valid == true`.
- boolean `false` is not real unless `value_valid == true`.
- stale state must be explicit.
- unavailable state must be explicit.
- last update timestamp must be owner-backed.
- unit metadata must not be guessed from capability name.
- default state means no value.

## 10. Diagnostics Snapshot Type Plan

Future struct concepts:

- `PublicNodeDiagnosticsSnapshot`
- `PublicCapabilityDiagnosticsSnapshot`
- `PublicProviderDiagnosticsSnapshot`

Conceptual shared fields:

- target ID or reference
- diagnostics available
- health known
- health ok
- stale
- last update timestamp
- last error code placeholder
- accepted count placeholder
- rejected count placeholder
- unavailable reason

Rules:

- zero counters are not healthy unless diagnostics are available and health is known.
- `health_ok == false` is not meaningful unless health is known.
- missing errors do not mean healthy.
- default state means unknown / unavailable.
- reads must not reset counters.
- reads must not trigger provider selection.
- reads must not refresh provider health.

## 11. Bounds Plan

Future bounds needed:

- max public nodes
- max public capabilities
- max node-capability links
- max capabilities per node
- max display name length
- max unit metadata length
- max provider metadata length
- max error literal length or error ID width
- counter widths
- timestamp width

Do not choose final values in this milestone unless already defined in `CYBER32_BIBLE.md`.

Current open bound decisions:

- public node count
- public capability count
- link count
- capabilities per node
- display name length
- source identity length
- unit metadata length
- provider metadata length
- diagnostic counter width
- timestamp width
- unavailable reason representation

Future implementation must use deterministic failure when bounded storage is full.

## 12. Default Initialization Rules

Every future public struct must have deterministic init/reset behavior.

Default rules:

- default node record means no valid node.
- default capability record means no valid capability.
- default mapping link means no active relationship.
- default value snapshot means no value.
- default diagnostics snapshot means diagnostics unavailable / unknown.
- default counters are not real diagnostics.
- default timestamps are unknown.
- default source identity is unavailable.
- default provider metadata is unavailable.
- default unit metadata is unavailable.
- default zero is not a real reading.

Reset helpers, constructors, or initialization functions may be planned later, but are not implemented in this milestone.

## 13. API Compatibility Rules

Compatibility rules:

- existing API structs may later map from these owner types.
- existing empty-state behavior remains valid.
- existing error literals remain valid.
- no breaking change to current API consumers.
- future new fields must be additive or mapped safely.
- app must tolerate unknown / unavailable forever.
- deterministic defaults after failed reads must remain unavailable state.
- `node_not_found` remains valid.
- `capability_not_found` remains valid.
- `PayloadValueType::NONE` remains no value.
- provider metadata remains diagnostic metadata.

Cyber32Api must continue to read through public owner methods only when real data is introduced.

## 14. File Placement Plan for Later Implementation

Possible future file placement:

```text
include/cyber32/public/public_owner_types.h
include/cyber32/public/public_node_record.h
include/cyber32/public/public_capability_record.h
include/cyber32/public/public_diagnostics_snapshot.h
```

Alternative project-local placement may be chosen later if it better matches existing source layout.

Do not create these files in this milestone.

Future placement rules:

- keep public owner types separate from private Registry arrays.
- keep owner types independent from Drivers, Devices, HAL, and transport code.
- avoid includes that pull WiFi, ESP-NOW, Arduino `String`, STL, or heap-oriented dependencies into public owner contracts.
- keep structs C++11-compatible and ESP32-safe.

## 15. Test Planning

Future tests should cover:

- default init means unavailable.
- zero values are not valid by default.
- `PayloadValueType::NONE` means no value.
- invalid node ID returns not found.
- invalid capability ID returns not found.
- stale state is explicit.
- diagnostics are unavailable by default.
- counters are ignored unless diagnostics are available.
- health is unknown by default.
- source identity is unavailable by default.
- struct sizes remain bounded.
- copy behavior is deterministic.
- full owner storage fails deterministically.
- repeated reads do not mutate state.

No tests are implemented in this milestone.

## 16. Minimal App Implications

Minimal App implications:

- app remains mock-only.
- future app can map these states to UI honestly.
- app must not assume zero means real.
- app must not assume missing errors mean healthy.
- app must not infer unavailable reason.
- app must not infer nodes, capabilities, values, or diagnostics from packets or logs.
- app must continue to use Cyber32Api only.

Future live UI should display:

- empty
- not found
- unavailable
- unknown
- stale
- no value
- unsupported
- blocked / rejected
- valid owner-backed data

All of those are normal Cyber32 states.

## 17. Open Questions

Open decisions:

- final type names
- exact enum names
- ID widths
- timestamp width
- counter width
- string buffer lengths
- whether unavailable reasons are enums or stable literals
- whether provider ID is public
- whether source MAC is public
- whether Core itself has a public node ID
- whether value metadata belongs in capability record or value snapshot
- whether display names are Core-owned, user-owned, or both
- whether diagnostic error codes are IDs, stable literals, or both
- whether source identity is separate from public node identity
- whether public indexes are stable enough for UI refreshes

## 18. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.26 - NodeDirectory Skeleton Plan
```

Purpose:

Plan the first narrow implementation skeleton for a bounded `NodeDirectory` owner without exposing fake nodes or changing existing API behavior.

## 19. Stop Conditions

Stop future work and return to architecture review if it tries to:

- implement types in this documentation milestone
- create owner stores before this type plan is approved
- create fake records
- create fake nodes
- create fake capabilities
- create fake sensor values
- create fake diagnostics
- treat default zero as real
- treat zero counters as healthy
- expose Registry internals
- expose private Registry arrays
- parse packets in Cyber32Api
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
