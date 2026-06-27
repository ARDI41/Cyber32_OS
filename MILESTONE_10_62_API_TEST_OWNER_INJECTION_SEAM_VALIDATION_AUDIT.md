# Milestone 10.62 - API Test Owner Injection Seam Validation Audit

## 1. Scope

Milestone 10.62 audits the Milestone 10.61 API test owner injection seam only.

This milestone is documentation only.

It does not:

* modify firmware source code
* modify `src/main.cpp`
* change `Cyber32Api` implementation
* change `PublicOwnerStore` implementation
* change `NodeCapabilityMap` implementation
* add production mapping records
* add fake mappings
* add fake capability summaries
* add fake nodes
* add fake capabilities
* infer capabilities from nodes
* infer mappings from public owner fields
* add discovery, provider, Registry, value, diagnostics, or transport bridges
* parse packets
* call HAL, Drivers, Devices, Modules, Services, Logic, or Runtime
* add provider selection
* add command execution
* add WebServer, HTTP, JSON, BLE, WiFi, or Minimal App transport
* expose private Registry arrays

## 2. Files Changed In 10.61

Milestone 10.61 modified:

* `src/api/cyber32_api.h`
* `src/api/cyber32_api.cpp`
* `src/app/validation/vertical_slice_validation.h`
* `src/app/validation/vertical_slice_validation.cpp`

No `src/main.cpp` change was made.

## 3. Implementation Summary

Milestone 10.61 added a safe validation/internal owner-store injection seam.

Implementation summary:

* default `Cyber32Api()` still uses the private internal production `PublicOwnerStore`.
* `Cyber32Api(PublicOwnerStore& owner_store)` was added for validation/internal owner-backed testing.
* API reads now use the selected owner store.
* production default behavior remains unchanged.
* no mutable owner store accessor is exposed by `Cyber32Api`.
* no production static owner store mutable accessor was added.
* no UI/App/Transport access was added.
* no transport was added.
* no `src/main.cpp` change was made.

The seam lets validation pass a caller-owned local `PublicOwnerStore` without mutating the production static owner store.

## 4. Positive Validation Coverage

`validateGetNodeCapabilitiesOwnerBackedPositivePath()` covers:

* injected local owner store starts empty.
* seeded node with no links returns success with count `0`.
* seeded node plus capability without link returns success with count `0`.
* seeded node plus capability plus explicit link returns success with count `1`.
* returned API node capability summary matches safely mapped seeded capability fields that are currently representable.
* unrelated node link is not returned.
* broken link to missing capability is skipped.
* output capacity is respected.
* local owner store reset clears seeded records.
* production default API remains unaffected after injected tests.
* no fake mappings.
* no inferred mappings.
* no fake capability summaries.
* no fake `CAP_TEMPERATURE`.
* no fake sensor values.
* no fake diagnostics.

Validation seeding uses a local `PublicOwnerStore` and controlled public owner methods only:

```text
mutableNodes().addNode(...)
mutableCapabilities().addCapability(...)
mutableNodeCapabilities().addLink(...)
```

This seeding is validation-only and caller-owned.

## 5. Production Default Behavior

Production default behavior remains unchanged:

* default `Cyber32Api` construction remains unchanged.
* production static owner store remains private.
* production default state remains empty.
* no production auto-seed exists.
* no production mapping records were added.
* no setup/loop integration exists.
* no `src/main.cpp` integration exists.
* no transport integration exists.
* no discovery/provider integration exists.
* no Registry integration exists.
* no value/diagnostics integration exists.

Milestone 10.61 creates a safe validation seam, not a production data source.

## 6. Access Boundary

Access boundary remains intact:

* `Cyber32Api` does not expose mutable owner store.
* UI/App/Transport cannot access injected owner store.
* injection seam is explicit.
* injection seam is validation/internal only.
* injected owner store is caller-owned.
* `Cyber32Api` does not take ownership of injected store.
* `Cyber32Api` must not outlive injected store.
* production static owner store remains private.

No global mutable owner accessor was added.

## 7. API Behavior Boundary

API behavior boundary remains intact:

* `getNodeCapabilities(...)` still uses owner-backed explicit mappings.
* missing node behavior remains safe.
* node with no links returns success with count `0`.
* node plus capability without link returns success with count `0`.
* node plus capability plus explicit link can return count `1` in injected validation context.
* broken links are skipped.
* unrelated node links are ignored.
* no values are returned.
* no diagnostics are returned.
* no provider info is returned.
* no quality data is returned.

The API remains a read-only public owner view.

## 8. Non-Inference Boundary

Milestone 10.61 preserves the non-inference boundary:

* no mapping is inferred from `PublicNodeRecord.capability_count`.
* no mapping is inferred from `PublicCapabilityRecord.owner_node_id`.
* no mapping is inferred from `CAP_TEMPERATURE`.
* no mapping is inferred from Registry state.
* no mapping is inferred from packets.
* no mapping is inferred from providers.
* no mapping is inferred from debug output.
* no mapping is inferred from sensor values.
* no mapping is inferred from diagnostics.

Only explicit owner-backed `NodeCapabilityMap` links can produce mapped capability output.

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

Validation stays inside local owner-backed public records.

## 10. Local Build Result

Build result:

```text
pio run: passed locally
```

Codex shell note:

The Codex shell may not have `pio` on `PATH`. If `pio` is unavailable in the Codex shell, local PlatformIO validation remains the build confirmation source.

## 11. Compatibility Statement

Milestone 10.61 adds a validation/internal owner injection seam for positive API testing, but does not change production default API behavior.

The data transition remains:

```text
empty -> owner-backed empty -> owner-backed validation-seeded -> future owner-backed real
```

Never:

```text
empty -> fake
```

## 12. Known Limitations

Known limitations after Milestone 10.61:

* injection seam is not live app transport.
* not cloud/local connection.
* not discovery bridge.
* not provider bridge.
* not Registry bridge.
* not `CapabilityValueStore`.
* not `DiagnosticsStore`.
* not production data creation.
* not external app API.
* no live Minimal App data yet.
* no snapshot transport yet.

The seam is for validation and internal owner-backed API tests only.

## 13. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.63 - Core API Snapshot Contract Plan
```

Purpose:

Plan a read-only API snapshot contract for Minimal App/mock transport that can expose nodes, capabilities, node-capability links, and summary data without adding live HTTP/BLE/WiFi transport or fake runtime data.

Alternative:

```text
Milestone 10.63 - getNodeCapabilities Positive Path Validation Expansion
```

## 14. Stop Conditions

Stop future work if it tries to:

* expose mutable production owner store publicly
* expose injection seam to UI/App/Transport as a transport contract
* create fake production data
* mutate production static state from validation unsafely
* add transport
* parse packets
* read Registry arrays
* call HAL, Drivers, Devices, Services, Logic, or Runtime
* add provider/discovery bridge before approval
* modify `src/main.cpp`
* add values/diagnostics without approved stores
