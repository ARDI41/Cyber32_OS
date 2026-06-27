# Milestone 10.45 - NodeDirectory Controlled Add Path Validation Audit

## 1. Scope

Milestone 10.45 audits the Milestone 10.44 `NodeDirectory` controlled add path implementation.

This milestone is documentation only.

It does not:

* modify firmware source code
* modify `src/main.cpp`
* change `NodeDirectory` implementation
* change `PublicOwnerStore` implementation
* change `Cyber32Api` behavior
* add real production node records
* add fake nodes, fake capabilities, fake sensor values, or fake diagnostics
* add discovery, pairing, trust, Registry, provider, or transport bridges
* parse packets
* call HAL, Drivers, Devices, Modules, Services, Logic, or Runtime
* add provider selection
* add command execution
* add WebServer, HTTP, JSON, BLE, WiFi, or live Minimal App transport
* expose private Registry arrays

## 2. Files Changed In 10.44

Milestone 10.44 modified:

* `include/cyber32/public/node_directory.h`
* `src/public/node_directory.cpp`
* `src/app/validation/vertical_slice_validation.h`
* `src/app/validation/vertical_slice_validation.cpp`

No `src/main.cpp` change was made.

## 3. Implementation Summary

Milestone 10.44 added:

```cpp
bool NodeDirectory::addNode(const PublicNodeRecord& record)
```

Implementation summary:

* `addNode` is bounded.
* `addNode` is deterministic.
* `addNode` does not allocate dynamically.
* `addNode` does not parse packets.
* `addNode` does not read Registry.
* `addNode` does not call HAL, Drivers, Devices, Services, Logic, or Runtime.
* `addNode` does not create capabilities.
* `addNode` does not create diagnostics.
* `addNode` does not seed production state.

The add path rejects default/invalid records, zero `node_id`, invalid lifecycle/visibility state, duplicate `node_id`, and full directory writes.

## 4. Validation Coverage

`validateNodeDirectoryControlledAddPath()` covers:

* default count is `0`.
* valid validation-only test node add succeeds.
* count becomes `1`.
* `readByIndex(0)` succeeds.
* read returns same owner-backed `node_id`.
* default/invalid record add fails.
* duplicate `node_id` add fails.
* full capacity failure is deterministic.
* failed add does not mutate count.
* reset clears added test nodes.
* `PublicOwnerStore.mutableNodes()` can seed a test node in validation context.
* `PublicOwnerStore` reset clears seeded node.
* no fake nodes.
* existing API empty default validation still passes.

Validation-only test records are deterministic owner-backed records used only by the validation harness.

## 5. Production Default API Behavior

Production default API behavior remains:

* `getNodeList(...)` still returns empty success by default.
* node detail methods still return `node_not_found` by default.
* `getNodeCapabilities(...)` still initializes `out_count = 0` and returns `node_not_found` by default.
* no node appears automatically.
* no production auto-seed.
* no `src/main.cpp` integration.
* no setup/loop integration.
* no transport integration.
* no discovery integration.
* no Registry integration.

Milestone 10.44 added a controlled add operation, not a production node source.

## 6. Test-Only Record Boundary

Validation boundary:

* validation-only test records are explicitly constructed in validation.
* they are owner-backed test records.
* they are not live hardware records.
* they are not fake production records.
* they do not imply discovery.
* they do not imply pairing/trust.
* they do not imply capability creation.
* they do not imply diagnostics/value data.
* they are not exposed to Minimal App as live records.

The test record proves storage mechanics only.

## 7. PublicOwnerStore Boundary

`PublicOwnerStore` boundary remains:

* `PublicOwnerStore.mutableNodes()` is used only for validation/test seeding.
* `Cyber32Api` read methods do not use `mutableNodes()`.
* UI/App/Transport do not access `mutableNodes()`.
* `PublicOwnerStore.nodes()` remains read-only for API reads.

No app-facing mutable owner path was introduced.

## 8. Safety Boundary

Safety boundary remains intact:

* no fake nodes
* no fake capabilities
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

Milestone 10.44 adds a controlled owner-backed add path, but does not change production default API behavior.

The transition remains:

```text
empty -> owner-backed empty -> owner-backed test-controlled record -> future owner-backed real
```

Never:

```text
empty -> fake
```

## 11. Known Limitations

Known limitations after Milestone 10.44:

* `addNode` is bool-only for now.
* no add result enum yet.
* no update path.
* no remove path.
* no `NodeCapabilityMap`.
* no `CapabilityDirectory` add path.
* no `CapabilityValueStore`.
* no `DiagnosticsStore`.
* no discovery bridge.
* no transport.
* no live app data.
* no production record creation path.

These limitations are intentional until the next controlled owner-store milestones are approved.

## 12. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.46 - CapabilityDirectory Controlled Add Path Plan
```

Purpose:

Plan the first bounded, test-only, owner-backed way to add a public capability record through `PublicOwnerStore`-controlled ownership without discovery, provider bridge, Registry, transport, or fake runtime behavior.

Alternative:

```text
Milestone 10.46 - NodeDirectory Add Result Type Plan
```

## 13. Stop Conditions

Stop future work if it tries to:

* create fake nodes
* auto-seed production nodes
* expose validation test nodes as live app records
* parse packets
* read Registry arrays
* call HAL/Drivers/Devices/Services/Logic/Runtime
* add discovery bridge before approval
* add transport before approval
* modify `src/main.cpp`
* expose mutable owner access to UI/App/Transport
* infer capabilities from node records
* add diagnostics/value data without approved stores
