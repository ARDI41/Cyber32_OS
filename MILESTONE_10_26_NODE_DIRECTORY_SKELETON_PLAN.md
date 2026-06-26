# Milestone 10.26 - NodeDirectory Skeleton Plan

## 1. Scope

Milestone 10.26 plans the first narrow implementation skeleton for a bounded `NodeDirectory` owner.

This milestone is documentation only.

It defines how a future empty-by-default `NodeDirectory` skeleton should behave before it is connected to Cyber32Api or any real discovery path.

This milestone does not:

- implement `NodeDirectory`
- add header files
- add source files
- implement real node list data
- create public node arrays in code
- connect `NodeDirectory` to Cyber32Api
- change API behavior
- change source code
- modify `src/main.cpp`

## 2. Background

Recent plans establish the owner-backed public data path:

- Public Node Record Contract exists.
- Public Owner Type Definitions Plan exists.
- API transition plan says empty states remain valid.
- Node API currently returns safe empty list / `node_not_found`.
- first `NodeDirectory` skeleton must not expose fake nodes.
- first `NodeDirectory` skeleton must be empty by default.

The first skeleton should prove ownership shape and empty-state behavior only.

It must not create real records, infer records from packets, read Registry internals, or change the current API result surface.

## 3. NodeDirectory Purpose

`NodeDirectory` is the conceptual future bounded owner of public node records.

It should eventually:

- store public node records
- expose safe count
- expose safe read by index
- expose safe lookup by node ID later
- enforce capacity
- keep records owner-backed only
- provide deterministic empty state
- prevent fake nodes
- preserve `node_not_found` and empty-list compatibility

`NodeDirectory` is not:

- a packet parser
- a Registry array wrapper
- a provider selector
- a discovery engine
- a pairing engine
- a transport endpoint
- a UI model

## 4. First Skeleton Behavior

Planned first skeleton behavior:

- default constructed / initialized empty
- `count` returns `0`
- `isEmpty` returns true if included
- any read by index fails deterministically
- no fake node records
- no discovery bridge
- no pairing bridge
- no Registry access
- no packet parsing
- no HAL access
- no Driver access
- no Device access
- no Service access
- no Logic access
- no Cyber32Api behavior change yet

The first skeleton should have no creation path for real public nodes unless a later implementation milestone explicitly approves one.

## 5. Future Files Plan

Possible future files:

```text
include/cyber32/public/node_directory.h
src/public/node_directory.cpp
```

Alternative project-appropriate placement may be chosen later if it better matches existing source layout.

Do not create files in this milestone.

Future placement rules:

- keep `NodeDirectory` separate from private Registry arrays.
- keep public owner types independent from Drivers, Devices, HAL, and transport code.
- avoid WiFi, ESP-NOW, Arduino `String`, STL, and heap-oriented dependencies.
- keep the skeleton C++11-compatible and ESP32-safe.

## 6. Future Public Types Dependency

The future skeleton depends on public owner type definitions from the type plan.

Conceptual dependencies:

- `PublicNodeRecord`
- `PublicNodeId`
- `PublicNodeIndex`
- `PublicLifecycleState`
- `PublicVisibilityState`
- `PublicTrustState`
- `PublicFreshnessState`

Type definitions should be implemented before or together with the skeleton only after explicit approval.

The skeleton should not invent temporary incompatible node structs that later need replacement.

If type definitions are not yet approved or implemented, the next implementation milestone should define the minimal firmware-safe public owner types first.

## 7. Core API Integration Plan

Cyber32Api should not be connected to `NodeDirectory` in the first skeleton unless a later implementation milestone explicitly approves it.

Future integration order:

1. implement empty `NodeDirectory` skeleton
2. validate standalone tests
3. connect `getNodeList(...)` to `NodeDirectory` only while preserving `count = 0`
4. connect node detail methods only after safe read contract exists
5. add owner-backed creation path later

Rules for future integration:

- current empty-state tests must continue passing.
- `node_not_found` must remain valid.
- no fake nodes may be returned.
- no API method may parse packets.
- no API method may read private Registry arrays.
- no API read may trigger discovery, pairing, provider selection, or hardware access.

## 8. Bounds and Capacity Plan

Future bound decisions:

- max public nodes
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

Future `NodeDirectory` operations conceptually:

- init / reset
- count
- capacity
- isEmpty
- readByIndex
- readById later
- add / update only through approved Core path later
- mark stale later
- remove / hide / block later

First skeleton should include only the minimum operations needed for empty standalone validation.

Recommended first-skeleton operation set:

- reset / begin equivalent
- count
- capacity if useful
- isEmpty if useful
- readByIndex returning deterministic failure when empty

Operations intentionally deferred:

- add node
- update node
- discovery import
- pairing import
- trust import
- stale management
- removal / hide / block
- API attachment

## 10. Default State Rules

Default state rules:

- reset means empty
- count is `0`
- records are invalid / unavailable by default
- no default record is ever a real node
- read by index fails when empty
- `not_found` remains valid
- empty list remains valid
- repeated reads do not mutate state
- capacity query does not mutate state

Default state must be deterministic across construction, reset, and validation.

## 11. Test Plan for Implementation Milestone

Future implementation tests should validate:

- default count is `0`
- reset count is `0`
- read index `0` fails when empty
- read out-of-range fails
- capacity query is deterministic
- no fake node data is returned
- repeated reads do not mutate state
- no dynamic allocation
- no Arduino `String`
- no STL containers
- `src/main.cpp` untouched
- existing API tests still pass

No tests are implemented in this milestone.

## 12. Minimal App Implications

Minimal App implications:

- no app change required
- app remains mock-only
- empty node state remains valid
- no live transport
- no real UI node cards yet
- app must not infer nodes from logs, packets, MAC addresses, or debug output
- app must continue to use Cyber32Api only

The first `NodeDirectory` skeleton should be invisible to the Minimal App until an approved API and transport milestone exposes owner-backed data.

## 13. Safety Invariants

Safety invariants:

- no fake nodes
- no fake capabilities
- no fake sensor values
- no fake diagnostics
- no private Registry access
- no private Registry arrays exposed
- no packet parsing
- no HAL calls
- no Driver calls
- no Device calls
- no Service calls
- no Logic calls
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
Milestone 10.27 - Public Owner Type Definitions Skeleton
```

Purpose:

Implement the minimal firmware-safe public owner type definitions needed by the `NodeDirectory` skeleton, with deterministic defaults and no owner stores yet.

Alternative if type definitions are already implemented:

```text
Milestone 10.27 - NodeDirectory Empty Skeleton Implementation
```

Purpose:

Implement the empty-by-default bounded `NodeDirectory` skeleton without exposing fake nodes or changing existing API behavior.

## 15. Stop Conditions

Stop future work and return to architecture review if it tries to:

- implement `NodeDirectory` in this documentation milestone
- expose fake nodes
- create fake node records
- connect to Cyber32Api before approval
- parse packets
- read Registry arrays
- call HAL
- call Drivers
- call Devices
- call Services
- call Logic
- modify `src/main.cpp`
- add live transport
- add WebServer before transport approval
- add HTTP before transport approval
- add JSON before transport approval
- add BLE before transport approval
- add WiFi app transport before transport approval
