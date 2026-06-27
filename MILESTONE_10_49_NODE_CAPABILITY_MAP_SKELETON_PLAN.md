# Milestone 10.49 - NodeCapabilityMap Skeleton Plan

## 1. Scope

Milestone 10.49 plans the first bounded `NodeCapabilityMap` skeleton only.

This milestone is documentation only.

It does not:

* modify firmware source code
* modify `src/main.cpp`
* add headers
* implement `NodeCapabilityMap`
* connect `NodeCapabilityMap` to `Cyber32Api`
* add real mapping records in code
* add fake mappings
* infer capabilities from nodes
* infer nodes from capabilities
* add discovery, provider, Registry, value, diagnostics, or transport bridges
* parse packets
* call HAL, Drivers, Devices, Modules, Services, Logic, or Runtime
* add provider selection
* add command execution
* add WebServer, HTTP, JSON, BLE, WiFi, or Minimal App transport
* expose private Registry arrays
* change existing API behavior

The goal is to plan a safe bounded owner-backed mapping store between public node records and public capability records.

## 2. Background

Current public owner state:

* `PublicOwnerStore` exists.
* `NodeDirectory` has a controlled test-only add path.
* `CapabilityDirectory` has a controlled test-only add path.
* `Cyber32Api` reads nodes and capabilities through `PublicOwnerStore`.
* `getNodeCapabilities(...)` currently remains empty, `node_not_found`, or default according to current behavior.
* no mapping owner exists yet.
* capabilities must not be inferred from nodes alone.
* nodes must not be inferred from capabilities alone.

The current state is intentionally conservative. Public nodes and public capabilities are separate owner-backed records, but there is no approved owner-backed relationship between them yet.

## 3. NodeCapabilityMap Purpose

`NodeCapabilityMap` is the future bounded owner of public node-to-capability links.

It should eventually:

* link a `PublicNodeId` to a `PublicCapabilityInstanceId`
* expose count
* expose read by index
* expose lookup by node later
* expose lookup by capability later
* enforce capacity
* preserve deterministic empty state
* remain owner-backed only

It is not:

* a Registry wrapper
* a packet parser
* a provider selector
* a value store
* a diagnostics store
* a discovery engine
* a transport endpoint
* a UI model

Mapping answers only:

```text
Which approved public capability records are linked to this approved public node record?
```

It does not prove that a capability has a fresh value, healthy provider, valid diagnostic state, or command permission.

## 4. First Skeleton Behavior

The first skeleton should behave as follows:

* default constructed empty
* `count()` returns `0`
* `isEmpty()` returns `true` if included
* `readByIndex(...)` fails when empty
* no fake links
* no inferred links
* no node validation against live hardware
* no capability validation against providers
* no Registry access
* no packet parsing
* no HAL, Driver, Device, Service, Logic, or Runtime access
* no `Cyber32Api` behavior change yet

The first skeleton should prove that a mapping owner can exist safely while exposing no links by default.

## 5. Future Files Plan

Possible future files:

* `include/cyber32/public/node_capability_map.h`
* `src/public/node_capability_map.cpp`

Do not create these files in Milestone 10.49.

## 6. Public Types Dependency

The future implementation should reuse existing public owner types from `include/cyber32/public/public_owner_types.h`.

Relevant existing types include:

* `PublicNodeId`
* `PublicCapabilityInstanceId`
* `PublicMappingLinkId`
* `PublicNodeCapabilityLink`
* `PublicNodeIndex` if needed
* `PublicCapabilityIndex` if needed

Do not invent incompatible temporary structs.

The link record should stay aligned with `PublicNodeCapabilityLink` unless a later explicit type revision milestone changes the public owner type contract.

## 7. Initial Operation Set

Recommended first skeleton operations:

```cpp
void reset();
uint8_t count() const;
uint8_t capacity() const;
bool isEmpty() const;
bool readByIndex(uint8_t index, PublicNodeCapabilityLink& out_link) const;
```

If existing type names or index conventions differ at implementation time, the implementation milestone should follow `public_owner_types.h` and existing public directory style.

Operations intentionally deferred:

* `addLink`
* `updateLink`
* `removeLink`
* `lookupByNode`
* `lookupByCapability`
* discovery import
* Registry import
* provider import
* API integration
* value attachment
* diagnostics attachment

The first skeleton is an empty owner and read boundary, not a relationship creation path.

## 8. Bounds and Capacity Plan

Capacity plan:

* fixed capacity only
* no heap allocation
* no STL containers
* no Arduino `String`
* deterministic failure when full later
* bounded copy behavior
* capacity should be compile-time deterministic

Do not choose final numbers in this plan unless an existing public-owner constant already defines them at implementation time.

The future skeleton should follow the existing public directory pattern: bounded storage, deterministic reset, deterministic failed reads, and no dynamic behavior.

## 9. Relationship Rules

Relationship rules:

* a mapping link references an existing public node ID and capability instance ID conceptually.
* the first skeleton does not validate existence because no `addLink` path exists yet.
* future `addLink` should validate node and capability existence through `PublicOwnerStore` or approved owner access.
* duplicate links should fail later.
* one node may have many capabilities later.
* one capability should normally belong to one public node unless multi-owner behavior is explicitly planned.
* no capability should appear in `getNodeCapabilities(...)` without an approved mapping link.

Important non-inference rules:

* `PublicNodeRecord.capability_count` must not be treated as proof that capability records exist.
* `PublicCapabilityRecord.owner_node_id` must not by itself cause `getNodeCapabilities(...)` to expose a capability before approved mapping integration.
* capability ID such as `CAP_TEMPERATURE` must not be used to fabricate a node relationship.
* packet source MAC, provider ID, Registry state, or debug serial output must not create a mapping.

## 10. API Integration Plan

`NodeCapabilityMap` should not be connected to `Cyber32Api` in the first skeleton.

`getNodeCapabilities(...)` should not change in the first skeleton.

Future API integration should happen only after:

A. skeleton implementation

B. controlled `addLink` plan

C. controlled `addLink` implementation

D. validation with seeded node, seeded capability, and seeded link

Future API integration rules:

* no inference from `NodeDirectory` alone
* no inference from `CapabilityDirectory` alone
* no packet parsing
* no Registry reads
* no provider selection
* no Services calls
* no HAL, Driver, Device, Module, Logic, or Runtime calls
* no fake capability summaries

## 11. PublicOwnerStore Integration Plan

Future `PublicOwnerStore` should own `NodeCapabilityMap`.

Future reset behavior:

* `PublicOwnerStore::reset()` should clear `NodeCapabilityMap`.
* reset should not create links.
* reset should not parse packets.
* reset should not import Registry/provider/discovery state.

Future read-only accessor:

```cpp
const NodeCapabilityMap& nodeCapabilities() const;
```

Future mutable accessor only for approved validation or bridge milestones:

```cpp
NodeCapabilityMap& mutableNodeCapabilities();
```

UI, App, Transport, WebServer, HTTP, JSON, BLE, and WiFi layers must not access mutable mapping owners.

## 12. Validation Plan For Implementation Milestone

Future implementation validation should prove:

* default map count is `0`
* reset count is `0`
* `isEmpty()` is true by default if included
* capacity is deterministic and greater than `0`
* read index `0` fails when empty
* read out-of-range fails
* failed read resets/defaults `PublicNodeCapabilityLink`
* repeated failed reads do not mutate count
* no fake links appear
* no inferred node-capability links appear
* `PublicOwnerStore` reset should eventually clear the map when integrated
* existing `NodeDirectory` tests still pass
* existing `CapabilityDirectory` tests still pass
* existing API default tests still pass
* `src/main.cpp` remains untouched

Validation should use public owner APIs and deterministic fixed objects only.

## 13. Production Default Behavior

Production default remains no mapping links.

Production default behavior:

* no node gains capabilities automatically
* no capability attaches to a node automatically
* no setup/loop integration
* no transport seeding
* no discovery/provider seeding
* no Registry seeding
* no fake mappings
* Minimal App still sees no live mapped node capabilities unless a future approved live source exists

The safe transition remains:

```text
empty -> owner-backed empty -> owner-backed test-controlled link -> future owner-backed real
```

Never:

```text
empty -> fake
```

## 14. Minimal App Implications

Minimal App implications:

* no app change is required.
* app remains mock-only.
* app must not infer node capabilities from separate node and capability lists.
* future live node capability display requires an approved `NodeCapabilityMap`-backed API path.
* app continues to use `Cyber32Api` only.
* app must not access `PublicOwnerStore` directly.
* app must not access mutable mapping owners.
* app must not parse packets, debug logs, MAC addresses, provider IDs, or Registry assumptions to build node capability groups.

Until mapping integration is approved, an empty node capability display is correct.

## 15. Safety Invariants

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

These invariants apply to the map itself, future API reads, Minimal App behavior, and any future bridge that writes public owner state.

## 16. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.50 - NodeCapabilityMap Empty Skeleton Implementation
```

Purpose:

Implement an empty-by-default bounded `NodeCapabilityMap` skeleton and validation coverage without changing API behavior.

Alternative:

```text
Milestone 10.50 - NodeCapabilityMap Controlled Add Link Plan
```

## 17. Stop Conditions

Stop future work if it tries to:

* infer capabilities from node records
* infer nodes from capability records
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
* change existing API behavior during the skeleton phase
* connect Minimal App directly to owner stores
