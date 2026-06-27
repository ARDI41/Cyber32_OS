# Milestone 10.64 - Core API Snapshot Contract Validation Audit

## 1. Scope

Milestone 10.64 audits the Milestone 10.63 Core API Snapshot Contract Plan only.

This milestone is documentation only.

It does not:

* modify firmware source code
* modify `src/main.cpp`
* implement a snapshot API
* add snapshot structs
* add HTTP
* add WebServer
* add JSON transport
* add BLE
* add WiFi transport
* add cloud transport
* add local transport
* add LoRa transport
* connect to Minimal App
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

## 2. Files Changed In 10.63

Milestone 10.63 modified:

* `MILESTONE_10_63_CORE_API_SNAPSHOT_CONTRACT_PLAN.md`

No firmware source files were changed.

## 3. Contract Summary

Milestone 10.63 defines a future Core API snapshot contract.

Contract summary:

* snapshot contract is a read-only data shape.
* snapshot contract is not a transport.
* snapshot contract does not imply HTTP, JSON, BLE, WiFi, or LoRa.
* snapshot contract does not mutate state.
* snapshot contract does not create records.
* snapshot contract does not infer mappings.
* snapshot contract does not add fake data.
* snapshot contract does not read Registry.
* snapshot contract does not parse packets.
* snapshot contract does not call HAL, Drivers, Devices, Services, Logic, or Runtime.

The plan separates bounded state representation from future transport encoding.

## 4. Owner-Backed Data Boundary

The snapshot may represent data only from:

* approved `Cyber32Api` read paths
* `PublicOwnerStore` read-only owner-backed data
* `NodeDirectory` public node records
* `CapabilityDirectory` public capability summary records
* `NodeCapabilityMap` explicit mapping links
* `getNodeCapabilities(...)` explicit mapping behavior

The snapshot must not use:

* mutable owner stores
* private Registry arrays
* packet data
* provider internals
* sensor runtime values
* diagnostics
* debug output
* fake demo records

This preserves the owner-backed API data boundary.

## 5. Snapshot Content Groups

Planned snapshot content groups:

* system summary
* node list
* capability list
* node-capability mappings
* optional per-node capability summaries
* error/status metadata

Deferred content:

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

The first snapshot contract should expose identity, visibility, relationship, and status metadata only where already owner-backed and approved.

## 6. Minimal App Boundary

Minimal App boundary:

* snapshot is useful for future Minimal App mock/live preview.
* Minimal App remains mock-only for now.
* no live transport exists yet.
* app must not access `PublicOwnerStore` directly.
* app must not access mutable owner stores.
* app must not infer mappings from separate node/capability lists.
* app should later consume the same snapshot shape regardless of Local, Cloud, or Gateway mode.

The snapshot contract is a future app data shape, not a live app integration.

## 7. Connection Mode Compatibility

Future UX compatibility:

* snapshot shape must be independent of connection mode.
* future connection modes may include Auto, Local, Cloud, and Gateway/LoRa.
* normal users should not need manual IP, URL, or port settings.
* advanced technical settings should remain hidden.
* discovery, pairing, local, cloud, and LoRa behavior are separate future contracts.

The snapshot should be a stable payload shape across connection paths.

## 8. Transport Separation

The snapshot contract is separate from:

* local discovery
* cloud sync
* BLE
* WiFi
* HTTP
* JSON encoding
* LoRa/gateway
* pairing
* authentication
* encryption
* retry/offline cache

No transport implementation is approved by Milestone 10.63 or this audit.

## 9. Bounds And Safety

Planned implementation must be:

* fixed maximum counts
* bounded arrays
* no dynamic allocation
* no STL
* no Arduino `String`
* caller-provided buffers preferred
* no out-of-bounds writes
* deterministic truncation if needed
* empty snapshot is valid

Any future snapshot implementation must remain ESP32-safe and bounded.

## 10. Production Default Behavior

Production default behavior:

* production default snapshot would be empty but valid.
* no production auto-seed.
* no fake demo data.
* no `src/main.cpp` integration.
* no setup/loop integration.
* no transport integration.
* no Minimal App live data yet.

Empty owner-backed state remains valid architecture.

## 11. Validation Plan Audit

Future validation should cover:

* empty production snapshot is valid
* injected seeded owner store can produce node/capability/link snapshot
* no fake data appears
* node capability summaries only follow explicit links
* broken links do not produce fake summaries
* output bounds are respected
* truncation is deterministic
* no Registry, HAL, packet, or provider calls
* `src/main.cpp` untouched
* existing API tests still pass

Validation should use the safe API test owner injection seam when seeded owner-backed data is required.

## 12. Safety Invariants

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

These invariants must remain active when snapshot structs or builders are eventually planned.

## 13. Compatibility Statement

Milestone 10.63 defines a future read-only snapshot contract, but does not implement snapshot API or transport.

The transition remains:

```text
owner-backed API data -> read-only snapshot contract -> future transport encoding
```

Never:

```text
owner-backed API data -> fake app demo data
```

## 14. Known Limitations

Known limitations:

* no snapshot API yet
* no snapshot structs yet
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

These are intentional deferrals.

## 15. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.65 - Minimal App Connection Mode Strategy Plan
```

Purpose:

Plan the client-facing connection mode UX and architecture for Auto, Local, Cloud, and future Gateway/LoRa modes without implementing transport or requiring users to manually enter IP, URL, or port settings.

Alternative:

```text
Milestone 10.65 - Core API Snapshot Struct Plan
```

## 16. Stop Conditions

Stop future work if it tries to:

* implement transport in a snapshot audit milestone
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
