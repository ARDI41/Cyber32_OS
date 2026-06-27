# Milestone 10.54 - NodeCapabilityMap Controlled Add Link Plan

## 1. Scope

Milestone 10.54 plans the first bounded `NodeCapabilityMap` `addLink` path only.

This milestone is documentation only.

It does not:

* modify firmware source code
* modify `src/main.cpp`
* implement `addLink`
* connect `NodeCapabilityMap` to `Cyber32Api`
* change `getNodeCapabilities(...)` behavior
* add real mapping records in code
* add fake mappings
* infer capabilities from nodes
* infer nodes from capabilities
* add `updateLink`
* add `removeLink`
* add `lookupByNode`
* add `lookupByCapability`
* add discovery, provider, Registry, value, diagnostics, or transport bridges
* parse packets
* call HAL, Drivers, Devices, Modules, Services, Logic, or Runtime
* add provider selection
* add command execution
* add WebServer, HTTP, JSON, BLE, WiFi, or Minimal App transport
* expose private Registry arrays
* change existing API behavior

The purpose is to plan controlled owner-backed relationship creation between approved public node records and approved public capability records before any API mapping integration.

## 2. Background

Current public owner state:

* `NodeDirectory` exists and has a controlled `addNode` path.
* `CapabilityDirectory` exists and has a controlled `addCapability` path.
* `NodeCapabilityMap` empty skeleton exists.
* `PublicOwnerStore` owns `NodeCapabilityMap`.
* `NodeCapabilityMap` is not connected to `Cyber32Api`.
* `getNodeCapabilities(...)` behavior remains unchanged.
* no mapping links exist yet.

The current empty state is intentional. Public nodes and public capabilities are separate owner-backed records, but no approved owner-backed relationship exists between them yet.

## 3. Why AddLink Is Needed

Public nodes and public capabilities are separate records.

Future `getNodeCapabilities(...)` must rely on explicit mapping links instead of inference.

Mappings must not be inferred from:

* node records alone
* capability records alone
* `PublicNodeRecord.capability_count`
* `PublicCapabilityRecord.owner_node_id`
* Registry state
* packets
* providers
* debug output
* `CAP_*` IDs such as `CAP_TEMPERATURE`

A controlled `addLink` path lets validation prove node-capability relationship behavior safely before API integration.

## 4. Add Link Principle

Future `NodeCapabilityMap::addLink` must be:

* explicit
* bounded
* deterministic
* owner-backed
* test-only at first
* no heap allocation
* no STL
* no Arduino `String`
* no hardware access
* no packet parsing
* no Registry access
* no provider selection
* no value creation
* no diagnostics creation
* no transport
* no fake runtime discovery
* no `src/main.cpp` change

Adding a link must mean only that an approved public relationship record exists. It must not imply a live value, healthy provider, radio signal, diagnostics record, or command permission.

## 5. Future Operation Name Options

Possible future method names:

* `addLink(...)`
* `addNodeCapabilityLink(...)`
* `addMapping(...)`
* `addOrUpdateLink(...)`
* `upsertLink(...)`

Recommended first method:

```cpp
bool addLink(const PublicNodeCapabilityLink& link);
```

Reason:

* simple
* mirrors `addNode` / `addCapability` style
* has no implicit update behavior
* avoids hidden merge policy
* makes duplicate and full-capacity failure testing easier

Avoid `addOrUpdateLink(...)` and `upsertLink(...)` for the first step because they imply overwrite or merge behavior before update policy is approved.

## 6. Link Validity Requirements

Future implementation should validate link structure using the exact fields in `include/cyber32/public/public_owner_types.h`.

Planned validity requirements:

* link must be explicitly valid according to `PublicNodeCapabilityLink` fields
* link ID must be valid and nonzero if the field exists
* node ID must be valid and nonzero
* capability instance ID must be valid and nonzero
* default all-zero link must fail
* placeholder links must fail
* fake mappings must fail
* link must not be created from node `capability_count`
* link must not be created from capability `owner_node_id` alone
* link must not be created from `CAP_TEMPERATURE`
* link must not imply sensor value exists
* link must not imply diagnostics or provider quality exists

If exact field names differ at implementation time, the implementation milestone must follow `public_owner_types.h`.

## 7. Existence Validation Plan

The key design question is whether `addLink` validates referenced node and capability existence.

Recommended staged approach:

* First implementation may validate link structure inside `NodeCapabilityMap` only if `NodeCapabilityMap` has no access to directories.
* `PublicOwnerStore`-level controlled link creation may later validate:
  * node exists in `NodeDirectory`
  * capability exists in `CapabilityDirectory`
  * link is not duplicate
* `NodeCapabilityMap` must not read Registry or providers to validate existence.
* `NodeCapabilityMap` must not call `NodeDirectory` or `CapabilityDirectory` unless explicitly approved by design.
* Cross-store validation should remain in `PublicOwnerStore` or another approved higher owner layer if needed.

Option A: `NodeCapabilityMap` validates link fields only.

Pros:

* smallest first implementation
* keeps `NodeCapabilityMap` independent
* avoids circular owner dependencies
* easy to validate in isolation

Cons:

* cannot prove referenced public node or capability exists
* requires a later owner-level validation path before real API exposure

Option B: `PublicOwnerStore` validates existence before mutating `NodeCapabilityMap`.

Pros:

* keeps cross-store relationship checks at the owner boundary
* can prove links only reference approved public records
* better fit before API integration

Cons:

* requires additional owner-store behavior
* should be planned separately from first map-local `addLink`

Safest architecture direction:

* `NodeCapabilityMap` owns link storage.
* `PublicOwnerStore` or an approved owner bridge validates cross-store existence.

## 8. Capacity And Failure Behavior

Planned behavior:

* fixed capacity only
* no heap allocation
* if full, add fails deterministically
* failed add does not mutate count
* failed add does not overwrite existing links
* invalid link fails deterministically
* duplicate link fails deterministically
* returned status should be compact and firmware-safe

Bool versus enum:

* `bool` is simplest and matches the first bounded operation style.
* enum result codes give better diagnostics but add contract surface.

Recommended first step:

```cpp
bool addLink(const PublicNodeCapabilityLink& link);
```

Use `bool` unless an existing public-owner pattern requires a compact enum result. A later milestone can introduce richer result codes only if validation or API diagnostics require them.

## 9. Duplicate Behavior

First duplicate policy:

* duplicate exact node ID plus capability instance ID should fail
* duplicate link ID should fail if link ID exists
* no overwrite
* no merge
* no update path
* one node may have many capabilities later
* one capability should normally belong to one node unless multi-owner behavior is explicitly planned

Duplicate rejection must not mutate existing links or count.

## 10. Test-Only Access Path

Future implementation should seed links only through validation or approved bridge context.

Planned first test path:

```cpp
PublicOwnerStore store;
store.mutableNodeCapabilities().addLink(link);
```

Rules:

* use `PublicOwnerStore::mutableNodeCapabilities().addLink(...)` only in validation/test context after implementation
* API read methods must not use `mutableNodeCapabilities()`
* UI, App, and Transport must not access `mutableNodeCapabilities()`
* no production seed path
* no `src/main.cpp` integration

## 11. API Behavior After First Test Link

After the first `addLink` implementation:

* `getNodeCapabilities(...)` should remain unchanged unless a separate API integration milestone approves it.
* test links are storage validation only.
* no Minimal App live mapping appears yet.
* no API method should expose mapping until a mapping API attachment plan and implementation are approved.
* future API integration should require seeded node, seeded capability, and seeded link validation.

This preserves the separation between owner-backed storage validation and UI-visible public API behavior.

## 12. Validation Plan For Implementation Milestone

Future implementation validation should prove:

* default `NodeCapabilityMap` count is `0`
* add valid validation-only link succeeds
* count becomes `1`
* `readByIndex(0)` succeeds
* read returns same owner-backed node ID and capability instance ID
* invalid/default link add fails
* duplicate exact node-capability link fails
* duplicate link ID fails if link ID exists
* full capacity failure is deterministic
* failed add does not mutate count
* reset clears added test links
* `PublicOwnerStore::mutableNodeCapabilities()` can seed a test link
* `PublicOwnerStore::reset()` clears seeded link
* no fake mappings
* no inferred mappings
* no fake nodes
* no fake capabilities
* no fake `CAP_TEMPERATURE`
* no fake sensor values
* no diagnostics
* no Registry, HAL, packet, provider, or transport calls
* `src/main.cpp` untouched
* existing `NodeDirectory` tests still pass
* existing `CapabilityDirectory` tests still pass
* existing `NodeCapabilityMap` empty skeleton tests still pass
* existing `PublicOwnerStore` ownership tests still pass
* existing API default tests still pass

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
* no value seeding
* no diagnostics seeding
* Minimal App still sees no live mapped node capabilities

The safe transition remains:

```text
empty -> owner-backed empty -> owner-backed test-controlled link -> future owner-backed real
```

Never:

```text
empty -> fake
```

## 14. Minimal App Implications

No Minimal App change is required.

Implications:

* app remains mock-only
* test-only links are not live app mappings
* app must not infer node capabilities from separate node and capability lists
* future live node capability display requires an approved `NodeCapabilityMap`-backed API path
* app continues to use `Cyber32Api` only

The app must not access `PublicOwnerStore`, `NodeCapabilityMap`, mutable owner methods, debug packets, packet source MACs, Registry assumptions, or provider internals.

## 15. Safety Invariants

Safety invariants:

* no fake mappings
* no inferred mappings
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
Milestone 10.55 - NodeCapabilityMap Controlled Add Link Implementation
```

Purpose:

Implement a bounded, test-only `addLink` path for `NodeCapabilityMap` and validation coverage without changing API behavior.

Alternative:

```text
Milestone 10.55 - PublicOwnerStore Validated Add Link Plan
```

## 17. Stop Conditions

Stop future work if it tries to:

* connect `NodeCapabilityMap` to API before approved mapping integration
* change `getNodeCapabilities(...)` behavior before approved mapping API attachment
* create fake mappings
* infer capabilities from nodes
* infer nodes from capabilities
* infer mapping from `capability_count`
* infer mapping from `owner_node_id`
* infer mapping from `CAP_TEMPERATURE`
* auto-seed production mappings
* parse packets
* read Registry arrays
* call HAL, Drivers, Devices, Services, Logic, or Runtime
* add discovery/provider bridge before approval
* add transport before approval
* modify `src/main.cpp`
* expose mutable mapping access to UI/App/Transport
* add values/diagnostics without approved stores
