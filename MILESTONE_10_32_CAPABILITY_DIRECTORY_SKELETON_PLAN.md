# Milestone 10.32 - CapabilityDirectory Skeleton Plan

## 1. Scope

Milestone 10.32 plans the first narrow implementation skeleton for a bounded `CapabilityDirectory` owner.

This milestone is documentation only.

It does not:

- modify firmware source code
- modify `src/main.cpp`
- create header files
- implement `CapabilityDirectory`
- implement real capability list data
- create public capability arrays in code
- connect `CapabilityDirectory` to `Cyber32Api`
- implement capability values
- implement provider metadata
- implement capability diagnostics
- implement node-to-capability mapping
- change existing API behavior

The goal is to define a safe future skeleton before implementation begins.

## 2. Background

Recent milestone documents establish the owner-backed public data path:

- Public Capability Record Contract exists.
- Public Owner Type Definitions Plan exists.
- Public owner type definitions now exist from Milestone 10.27.
- API transition planning says empty states remain valid.
- Capability API currently returns a safe empty list.
- Capability detail methods currently return `capability_not_found`.
- Node API now has an empty owner-backed `NodeDirectory` path.

The first `CapabilityDirectory` skeleton must not expose fake capabilities.

The first skeleton must be empty by default.

Current empty Capability API behavior is correct architecture, not a bug.

## 3. CapabilityDirectory Purpose

`CapabilityDirectory` is the future bounded owner of public capability records.

It should eventually:

- store public capability records
- expose safe count
- expose safe read by index
- expose safe lookup by capability instance ID later
- enforce capacity
- keep records owner-backed only
- provide deterministic empty state
- preserve `capability_not_found` and empty-list compatibility

`CapabilityDirectory` is not:

- a packet parser
- a Registry array wrapper
- a provider selector
- a driver, device, or module scanner
- a value store
- a diagnostics store
- a transport endpoint
- a UI model

## 4. First Skeleton Behavior

Planned first skeleton behavior:

- default constructed / initialized empty
- `count()` returns `0`
- `isEmpty()` returns true if included
- any read by index fails deterministically
- no fake capability records
- no fake `CAP_TEMPERATURE`
- no value owner
- no provider owner
- no diagnostics owner
- no node-to-capability mapping
- no Registry access
- no packet parsing
- no HAL access
- no Driver access
- no Device access
- no Module access
- no Service access
- no Logic access
- no Runtime access
- no `Cyber32Api` behavior change yet

The first skeleton should have no creation path for real public capabilities unless a later implementation milestone explicitly approves one.

## 5. Future Files Plan

Possible future files:

```text
include/cyber32/public/capability_directory.h
src/public/capability_directory.cpp
```

Alternative project-appropriate placement may be chosen later if it better matches existing source layout.

Do not create these files in this milestone.

Future placement rules:

- keep `CapabilityDirectory` separate from private Registry arrays
- keep public owner types independent from Drivers, Devices, HAL, and transport code
- avoid WiFi, ESP-NOW, Arduino `String`, STL, and heap-oriented dependencies
- keep the skeleton C++11-compatible and ESP32-safe

## 6. Future Public Types Dependency

The future skeleton depends on public owner type definitions from Milestone 10.27.

Required public owner types:

- `PublicCapabilityRecord`
- `PublicCapabilityInstanceId`
- `PublicCapabilityIndex`
- `PublicLifecycleState`
- `PublicVisibilityState`
- `PublicAvailabilityState`
- `PublicFreshnessState`

The skeleton should reuse the existing public owner types.

It must not invent temporary incompatible capability structs that later need replacement.

## 7. Core API Integration Plan

`Cyber32Api` should not be connected to `CapabilityDirectory` in the first skeleton unless a later implementation milestone explicitly approves it.

Future integration order:

1. Implement empty `CapabilityDirectory` skeleton.
2. Validate standalone tests.
3. Connect `getCapabilityList(...)` to `CapabilityDirectory` only while preserving `count = 0`.
4. Connect capability detail methods only after safe read contract exists.
5. Add owner-backed creation path later.
6. Attach values only through `CapabilityValueStore` later.
7. Attach diagnostics only through `DiagnosticsStore` later.

Rules for future API integration:

- current empty-state tests must continue passing
- `capability_not_found` must remain valid
- no fake capabilities may be returned
- no API method may parse packets
- no API method may read private Registry arrays
- no API read may trigger provider selection, discovery, pairing, mapping, or hardware access

## 8. Bounds And Capacity Plan

Future bound decisions:

- maximum public capabilities
- deterministic failure when full
- no heap allocation
- fixed-size array only
- no dynamic strings
- no Arduino `String`
- no STL containers
- bounded copy behavior

Do not choose final numbers in this milestone unless already defined.

The first skeleton may expose capacity only if needed for standalone validation, but capacity must be deterministic and compile-time bounded.

Full-store behavior should later return a compact deterministic failure, not overwrite existing records or allocate more memory.

## 9. Public Operations Plan

Future `CapabilityDirectory` operations conceptually:

- init / reset
- count
- capacity
- isEmpty
- readByIndex
- readById later
- add / update only through approved Core path later
- mark stale later
- disable / hide / block later
- remove later

Recommended first-skeleton operation set:

```cpp
reset()
count()
capacity()
isEmpty()
readByIndex(PublicCapabilityIndex index, PublicCapabilityRecord& out_record)
```

Operations intentionally deferred:

- add capability
- update capability
- provider import
- Registry import
- discovery import
- node mapping
- value attachment
- diagnostics attachment
- API attachment

## 10. Default State Rules

Default state rules:

- reset means empty
- count is `0`
- records are invalid / unavailable by default
- no default record is ever a real capability
- no default `CAP_*` record is real
- read by index fails when empty
- `capability_not_found` remains valid
- empty capability list remains valid
- repeated reads do not mutate state
- capacity query does not mutate state

Default state must be deterministic across construction, reset, and validation.

## 11. Test Plan For Implementation Milestone

Future implementation tests should validate:

- default count is `0`
- reset count is `0`
- `isEmpty()` is true by default
- capacity query is deterministic and greater than `0`
- read index `0` fails when empty
- read out-of-range fails
- failed read returns / defaults unavailable `PublicCapabilityRecord`
- repeated failed reads do not mutate count
- no fake capability validity appears
- no fake `CAP_TEMPERATURE` appears
- no dynamic allocation
- `src/main.cpp` untouched
- existing API tests still pass

No tests are implemented in this milestone.

## 12. Minimal App Implications

Minimal App implications:

- no app change required
- app remains mock-only
- empty capability state remains valid
- no live transport
- no real UI capability cards yet
- no real sensor values yet
- app must not infer capabilities from logs, packets, MAC addresses, provider names, driver names, or debug output
- app must continue to use `Cyber32Api` only

The first `CapabilityDirectory` skeleton should be invisible to the Minimal App until an approved API and transport milestone exposes owner-backed data.

## 13. Safety Invariants

Safety invariants:

- no fake capabilities
- no fake `CAP_TEMPERATURE`
- no fake sensor values
- no fake diagnostics
- no private Registry access
- no private Registry arrays exposed
- no packet parsing
- no HAL calls
- no Driver calls
- no Device calls
- no Module calls
- no Service calls
- no Logic calls
- no Runtime calls
- no provider selection
- no command execution
- no `src/main.cpp` changes
- no app transport
- no WebServer
- no HTTP
- no JSON
- no BLE
- no WiFi app transport

## 14. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.33 - CapabilityDirectory Empty Skeleton Implementation
```

Purpose:

Implement the empty-by-default bounded `CapabilityDirectory` skeleton without exposing fake capabilities or changing existing API behavior.

## 15. Stop Conditions

Stop future work and return to architecture review if it tries to:

- implement `CapabilityDirectory` in this documentation milestone
- expose fake capabilities
- create fake capability records
- create fake `CAP_TEMPERATURE`
- create fake sensor values
- connect to `Cyber32Api` before approval
- parse packets
- read Registry arrays
- call HAL
- call Drivers
- call Devices
- call Modules
- call Services
- call Logic
- call Runtime
- modify `src/main.cpp`
- add live transport
- add WebServer before transport approval
- add HTTP before transport approval
- add JSON before transport approval
- add BLE before transport approval
- add WiFi app transport before transport approval
