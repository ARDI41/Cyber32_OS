# Milestone 10.36 - CapabilityDirectory API Empty Attachment Validation Audit

## 1. Scope

Milestone 10.36 audits the Milestone 10.35 `CapabilityDirectory` API empty attachment.

This audit is documentation only.

It does not:

- modify firmware source code
- modify `src/main.cpp`
- change `CapabilityDirectory`
- change `Cyber32Api`
- add real capability data
- add discovery, provider, Registry, node-to-capability, value, or diagnostics bridges
- parse packets
- call HAL, Drivers, Devices, Modules, Services, Logic, or Runtime
- add provider selection
- add command execution
- add WebServer, HTTP, JSON, BLE, WiFi, or Minimal App transport

The audit scope is limited to confirming that the empty-by-default `CapabilityDirectory` is now the internal source of Capability API empty state while preserving all external Capability API behavior.

## 2. Files Changed In 10.35

Milestone 10.35 modified:

- `src/api/cyber32_api.cpp`
- `src/app/validation/vertical_slice_validation.cpp`

No `src/main.cpp` changes were made.

## 3. Attachment Summary

Milestone 10.35 added a provisional internal static `CapabilityDirectory` in `src/api/cyber32_api.cpp`.

Attachment behavior:

- `getCapabilityList(...)` now reads `CapabilityDirectory::count()`
- capability detail methods call `CapabilityDirectory::readByIndex(...)`
- failed reads still map to `capability_not_found`
- `getCapabilityValue(...)` still returns `capability_not_found` while no capability exists
- `getCapabilityAvailability(...)` still returns `capability_not_found` while no capability exists
- `getCapabilityProviderInfo(...)` still returns `capability_not_found` while no capability exists
- `getCapabilityQuality(...)` still returns `capability_not_found` while no capability exists
- `CapabilityDirectory` remains empty by default

The attachment changes the internal empty-state source path only. It does not create capabilities or expose real capability data.

## 4. External API Behavior Validation

Expected external Capability API behavior remains:

- `getCapabilityList(...)` returns `ok = true`, `error_code = "none"`, `count = 0`
- `getCapabilitySummary(...)` returns `capability_not_found`
- `getCapabilityIdentity(...)` returns `capability_not_found`
- `getCapabilityValue(...)` returns `capability_not_found`
- `getCapabilityAvailability(...)` returns `capability_not_found`
- `getCapabilityProviderInfo(...)` returns `capability_not_found`
- `getCapabilityQuality(...)` returns `capability_not_found`

No app-visible real capability records appear after Milestone 10.35.

## 5. Safety Boundary

Milestone 10.35 preserves these safety boundaries:

- no fake capabilities
- no fake `CAP_TEMPERATURE`
- no fake sensor values
- no fake diagnostics
- no real capability data
- no Registry access in the new CapabilityDirectory attachment path
- no packet parsing
- no provider selection
- no command execution
- no HAL calls
- no Driver calls
- no Device calls
- no Module calls
- no Service calls
- no Logic calls
- no Runtime calls
- no transport added

Existing API methods outside the Capability API retain their previous dependencies and behavior. This audit concerns only the new CapabilityDirectory empty attachment path.

## 6. `src/main.cpp` Boundary

`src/main.cpp` remains untouched.

Milestone 10.35 did not add:

- setup integration
- loop integration
- app initialization
- transport initialization
- boot-time public owner initialization

The provisional `CapabilityDirectory` owner is internal to the API implementation and remains empty by default.

## 7. Validation Coverage

Validation coverage after Milestone 10.35:

- existing Capability API empty-state validation remains
- `getCapabilityList(...)` empty success validation remains
- capability detail `capability_not_found` validation remains
- deterministic default validation for capability detail outputs remains
- repeated-read stability validation remains
- Capability API count bound check was added against `CAPABILITY_DIRECTORY_MAX_PUBLIC_CAPABILITIES`
- `CapabilityDirectory` empty skeleton validation remains
- full validation path should still pass when built and run in an environment with PlatformIO available

The validation continues to prove that no fake capability summaries, fake `CAP_TEMPERATURE`, fake sensor values, or fake diagnostics appear.

## 8. Local Build Result

Codex attempted:

```text
pio run
```

Result in the Codex shell:

```text
pio is not available on PATH
```

Local PlatformIO validation is required in the user's environment.

Current build status for this audit:

```text
pio run: pending local/user validation
```

## 9. Compatibility Statement

Milestone 10.35 is behavior-preserving.

It changes the internal empty-state source path for Capability API reads, but it does not change app-visible Capability API results.

The API still reports:

- empty capability list success
- `capability_not_found` for all capability detail reads
- no values
- no provider metadata
- no quality/diagnostics data

This preserves the transition rule:

```text
The API must not switch from empty to fake.
It must switch only from empty to owner-backed.
```

## 10. Known Limitations

Known limitations after Milestone 10.35:

- `CapabilityDirectory` is a provisional static owner
- no public owner lifetime plan yet
- no capability creation path
- no discovery bridge
- no provider bridge
- no Registry bridge
- no node-to-capability mapping
- no real capability records
- no `CapabilityValueStore`
- no `DiagnosticsStore`
- no Minimal App live transport
- no persistence for public owner records
- no production initialization path for public owner stores

These limitations are intentional. The system is still in the safe empty-owner phase.

## 11. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.37 - Public Owner Lifetime Plan
```

Purpose:

Plan the long-term ownership and initialization model for public owner stores before adding real records.

This should decide whether public owner stores are:

- provisional API statics
- dedicated public owner module instances
- Core context-owned objects
- another explicitly initialized owner model

No real records should be added before the owner lifetime and creation path are approved.

## 12. Stop Conditions

Stop future work and return to architecture review if it tries to:

- create fake capabilities
- create fake `CAP_TEMPERATURE`
- add real records without an owner-backed creation path
- parse packets in API
- read Registry arrays
- call HAL
- call Drivers
- call Devices
- call Modules
- call Services
- call Logic
- call Runtime
- trigger provider selection
- execute commands
- modify `src/main.cpp`
- add live transport
- add WebServer before transport approval
- add HTTP before transport approval
- add JSON before transport approval
- add BLE before transport approval
- add WiFi app transport before transport approval
