# Milestone 10.48 - CapabilityDirectory Controlled Add Path Validation Audit

## 1. Scope

Milestone 10.48 audits the Milestone 10.47 `CapabilityDirectory` controlled add path implementation.

This milestone is documentation only.

It does not:

* modify firmware source code
* modify `src/main.cpp`
* change `CapabilityDirectory` implementation
* change `PublicOwnerStore` implementation
* change `Cyber32Api` behavior
* add real production capability records
* add fake capabilities, fake `CAP_TEMPERATURE`, fake sensor values, fake diagnostics, or fake nodes
* add discovery, provider, Registry, node-to-capability, value, or diagnostics bridges
* parse packets
* call HAL, Drivers, Devices, Modules, Services, Logic, or Runtime
* add provider selection
* add command execution
* add WebServer, HTTP, JSON, BLE, WiFi, or live Minimal App transport
* expose private Registry arrays

## 2. Files Changed In 10.47

Milestone 10.47 modified:

* `include/cyber32/public/capability_directory.h`
* `src/public/capability_directory.cpp`
* `src/app/validation/vertical_slice_validation.h`
* `src/app/validation/vertical_slice_validation.cpp`

No `src/main.cpp` change was made.

## 3. Implementation Summary

Milestone 10.47 added:

```cpp
bool CapabilityDirectory::addCapability(const PublicCapabilityRecord& record)
```

Implementation summary:

* `addCapability` is bounded.
* `addCapability` is deterministic.
* `addCapability` does not allocate dynamically.
* `addCapability` does not parse packets.
* `addCapability` does not read Registry.
* `addCapability` does not call HAL, Drivers, Devices, Services, Logic, or Runtime.
* `addCapability` does not create values.
* `addCapability` does not create diagnostics.
* `addCapability` does not create provider metadata.
* `addCapability` does not seed production state.

The add path rejects default/invalid records, zero capability ID, zero capability instance ID, invalid lifecycle/visibility state, duplicate capability instance ID, and full directory writes.

## 4. Validation Coverage

`validateCapabilityDirectoryControlledAddPath()` covers:

* default count is `0`.
* valid validation-only test capability add succeeds.
* count becomes `1`.
* `readByIndex(0)` succeeds.
* read returns same owner-backed capability instance ID.
* default/invalid record add fails.
* duplicate capability instance ID add fails.
* full capacity failure is deterministic.
* failed add does not mutate count.
* reset clears added test capabilities.
* `PublicOwnerStore.mutableCapabilities()` can seed a test capability in validation context.
* `PublicOwnerStore` reset clears seeded capability.
* no fake capabilities.
* no fake `CAP_TEMPERATURE`.
* no fake sensor values.
* no fake diagnostics.
* existing API empty default validation still passes.
* existing `NodeDirectory` controlled add validation still passes.

Validation-only test capabilities are deterministic owner-backed records used only by the validation harness.

## 5. Production Default API Behavior

Production default API behavior remains:

* `getCapabilityList(...)` still returns empty success by default.
* capability detail methods still return `capability_not_found` by default.
* `getCapabilityValue(...)` still returns `capability_not_found` by default while no capability exists.
* `getCapabilityAvailability(...)` still returns `capability_not_found` by default while no capability exists.
* `getCapabilityProviderInfo(...)` still returns `capability_not_found` by default while no capability exists.
* `getCapabilityQuality(...)` still returns `capability_not_found` by default while no capability exists.
* no capability appears automatically.
* no production auto-seed.
* no `src/main.cpp` integration.
* no setup/loop integration.
* no transport integration.
* no discovery/provider integration.
* no Registry integration.
* no value/diagnostics integration.

Milestone 10.47 added a controlled add operation, not a production capability source.

## 6. Test-Only Record Boundary

Validation boundary:

* validation-only test capabilities are explicitly constructed in validation.
* they are owner-backed test records.
* they are not live hardware records.
* they are not fake production records.
* they do not imply discovery.
* they do not imply provider bridge.
* they do not imply node-to-capability mapping.
* they do not imply sensor values.
* they do not imply diagnostics/provider quality.
* they are not exposed to Minimal App as live records.

The test capability proves storage mechanics only.

## 7. PublicOwnerStore Boundary

`PublicOwnerStore` boundary remains:

* `PublicOwnerStore.mutableCapabilities()` is used only for validation/test seeding.
* `Cyber32Api` read methods do not use `mutableCapabilities()`.
* UI/App/Transport do not access `mutableCapabilities()`.
* `PublicOwnerStore.capabilities()` remains read-only for API reads.

No app-facing mutable owner path was introduced.

## 8. Safety Boundary

Safety boundary remains intact:

* no fake capabilities
* no fake `CAP_TEMPERATURE`
* no fake sensor values
* no fake diagnostics
* no fake nodes
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
* no WebServer/HTTP/JSON/BLE/WiFi
* no Minimal App live transport

The controlled add path remains a bounded owner-store operation.

## 9. Local Build Result

Build result:

```text
pio run: passed locally
```

Codex shell note:

The Codex shell may not have `pio` on `PATH`. If `pio` is unavailable in the Codex shell, local PlatformIO validation remains the build confirmation source.

## 10. Compatibility Statement

Milestone 10.47 adds a controlled owner-backed capability add path, but does not change production default API behavior.

The transition remains:

```text
empty -> owner-backed empty -> owner-backed test-controlled record -> future owner-backed real
```

Never:

```text
empty -> fake
```

## 11. Known Limitations

Known limitations after Milestone 10.47:

* `addCapability` is bool-only for now.
* no add result enum yet.
* no update path.
* no remove path.
* no `NodeCapabilityMap`.
* no `CapabilityValueStore`.
* no `DiagnosticsStore`.
* no provider bridge.
* no discovery bridge.
* no transport.
* no live app data.
* no production record creation path.
* no real sensor values.
* no provider quality/diagnostics.

These limitations are intentional until mapping, value, diagnostics, and live-source milestones are approved.

## 12. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.49 - NodeCapabilityMap Skeleton Plan
```

Purpose:

Plan the first bounded owner-backed mapping store linking public nodes to public capabilities without inferring capabilities from nodes, parsing packets, using Registry, or adding transport.

Alternative:

```text
Milestone 10.49 - CapabilityDirectory Add Result Type Plan
```

## 13. Stop Conditions

Stop future work if it tries to:

* create fake capabilities
* create fake `CAP_TEMPERATURE`
* create fake sensor values
* expose validation test capabilities as live app records
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
* infer node-to-capability mapping without approved `NodeCapabilityMap`
