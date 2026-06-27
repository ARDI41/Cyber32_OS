# Milestone 10.63 - Core API Snapshot Contract Plan

## 1. Scope

Milestone 10.63 plans a Core API snapshot contract only.

This milestone is documentation only.

It does not:

* modify firmware source code
* modify `src/main.cpp`
* implement a snapshot API
* add HTTP
* add WebServer
* add JSON transport
* add BLE
* add WiFi transport
* add LoRa transport
* connect to Minimal App
* add cloud or local transport
* add production records
* add fake nodes
* add fake capabilities
* add fake mappings
* add fake `CAP_TEMPERATURE`
* add fake sensor values
* add fake diagnostics
* add provider, discovery, Registry, value, or diagnostics bridges
* parse packets
* call HAL, Drivers, Devices, Modules, Services, Logic, or Runtime
* expose private Registry arrays
* expose mutable owner stores
* change existing API behavior

The purpose is to define a stable read-only snapshot data shape for future Minimal App and transport layers before any transport or implementation milestone.

## 2. Background

Current public API state:

* `PublicOwnerStore` owns `NodeDirectory`, `CapabilityDirectory`, and `NodeCapabilityMap`.
* `getNodeCapabilities(...)` can read explicit owner-backed mapping links.
* positive injected API validation exists through a validation/internal owner-store seam.
* production default remains empty.
* Minimal App still has no live transport.
* the app needs a stable snapshot shape before local/cloud transport is added.

The current API can expose owner-backed empty and validation-seeded views, but no transport contract exists yet.

## 3. Snapshot Principle

The snapshot is a read-only data shape, not a transport.

It must:

* use only public API / owner-backed data
* not mutate state
* not create records
* not infer mappings
* not add fake data
* not read Registry
* not parse packets
* not call HAL, Drivers, Devices, Services, Logic, or Runtime
* not imply cloud, local, BLE, WiFi, HTTP, JSON, or LoRa implementation

The snapshot is a contract for bounded state representation. Transport encoding and delivery are separate future decisions.

## 4. Intended Future Consumers

Intended future consumers:

* Minimal App mock/live preview
* future local transport adapter
* future cloud transport adapter
* future gateway transport adapter
* future debug/test harness

Clarifications:

* snapshot contract is not UI-only.
* snapshot contract is not HTTP-specific.
* snapshot contract is not JSON-specific yet.
* transport encoding is deferred.

The same snapshot shape should be reusable across future connection modes.

## 5. Snapshot Content Model

First snapshot content groups:

* system summary
* node list
* capability list
* node-capability mappings
* per-node capability summaries
* error/status metadata

Explicitly deferred:

* live sensor values
* diagnostics
* provider info
* quality
* command execution
* event stream
* logs
* firmware update
* pairing
* cloud account state

The first contract should focus on public owner identity, visibility, and relationship data.

## 6. Required Fields - System Summary

Planned minimal system fields:

* `api_version` or `contract_version`
* `node_count`
* `capability_count`
* `mapping_count`
* `snapshot_status`
* `generated_at_ms` if safe public time exists, otherwise deferred
* truncation flags if needed

Do not require HAL time if it is not already safe through public API.

The summary must treat empty production state as valid.

## 7. Required Fields - Nodes

Each node entry should come from public node data only.

Planned fields:

* `node_id`
* lifecycle / visibility / status fields if already public
* display/name fields only if already public
* `capability_count` only as display metadata, not mapping proof

Forbidden fields:

* fake hardware info
* packet MAC unless approved public identity exists
* Registry/private data
* driver or device details

Node entries must not imply capabilities unless explicit mappings exist.

## 8. Required Fields - Capabilities

Each capability entry should come from public capability summary data only.

Planned fields:

* `capability_instance_id`
* `capability_id`
* lifecycle / visibility / availability fields if already public
* display/name/unit fields only if already public

Forbidden fields:

* sensor value
* provider info
* diagnostics
* quality score
* fabricated `CAP_TEMPERATURE`

Capability entries must not imply values or diagnostics.

## 9. Required Fields - Node-Capability Mappings

Each mapping entry should come from explicit owner-backed mapping links.

Planned fields:

* `node_id`
* `capability_instance_id`
* `link_id` if public
* visibility / lifecycle if public

Rules:

* no inferred mapping
* no mapping from `capability_count`
* no mapping from `owner_node_id`
* no mapping from `CAP_TEMPERATURE`

Mappings are relationship records only. They do not prove a live value, provider, signal, diagnostic, or command permission exists.

## 10. Per-Node Capability Summary View

Optional derived view:

* for each node, include summaries of explicitly linked capabilities
* this must use `getNodeCapabilities(...)` behavior
* no values
* no diagnostics
* no provider info
* no quality data
* no fake summaries

This view may duplicate information already present in node, capability, and mapping arrays.

Recommendation:

* first contract can include raw arrays
* per-node summary view may be optional or deferred to avoid duplication

The app can derive per-node display groups from raw arrays if the mapping array is complete and bounded.

## 11. Error And Status Model

Planned error/status model:

* snapshot read can succeed with empty arrays
* empty production default is valid
* missing data must not become fake placeholders
* broken links may be skipped or reported as internal status
* truncation should be represented deterministically if output limits exist
* transport errors are deferred to transport contract

Status should distinguish:

* success with empty data
* partial/truncated data
* unsupported/deferred fields
* internal owner inconsistency

No state should convert default zero values into real data.

## 12. Size And Bounds

Snapshot implementation must remain bounded.

Planned constraints:

* fixed maximum counts
* bounded arrays
* no dynamic allocation
* no STL
* no Arduino `String`
* caller-provided buffers preferred
* snapshot builder must never write out of bounds
* partial/truncated behavior must be deterministic

Any future monolithic snapshot struct must define fixed array capacities before implementation.

## 13. Future API Shape Options

Option A: `getSnapshot(ApiSnapshot& out)`

Benefits:

* simple caller experience
* one call to fill snapshot

Risks:

* large struct
* harder to keep bounded
* may duplicate existing API methods
* can tempt transport-specific concerns into Core API

Option B: `fillSnapshot(ApiSnapshotBuffer& out)`

Benefits:

* caller-provided buffer can make bounds explicit
* better fit for fixed storage

Risks:

* more complex contract
* still risks monolithic growth

Option C: separate existing calls only

Examples:

* `getNodeList(...)`
* `getCapabilityList(...)`
* `getNodeCapabilities(...)`

Benefits:

* already bounded
* keeps Core API small
* avoids monolithic snapshot memory pressure

Risks:

* transport/app adapter must orchestrate multiple calls

Option D: transport builds snapshot outside Core API by calling existing APIs

Benefits:

* keeps Core API read-only and bounded
* transport can choose encoding later
* no Core dependency on HTTP/JSON/BLE/WiFi

Risks:

* transport snapshot builder needs a clear contract

Recommended safest first direction:

* do not add a monolithic snapshot yet
* define contract first
* future implementation may use existing API calls to build a transport snapshot outside Core API
* Core remains read-only and bounded

## 14. Minimal App UX Implications

Minimal App implications:

* app can later consume one stable snapshot shape
* app should not care whether snapshot came from local, cloud, or gateway transport
* app must not require manual IP / URL / port from normal users
* future connection modes may be Auto, Local, Cloud, or Gateway/LoRa
* advanced technical settings should be hidden
* snapshot contract must remain independent of connection mode

The app must not infer capabilities from separate lists unless explicit mapping data is present.

## 15. Transport Separation

Snapshot contract is separate from:

* local discovery
* cloud sync
* BLE
* WiFi
* HTTP
* LoRa/gateway
* pairing
* authentication
* encryption
* retry/offline cache

These require later milestones.

No transport implementation is approved by this plan.

## 16. Production Default Behavior

Production default behavior:

* default snapshot should be empty but valid
* no production auto-seed
* no fake demo data
* no `src/main.cpp` integration
* no setup/loop integration
* no transport integration
* no Minimal App live data yet

Empty is a valid owner-backed state, not a failure.

## 17. Validation Plan For Future Implementation

Future validation should prove:

* empty production snapshot is valid
* seeded injected owner store can produce node/capability/link snapshot
* no fake data appears
* node capability summaries only follow explicit links
* broken links do not produce fake summaries
* output bounds are respected
* truncation is deterministic
* no Registry, HAL, packet, or provider calls
* `src/main.cpp` untouched
* existing API tests still pass

Validation should use local owner-backed stores and fixed buffers only.

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

## 19. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.64 - Core API Snapshot Contract Validation Audit
```

Purpose:

Audit the Core API Snapshot Contract Plan before any implementation or transport decision.

Alternative:

```text
Milestone 10.64 - Minimal App Connection Mode Strategy Plan
```

## 20. Stop Conditions

Stop future work if it tries to:

* implement transport in a snapshot contract milestone
* add HTTP, WebServer, JSON, BLE, WiFi, or LoRa
* connect Minimal App directly
* create fake snapshot data
* infer mappings without explicit links
* expose mutable owner stores
* parse packets
* read Registry arrays
* call HAL, Drivers, Devices, Services, Logic, or Runtime
* add provider/discovery/value/diagnostics before approved stores/contracts
* modify `src/main.cpp`
