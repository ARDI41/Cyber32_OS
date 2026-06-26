# Milestone 10.24 - API Real Empty-to-Owned Transition Plan

## 1. Scope

Milestone 10.24 plans how existing safe empty-state Cyber32Api methods transition to future owner-backed real data.

This milestone is documentation only.

It defines how current empty-list, `node_not_found`, `capability_not_found`, unavailable, stale, and no-value semantics remain valid while future public owners are introduced.

This milestone does not:

- implement real node data
- implement real capability data
- implement real capability values
- implement diagnostics storage
- create public arrays in code
- add provider selection
- parse packets
- change API behavior
- change source code
- modify `src/main.cpp`

## 2. Background

Current Core OS and API state:

- System API has read-only summary methods.
- Node API has a safe empty list.
- Node detail methods return `node_not_found` while no public node owner exists.
- Capability API has a safe empty list.
- Capability detail methods return `capability_not_found` while no public capability owner exists.
- Minimal App is mock-only and empty-state safe.
- Public Node Record Contract exists.
- Public Capability Record Contract exists.
- Node-to-Capability Mapping Contract exists.
- Owner-backed value read path plan exists.
- Owner-backed diagnostics read path plan exists.

Current empty-state behavior is correct architecture, not a temporary bug.

The next phase of Cyber32 must preserve that correctness while adding real data only from approved owner-backed stores.

## 3. Transition Principle

Permanent transition rule:

```text
The API must not switch from empty to fake.
It must switch only from empty to owner-backed.
```

Long-term valid API states include:

- empty
- not found
- unavailable
- stale
- no value
- unsupported
- blocked / rejected

An empty response may remain valid forever.

Real data may appear only when an approved Core owner exists and the API reads that owner through a bounded public contract.

## 4. Future Owner Stack

Conceptual future owner stack:

- `NodeDirectory`
- `CapabilityDirectory`
- `NodeCapabilityMap`
- `CapabilityValueStore`
- `DiagnosticsStore`

None of these are implemented in this milestone.

### NodeDirectory

Conceptual owner for bounded public node records.

It should eventually expose safe read-only node identity, lifecycle, status, and visibility state to Cyber32Api.

### CapabilityDirectory

Conceptual owner for bounded public capability records.

It should eventually expose safe read-only capability identity, visibility, lifecycle, and metadata state to Cyber32Api.

### NodeCapabilityMap

Conceptual owner for bounded public node-to-capability links.

It should eventually answer which public capabilities belong to a public node without API-side inference.

### CapabilityValueStore

Conceptual owner for public capability value snapshots.

It should eventually expose valid, stale, unavailable, and no-value states without treating default zero as real data.

### DiagnosticsStore

Conceptual owner for node, capability, provider, freshness, power, signal, and security diagnostics snapshots.

It should eventually expose diagnostics without treating zero counters as healthy.

## 5. API State Matrix

| State | Expected API Behavior |
| --- | --- |
| No `NodeDirectory` | `getNodeList(...)` succeeds with `count = 0`; node detail methods return `node_not_found`. |
| `NodeDirectory` exists but empty | `getNodeList(...)` succeeds with `count = 0`; node detail methods return `node_not_found`. |
| Node exists but no capabilities | `getNodeList(...)` returns the node; `getNodeCapabilities(...)` succeeds with `out_count = 0`. |
| Node exists with mapped capabilities | `getNodeCapabilities(...)` returns bounded capability summaries from `NodeCapabilityMap`. |
| `CapabilityDirectory` missing or empty | `getCapabilityList(...)` succeeds with `count = 0`; capability detail methods return `capability_not_found`. |
| Capability exists but no value | `getCapabilityValue(...)` returns unavailable / no value with `PayloadValueType::NONE`. |
| Capability exists with stale value | `getCapabilityValue(...)` reports stale or unavailable according to the approved value contract. |
| Capability exists with fresh valid value | `getCapabilityValue(...)` returns explicit value type and valid owner-backed value. |
| Diagnostics missing | diagnostics methods return unavailable / unknown, not healthy. |
| Diagnostics exists | diagnostics methods return bounded owner-backed diagnostics snapshots. |

No state in this matrix permits fake nodes, fake capabilities, fake sensor values, fake diagnostics, or inferred data.

## 6. Existing API Methods Transition Plan

### System API

Methods:

- `getSystemIdentity(...)`
- `getSystemFirmware(...)`
- `getSystemRuntime(...)`
- `getSystemModes(...)`
- `getSystemMemory(...)`
- `getSystemSummary(...)`

Current safe behavior:

- methods return stable read-only placeholders or existing safe Runtime-backed fields.
- summary composes child System API methods.

Future owner-backed source:

- Core identity owner
- firmware/build metadata owner
- Runtime public state
- future memory/capacity summary owner if approved

Compatibility:

- placeholder literals remain valid until owner-backed values exist.
- missing optional system details must remain unavailable or placeholder, not fake.

Forbidden shortcuts:

- no HAL reads through API
- no driver reads
- no WiFi or transport reads
- no heap probing unless approved as a bounded owner-backed system state source

### Node API

Methods:

- `getNodeList(...)`
- `getNodeSummary(...)`
- `getNodeIdentity(...)`
- `getNodeStatus(...)`
- `getNodePower(...)`
- `getNodeSignal(...)`
- `getNodeDiagnostics(...)`
- `getNodeCapabilities(...)`

Current safe behavior:

- `getNodeList(...)` succeeds with `count = 0`.
- detail methods return `node_not_found`.
- `getNodeCapabilities(...)` initializes `out_count = 0` and fails while no node exists.

Future owner-backed source:

- `NodeDirectory` for public node records
- `NodeCapabilityMap` for node-to-capability links
- `DiagnosticsStore` for diagnostics, power, and signal when available

Compatibility:

- empty node list remains valid.
- `node_not_found` remains valid.
- a node with zero capabilities is valid and should return success with `out_count = 0`.
- unavailable power, signal, and diagnostics remain valid.

Forbidden shortcuts:

- no packet parsing
- no private Registry array reads
- no provider inference
- no MAC-derived fake nodes
- no driver/device/HAL reads
- no Services or Logic calls from API read methods

### Capability API

Methods:

- `getCapabilityList(...)`
- `getCapabilitySummary(...)`
- `getCapabilityIdentity(...)`
- `getCapabilityValue(...)`
- `getCapabilityAvailability(...)`
- `getCapabilityProviderInfo(...)`
- `getCapabilityQuality(...)`

Current safe behavior:

- `getCapabilityList(...)` succeeds with `count = 0`.
- detail methods return `capability_not_found`.
- deterministic defaults represent no real capability data.

Future owner-backed source:

- `CapabilityDirectory` for public capability records
- `CapabilityValueStore` for values and freshness
- `DiagnosticsStore` for quality, provider health, diagnostics, and unavailable reasons
- `NodeCapabilityMap` for owner-node relationship if approved

Compatibility:

- empty capability list remains valid.
- `capability_not_found` remains valid.
- capability can exist without a value.
- capability can exist with stale or unavailable value state.
- capability diagnostics can be unavailable.

Forbidden shortcuts:

- no fake `CAP_TEMPERATURE`
- no default zero as a reading
- no provider selection
- no packet parsing
- no direct provider reads
- no Registry internals
- no hardware reads

## 7. Error and Unavailable Semantics

Stable meanings:

- `ok = false` with `node_not_found`: requested public node is not available through the Node API.
- `ok = false` with `capability_not_found`: requested public capability is not available through the Capability API.
- `ok = true` with `count = 0`: the list read succeeded and is empty.
- unavailable: owner-backed object may exist, but the requested state is not currently available.
- stale: state exists but is not fresh/current.
- no value: no valid value exists; this is not numeric zero.
- unknown: state is not known by an owner-backed source.
- unsupported: feature or diagnostic category is not supported by the owner.
- blocked / rejected: state is intentionally blocked by policy.
- full / capacity error later: owner exists but cannot accept more bounded records.

Important clarifications:

- `ok = true` with `count = 0` is not an error.
- not found is not fake data.
- unavailable is not healthy.
- stale is not fresh.
- no value is not zero.
- zero counters do not mean healthy unless diagnostics explicitly say counters are valid and health is known.

## 8. Backward Compatibility Rules

Backward compatibility rules:

- existing tests for empty states must continue passing.
- current Minimal App mock assumptions remain valid.
- UI clients must tolerate empty lists forever.
- new owner-backed data must be additive.
- failure paths must remain deterministic.
- existing `node_not_found` and `capability_not_found` behavior remains valid.
- existing unavailable diagnostics state remains valid.
- API structs must remain bounded.
- future firmware changes must avoid dynamic allocation.
- future firmware changes must avoid Arduino `String`.
- future firmware changes must avoid STL containers.

Real data must not require breaking the safe empty-state contract.

## 9. Implementation Gating Plan

Recommended future implementation order:

1. Milestone 10.25 - Public Owner Type Definitions Plan
2. Milestone 10.26 - NodeDirectory Skeleton Plan
3. Milestone 10.27 - CapabilityDirectory Skeleton Plan
4. Milestone 10.28 - NodeCapabilityMap Skeleton Plan
5. Milestone 10.29 - CapabilityValueStore Skeleton Plan
6. Milestone 10.30 - DiagnosticsStore Skeleton Plan
7. Milestone 10.31 - API Owner-Backed Node List Implementation
8. Milestone 10.32 - API Owner-Backed Capability List Implementation
9. Milestone 10.33 - API Owner-Backed getNodeCapabilities Implementation
10. Milestone 10.34 - API Owner-Backed getCapabilityValue Implementation
11. Milestone 10.35 - API Owner-Backed Diagnostics Implementation
12. Milestone 10.36 - Minimal App Live Transport Boundary Plan

Each milestone should remain narrow, gated, validated, and reversible.

No live transport should be introduced before owner-backed Core API state exists.

## 10. Minimal App Implications

Minimal App implications:

- app remains mock-only until an approved transport milestone.
- current empty screens remain correct.
- future live app must handle every API state in this plan.
- app must not infer nodes from packets, logs, MAC addresses, or debug output.
- app must not infer capabilities from packets, providers, hardware names, or Registry assumptions.
- app must not show default zeros as real values.
- app must not treat unavailable diagnostics as healthy.
- app must not treat missing errors as healthy.
- app must not require nodes or capabilities to exist.
- app must continue to use Cyber32 API only.

The app should be designed so empty, unavailable, stale, and no-value states are normal UI states.

## 11. Test Strategy

Future validation should cover:

- no owner -> empty list
- owner exists but empty -> empty list
- owner has node -> node list returns node
- missing node -> `node_not_found`
- node with zero capabilities -> success with `out_count = 0`
- node with mapped capabilities -> bounded capability summaries
- capability owner missing or empty -> empty list / `capability_not_found`
- capability exists with no value -> no value / unavailable
- capability exists with stale value -> stale
- capability exists with fresh value -> valid explicit value
- diagnostics missing -> unknown / unavailable
- diagnostics exists -> owner-backed diagnostics
- output buffer too small -> deterministic bounded behavior
- full store -> deterministic failure
- repeated reads do not mutate state
- read methods do not reset counters
- read methods do not trigger provider selection

No tests are implemented in this milestone.

## 12. Safety Invariants

Safety invariants:

- no fake nodes
- no fake capabilities
- no fake values
- no fake diagnostics
- no private Registry access
- no private Registry arrays exposed
- no packet parsing in Cyber32Api
- no provider selection from reads
- no provider refresh from reads
- no diagnostics counter reset from reads
- no app or transport access to HAL, Drivers, Devices, Modules, Services, or Logic
- no `src/main.cpp` changes
- no WebServer before transport plan approval
- no HTTP before transport plan approval
- no JSON before transport plan approval
- no BLE before transport plan approval
- no WiFi app transport before transport plan approval
- no default zeros as real data
- no zero counters as healthy diagnostics

## 13. Open Questions

Open decisions:

- final owner component names
- final public store bounds
- stable ID widths
- unavailable reason enum or stable literals
- stale timeout
- diagnostics timeout
- whether System API also needs owner-backed public state
- how Runtime selected payload relates to `CapabilityValueStore`
- whether Core itself is represented as a local node
- when pairing / trust gates public visibility
- whether live transport sees pending discovery records
- whether pending discovery uses Node API or a separate Discovery API
- whether blocked/rejected records are hidden or diagnostic-only
- whether source MAC is public before pairing
- how owner-backed state persists across reboot later

## 14. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.25 - Public Owner Type Definitions Plan
```

Purpose:

Plan the shared firmware-safe public types, enums, and structs needed before implementing `NodeDirectory`, `CapabilityDirectory`, `NodeCapabilityMap`, `CapabilityValueStore`, and `DiagnosticsStore`.

## 15. Stop Conditions

Stop future work and return to architecture review if it tries to:

- implement owner stores before this transition plan is approved
- replace empty states with fake records
- treat zero or default values as real
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
