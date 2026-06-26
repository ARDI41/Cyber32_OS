# Milestone 10.31 - NodeDirectory API Empty Attachment Validation Audit

## 1. Scope

Milestone 10.31 audits the Milestone 10.30 `NodeDirectory` API empty attachment implementation.

This milestone is documentation only.

It audits:

- the internal empty-state source change for Node API reads
- the external Node API behavior after the attachment
- validation coverage for the empty attachment
- safety boundaries preserved by the implementation

This milestone does not modify firmware source code, change `NodeDirectory`, change `Cyber32Api`, add records, add transport, or alter current API behavior.

## 2. Files Changed In Milestone 10.30

Milestone 10.30 modified:

- `src/api/cyber32_api.cpp`
- `src/app/validation/vertical_slice_validation.cpp`

No `src/main.cpp` changes were made.

## 3. Attachment Summary

Milestone 10.30 attached Node API empty reads to the empty `NodeDirectory` skeleton.

Implementation summary:

- a provisional internal static `NodeDirectory` was added in `cyber32_api.cpp`
- `getNodeList(...)` now reads `NodeDirectory::count()`
- node detail methods call `NodeDirectory::readByIndex(...)`
- failed `readByIndex(...)` calls still map to `node_not_found`
- `getNodeCapabilities(...)` still initializes `out_count = 0`
- `NodeDirectory` remains empty by default
- no records are added
- no real node creation path exists
- no app-visible real node data appears

The attachment changes the internal empty-state source path only.

## 4. External API Behavior Validation

Expected external Node API behavior remains unchanged.

Current expected behavior:

- `getNodeList(...)` returns `true`
- `getNodeList(...)` sets `ok = true`
- `getNodeList(...)` sets `error_code = "none"`
- `getNodeList(...)` sets `count = 0`
- `getNodeSummary(...)` returns `false` with `error_code = "node_not_found"`
- `getNodeIdentity(...)` returns `false` with `error_code = "node_not_found"`
- `getNodeStatus(...)` returns `false` with `error_code = "node_not_found"`
- `getNodePower(...)` returns `false` with `error_code = "node_not_found"`
- `getNodeSignal(...)` returns `false` with `error_code = "node_not_found"`
- `getNodeDiagnostics(...)` returns `false` with `error_code = "node_not_found"`
- `getNodeCapabilities(...)` initializes `out_count = 0`
- `getNodeCapabilities(...)` returns `false` while no node exists

No fake node summary, identity, status, power, signal, diagnostics, or capability data is exposed.

## 5. Safety Boundary

Milestone 10.30 preserved the required safety boundary.

Confirmed:

- no fake nodes
- no real node data
- no fake capabilities
- no fake sensor values
- no fake diagnostics
- no Registry access for Node API data
- no packet parsing
- no provider selection
- no HAL calls
- no Driver calls
- no Device calls
- no Module calls
- no Service calls for Node API data
- no Logic calls
- no Runtime calls for Node API data
- no transport added

The Node API still reads only the empty public-owner path for node data.

## 6. `src/main.cpp` Boundary

Milestone 10.30 preserved the startup boundary.

Confirmed:

- `src/main.cpp` untouched
- no `setup()` integration
- no `loop()` integration
- no hidden app initialization
- no hidden transport initialization
- no production bootstrap change

## 7. Validation Coverage

Validation coverage after Milestone 10.30:

- existing Node API empty-state validation remains
- `getNodeList(...)` success with `count = 0` remains validated
- node detail `node_not_found` responses remain validated
- deterministic default fields for failed node detail reads remain validated
- `getNodeCapabilities(...)` `out_count = 0` behavior remains validated
- repeated Node API reads remain validated for deterministic behavior
- Node API count bound check was added against `NODE_DIRECTORY_MAX_PUBLIC_NODES`
- `NodeDirectory` empty skeleton validation remains
- full validation path should still pass when built in an environment with PlatformIO available

The validation proves the public behavior remains empty and deterministic.

## 8. Local Build Result

Codex attempted:

```text
pio run
```

Result in the Codex shell:

```text
pio is not available on PATH
```

Therefore, local PlatformIO validation is still required in an environment where `pio` is installed and available.

Build status for this audit:

- Codex build verification: pending due to missing `pio`
- local/user `pio run`: pending unless separately verified by the user

## 9. Compatibility Statement

Milestone 10.30 is behavior-preserving.

It changes the internal empty-state source path from a direct API empty response to an empty `NodeDirectory` read path.

It does not change app-visible Node API results.

The API still reports:

- empty node list
- `node_not_found` for node detail reads
- no node capabilities while no node exists

This preserves the Cyber32 rule:

```text
The API must not switch from empty to fake.
It must switch only from empty to owner-backed.
```

## 10. Known Limitations

Known limitations after Milestone 10.30:

- `NodeDirectory` is a provisional static owner inside the API implementation
- no public owner lifetime plan exists yet
- no explicit Core context owns public stores yet
- no node creation path exists
- no discovery bridge exists
- no pairing bridge exists
- no trust bridge exists
- no real node records exist
- no capability mapping exists
- no `NodeCapabilityMap` attachment exists
- no Minimal App live transport exists
- no provider ingestion path is connected to public node records
- no persistence exists for future public nodes

These limitations are intentional for the empty attachment milestone.

## 11. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.32 - CapabilityDirectory Skeleton Plan
```

Purpose:

Plan the empty-by-default bounded `CapabilityDirectory` owner skeleton before implementing it.

Rationale:

Node API now has an empty owner-backed path. The next UI-critical owner is the public capability owner, which will eventually support capability list and capability detail APIs without fake data.

## 12. Stop Conditions

Stop future work and return to architecture review if it tries to:

- create fake nodes
- add real node records without an owner-backed creation path
- add fake capabilities
- add fake sensor values
- add fake diagnostics
- parse packets
- read Registry private arrays
- call HAL
- call Drivers
- call Devices
- call Modules
- call Services
- call Logic
- call Runtime for node data
- modify `src/main.cpp` without explicit approval
- add live transport
- add WebServer before transport approval
- add HTTP before transport approval
- add JSON before transport approval
- add BLE before transport approval
- add WiFi app transport before transport approval
