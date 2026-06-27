# Milestone 10.38 - PublicOwnerStore Skeleton Plan

## 1. Scope

Milestone 10.38 plans a future `PublicOwnerStore` skeleton only.

This milestone is documentation only.

It does not:

* modify firmware source code
* modify `src/main.cpp`
* add headers
* implement `PublicOwnerStore`
* move `NodeDirectory`
* move `CapabilityDirectory`
* change `Cyber32Api`
* add real node records
* add real capability records
* add node or capability creation
* add discovery, provider, Registry, node-to-capability, value, or diagnostics bridges
* parse packets
* add transport, WebServer, HTTP, JSON, BLE, or WiFi behavior
* connect to Minimal App
* expose private Registry arrays
* add fake nodes, fake capabilities, fake `CAP_TEMPERATURE`, fake sensor values, or fake diagnostics
* change existing API behavior

The goal is to plan a central public owner module before adding real records, so public API-safe owner stores have an explicit lifetime model.

## 2. Background

Current public owner state:

* `NodeDirectory` exists and is attached to the Node API as an empty source.
* `CapabilityDirectory` exists and is attached to the Capability API as an empty source.
* both are currently provisional API statics.
* external Node API behavior remains empty/not-found.
* external Capability API behavior remains empty/not-found.
* no real public node records exist yet.
* no real public capability records exist yet.
* no public node-to-capability mapping exists yet.
* no public value store exists yet.
* no public diagnostics store exists yet.

The Public Owner Lifetime Plan recommends not adding real records directly into provisional API statics.

A central public owner store is needed before real records so initialization, reset, read boundaries, and future write/update permissions are explicit and testable.

## 3. PublicOwnerStore Purpose

`PublicOwnerStore` is the future central owner of public API-safe stores.

It should eventually own:

* `NodeDirectory`
* `CapabilityDirectory`
* `NodeCapabilityMap`
* `CapabilityValueStore`
* `DiagnosticsStore`
* possible `ProviderPublicInfoStore` later
* possible `PublicEventSnapshotStore` later

It is not:

* Registry
* Runtime
* Service
* Logic
* Driver
* Device
* HAL
* transport
* UI model
* packet parser
* provider selector

`PublicOwnerStore` exists to provide an explicit owner lifetime for bounded public API-safe state. It does not replace Registry, Runtime, Services, Logic, Drivers, Devices, Modules, HAL, or packet transport.

## 4. First Skeleton Behavior

The first future skeleton should:

* default to empty
* reset all owned public stores to empty
* create no records
* access no hardware
* run no discovery
* import no provider data
* import no Registry data
* change no API behavior yet
* require no `src/main.cpp` change
* add no transport

The skeleton should preserve the current external API behavior:

* Node API list returns empty success
* Node detail methods return `node_not_found`
* Capability API list returns empty success
* Capability detail methods return `capability_not_found`

## 5. Future Files Plan

Possible future files:

* `include/cyber32/public/public_owner_store.h`
* `src/public/public_owner_store.cpp`

Alternative placement may be chosen later if it matches the existing project layout better.

Do not create these files in Milestone 10.38.

## 6. Ownership Contents

Initial future skeleton contents should include:

* `NodeDirectory node_directory`
* `CapabilityDirectory capability_directory`

Deferred contents:

* `NodeCapabilityMap`
* `CapabilityValueStore`
* `DiagnosticsStore`
* `ProviderPublicInfoStore`
* `PublicEventSnapshotStore`

The initial skeleton should own only enough to centralize current empty public owner state. Mapping, value, diagnostics, provider public info, and event snapshot stores require separate approved plans and implementations.

## 7. Public Accessors Plan

Future read-only accessors for API use:

```cpp
const NodeDirectory& nodes() const;
const CapabilityDirectory& capabilities() const;
```

Future mutable/internal accessors only for approved bridge milestones:

```cpp
NodeDirectory& mutableNodes();
CapabilityDirectory& mutableCapabilities();
```

Mutable access must not be used by UI/App/Transport.

Mutable access must not be used inside API read methods to create records.

API reads should observe public owner stores only through read-only accessors once the API migration milestone is approved.

## 8. Reset / Init Plan

Future reset/init behavior:

* constructor/default state is empty
* `reset()` resets `NodeDirectory`
* `reset()` resets `CapabilityDirectory`
* `reset()` later resets mapping/value/diagnostics stores when they exist
* `reset()` does not call hardware
* `reset()` does not parse packets
* `reset()` does not read Registry
* `reset()` does not trigger discovery
* `reset()` does not notify app/transport

No production bootstrap change is approved in this milestone.

## 9. API Integration Plan

Future staged integration:

A. implement `PublicOwnerStore` empty skeleton standalone

B. validate reset and empty accessors

C. add API internal accessor to `PublicOwnerStore`

D. migrate Node API from direct static `NodeDirectory` to `PublicOwnerStore.nodes()`

E. migrate Capability API from direct static `CapabilityDirectory` to `PublicOwnerStore.capabilities()`

F. preserve external empty behavior throughout

G. only later add controlled creation paths

No API integration is performed in Milestone 10.38.

## 10. Dependency Rules

`PublicOwnerStore` may depend on:

* `NodeDirectory`
* `CapabilityDirectory`
* public owner types

It must not depend on:

* Registry internals
* HAL
* Drivers
* Devices
* Modules
* Services
* Logic
* Runtime
* packets
* WiFi
* ESP-NOW
* WebServer
* HTTP
* JSON
* BLE
* Minimal App
* Arduino `String`
* STL containers
* heap allocation

The future implementation must remain bounded and ESP32-safe.

## 11. Validation Plan For Future Implementation

Future validation should prove:

* default store has empty `NodeDirectory`
* default store has empty `CapabilityDirectory`
* reset preserves empty state
* node count remains `0`
* capability count remains `0`
* read-only accessors do not mutate state
* repeated reads are deterministic
* no fake node appears
* no fake capability appears
* no fake `CAP_TEMPERATURE` appears
* no API behavior changes
* `src/main.cpp` remains untouched

No validation code is added in Milestone 10.38.

## 12. Real Record Policy

The `PublicOwnerStore` skeleton must not add real records.

Real records require later approved milestones:

* `NodeDirectory` controlled add path plan
* `NodeDirectory` controlled add path implementation
* `CapabilityDirectory` controlled add path plan
* `CapabilityDirectory` controlled add path implementation
* `NodeCapabilityMap` plan and implementation
* `CapabilityValueStore` plan and implementation
* `DiagnosticsStore` plan and implementation

No real public record should appear without an owner-backed creation/update contract, bounded capacity behavior, deterministic failure behavior, validation coverage, no fake data, and no direct UI/App/Transport write path.

## 13. Minimal App Implications

Minimal App implications:

* no app change is required
* app remains mock-only
* no live transport exists yet
* app must not access `PublicOwnerStore` directly
* app must continue to use `Cyber32Api` only

The Minimal App should continue to treat empty node and capability lists as valid API states until owner-backed real records and an approved transport exist.

## 14. Safety Invariants

Safety invariants:

* no fake nodes
* no fake capabilities
* no fake `CAP_TEMPERATURE`
* no fake sensor values
* no fake diagnostics
* no private Registry arrays exposed
* no packet parsing
* no HAL/Driver/Device/Service/Logic/Runtime calls
* no provider selection
* no command execution
* no `src/main.cpp` changes
* no app transport
* no WebServer/HTTP/JSON/BLE/WiFi

The public owner path must move from empty to owner-backed empty before it ever moves to owner-backed real.

It must never move from empty to fake.

## 15. Open Questions

Open questions:

* final class name: `PublicOwnerStore`, `PublicDataStore`, `Cyber32PublicState`, or other
* whether API owns a static `PublicOwnerStore` accessor
* whether future Core context owns `PublicOwnerStore`
* when provisional API statics are removed
* how tests access reset safely
* which bridge is allowed first to write real records
* whether mutable accessors should be hidden behind friend/internal functions
* whether reset should be public or internal only

## 16. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.39 - PublicOwnerStore Empty Skeleton Implementation
```

Purpose:

Implement an empty-by-default `PublicOwnerStore` skeleton that owns `NodeDirectory` and `CapabilityDirectory` without moving API behavior yet.

## 17. Stop Conditions

Stop future work if it tries to:

* implement `PublicOwnerStore` in this documentation milestone
* move API owners before approval
* add real records
* create fake nodes
* create fake capabilities
* create fake `CAP_TEMPERATURE`
* parse packets
* read Registry arrays
* call HAL/Drivers/Devices/Services/Logic/Runtime
* modify `src/main.cpp`
* add live transport
* connect Minimal App directly to owners
* add WebServer/HTTP/JSON/BLE/WiFi before transport approval
