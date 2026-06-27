# Milestone 10.43 - NodeDirectory Controlled Add Path Plan

## 1. Scope

Milestone 10.43 plans the first bounded, test-only, owner-backed add path for public node records.

This milestone is documentation only.

It does not:

* modify firmware source code
* modify `src/main.cpp`
* implement `addNode`
* add real node records in code
* add fake nodes
* add discovery, pairing, trust, Registry, provider, or transport bridges
* parse packets
* call HAL, Drivers, Devices, Modules, Services, Logic, or Runtime
* add provider selection
* add command execution
* add WebServer, HTTP, JSON, BLE, WiFi, or live Minimal App transport
* expose private Registry arrays
* change existing API behavior

The goal is to define a safe creation contract before any public node record can be added.

## 2. Background

Current public owner state:

* `PublicOwnerStore` exists.
* `PublicOwnerStore` owns `NodeDirectory` and `CapabilityDirectory`.
* `Cyber32Api` reads `NodeDirectory` through `PublicOwnerStore.nodes()`.
* `NodeDirectory` is empty by default.
* Node API currently returns empty list / `node_not_found`.
* no real public node records exist yet.

The next step must be controlled and test-only. It must prove owner-backed node creation behavior without introducing live discovery, Registry integration, packet parsing, transport, or production bootstrap changes.

## 3. Why Controlled Add Path Is Needed

Real public node records require owner-backed creation.

API reads must not create records.

UI/App/Transport must not create records directly.

Discovery bridge is not approved yet.

Registry bridge is not approved yet.

A test-only controlled add path lets validation prove bounded creation, duplicate handling, full-store behavior, reset behavior, and API read behavior safely before live data sources exist.

## 4. Add Path Principle

`NodeDirectory` may eventually accept records only through an approved bounded method.

The first add path must be:

* explicit
* bounded
* deterministic
* test-only
* no hardware access
* no packet parsing
* no Registry access
* no transport
* no fake runtime discovery
* no `src/main.cpp` change

No node should become public unless an approved owner-backed method accepts a valid public node record.

## 5. Future Operation Name / Options

Possible future method names:

* `addNode(...)`
* `addOrUpdateNode(...)`
* `insertRecord(...)`
* `upsertRecord(...)`
* `addPublicNode(...)`

Recommended first name:

```cpp
addNode(const PublicNodeRecord& record)
```

Reason:

* simple
* no implicit update behavior
* easier to test duplicate/full-store failure
* avoids hidden merge policy
* keeps add/update separation clear

Avoid `addOrUpdateNode(...)` and `upsertRecord(...)` for the first implementation because they imply overwrite or merge policy that has not been approved.

## 6. Record Validity Requirements

Future `addNode(...)` should reject records that are not explicitly valid.

Planned checks:

* record must be explicitly valid.
* `node_id` must be nonzero or valid according to the public node contract.
* lifecycle state must not be unknown/none if the record is intended active.
* visibility state must be public/visible/known according to the existing enum model.
* trust state may remain unknown unless a later trust/pairing milestone requires otherwise.
* no default all-zero record is accepted as real.
* no fake placeholder is accepted.
* no automatic hardware/provider inference is performed.

The first add path should treat invalid or incomplete records as deterministic failures.

## 7. Capacity And Failure Behavior

Planned behavior:

* fixed capacity only.
* no heap allocation.
* if full, add fails deterministically.
* failed add does not mutate existing records.
* duplicate `node_id` behavior must be defined.
* invalid record fails deterministically.
* returned status should be compact and firmware-safe.

Return type options:

* `bool`
* compact enum/result type

Recommended safe first option:

```cpp
bool addNode(const PublicNodeRecord& record)
```

Reason:

* smallest first implementation
* enough to validate success/failure paths
* avoids introducing a result enum before failure categories stabilize

If diagnostics need more detail later, a follow-up milestone can introduce a compact `NodeDirectoryAddResult` enum without changing the initial safety rules.

## 8. Duplicate Behavior

First duplicate policy:

* duplicate `node_id` should fail in the first implementation.
* no silent overwrite.
* no merge.
* no update path yet.
* add/update separation prevents accidental state mutation.

An update path can be planned later after public node lifecycle, trust, freshness, and diagnostics rules are clearer.

## 9. Test-Only Access Path

Tests may add records safely only through approved validation context.

Planned test access:

* use `PublicOwnerStore.mutableNodes()` only in validation/test context.
* API read methods must not use `mutableNodes()`.
* UI/App/Transport must not access `mutableNodes()`.
* no production discovery path yet.
* no `src/main.cpp` integration.

The mutable accessor remains an internal bridge seam for approved Core milestones, not a UI/App/Transport contract.

## 10. API Behavior After First Test Add

Once a test-controlled node exists in a validation scenario:

* `getNodeList(...)` may return count `1` only where the store was explicitly seeded by validation.
* node detail methods may return owner-backed mapped fields.
* `getNodeCapabilities(...)` should still return count `0` or `node_not_found` according to `NodeCapabilityMap` availability rules.
* no capability inference from node alone.
* no diagnostics/value data unless stores exist.
* production default remains empty.

The first controlled add path does not imply live node discovery, app-visible production data, or automatic capability creation.

## 11. Required Future Mapping From PublicNodeRecord To API Responses

Future API mapping should follow these rules:

* `getNodeSummary(...)` maps only available public fields.
* `getNodeIdentity(...)` maps only identity fields.
* `getNodeStatus(...)` maps lifecycle/visibility/trust/availability fields if present.
* `getNodePower(...)` remains unavailable unless public power/diagnostic owner exists.
* `getNodeSignal(...)` remains unavailable unless diagnostics/signal owner exists.
* `getNodeDiagnostics(...)` remains unavailable unless `DiagnosticsStore` exists.
* no hardware reads.

No API method should infer missing fields from hardware, packets, Registry arrays, providers, logs, or debug output.

## 12. Validation Plan For Implementation Milestone

Future implementation validation should cover:

* default `NodeDirectory` count `0`.
* add valid test node succeeds.
* count becomes `1`.
* `readByIndex(0)` succeeds.
* read returns same owner-backed node ID.
* invalid default record add fails.
* duplicate `node_id` add fails.
* full capacity failure is deterministic.
* failed add does not mutate count.
* reset clears added test node.
* `PublicOwnerStore.mutableNodes()` can seed a test node.
* `Cyber32Api` reads seeded owner-backed node only in controlled validation path if implementation milestone approves API-seeded validation.
* no fake nodes.
* no Registry/HAL/packet/transport calls.
* `src/main.cpp` untouched.

The validation must prove production default remains empty.

## 13. Production Default Behavior

Production default behavior remains:

* default production owner store remains empty.
* no records are created automatically.
* no setup/loop integration.
* no transport seeding.
* no discovery seeding.
* no Registry seeding.
* Minimal App still sees empty list unless a future approved live source exists.

Test-only records must not become production bootstrap behavior.

## 14. Minimal App Implications

Minimal App implications:

* no app change required.
* app remains mock-only.
* test-only node records are not live app records.
* app must not infer real hardware from test records.
* future app live path still requires approved transport and owner-backed real source.

The app must continue to use `Cyber32Api` only and must not access `PublicOwnerStore` or `NodeDirectory` directly.

## 15. Safety Invariants

Safety invariants:

* no fake nodes
* no fake capabilities
* no fake sensor values
* no fake diagnostics
* no private Registry arrays exposed
* no packet parsing
* no HAL/Driver/Device/Service/Logic/Runtime calls
* no provider selection
* no command execution
* no `src/main.cpp` changes
* no app transport
* no WebServer/HTTP/JSON/BLE/WiFi

The controlled add path must create only explicit owner-backed test records during approved validation.

## 16. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.44 - NodeDirectory Controlled Add Path Implementation
```

Purpose:

Implement a bounded test-only `addNode` path for `NodeDirectory` and validation coverage without changing default production API behavior.

Alternative:

```text
Milestone 10.44 - NodeDirectory Add Result Type Plan
```

## 17. Stop Conditions

Stop future work if it tries to:

* create fake nodes
* auto-seed production nodes
* parse packets
* read Registry arrays
* call HAL/Drivers/Devices/Services/Logic/Runtime
* add discovery bridge before approval
* add transport before approval
* modify `src/main.cpp`
* expose mutable owner access to UI/App/Transport
* infer capabilities from node records
* add diagnostics/value data without approved stores
