# Milestone 10.51 - PublicOwnerStore NodeCapabilityMap Ownership Plan

## 1. Scope

Milestone 10.51 plans future `PublicOwnerStore` ownership of `NodeCapabilityMap` only.

This milestone is documentation only.

It does not:

* modify firmware source code
* modify `src/main.cpp`
* implement ownership
* connect `NodeCapabilityMap` to `Cyber32Api`
* change `getNodeCapabilities(...)` behavior
* add real mapping records
* add fake mappings
* infer capabilities from nodes
* infer nodes from capabilities
* add `addLink`, `updateLink`, `removeLink`, `lookupByNode`, or `lookupByCapability`
* add discovery, provider, Registry, value, diagnostics, or transport bridges
* parse packets
* call HAL, Drivers, Devices, Modules, Services, Logic, or Runtime
* add provider selection
* add command execution
* add WebServer, HTTP, JSON, BLE, WiFi, or Minimal App transport
* expose private Registry arrays
* change existing API behavior

The purpose is to plan how `PublicOwnerStore` should own `NodeCapabilityMap` so mapping lifetime, reset behavior, and access boundaries are explicit before any `addLink` or API integration milestone.

## 2. Background

Current public owner state:

* `PublicOwnerStore` currently owns `NodeDirectory` and `CapabilityDirectory`.
* `NodeCapabilityMap` empty skeleton exists.
* `NodeCapabilityMap` is not yet owned by `PublicOwnerStore`.
* `NodeCapabilityMap` is not connected to `Cyber32Api`.
* `getNodeCapabilities(...)` behavior remains unchanged.
* no mapping links exist yet.

Current external API behavior remains correct:

* Node API default behavior remains empty or `node_not_found`.
* Capability API default behavior remains empty or `capability_not_found`.
* `getNodeCapabilities(...)` still exposes no owner-backed mapping data.

This empty behavior is architecture, not a bug. A mapping owner must be attached deliberately before any node can expose capabilities through the public API.

## 3. Ownership Principle

`PublicOwnerStore` should become the central owner of public mapping state.

Meaning:

* `PublicOwnerStore` owns `NodeCapabilityMap`.
* `PublicOwnerStore::reset()` clears `NodeCapabilityMap`.
* API reads may later use a read-only accessor only.
* a mutable accessor may be used only by approved validation or bridge milestones.
* UI, App, Transport, WebServer, HTTP, JSON, BLE, and WiFi layers must not access the mutable mapping owner.

This keeps mapping state in the same public owner lifetime boundary as public nodes and public capabilities.

## 4. Proposed Future PublicOwnerStore Contents

Future `PublicOwnerStore` contents:

```cpp
NodeDirectory node_directory_;
CapabilityDirectory capability_directory_;
NodeCapabilityMap node_capability_map_;
```

Deferred stores:

* `CapabilityValueStore`
* `DiagnosticsStore`
* `ProviderPublicInfoStore`
* `PublicEventSnapshotStore`

`NodeCapabilityMap` ownership should be added before values or diagnostics, because values and diagnostics must not be used to invent node-to-capability relationships.

## 5. Accessor Plan

Future read-only accessor:

```cpp
const NodeCapabilityMap& nodeCapabilities() const;
```

Future mutable accessor:

```cpp
NodeCapabilityMap& mutableNodeCapabilities();
```

Accessor rules:

* `Cyber32Api` reads may later use only `nodeCapabilities()`.
* `Cyber32Api` read methods must not use `mutableNodeCapabilities()`.
* UI, App, Transport, WebServer, HTTP, JSON, BLE, and WiFi must not use `mutableNodeCapabilities()`.
* `mutableNodeCapabilities()` is internal, test-only, or approved bridge-only.
* mutable access must not be used to create records during API read methods.

## 6. Reset / Init Plan

Future reset/init behavior:

* constructor/default state remains empty.
* `reset()` clears `NodeDirectory`.
* `reset()` clears `CapabilityDirectory`.
* `reset()` clears `NodeCapabilityMap`.
* reset must not create links.
* reset must not infer links.
* reset must not parse packets.
* reset must not read Registry.
* reset must not trigger discovery import.
* reset must not trigger provider import.
* reset must not notify app or transport.

Reset should preserve the current safe public API default behavior: no public mappings appear unless a future approved owner-backed creation path adds them.

## 7. Dependency Rules

`PublicOwnerStore` may include:

* `node_directory.h`
* `capability_directory.h`
* `node_capability_map.h`

`PublicOwnerStore` must not include:

* Registry internals
* HAL
* Drivers
* Devices
* Modules
* Services
* Logic
* Runtime
* packets
* WiFi
* ESP-NOW
* WebServer
* HTTP
* JSON
* BLE
* Minimal App
* Arduino `String`
* STL containers
* heap allocation

The ownership change should remain a bounded public owner composition change only.

## 8. API Behavior Plan

This ownership integration must not change `Cyber32Api` behavior.

After ownership implementation:

* `getNodeCapabilities(...)` remains unchanged.
* Node API default behavior remains unchanged.
* Capability API default behavior remains unchanged.
* no mapping appears automatically.
* no node gains capabilities automatically.
* no capability attaches to a node automatically.
* no API method infers mapping from `NodeDirectory`.
* no API method infers mapping from `CapabilityDirectory`.

Future API mapping integration requires a separate approved milestone after `NodeCapabilityMap` ownership and controlled link creation are validated.

## 9. Validation Plan For Implementation Milestone

Future implementation validation should prove:

* `PublicOwnerStore` default `NodeCapabilityMap` count is `0`.
* `PublicOwnerStore` reset clears `NodeCapabilityMap`.
* read-only `nodeCapabilities()` accessor does not mutate state.
* `mutableNodeCapabilities()` exists only for future approved tests or bridges if implemented.
* no fake links appear.
* no inferred links appear.
* `NodeDirectory` tests still pass.
* `CapabilityDirectory` tests still pass.
* `NodeCapabilityMap` tests still pass.
* API default tests still pass.
* `src/main.cpp` remains untouched.

Validation should not add links, seed production state, infer mapping, parse packets, or connect the map to API reads.

## 10. Production Default Behavior

Production default remains no mapping links.

Production default behavior:

* no setup/loop integration
* no transport seeding
* no discovery/provider seeding
* no Registry seeding
* no automatic node-to-capability links
* no live mapped node capabilities visible to Minimal App

Owning an empty `NodeCapabilityMap` is not the same as exposing live mappings.

## 11. Minimal App Implications

Minimal App implications:

* no app change is required.
* app remains mock-only.
* app must not access `PublicOwnerStore` directly.
* app must not access `NodeCapabilityMap` directly.
* app must not infer node capabilities from node list plus capability list.
* future mapped display requires approved `NodeCapabilityMap`-backed API integration.

Until that future API integration exists, empty node capability state remains correct.

## 12. Safety Invariants

Safety invariants:

* no fake mappings
* no fake nodes
* no fake capabilities
* no fake `CAP_TEMPERATURE`
* no fake sensor values
* no fake diagnostics
* no private Registry arrays exposed
* no packet parsing
* no HAL calls
* no Driver calls
* no Device calls
* no Module calls
* no Service calls
* no Logic calls
* no Runtime calls
* no provider selection
* no command execution
* no `src/main.cpp` changes
* no app transport
* no WebServer, HTTP, JSON, BLE, or WiFi

These invariants apply to the ownership integration itself and to every future milestone that writes or reads mapping state.

## 13. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.52 - PublicOwnerStore NodeCapabilityMap Ownership Implementation
```

Purpose:

Add `NodeCapabilityMap` as an empty owned store inside `PublicOwnerStore` with reset/accessor validation, without changing API behavior.

Alternative:

```text
Milestone 10.52 - NodeCapabilityMap Controlled Add Link Plan
```

## 14. Stop Conditions

Stop future work if it tries to:

* connect `NodeCapabilityMap` to API in this ownership plan
* add mapping links
* infer capabilities from nodes
* infer nodes from capabilities
* create fake mappings
* auto-seed production mappings
* parse packets
* read Registry arrays
* call HAL, Drivers, Devices, Services, Logic, or Runtime
* add discovery/provider bridge before approval
* add transport before approval
* modify `src/main.cpp`
* expose mutable mapping access to UI/App/Transport
* add values/diagnostics without approved stores
* change existing API behavior during ownership integration
