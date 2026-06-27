# Milestone 10.59 - getNodeCapabilities Owner-Backed Mapping API Validation Audit

## 1. Scope

Milestone 10.59 audits the Milestone 10.58 `getNodeCapabilities(...)` owner-backed mapping API implementation only.

This milestone is documentation only.

It does not:

* modify firmware source code
* modify `src/main.cpp`
* change `Cyber32Api` implementation
* change `PublicOwnerStore` implementation
* change `NodeCapabilityMap` implementation
* add an API owner injection seam
* add production mapping records
* add fake mappings
* add fake capability summaries
* infer capabilities from nodes
* infer nodes from capabilities
* add discovery, provider, Registry, value, diagnostics, or transport bridges
* parse packets
* call HAL, Drivers, Devices, Modules, Services, Logic, or Runtime
* add provider selection
* add command execution
* add WebServer, HTTP, JSON, BLE, WiFi, or Minimal App transport
* expose private Registry arrays

## 2. Files Changed In 10.58

Milestone 10.58 modified:

* `src/api/cyber32_api.cpp`
* `src/app/validation/vertical_slice_validation.cpp`

No `src/main.cpp` change was made.

## 3. Implementation Summary

Milestone 10.58 updated `Cyber32Api::getNodeCapabilities(...)` so the method can read explicit owner-backed mappings.

Implementation summary:

* `getNodeCapabilities(...)` now uses read-only `PublicOwnerStore` accessors.
* it uses `PublicOwnerStore::nodes()`.
* it uses `PublicOwnerStore::nodeCapabilities()`.
* it uses `PublicOwnerStore::capabilities()`.
* it confirms the requested node exists before producing mapping output.
* it scans `NodeCapabilityMap` by node ID.
* it scans `CapabilityDirectory` by capability instance ID.
* it maps linked `PublicCapabilityRecord` records to API node capability summary output.
* it skips broken links deterministically.
* it respects `max_count`.
* it initializes `out_count` to `0`.
* it does not use mutable owner accessors.
* it does not create mappings.
* it does not infer mappings.
* it does not create fake summaries.
* it does not connect transport.
* it does not call Registry, HAL, Drivers, Devices, Services, Logic, or Runtime.

The implementation remains read-only from the API perspective.

## 4. Behavior Coverage

Expected behavior after Milestone 10.58:

* missing node returns `false` according to existing API convention.
* missing node keeps `out_count = 0`.
* missing node does not write the output buffer.
* existing node with no links returns success with `out_count = 0`.
* explicit mapped links are filtered by requested node ID.
* broken links to missing capability records are skipped.
* unrelated node links are ignored.
* output is bounded by `max_count`.
* summary output contains only safe mapped public capability summary fields.
* no values are exposed.
* no diagnostics are exposed.
* no provider info is exposed.
* no quality data is exposed.

Current `ApiNodeCapabilitySummary` has limited fields. Milestone 10.58 therefore avoids fabricating capability names, text IDs, values, timestamps, provider details, quality, diagnostics, or units.

## 5. Validation Coverage

Milestone 10.58 added default-path validation for missing-node `getNodeCapabilities(...)`.

Validation added:

* missing-node call returns failure according to existing convention.
* missing-node call keeps `out_count = 0`.
* missing-node call does not write the output buffer.
* existing API default behavior remains safe.

Validation deferred:

* seeded owner-backed API mapping validation was deferred.

Reason:

* `Cyber32Api` currently uses a private static `PublicOwnerStore`.
* no approved safe owner injection seam exists yet.
* unsafe mutation of the production static API owner store was intentionally avoided.
* future seeded mapping validation needs an approved API test owner injection seam or equivalent safe validation harness.

This preserves the boundary that validation must not smuggle production state changes through hidden static owners.

## 6. Production Default Behavior

Production default behavior remains unchanged:

* production default still has no mappings.
* no production auto-seed exists.
* no node gains capabilities automatically.
* no capability attaches to node automatically.
* no fake summaries appear.
* no setup/loop integration exists.
* no `src/main.cpp` integration exists.
* no transport integration exists.
* no discovery/provider integration exists.
* no Registry integration exists.
* no value/diagnostics integration exists.

Milestone 10.58 changes read behavior readiness, not production data creation.

## 7. API Access Boundary

API access boundary remains intact:

* `Cyber32Api` uses only read-only accessors.
* `Cyber32Api` does not use `mutableNodes()`.
* `Cyber32Api` does not use `mutableCapabilities()`.
* `Cyber32Api` does not use `mutableNodeCapabilities()`.
* UI, App, and Transport still cannot access owner stores directly.
* Minimal App must still use API only in future.

Owner-backed mappings remain internal Core state.

## 8. Non-Inference Boundary

Milestone 10.58 preserves the non-inference boundary:

* no mapping is inferred from `PublicNodeRecord.capability_count`.
* no mapping is inferred from `PublicCapabilityRecord.owner_node_id`.
* no mapping is inferred from `CAP_TEMPERATURE`.
* no mapping is inferred from Registry state.
* no mapping is inferred from packets.
* no mapping is inferred from providers.
* no mapping is inferred from debug output.
* no mapping is inferred from sensor values.
* no mapping is inferred from diagnostics.

Only explicit `NodeCapabilityMap` links can cause mapped capability output.

## 9. Safety Boundary

Safety boundary remains intact:

* no fake mappings
* no fake nodes
* no fake capabilities
* no fake `CAP_TEMPERATURE`
* no fake sensor values
* no fake diagnostics
* no fake capability summaries
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

The API mapping implementation is a bounded read path only.

## 10. Local Build Result

Build result:

```text
pio run: passed locally
```

Codex shell note:

The Codex shell may not have `pio` on `PATH`. If `pio` is unavailable in the Codex shell, local PlatformIO validation remains the build confirmation source.

## 11. Compatibility Statement

Milestone 10.58 attaches `getNodeCapabilities(...)` to owner-backed explicit mapping reads, but does not change production default data creation behavior.

The transition remains:

```text
empty -> owner-backed empty -> owner-backed test-controlled link -> future owner-backed real
```

Never:

```text
empty -> fake
```

## 12. Known Limitations

Known limitations after Milestone 10.58:

* seeded mapping API validation is deferred.
* no API test owner injection seam exists yet.
* no `lookupByNode` optimization exists.
* no `lookupByCapability` optimization exists.
* no full seeded positive-path validation exists yet for `getNodeCapabilities(...)`.
* no `CapabilityValueStore` exists.
* no `DiagnosticsStore` exists.
* no provider bridge exists.
* no discovery bridge exists.
* no transport exists.
* no live app data exists.
* no production mapping creation path exists.
* broken links are skipped, not diagnosed yet.
* truncation behavior may need richer status later if the current API has no partial/truncated error code.

These limitations are intentional until the next validation and owner-lifetime milestones approve a safe seeded API test path.

## 13. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.60 - API Test Owner Injection Seam Plan
```

Purpose:

Plan a safe validation-only seam that allows tests to seed an owner-backed `PublicOwnerStore` for `Cyber32Api` positive-path validation without mutating production static state, adding transport, or exposing mutable owner access to UI/App/Transport.

Alternative:

```text
Milestone 10.60 - getNodeCapabilities Positive Mapping Validation Harness Plan
```

## 14. Stop Conditions

Stop future work if it tries to:

* mutate production static API owner store unsafely from validation
* expose mutable owner access to UI/App/Transport
* create fake mappings
* infer capabilities from nodes
* infer mappings from `owner_node_id`
* infer mapping from `CAP_TEMPERATURE`
* create fake capability summaries
* create fake sensor values
* parse packets
* read Registry arrays
* call HAL, Drivers, Devices, Services, Logic, or Runtime
* add provider bridge before approval
* add discovery bridge before approval
* add transport before approval
* modify `src/main.cpp`
* add values/diagnostics without approved stores
