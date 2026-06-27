# Milestone 10.67 - Core API Snapshot Struct Plan

## 1. Scope

Milestone 10.67 plans future Core API snapshot structs only.

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

The purpose is to plan a firmware-safe snapshot struct layer that represents owner-backed Core API data in a bounded read-only format for future transport/mock consumers.

## 2. Background

Current planning state:

* Core API Snapshot Contract exists.
* Minimal App Connection Mode Strategy exists.
* Minimal App should later consume the same logical snapshot through Auto, Local, Cloud, and Gateway modes.
* Core OS remains transport-independent.
* snapshot struct layer should be bounded and firmware-safe.

The next step is to plan the shape and memory model before adding any headers, structs, builders, or transports.

## 3. Snapshot Struct Principle

Snapshot structs must be:

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

Snapshot structs represent approved public owner/API state only.

## 4. Proposed Future File Plan

Possible future files:

* `include/cyber32/api/api_snapshot_types.h`
* `src/api/api_snapshot_types.cpp` if needed
* `include/cyber32/api/api_snapshot_builder.h`
* `src/api/api_snapshot_builder.cpp`

Do not create these files in Milestone 10.67.

The implementation milestone should follow existing project include/source layout and naming conventions.

## 5. Snapshot Top-Level Struct

Future top-level struct concept:

```cpp
struct ApiSnapshot {
    // conceptual only
};
```

Conceptual fields:

* `contract_version`
* `status`
* `node_count`
* `capability_count`
* `mapping_count`
* `nodes[]`
* `capabilities[]`
* `links[]`
* flags / truncation metadata

Do not finalize field names in this plan if existing style differs.

The implementation milestone must follow existing API naming conventions.

## 6. Fixed Capacity Constants

Future snapshot structs should use compile-time bounded capacities:

* max nodes
* max capabilities
* max links
* optional max per-node summaries later

Rules:

* capacities should align with existing `PublicOwnerStore` / directory capacities where possible
* no dynamic allocation
* no vector/list/map
* truncation must be deterministic
* capacity values should not exceed firmware-safe memory budget

Initial implementation should prefer small bounds already proven by public owner stores.

## 7. Node Snapshot Entry

Node entry fields should come from public node data only.

Planned fields:

* `node_id`
* lifecycle / visibility / status if already public
* display/name fields if already public and bounded
* `capability_count` as display metadata only, not mapping proof

Forbidden fields:

* private MAC unless approved public identity exists
* packet data
* debug data
* Registry/private data
* driver/device/HAL details

Node snapshot entries must not imply mappings without explicit link entries.

## 8. Capability Snapshot Entry

Capability entry fields should come from public capability summary only.

Planned fields:

* `capability_instance_id`
* `capability_id`
* lifecycle / visibility / availability if already public
* display/name/unit fields only if already public and bounded

Forbidden fields:

* live value
* hardware timestamp
* provider info
* diagnostics
* quality score
* fabricated `CAP_TEMPERATURE`

Capability entries should describe public capability identity/metadata, not readings.

## 9. Mapping Snapshot Entry

Mapping entry fields:

* `link_id` if public
* `node_id`
* `capability_instance_id`
* lifecycle / visibility if public

Rules:

* no inferred mapping
* no mapping from `capability_count`
* no mapping from `owner_node_id`
* no mapping from `CAP_TEMPERATURE`

Mapping entries are relationship records only.

## 10. Status And Flags

Planned snapshot status fields:

* `ok` / status enum if existing style supports it
* generated count fields
* `truncated_nodes`
* `truncated_capabilities`
* `truncated_links`
* `broken_link_count` if safe/internal
* `error_code` if existing API convention supports it

Do not define transport errors here.

Do not define cloud/local/BLE/WiFi status here.

Transport status belongs to future transport contracts.

## 11. String / Bounded Text Policy

Planned text policy:

* avoid dynamic strings
* use fixed char arrays only if already established in API style
* prefer numeric IDs and enum fields
* display labels may be deferred if bounded string policy is not approved
* no Arduino `String`

The first snapshot structs should not require user-visible labels to be useful.

## 12. Builder Model Options

Option A: `Cyber32Api::getSnapshot(ApiSnapshot& out)`

Benefits:

* simple API surface
* caller gets one complete snapshot

Risks:

* may grow into a monolithic API method
* can increase memory pressure
* may blur Core API and transport needs

Option B: `ApiSnapshotBuilder` builds from `Cyber32Api` read methods

Benefits:

* separates builder from existing API
* can reuse read-only APIs
* easier to keep transport-independent

Risks:

* introduces another API layer

Option C: Transport layer builds snapshot by calling existing API methods

Benefits:

* no new Core snapshot builder required initially
* transport-specific concerns remain outside Core API

Risks:

* repeated logic across transports unless shared later

Option D: Snapshot builder reads `PublicOwnerStore` directly through read-only accessors

Benefits:

* direct access to owner-backed arrays
* avoids reformatting through multiple API calls

Risks:

* tighter coupling to public owner store
* must strictly avoid mutable access

Recommended safest first approach:

* keep snapshot builder separate from transport
* use `Cyber32Api` / read-only owner access
* avoid mutable owner stores
* preserve production default behavior
* defer implementation decision to a later milestone

## 13. Buffer Ownership

Planned buffer ownership rules:

* caller owns output snapshot buffer
* builder fills caller-provided struct
* builder initializes counts to `0` before work
* builder never writes past capacity
* builder returns success even when empty if valid
* builder reports deterministic truncation if output fills

The builder must not allocate.

## 14. Empty Snapshot Behavior

Planned empty behavior:

* empty production snapshot is valid
* `node_count = 0`
* `capability_count = 0`
* `mapping_count = 0`
* no fake placeholder rows
* no demo node
* no demo `CAP_TEMPERATURE`
* no fake value

Empty is a valid state, not an error.

## 15. Seeded Validation Snapshot Behavior

Future seeded validation should prove:

* injected owner store with node/capability/link can produce snapshot counts greater than `0`
* snapshot nodes match seeded public nodes
* snapshot capabilities match seeded public capability summaries
* snapshot links match seeded explicit links
* node capability summaries, if included later, follow explicit mapping only
* no fake values, diagnostics, or provider data

Validation data must remain local, deterministic, and owner-backed.

## 16. Minimal App Compatibility

Snapshot struct should map cleanly to future Minimal App DTO/model.

Rules:

* app connection mode should not change snapshot semantics
* Auto / Local / Cloud / Gateway all consume the same logical shape
* app should not infer mappings from separate arrays unless contract explicitly says so
* app should not require transport-specific fields in Core snapshot

Core snapshot is a data contract, not an app UX model.

## 17. Transport Separation

Snapshot structs are separate from:

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

Transport encoding and delivery require later milestones.

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

## 19. Validation Plan For Implementation Milestone

Future implementation validation should prove:

* default snapshot initialized empty
* empty production snapshot valid
* seeded injected owner store produces correct counts
* seeded node appears once
* seeded capability appears once
* seeded link appears once
* broken links do not create fake capability summaries
* bounds and truncation deterministic
* output initialized before fill
* no fake data
* existing API tests still pass
* `src/main.cpp` untouched
* no transport added

Validation should use fixed buffers and local owner-backed test data only.

## 20. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.68 - Core API Snapshot Struct Validation Audit
```

Purpose:

Audit the snapshot struct plan before any implementation.

Alternative:

```text
Milestone 10.68 - Core API Snapshot Types Skeleton Implementation
```

## 21. Stop Conditions

Stop future work if it tries to:

* implement snapshot structs in this plan milestone
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
