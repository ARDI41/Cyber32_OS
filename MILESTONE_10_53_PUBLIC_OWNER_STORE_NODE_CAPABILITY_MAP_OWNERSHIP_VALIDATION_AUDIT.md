# Milestone 10.53 - PublicOwnerStore NodeCapabilityMap Ownership Validation Audit

## 1. Scope

Milestone 10.53 audits the Milestone 10.52 `PublicOwnerStore` ownership of `NodeCapabilityMap`.

This milestone is documentation only.

It does not:

* modify firmware source code
* modify `src/main.cpp`
* change `PublicOwnerStore` implementation
* change `NodeCapabilityMap` implementation
* change `Cyber32Api` behavior
* connect `NodeCapabilityMap` to `Cyber32Api`
* change `getNodeCapabilities(...)` behavior
* add mapping records
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

## 2. Files Changed In 10.52

Milestone 10.52 modified:

* `include/cyber32/public/public_owner_store.h`
* `src/public/public_owner_store.cpp`
* `src/app/validation/vertical_slice_validation.h`
* `src/app/validation/vertical_slice_validation.cpp`

No `src/main.cpp` change was made.

## 3. Implementation Summary

Milestone 10.52 added empty `NodeCapabilityMap` ownership to `PublicOwnerStore`.

Implementation summary:

* `PublicOwnerStore` now owns `NodeCapabilityMap`.
* `const NodeCapabilityMap& nodeCapabilities() const` was added.
* `NodeCapabilityMap& mutableNodeCapabilities()` was added.
* `PublicOwnerStore::reset()` now resets the owned `NodeCapabilityMap`.
* ownership is empty by default.
* ownership does not create mappings.
* ownership does not infer mappings.
* ownership does not connect to `Cyber32Api`.
* ownership does not change `getNodeCapabilities(...)` behavior.

This milestone adds lifetime ownership only. It does not add mapping creation or API visibility.

## 4. Validation Coverage

`validatePublicOwnerStoreNodeCapabilityMapOwnership()` covers:

* `PublicOwnerStore` default `nodeCapabilities().count()` is `0`.
* `PublicOwnerStore` default `nodeCapabilities().isEmpty()` is true.
* `PublicOwnerStore` reset keeps `nodeCapabilities().count()` at `0`.
* read-only `nodeCapabilities()` accessor does not mutate state.
* `mutableNodeCapabilities()` returns access to the owned map.
* mutable access does not create links by itself.
* no fake links appear.
* no inferred links appear.
* existing `NodeDirectory` tests still pass.
* existing `CapabilityDirectory` tests still pass.
* existing `NodeCapabilityMap` tests still pass.
* existing `PublicOwnerStore` tests still pass.
* existing API default tests still pass.

Because `NodeCapabilityMap` currently has no `addLink` path, validation remains empty-state only and does not invent links.

## 5. API Behavior

API behavior remains unchanged:

* `NodeCapabilityMap` is not connected to `Cyber32Api`.
* `getNodeCapabilities(...)` behavior remains unchanged.
* Node API default behavior remains unchanged.
* Capability API default behavior remains unchanged.
* no node gains capabilities automatically.
* no capability attaches to node automatically.
* no API method infers mapping from `NodeDirectory`.
* no API method infers mapping from `CapabilityDirectory`.

The API still requires a future approved mapping integration milestone before mapped node capabilities can be exposed.

## 6. Ownership Boundary

Ownership boundary:

* `PublicOwnerStore` owns mapping lifetime.
* `PublicOwnerStore::reset()` clears the mapping owner.
* `nodeCapabilities()` is read-only.
* `mutableNodeCapabilities()` is internal, test-only, or approved bridge-only.
* UI, App, and Transport must not access `mutableNodeCapabilities()`.
* ownership does not imply live mappings.
* ownership does not imply API visibility.

The ownership path remains:

```text
PublicOwnerStore
-> NodeDirectory
-> CapabilityDirectory
-> NodeCapabilityMap
```

It remains separate from Registry, packet parsing, provider selection, and UI transport.

## 7. Safety Boundary

Safety boundary remains intact:

* no fake mappings
* no fake nodes
* no fake capabilities
* no fake `CAP_TEMPERATURE`
* no fake sensor values
* no fake diagnostics
* no Registry access
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
* no WebServer, HTTP, JSON, BLE, or WiFi
* no Minimal App live transport

Milestone 10.52 is a bounded ownership expansion only.

## 8. Local Build Result

Build result:

```text
pio run: passed locally
```

Codex shell note:

The Codex shell may not have `pio` on `PATH`. If `pio` is unavailable in the Codex shell, local PlatformIO validation remains the build confirmation source.

## 9. Compatibility Statement

Milestone 10.52 adds empty `NodeCapabilityMap` ownership to `PublicOwnerStore`, but does not change production default API behavior.

The transition remains:

```text
empty -> owner-backed empty -> owner-backed test-controlled link -> future owner-backed real
```

Never:

```text
empty -> fake
```

## 10. Known Limitations

Known limitations after Milestone 10.52:

* no `addLink` yet
* no `updateLink`
* no `removeLink`
* no `lookupByNode`
* no `lookupByCapability`
* no API integration
* no `getNodeCapabilities(...)` mapping behavior
* no `CapabilityValueStore`
* no `DiagnosticsStore`
* no provider bridge
* no discovery bridge
* no transport
* no live app data
* no production mapping creation path

These limitations are intentional until controlled mapping creation and API integration are approved.

## 11. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.54 - NodeCapabilityMap Controlled Add Link Plan
```

Purpose:

Plan the first bounded, test-only, owner-backed `addLink` path that links an approved public node record to an approved public capability record without discovery, Registry, provider bridge, transport, or fake runtime behavior.

Alternative:

```text
Milestone 10.54 - NodeCapabilityMap Ownership API Attachment Plan
```

## 12. Stop Conditions

Stop future work if it tries to:

* connect `NodeCapabilityMap` to API before approved mapping integration
* change `getNodeCapabilities(...)` behavior before `addLink` exists
* add fake mappings
* infer capabilities from nodes
* infer nodes from capabilities
* auto-seed production mappings
* parse packets
* read Registry arrays
* call HAL, Drivers, Devices, Services, Logic, or Runtime
* add discovery/provider bridge before approval
* add transport before approval
* modify `src/main.cpp`
* expose mutable mapping access to UI/App/Transport
* add values/diagnostics without approved stores
