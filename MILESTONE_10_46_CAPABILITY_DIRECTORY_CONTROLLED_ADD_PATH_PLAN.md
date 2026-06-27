# Milestone 10.46 - CapabilityDirectory Controlled Add Path Plan

## 1. Scope

Milestone 10.46 plans the first bounded, test-only, owner-backed add path for public capability records.

This milestone is documentation only.

It does not:

* modify firmware source code
* modify `src/main.cpp`
* implement `addCapability`
* add real capability records in code
* add fake capabilities
* add fake `CAP_TEMPERATURE`
* add fake sensor values
* add fake diagnostics
* add discovery, provider, Registry, node-to-capability, value, or diagnostics bridges
* parse packets
* call HAL, Drivers, Devices, Modules, Services, Logic, or Runtime
* add provider selection
* add command execution
* add WebServer, HTTP, JSON, BLE, WiFi, or live Minimal App transport
* expose private Registry arrays
* change existing API behavior

The goal is to define a safe creation contract before any public capability record can be added.

## 2. Background

Current public owner state:

* `PublicOwnerStore` exists.
* `PublicOwnerStore` owns `NodeDirectory` and `CapabilityDirectory`.
* `Cyber32Api` reads `CapabilityDirectory` through `PublicOwnerStore.capabilities()`.
* `CapabilityDirectory` is empty by default.
* Capability API currently returns empty list / `capability_not_found`.
* no real public capability records exist yet.
* `NodeDirectory` now has a controlled test-only add path.

The next capability step must be controlled and test-only. It must prove owner-backed capability creation behavior without introducing live discovery, provider bridge, Registry integration, value storage, diagnostics storage, packet parsing, transport, or production bootstrap changes.

## 3. Why Controlled Add Path Is Needed

Real public capabilities require owner-backed creation.

API reads must not create capabilities.

UI/App/Transport must not create capabilities directly.

Provider bridge is not approved yet.

Registry bridge is not approved yet.

Discovery bridge is not approved yet.

`CapabilityValueStore` does not exist yet.

`DiagnosticsStore` does not exist yet.

A test-only controlled add path lets validation prove bounded creation, duplicate handling, full-store behavior, reset behavior, and API read behavior safely before live data sources exist.

## 4. Add Path Principle

`CapabilityDirectory` may eventually accept records only through an approved bounded method.

The first add path must be:

* explicit
* bounded
* deterministic
* test-only
* no hardware access
* no packet parsing
* no Registry access
* no provider selection
* no value creation
* no diagnostics creation
* no transport
* no fake runtime discovery
* no `src/main.cpp` change

No capability should become public unless an approved owner-backed method accepts a valid public capability record.

## 5. Future Operation Name / Options

Possible future method names:

* `addCapability(...)`
* `addOrUpdateCapability(...)`
* `insertRecord(...)`
* `upsertRecord(...)`
* `addPublicCapability(...)`

Recommended first name:

```cpp
addCapability(const PublicCapabilityRecord& record)
```

Reason:

* simple
* no implicit update behavior
* easier to test duplicate/full-store failure
* avoids hidden merge policy
* keeps add/update separation clear

Avoid `addOrUpdateCapability(...)` and `upsertRecord(...)` for the first implementation because they imply overwrite or merge policy that has not been approved.

## 6. Record Validity Requirements

Future `addCapability(...)` should reject records that are not explicitly valid.

Planned checks:

* record must be explicitly valid.
* capability instance ID must be valid/nonzero according to the public capability contract.
* capability ID must be valid/nonzero according to the current type contract.
* lifecycle state must not be unknown/none if the record is intended active.
* visibility state must be public/visible/known according to the existing enum model.
* availability may be unavailable/unknown if no value/provider exists yet.
* freshness may be unknown/stale until a value owner exists.
* no default all-zero record is accepted as real.
* no fake placeholder is accepted.
* no fake `CAP_TEMPERATURE` is accepted unless explicitly constructed as a validation-only owner-backed test record with required fields.
* no automatic provider/hardware inference is performed.
* no default zero value is treated as a real sensor value.

The first add path should treat invalid or incomplete records as deterministic failures.

## 7. Capacity And Failure Behavior

Planned behavior:

* fixed capacity only.
* no heap allocation.
* if full, add fails deterministically.
* failed add does not mutate existing records.
* duplicate capability instance ID behavior must be defined.
* duplicate capability ID may or may not be allowed later depending on multi-instance rules.
* invalid record fails deterministically.
* returned status should be compact and firmware-safe.

Return type options:

* `bool`
* compact enum/result type

Recommended safe first option:

```cpp
bool addCapability(const PublicCapabilityRecord& record)
```

Reason:

* smallest first implementation
* enough to validate success/failure paths
* follows the current `NodeDirectory::addNode(...)` first-step pattern
* avoids introducing a result enum before failure categories stabilize

If diagnostics need more detail later, a follow-up milestone can introduce a compact `CapabilityDirectoryAddResult` enum without changing the initial safety rules.

## 8. Duplicate Behavior

First duplicate policy:

* duplicate capability instance ID should fail.
* no silent overwrite.
* no merge.
* no update path yet.
* capability ID alone may not be unique long term because multiple providers/instances may expose the same `CAP_*`.
* first implementation should define the exact duplicate key using existing `PublicCapabilityRecord` fields.

Recommended first duplicate key:

```text
capability_instance_id
```

This keeps multi-instance capability support possible later while preventing accidental overwrite in the first add path.

## 9. Test-Only Access Path

Tests may add records safely only through approved validation context.

Planned test access:

* use `PublicOwnerStore.mutableCapabilities()` only in validation/test context.
* API read methods must not use `mutableCapabilities()`.
* UI/App/Transport must not access `mutableCapabilities()`.
* no production provider path yet.
* no Registry import path yet.
* no `src/main.cpp` integration.

The mutable accessor remains an internal bridge seam for approved Core milestones, not a UI/App/Transport contract.

## 10. API Behavior After First Test Add

Once a test-controlled capability exists in a validation scenario:

* `getCapabilityList(...)` may return count `1` only where the store was explicitly seeded by validation.
* capability detail methods may return owner-backed mapped fields if API-seeded validation is approved.
* `getCapabilityValue(...)` should still return no value/unavailable or `capability_not_found` according to approved value-store availability rules.
* no sensor value inference from capability record alone.
* no provider diagnostics unless stores exist.
* production default remains empty.

The first controlled add path does not imply live capability discovery, provider import, app-visible production data, automatic value creation, or diagnostics creation.

## 11. Required Future Mapping From PublicCapabilityRecord To API Responses

Future API mapping should follow these rules:

* `getCapabilitySummary(...)` maps only available public fields.
* `getCapabilityIdentity(...)` maps only identity fields.
* `getCapabilityAvailability(...)` maps availability/freshness if present.
* `getCapabilityValue(...)` remains unavailable/no value unless `CapabilityValueStore` exists.
* `getCapabilityProviderInfo(...)` remains unavailable unless provider public info exists.
* `getCapabilityQuality(...)` remains unavailable unless diagnostics/quality store exists.
* no hardware reads.
* no provider selection.

No API method should infer missing fields from hardware, packets, Registry arrays, providers, logs, debug output, or default numeric values.

## 12. Validation Plan For Implementation Milestone

Future implementation validation should cover:

* default `CapabilityDirectory` count `0`.
* add valid validation-only test capability succeeds.
* count becomes `1`.
* `readByIndex(0)` succeeds.
* read returns same owner-backed capability instance ID.
* invalid default record add fails.
* duplicate capability instance ID add fails.
* full capacity failure is deterministic.
* failed add does not mutate count.
* reset clears added test capability.
* `PublicOwnerStore.mutableCapabilities()` can seed test capability.
* `PublicOwnerStore` reset clears seeded capability.
* no fake capabilities.
* no fake `CAP_TEMPERATURE` unless explicitly validation-only and owner-backed.
* no fake sensor values.
* no Registry/HAL/packet/provider/transport calls.
* `src/main.cpp` untouched.
* existing Node API default empty tests still pass.
* existing Capability API default empty tests still pass.
* existing `PublicOwnerStore` tests still pass.

The validation must prove production default remains empty.

## 13. Production Default Behavior

Production default behavior remains:

* default production owner store remains empty.
* no capabilities are created automatically.
* no setup/loop integration.
* no transport seeding.
* no discovery/provider seeding.
* no Registry seeding.
* no value seeding.
* no diagnostics seeding.
* Minimal App still sees empty capability list unless a future approved live source exists.

Test-only records must not become production bootstrap behavior.

## 14. Minimal App Implications

Minimal App implications:

* no app change required.
* app remains mock-only.
* test-only capability records are not live app records.
* app must not infer real sensor hardware from test records.
* future app live path still requires approved transport and owner-backed real source.
* app must continue to use `Cyber32Api` only.

The app must not access `PublicOwnerStore` or `CapabilityDirectory` directly.

## 15. Safety Invariants

Safety invariants:

* no fake capabilities
* no fake `CAP_TEMPERATURE`
* no fake sensor values
* no fake diagnostics
* no fake nodes
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
Milestone 10.47 - CapabilityDirectory Controlled Add Path Implementation
```

Purpose:

Implement a bounded test-only `addCapability` path for `CapabilityDirectory` and validation coverage without changing default production API behavior.

Alternative:

```text
Milestone 10.47 - CapabilityDirectory Add Result Type Plan
```

## 17. Stop Conditions

Stop future work if it tries to:

* create fake capabilities
* create fake `CAP_TEMPERATURE`
* create fake sensor values
* auto-seed production capabilities
* parse packets
* read Registry arrays
* call HAL/Drivers/Devices/Services/Logic/Runtime
* add provider bridge before approval
* add discovery bridge before approval
* add transport before approval
* modify `src/main.cpp`
* expose mutable owner access to UI/App/Transport
* infer values from capability records
* add diagnostics/provider quality without approved stores
