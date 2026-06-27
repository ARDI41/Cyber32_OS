# Milestone 10.72 - Core API Snapshot Builder Plan Validation Audit

## 1. Scope

This milestone audits the Milestone 10.71 Core API Snapshot Builder Plan only.

This milestone is documentation only.

It does not:

* modify firmware source code
* modify `src/main.cpp`
* implement snapshot builder code
* add transport
* add JSON encoding
* add HTTP, WebServer, BLE, WiFi, cloud, local, LoRa, or gateway behavior
* connect to Minimal App
* add fake data
* change existing API behavior

## 2. Files Changed In 10.71

Milestone 10.71 modified:

* `MILESTONE_10_71_CORE_API_SNAPSHOT_BUILDER_PLAN.md`

No firmware headers, source files, builder files, transport files, or `src/main.cpp` changes were made in Milestone 10.71.

## 3. Builder Plan Summary

Milestone 10.71 plans a future snapshot builder that fills `ApiSnapshot` from owner-backed public data.

Planned builder properties:

* read-only
* bounded
* deterministic
* caller-provided output
* no dynamic allocation
* no STL
* no Arduino `String`
* no transport dependency
* no JSON dependency
* no fake data
* no mutable owner access

The plan preserves the API-first and capability-first direction established in the Cyber32 Bible and snapshot contract milestones.

## 4. Planned Future Files

Planned future files:

* `include/cyber32/api/api_snapshot_builder.h`
* `src/api/api_snapshot_builder.cpp`

Confirmed:

* these files were not created in Milestone 10.71
* this audit does not create them
* no snapshot builder implementation exists yet

## 5. Builder Input Model Audit

Milestone 10.71 evaluated four input models:

* builder takes `Cyber32Api&` and calls public API methods
* builder takes `const PublicOwnerStore&` and reads owner stores directly
* builder is `Cyber32Api::fillSnapshot(ApiSnapshot& out)`
* transport layer builds snapshot by calling existing `Cyber32Api` methods

Recommended direction:

* use a separate `ApiSnapshotBuilder`
* use read-only input only
* prefer `Cyber32Api` read-only methods where possible
* allow explicit read-only owner access only where a complete API method does not yet exist
* avoid mutable owner stores
* avoid transport coupling
* preserve production default behavior

## 6. Builder Output Model Audit

Planned output behavior:

* caller owns the `ApiSnapshot` output buffer
* builder resets or initializes output before filling
* builder fills counts deterministically
* builder never writes past capacity
* builder returns compact status or `bool`
* empty snapshot is valid
* truncation flags are deterministic if capacity is exceeded
* output contains no fake placeholder rows

This output model is bounded and ESP32-safe.

## 7. Snapshot Fill Order Audit

Planned fill order:

1. reset output
2. fill system/status metadata
3. fill node entries
4. fill capability entries
5. fill mapping entries
6. optionally fill per-node summaries later if approved
7. set final counts and truncation flags

The plan keeps partially filled snapshots deterministic if a future builder returns failure.

## 8. Node Filling Audit

Confirmed node filling rules:

* include only public node records
* no fake nodes
* no private Registry data
* no packet data
* no hardware reads
* `capability_count` may be copied only as display metadata
* `capability_count` must not create mappings

Node entries must not expose private MAC or hardware details unless a future public identity contract explicitly approves them.

## 9. Capability Filling Audit

Confirmed capability filling rules:

* include only public capability summary fields
* no fake capabilities
* no fake `CAP_TEMPERATURE`
* no sensor values
* no diagnostics
* no provider info
* no quality data
* no hardware timestamps

Capability snapshot entries remain metadata records, not live sensor readings.

## 10. Mapping Filling Audit

Confirmed mapping filling rules:

* include only explicit `NodeCapabilityMap` links
* no inferred mappings
* no mapping from `capability_count`
* no mapping from `owner_node_id`
* no mapping from `CAP_TEMPERATURE`
* no Registry, packet, provider, debug, sensor-value, or diagnostic inference

Mappings are relationship records only.

## 11. Broken Link Audit

Confirmed broken link plan:

* raw link array may include only stored links if reading the map directly
* per-node summaries should skip broken links as `getNodeCapabilities(...)` already does
* no placeholder capability is created
* no fake summary is created
* `broken_link_count` may be used only if already planned and safe

Broken links must never fabricate capability rows or values.

## 12. Bounds And Truncation Audit

Confirmed bounds and truncation plan:

* node truncation deterministic
* capability truncation deterministic
* link truncation deterministic
* truncation flags set if supported
* no out-of-bounds writes
* no dynamic expansion
* no partial memory corruption

Ordering should remain stable according to the owner-backed source.

## 13. Validation Plan Audit

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

Confirmed:

* builder output should map to future Minimal App DTOs
* connection mode must not change snapshot semantics
* Auto, Local, Cloud, and Gateway should consume the same logical snapshot
* the app should not need Core internals
* the app should not infer mappings beyond the contract

The builder remains a Core-side bounded representation step, not an app transport.

## 15. Transport Separation

The builder remains separate from:

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

Transport encoding remains a future milestone after builder planning and validation.

## 16. Production Default Behavior

Confirmed production default behavior:

* default builder output should be empty but valid
* no production auto-seed
* no fake demo data
* no setup/loop integration
* no `src/main.cpp` integration
* no transport integration
* no Minimal App live data yet

## 17. Safety Invariants

Confirmed safety invariants:

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

## 18. Compatibility Statement

Milestone 10.71 plans a future read-only snapshot builder, but does not implement builder code, transport encoding, JSON, or Minimal App integration.

The transition remains:

```text
owner-backed API data
-> bounded snapshot types
-> future snapshot builder
-> future transport encoding
```

Never:

```text
owner-backed API data
-> fake app demo data
```

## 19. Known Limitations

Known limitations:

* no snapshot builder yet
* no snapshot API yet
* no JSON encoding
* no HTTP/WebServer
* no BLE/WiFi
* no LoRa/Gateway transport
* no live Minimal App data
* no values
* no diagnostics
* no provider info
* no pairing/authentication
* no offline cache
* no cloud/local routing

## 20. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.73 - Core API Snapshot Builder Empty Skeleton Implementation
```

Purpose:

Implement an empty bounded `ApiSnapshotBuilder` skeleton that can reset/fill an empty `ApiSnapshot` without transport, JSON, fake data, mutable owner access, or `Cyber32Api` behavior changes.

Alternative:

```text
Milestone 10.73 - Core API Snapshot Builder Input Model Plan
```

## 21. Stop Conditions

Stop future work and return to architecture review if it tries to:

* implement snapshot builder in this audit milestone
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
