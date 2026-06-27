# Milestone 10.68 - Core API Snapshot Struct Validation Audit

## 1. Scope

Milestone 10.68 audits the Milestone 10.67 Core API Snapshot Struct Plan only.

This milestone is documentation only.

It does not:

* modify firmware source code
* modify `src/main.cpp`
* add headers
* implement snapshot structs
* implement a snapshot builder
* add HTTP
* add WebServer
* add JSON transport
* add BLE
* add WiFi transport
* add cloud, local, LoRa, or gateway transport
* connect to Minimal App
* add production records
* add fake nodes, capabilities, mappings, values, or diagnostics
* add provider, discovery, Registry, value, or diagnostics bridges
* parse packets
* call HAL, Drivers, Devices, Modules, Services, Logic, or Runtime
* expose private Registry arrays
* expose mutable owner stores
* change existing API behavior

No implementation, transport, or JSON encoding is approved by this audit.

## 2. Files Changed In 10.67

Milestone 10.67 modified:

* `MILESTONE_10_67_CORE_API_SNAPSHOT_STRUCT_PLAN.md`

No firmware source files, headers, snapshot structs, or builders were created.

## 3. Struct Plan Summary

Milestone 10.67 plans future snapshot structs as:

* read-only output containers
* bounded
* deterministic
* caller-owned or stack/static safe
* no heap allocation
* no STL
* no Arduino `String`
* no transport dependency
* no JSON dependency
* no fake data
* no Registry/private data
* no packet parsing
* no hardware calls

The planned structs represent approved public owner/API state only.

## 4. Planned Future Files

Planned future files:

* `include/cyber32/api/api_snapshot_types.h`
* `src/api/api_snapshot_types.cpp` if needed
* `include/cyber32/api/api_snapshot_builder.h`
* `src/api/api_snapshot_builder.cpp`

Confirmed:

* these files were not created in Milestone 10.67.
* this audit does not create them.

## 5. Top-Level Snapshot Model Audit

Planned `ApiSnapshot` concept includes:

* `contract_version`
* `status`
* `node_count`
* `capability_count`
* `mapping_count`
* `nodes[]`
* `capabilities[]`
* `links[]`
* flags / truncation metadata

Field names are not final if existing project style differs.

Any future implementation milestone must follow existing naming conventions and bounded API style.

## 6. Capacity And Bounds Audit

Confirmed plan:

* compile-time bounded capacities are planned.
* capacities should align with existing `PublicOwnerStore` / directory capacities where possible.
* no dynamic allocation.
* no vector/list/map.
* truncation deterministic.
* firmware memory budget must be respected.

Snapshot capacity must be explicit before implementation.

## 7. Node Entry Audit

Node snapshot entry may include only public node data:

* `node_id`
* lifecycle / visibility / status if public
* display/name fields only if already public and bounded
* `capability_count` as display metadata only, not mapping proof

Node entry must not include:

* fake hardware info
* private MAC unless approved public identity exists
* packet data
* debug data
* Registry data

Node entries must not fabricate relationships.

## 8. Capability Entry Audit

Capability snapshot entry may include only public capability summary data:

* `capability_instance_id`
* `capability_id`
* lifecycle / visibility / availability if public
* display/name/unit fields only if already public and bounded

Capability entry must not include:

* live value
* hardware timestamp
* provider info
* diagnostics
* quality score
* fabricated `CAP_TEMPERATURE`

Capability entries are metadata records, not readings.

## 9. Mapping Entry Audit

Mapping snapshot entry may include:

* `link_id` if public
* `node_id`
* `capability_instance_id`
* lifecycle / visibility if public

Mapping entry must not be inferred from:

* `capability_count`
* `owner_node_id`
* `CAP_TEMPERATURE`
* Registry
* packets
* providers
* debug output
* sensor values
* diagnostics

Mappings must come only from explicit owner-backed links.

## 10. Status And Flags Audit

Planned status fields may include:

* `ok` / status enum if existing style supports it
* generated count fields
* `truncated_nodes`
* `truncated_capabilities`
* `truncated_links`
* `broken_link_count` if safe/internal
* `error_code` if existing API convention supports it

Transport errors are not part of snapshot structs.

Cloud/local/BLE/WiFi status is not part of Core snapshot structs.

## 11. String And Bounded Text Policy Audit

Confirmed string policy:

* dynamic strings avoided.
* fixed char arrays only if already established in API style.
* numeric IDs and enum fields preferred.
* display labels may be deferred until bounded string policy is approved.
* Arduino `String` prohibited.

The first snapshot types should not require display labels.

## 12. Builder Model Audit

Milestone 10.67 evaluated these builder options:

* `Cyber32Api::getSnapshot(ApiSnapshot& out)`
* `ApiSnapshotBuilder` builds from `Cyber32Api` read methods
* transport layer builds snapshot by calling existing API methods
* snapshot builder reads `PublicOwnerStore` directly through read-only accessors

Recommended direction:

* keep snapshot builder separate from transport
* use `Cyber32Api` / read-only owner access
* avoid mutable owner stores
* preserve production default behavior
* defer implementation decision

This prevents transport requirements from leaking into Core API.

## 13. Buffer Ownership Audit

Confirmed buffer ownership model:

* caller owns output snapshot buffer
* builder fills caller-provided struct
* counts initialized to `0` before work
* no writes past capacity
* success possible with empty valid snapshot
* deterministic truncation if output fills

No heap ownership or transfer is planned.

## 14. Empty Snapshot Behavior

Confirmed empty behavior:

* empty production snapshot is valid
* `node_count = 0`
* `capability_count = 0`
* `mapping_count = 0`
* no fake placeholder rows
* no demo node
* no demo `CAP_TEMPERATURE`
* no fake value

Empty remains a valid owner-backed state.

## 15. Seeded Validation Snapshot Behavior

Future validation should prove:

* injected owner store with node/capability/link can produce snapshot counts greater than `0`
* snapshot nodes match seeded public nodes
* snapshot capabilities match seeded public capability summaries
* snapshot links match seeded explicit links
* node capability summaries, if included later, follow explicit mapping only
* no fake values, diagnostics, or provider data

Seeded validation must use local owner-backed records and the approved API test injection seam.

## 16. Minimal App Compatibility Audit

Confirmed:

* snapshot struct should map cleanly to future Minimal App DTO/model.
* app connection mode should not change snapshot semantics.
* Auto / Local / Cloud / Gateway consume the same logical shape.
* app should not infer mappings unless contract explicitly says so.
* app should not require transport-specific fields in Core snapshot.

Snapshot structs are Core data contracts, not transport or UI implementations.

## 17. Transport Separation Audit

Snapshot structs remain separate from:

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

Transport encoding and delivery require separate milestones.

## 18. Safety Invariants

Safety invariants:

* no fake nodes
* no fake capabilities
* no fake mappings
* no fake `CAP_TEMPERATURE`
* no fake sensor values
* no fake diagnostics
* no fake provider data
* no private Registry arrays exposed
* no packet parsing
* no HAL calls
* no Driver calls
* no Device calls
* no Module calls
* no Service calls
* no Logic calls
* no Runtime calls
* no command execution
* no mutable owner access
* no transport
* no WebServer, HTTP, JSON, BLE, WiFi, or LoRa

These invariants apply before and during any future snapshot implementation.

## 19. Compatibility Statement

Milestone 10.67 defines future bounded snapshot structs and buffer model, but does not implement snapshot types, snapshot builder, or transport.

The transition remains:

```text
owner-backed API data -> bounded snapshot structs -> future snapshot builder -> future transport encoding
```

Never:

```text
owner-backed API data -> fake app demo data
```

## 20. Known Limitations

Known limitations:

* no snapshot structs yet
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

These are planned deferrals, not defects.

## 21. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.69 - Core API Snapshot Types Skeleton Implementation
```

Purpose:

Implement empty bounded snapshot type definitions only, without snapshot builder, transport, JSON, HTTP, BLE, WiFi, LoRa, fake data, or API behavior change.

Alternative:

```text
Milestone 10.69 - Core API Snapshot Builder Plan
```

## 22. Stop Conditions

Stop future work if it tries to:

* implement snapshot structs in this audit milestone
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
* add provider/discovery/value/diagnostics before approved contracts
