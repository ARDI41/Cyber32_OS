# Milestone 10.71 - Core API Snapshot Builder Plan

## 1. Scope

This milestone plans a future Core API snapshot builder only.

This milestone is documentation only.

It does not:

* modify firmware source code
* modify `src/main.cpp`
* implement a snapshot builder
* add `Cyber32Api::getSnapshot`
* connect snapshot building to transport
* add JSON encoding
* add HTTP, WebServer, BLE, WiFi, cloud, local, LoRa, or gateway transport
* connect to Minimal App
* add production records
* add fake nodes, capabilities, mappings, values, diagnostics, or `CAP_TEMPERATURE`
* add provider, discovery, Registry, value, or diagnostics bridges
* parse packets
* call HAL, Drivers, Devices, Modules, Services, Logic, or Runtime
* expose private Registry arrays
* expose mutable owner stores
* change existing API behavior

The purpose is to plan how a future builder should fill `ApiSnapshot` from owner-backed public API data safely and deterministically.

## 2. Background

Current state:

* `ApiSnapshot` type skeleton exists.
* `ApiSnapshotNodeEntry`, `ApiSnapshotCapabilityEntry`, and `ApiSnapshotLinkEntry` exist.
* snapshot capacities are bounded and aligned with public owner capacities.
* `Cyber32Api` can read owner-backed empty Node API and Capability API views.
* `getNodeCapabilities(...)` can read explicit owner-backed node-capability mappings.
* positive owner injection validation exists through the internal validation owner seam.
* Minimal App connection strategy exists, but no live transport exists.

A snapshot builder is needed before any transport encoding so future consumers can share one bounded, API-safe snapshot shape.

## 3. Builder Principle

The future snapshot builder must be:

* read-only
* bounded
* deterministic
* caller-buffer based
* no heap allocation
* no STL
* no Arduino `String`
* no transport dependency
* no JSON dependency
* no fake data
* no mutable owner access
* no Registry access
* no packet parsing
* no hardware, service, logic, or runtime calls

The builder must produce a representation of approved public owner/API data only.

## 4. Proposed Future Files

Possible future files:

* `include/cyber32/api/api_snapshot_builder.h`
* `src/api/api_snapshot_builder.cpp`

Do not create those files in this milestone.

## 5. Builder Input Model

### Option A: Builder Takes `Cyber32Api&`

The builder accepts a `Cyber32Api&` and fills `ApiSnapshot` by calling public read-only API methods.

Benefits:

* keeps builder aligned with public API contracts
* avoids private owner access
* preserves API-first architecture
* works with the existing validation injection seam

Risks:

* may require additional public read methods before complete snapshots are possible
* may duplicate list/detail calls unless carefully bounded

### Option B: Builder Takes `const PublicOwnerStore&`

The builder accepts a `const PublicOwnerStore&` and reads owner stores directly through read-only accessors.

Benefits:

* simple mapping from owner records to snapshot entries
* avoids mutable owner access
* easy to test with validation-owned stores

Risks:

* bypasses the API-first preference if used as the primary app-facing path
* requires careful boundary language so it does not become a backdoor around `Cyber32Api`

### Option C: Builder Is A `Cyber32Api` Method

Example future shape:

```cpp
bool Cyber32Api::fillSnapshot(ApiSnapshot& out_snapshot);
```

Benefits:

* simple caller experience
* keeps snapshot generation under the API namespace

Risks:

* can grow `Cyber32Api` into a large snapshot assembly layer
* may blur method-level read contracts and aggregate snapshot construction

### Option D: Transport Layer Builds Snapshot

The transport layer calls existing `Cyber32Api` methods and assembles a snapshot.

Benefits:

* no new builder class
* transport can decide encoding details later

Risks:

* couples snapshot semantics to transport
* risks duplicated snapshot logic across local, cloud, gateway, and future modes
* makes consistent validation harder

### Recommended First Direction

Prefer a separate `ApiSnapshotBuilder`.

The first implementation should use either:

* `Cyber32Api` read-only methods, or
* explicit read-only owner access where a complete API method does not yet exist

It must:

* avoid mutable owner stores
* avoid transport coupling
* preserve production default behavior
* preserve empty default output as valid
* avoid fake data

## 6. Builder Output Model

Future builder output model:

* caller owns the `ApiSnapshot` output buffer
* builder resets/initializes output before filling
* builder fills counts deterministically
* builder never writes past capacity
* builder returns `bool` or a compact status
* empty snapshot is valid
* truncation flags are set deterministically if capacity is exceeded
* output contains no fake placeholder rows

The builder must not allocate memory, grow arrays dynamically, or retain pointers to caller-owned buffers.

## 7. Snapshot Fill Order

Planned fill order:

1. reset output
2. fill system/status metadata
3. fill node entries
4. fill capability entries
5. fill mapping entries
6. optionally fill per-node summaries later if approved
7. set final counts and truncation flags

The fill order must keep partially filled snapshots deterministic even if a future builder returns failure.

## 8. Node Filling Rules

Node filling must:

* include only public node records
* avoid fake nodes
* avoid private Registry data
* avoid packet data
* avoid hardware reads
* copy `capability_count` only as display metadata
* never use `capability_count` to create mappings

Node entries must not include private MAC data unless a future public identity contract explicitly approves it.

## 9. Capability Filling Rules

Capability filling must:

* include only public capability summary fields
* avoid fake capabilities
* avoid fake `CAP_TEMPERATURE`
* avoid sensor values
* avoid diagnostics
* avoid provider info
* avoid quality data
* avoid hardware timestamps

Capability entries describe public identity/availability metadata, not readings.

## 10. Mapping Filling Rules

Mapping filling must:

* include only explicit `NodeCapabilityMap` links
* avoid inferred mappings
* avoid mapping from `capability_count`
* avoid mapping from `owner_node_id`
* avoid mapping from `CAP_TEMPERATURE`
* avoid Registry, packet, provider, debug, sensor-value, or diagnostic inference

Mappings are relationship records only.

## 11. Broken Link Handling

Broken link handling plan:

* raw link array may include only stored links if reading the map directly
* per-node summaries should skip broken links as `getNodeCapabilities(...)` already does
* no placeholder capability should be created for a broken link
* no fake summary should be created for a broken link
* `broken_link_count` may be updated only if safe and already represented by `ApiSnapshot`

Broken links must never cause fabricated capability rows.

## 12. Bounds And Truncation

Future builder must enforce bounds:

* if node count exceeds snapshot capacity, truncate deterministically
* if capability count exceeds snapshot capacity, truncate deterministically
* if link count exceeds snapshot capacity, truncate deterministically
* set truncation flags
* never write out of bounds
* never corrupt partial output memory
* never dynamically expand storage

Truncation should preserve stable ordering from the owner-backed source.

## 13. Validation Plan For Implementation Milestone

Future implementation validation should cover:

* empty production/default snapshot is valid
* injected seeded owner store produces `node_count == 1`
* seeded capability produces `capability_count == 1`
* seeded link produces `mapping_count == 1`
* reset clears output before fill
* no fake nodes appear
* no fake capabilities appear
* no fake mappings appear
* broken links do not create fake summaries
* truncation flags are deterministic if capacity is exceeded
* no Registry, HAL, packet, provider, service, logic, or runtime calls
* no mutable owner access
* no transport
* `src/main.cpp` untouched
* existing API tests still pass

Validation-only seeded records must use deterministic numeric IDs and must not use fake `CAP_TEMPERATURE`.

## 14. Minimal App Compatibility

Builder output should map cleanly to future Minimal App DTOs.

Connection mode must not change snapshot semantics:

* Auto
* Local
* Cloud
* Gateway

All modes should consume the same logical snapshot.

The app should not need Core internals and should not infer mappings beyond the snapshot contract.

## 15. Transport Separation

The snapshot builder remains separate from:

* HTTP
* WebServer
* JSON encoding
* BLE
* WiFi
* cloud sync
* local discovery
* LoRa/gateway
* pairing/auth/encryption
* offline cache/retry

Transport encoding can be planned after the builder contract and validation are approved.

## 16. Production Default Behavior

Default builder output should be empty but valid.

Production default behavior must remain:

* no production auto-seed
* no fake demo data
* no setup/loop integration
* no `src/main.cpp` integration
* no transport integration
* no Minimal App live data yet

## 17. Safety Invariants

The future builder must preserve:

* no fake nodes
* no fake capabilities
* no fake mappings
* no fake `CAP_TEMPERATURE`
* no fake sensor values
* no fake diagnostics
* no fake provider data
* no private Registry arrays exposed
* no packet parsing
* no HAL, Driver, Device, Service, Logic, or Runtime calls
* no command execution
* no mutable owner access
* no transport
* no WebServer, HTTP, JSON, BLE, WiFi, or LoRa

## 18. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.72 - Core API Snapshot Builder Plan Validation Audit
```

Purpose:

Audit the snapshot builder plan before implementing any builder code.

Alternative:

```text
Milestone 10.72 - Core API Snapshot Builder Empty Skeleton Implementation
```

## 19. Stop Conditions

Stop future work and return to architecture review if it tries to:

* implement snapshot builder in this plan milestone
* add transport
* add JSON, HTTP, WebServer, BLE, WiFi, or LoRa
* connect Minimal App directly
* create fake snapshot data
* infer mappings without explicit links
* expose mutable owner stores
* parse packets
* read Registry arrays
* call HAL, Drivers, Devices, Services, Logic, or Runtime
* modify `src/main.cpp`
* add provider, discovery, value, or diagnostics before approved contracts
