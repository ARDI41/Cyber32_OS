# Milestone 10.40 - PublicOwnerStore API Migration Plan

## 1. Scope

Milestone 10.40 plans the future API migration from provisional direct API static `NodeDirectory` and `CapabilityDirectory` owners to `PublicOwnerStore` read-only accessors.

This milestone is documentation only.

It does not:

* modify firmware source code
* modify `src/main.cpp`
* migrate `Cyber32Api`
* move `NodeDirectory` in code
* move `CapabilityDirectory` in code
* add real node records
* add real capability records
* add node or capability creation
* add discovery, provider, Registry, node-to-capability, value, or diagnostics bridges
* parse packets
* call HAL, Drivers, Devices, Modules, Services, Logic, or Runtime
* add provider selection
* add command execution
* add WebServer, HTTP, JSON, BLE, WiFi, or live Minimal App transport
* expose private Registry arrays
* add fake nodes, fake capabilities, fake `CAP_TEMPERATURE`, fake sensor values, or fake diagnostics
* change existing API behavior

The purpose is to define the behavior-preserving migration path before code changes are made.

## 2. Background

Current state:

* `NodeDirectory` exists.
* `CapabilityDirectory` exists.
* Node API currently reads from a provisional API static `NodeDirectory`.
* Capability API currently reads from a provisional API static `CapabilityDirectory`.
* `PublicOwnerStore` now exists as an empty skeleton and owns `NodeDirectory` plus `CapabilityDirectory`.
* `PublicOwnerStore` is not connected to `Cyber32Api` yet.
* external Node API behavior remains empty/not-found.
* external Capability API behavior remains empty/not-found.

The next migration should remove duplicate empty owner paths from API reads without changing what callers see.

## 3. Migration Principle

Migration must be behavior-preserving.

That means:

* Node list remains empty success.
* Node detail methods remain `node_not_found`.
* Capability list remains empty success.
* Capability detail methods remain `capability_not_found`.
* no fake nodes appear.
* no fake capabilities appear.
* no fake `CAP_TEMPERATURE` appears.
* no fake sensor values appear.
* no app-visible behavior changes.

The API must continue to move only from empty to owner-backed empty, not from empty to fake.

## 4. Current Ownership Before Migration

Before migration:

* direct API static `NodeDirectory` exists in `cyber32_api.cpp`.
* direct API static `CapabilityDirectory` exists in `cyber32_api.cpp`.
* `PublicOwnerStore` exists separately.
* duplicate empty stores are acceptable only temporarily.
* real records must not be added while duplicate stores exist.

The duplicate empty stores are a transitional implementation detail. They are acceptable because both remain empty and expose no real or fake data.

## 5. Target Ownership After Migration

After migration, the target is:

* one provisional API static `PublicOwnerStore`.
* Node API reads through `publicOwnerStore().nodes()`.
* Capability API reads through `publicOwnerStore().capabilities()`.
* direct API static `NodeDirectory` is removed or no longer used.
* direct API static `CapabilityDirectory` is removed or no longer used.
* `PublicOwnerStore` remains empty.
* `Cyber32Api` remains read-only.

This target does not introduce real records, creation paths, mapping, values, diagnostics, or transport.

## 6. Node API Migration Plan

Future Node API migration should:

* replace `apiNodeDirectory()` usage with `apiPublicOwnerStore().nodes()`.
* keep failed read mapping to `node_not_found`.
* keep `getNodeList(...)` count `0`.
* keep `getNodeCapabilities(...)` `out_count` initialization behavior.
* create no records.
* avoid `mutableNodes()` in API read methods.

Affected methods:

* `getNodeList`
* `getNodeSummary`
* `getNodeIdentity`
* `getNodeStatus`
* `getNodePower`
* `getNodeSignal`
* `getNodeDiagnostics`
* `getNodeCapabilities`

Node API reads must stay read-only and must not infer nodes from packets, MAC addresses, providers, Registry internals, hardware, diagnostics, or debug output.

## 7. Capability API Migration Plan

Future Capability API migration should:

* replace `apiCapabilityDirectory()` usage with `apiPublicOwnerStore().capabilities()`.
* keep failed read mapping to `capability_not_found`.
* keep `getCapabilityList(...)` count `0`.
* keep value/availability/provider/quality behavior as `capability_not_found` while no capability exists.
* create no records.
* avoid `mutableCapabilities()` in API read methods.

Affected methods:

* `getCapabilityList`
* `getCapabilitySummary`
* `getCapabilityIdentity`
* `getCapabilityValue`
* `getCapabilityAvailability`
* `getCapabilityProviderInfo`
* `getCapabilityQuality`

Capability API reads must stay read-only and must not infer capabilities from packets, providers, Registry internals, hardware, diagnostics, or debug output.

## 8. PublicOwnerStore Accessor Plan

Plan a safe internal API accessor:

```cpp
static PublicOwnerStore& apiPublicOwnerStore();
```

or a project-style equivalent.

Rules:

* accessor returns a deterministic static store.
* no heap allocation.
* no hidden records.
* no reset called on every API read.
* no app/transport initialization.
* no `src/main.cpp` change.

The accessor is an internal API implementation detail until a later Core context or bootstrap owner is explicitly approved.

## 9. Removal / Deprecation Plan

Future migration should:

* remove or stop using direct `apiNodeDirectory()` after migration.
* remove or stop using direct `apiCapabilityDirectory()` after migration.
* avoid duplicate owner reads after migration.
* keep direct owners only if temporarily retained as dead code during a narrow implementation step, then remove them before adding real records.
* introduce no real records until duplicate static owners are gone or formally isolated.

After migration, all Node and Capability API empty owner reads should flow through `PublicOwnerStore`.

## 10. Validation Plan For Migration Implementation

Future validation should prove:

* `getNodeList(...)` still returns count `0`.
* node details still return `node_not_found`.
* `getNodeCapabilities(...)` still initializes `out_count = 0` and returns `node_not_found` while no node exists.
* `getCapabilityList(...)` still returns count `0`.
* capability details still return `capability_not_found`.
* no fake node appears.
* no fake capability appears.
* no fake `CAP_TEMPERATURE` appears.
* `PublicOwnerStore` empty skeleton tests still pass.
* `NodeDirectory` tests still pass.
* `CapabilityDirectory` tests still pass.
* repeated API reads are deterministic.
* full validation passes.

Validation must confirm the API migration is an internal empty-owner source change only.

## 11. Minimal App Implications

Minimal App implications:

* no app change is required.
* app remains mock-only.
* future live app still sees empty lists after migration.
* no transport exists yet.
* app must not access `PublicOwnerStore` directly.

The app continues to use `Cyber32Api` only. It must not infer records from owner stores, packets, logs, MAC addresses, Registry internals, providers, or debug output.

## 12. Safety Invariants

Safety invariants:

* no fake nodes.
* no fake capabilities.
* no fake `CAP_TEMPERATURE`.
* no fake sensor values.
* no fake diagnostics.
* no private Registry arrays exposed.
* no packet parsing.
* no HAL/Driver/Device/Module/Service/Logic/Runtime calls from API reads.
* no provider selection.
* no command execution.
* no `src/main.cpp` changes.
* no app transport.
* no WebServer/HTTP/JSON/BLE/WiFi.

API read methods must use read-only owner access and must not use mutable public owner access to create state.

## 13. Known Limitations After Migration

After the future migration:

* `PublicOwnerStore` is still a provisional API static.
* no Core context exists yet.
* no real records exist.
* no creation paths exist.
* no mapping/value/diagnostics stores exist.
* no live transport exists.
* API remains read-only and empty.

The migration improves ownership consolidation but does not introduce real data.

## 14. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.41 - PublicOwnerStore API Empty Migration Implementation
```

Purpose:

Migrate `Cyber32Api` Node and Capability empty read paths to `PublicOwnerStore` read-only accessors while preserving all current external API behavior.

## 15. Stop Conditions

Stop future work if it tries to:

* add real records during migration
* use mutable accessors in API read methods
* create fake nodes
* create fake capabilities
* create fake `CAP_TEMPERATURE`
* parse packets
* read Registry arrays
* call HAL/Drivers/Devices/Services/Logic/Runtime from API reads
* trigger provider selection
* modify `src/main.cpp`
* add live transport
* connect Minimal App directly to `PublicOwnerStore`
* add WebServer/HTTP/JSON/BLE/WiFi before transport approval
