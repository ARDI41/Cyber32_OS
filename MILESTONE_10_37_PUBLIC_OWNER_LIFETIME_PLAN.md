# Milestone 10.37 - Public Owner Lifetime Plan

## 1. Scope

Milestone 10.37 plans the lifetime, initialization, reset, and ownership model for Cyber32 public owner stores.

This milestone is documentation only.

It does not:

- modify firmware source code
- modify `src/main.cpp`
- add headers
- implement a Core context
- move `NodeDirectory`
- move `CapabilityDirectory`
- add real node records
- add real capability records
- add discovery, provider, Registry, node-to-capability, value, or diagnostics bridges
- parse packets
- add transport, WebServer, HTTP, JSON, BLE, or WiFi behavior
- change existing API behavior

The goal is to define the long-term public owner lifetime model before real records are added.

## 2. Background

Current public owner state:

- public owner types exist
- `NodeDirectory` skeleton exists
- Node API reads from an empty `NodeDirectory`
- `CapabilityDirectory` skeleton exists
- Capability API reads from an empty `CapabilityDirectory`
- both owners are currently provisional internal static API owners
- external API behavior remains empty/not-found
- no real public node records exist yet
- no real public capability records exist yet
- no node-to-capability mapping exists yet
- no public value store exists yet
- no public diagnostics store exists yet

Current external behavior remains correct:

- Node list returns empty success
- Node detail methods return `node_not_found`
- Capability list returns empty success
- Capability detail methods return `capability_not_found`

This empty behavior is architecture, not a bug.

## 3. Problem Statement

A lifetime plan is required before real records are added.

The current provisional static API owners are acceptable for empty attachment because:

- they avoid `src/main.cpp` changes
- they preserve deterministic empty state
- they are easy to validate
- they do not create records
- they do not expose fake data

However, real records require explicit ownership because:

- initialization order must be deterministic
- reset behavior must be defined
- creation paths must be deliberate and testable
- records must not be created inside API read methods
- transport and app layers must not initialize public stores
- API must remain read-only from the public owner perspective
- future bridges need approved write/update boundaries
- static hidden lifetime becomes risky once state carries real records

The next real-data phase must not smuggle creation through API statics, packet reads, Registry internals, or UI pressure.

## 4. Public Owner Stores In Scope

This plan covers public API-safe owner stores only.

Current stores:

- `NodeDirectory`
- `CapabilityDirectory`

Future stores:

- `NodeCapabilityMap`
- `CapabilityValueStore`
- `DiagnosticsStore`
- possible `ProviderPublicInfoStore`
- possible `PublicEventSnapshotStore`

These stores are distinct from private Registry internals.

Public owner stores expose bounded, API-safe views. They do not replace Registry, Runtime, Services, Logic, Drivers, Devices, Modules, HAL, or packet transport.

## 5. Ownership Model Options

### A. API Static Owners

Description:

Public stores live as internal static objects inside the API implementation.

Benefits:

- smallest implementation footprint
- no bootstrap changes
- no `src/main.cpp` changes
- simple empty-state validation
- suitable for current empty attachment

Risks:

- hidden lifetime once records become real
- unclear reset ownership
- difficult to coordinate multiple public stores
- write paths could accidentally appear inside API
- production initialization remains implicit

Impact on `src/main.cpp`:

- none for empty phase

Testability:

- good for empty-state validation
- weaker for controlled real-record lifecycle validation

Suitability for empty phase:

- acceptable

Suitability for real records:

- not recommended without explicit approval

### B. Dedicated Public Owner Module

Description:

A dedicated module owns all public owner stores and exposes reset/init, read accessors, and approved bounded update methods.

Benefits:

- clear public-state ownership
- one place for reset and initialization
- can group `NodeDirectory`, `CapabilityDirectory`, `NodeCapabilityMap`, `CapabilityValueStore`, and `DiagnosticsStore`
- keeps API reads separate from record creation
- easy to validate as a bounded owner

Risks:

- requires new firmware structure in a future milestone
- needs lifetime and dependency decisions
- may eventually require bootstrap integration

Impact on `src/main.cpp`:

- none if introduced only in validation or API-internal tests
- possible future bootstrap change only with explicit approval

Testability:

- strong
- can validate default empty state, reset, and controlled writes without hardware

Suitability for empty phase:

- good, but more structure than strictly needed

Suitability for real records:

- recommended first real-record ownership step

### C. Runtime-Owned Public Stores

Description:

Runtime owns public stores directly.

Benefits:

- Runtime already has a defined firmware lifetime
- may simplify future scheduling coordination
- could allow public snapshots to align with runtime ticks later

Risks:

- blurs Runtime responsibility
- Runtime should remain scheduler/state owner, not public API data owner
- may create pressure for Runtime to own public policy
- could complicate tests and architecture boundaries
- may encourage API reads to depend on Runtime state

Impact on `src/main.cpp`:

- likely requires broader bootstrap changes later

Testability:

- weaker than a dedicated public owner module for early owner validation
- may require Runtime construction/setup to test public stores

Suitability for empty phase:

- not needed

Suitability for real records:

- not recommended for first real-record phase unless explicitly approved later

### D. Core Context Object

Description:

A future Core context object owns public stores and possibly other top-level system components.

Benefits:

- explicit lifetime
- can own multiple public stores consistently
- avoids hidden API statics
- provides clean dependency injection into API later
- can become production-grade owner model

Risks:

- broader architectural change
- may require `src/main.cpp` or bootstrap approval
- may be too large before the first controlled public record tests

Impact on `src/main.cpp`:

- likely future impact if used as production bootstrap owner
- no impact in this documentation milestone

Testability:

- strong if context can be constructed in validation without hardware
- weaker if tied too early to main firmware setup

Suitability for empty phase:

- possible but larger than needed

Suitability for real records:

- strong long-term option after explicit planning

### E. Hybrid Phased Model

Description:

Use provisional API statics only for empty attachment, then introduce a dedicated public owner module before real records, and later migrate into a Core context if needed.

Benefits:

- preserves current working empty behavior
- avoids risky broad refactor now
- creates a safer path before real records
- allows validation of owner module independently
- supports later Core context migration

Risks:

- temporary duplication/migration work
- requires discipline to prevent real records from being added to provisional API statics

Impact on `src/main.cpp`:

- none during planning and standalone skeleton phases
- possible later only with explicit milestone approval

Testability:

- strong
- supports isolated public owner validation before live integration

Suitability for empty phase:

- best fit for current project

Suitability for real records:

- recommended path

## 6. Recommended Phased Model

Recommended model:

Phase A:
Current provisional API statics are allowed only for empty attachment.

Phase B:
Introduce a dedicated public owner module before adding real records.

Phase C:
The public owner module owns:

- `NodeDirectory`
- `CapabilityDirectory`
- `NodeCapabilityMap`
- `CapabilityValueStore`
- `DiagnosticsStore`

Phase D:
`Cyber32Api` reads from the public owner module through read-only accessors.

Phase E:
Approved Core bridges may write/update public stores through bounded methods.

Phase F:
A future Core context or bootstrap owner may later own the public owner module if explicitly approved.

Rules:

- real record creation must not be added directly into API read methods
- real record creation must not be hidden inside API static owners unless explicitly approved
- API reads remain read-only
- UI/App/Transport never write to public owner stores directly
- empty behavior remains valid until owner-backed records exist

## 7. Public Owner Module Concept

Define a future module concept named provisionally:

```text
PublicOwnerStore
```

Alternative names may be:

- `PublicDataStore`
- `Cyber32PublicState`
- `PublicStateStore`

The module should eventually:

- own `NodeDirectory`
- own `CapabilityDirectory`
- own `NodeCapabilityMap`
- own `CapabilityValueStore`
- own `DiagnosticsStore`
- expose reset/init
- expose read-only accessors for `Cyber32Api`
- expose write/update methods only to approved Core bridge milestones
- remain independent from UI/App/Transport
- remain independent from HAL, Drivers, Devices, Services, Logic, Runtime, packets, WiFi, BLE, WebServer, and HTTP
- avoid heap allocation
- avoid Arduino `String`
- avoid STL containers

The module must not:

- parse packets
- read Registry arrays directly
- call HAL
- call Drivers
- call Devices
- call Services
- call Logic
- call Runtime
- select providers
- execute commands
- fabricate public records

Do not implement this module in Milestone 10.37.

## 8. Initialization Rules

Rules:

- default state is empty
- construction is deterministic
- explicit reset sets all public stores empty/unavailable
- no hidden initialization from API reads
- no hidden initialization from app/transport
- no hidden initialization from WebServer/HTTP/JSON/BLE/WiFi
- no `src/main.cpp` change unless a future milestone explicitly approves it
- no heap allocation
- no Arduino `String`
- no STL containers

Future initialization may be:

- standalone validation construction
- API-internal owner accessor
- dedicated public owner module accessor
- later Core context bootstrap

But no production bootstrap change is approved in this milestone.

## 9. Reset Rules

Rules:

* reset clears `NodeDirectory`
* reset clears `CapabilityDirectory`
* reset clears `NodeCapabilityMap` when it exists
* reset clears `CapabilityValueStore` when it exists
* reset clears `DiagnosticsStore` when it exists
* reset must not touch hardware
* reset must not clear private Registry unless explicitly approved
* reset must not parse packets
* reset must not trigger discovery
* reset must not select providers
* reset must not notify app/transport in this phase
* reset must preserve deterministic empty public API behavior

## 10. Read/Write Boundary

Read boundary:

* `Cyber32Api` reads public owner stores only
* `Cyber32Api` does not create records during read methods
* `Cyber32Api` does not trigger discovery during reads
* `Cyber32Api` does not call HAL, Drivers, Devices, Modules, Services, Logic, or Runtime during public owner reads

Write boundary:

* UI/App/Transport may not write public owner stores
* UI/App/Transport may call only `Cyber32Api` public methods
* approved Core bridge milestones may write into public stores later
* discovery/provider bridges may write only through approved bounded methods later
* Registry/HAL/Drivers/Devices/Modules/Services/Logic must not be called directly by API reads

## 11. Future Creation Path Policy

Real records require separate approved milestones:

* `NodeDirectory` controlled add path plan
* `NodeDirectory` controlled add path implementation
* `CapabilityDirectory` controlled add path plan
* `CapabilityDirectory` controlled add path implementation
* `NodeCapabilityMap` skeleton plan
* `NodeCapabilityMap` implementation
* `CapabilityValueStore` skeleton plan
* `CapabilityValueStore` implementation
* `DiagnosticsStore` skeleton plan
* `DiagnosticsStore` implementation

No real public record should appear without:

* owner-backed creation/update contract
* bounded capacity behavior
* deterministic failure behavior
* validation coverage
* no fake data
* no direct UI/App/Transport write path

## 12. Compatibility With Current API

Current behavior remains valid:

* Node API list returns empty success
* Node detail methods return `node_not_found`
* Capability API list returns empty success
* Capability detail methods return `capability_not_found`
* no values are exposed
* no provider metadata is exposed
* no diagnostics are exposed
* Minimal App remains mock-only
* no transport is added

The API transition remains:

```text
empty -> owner-backed empty -> owner-backed real
```

Never:

```text
empty -> fake
```

## 13. Minimal App Implications

Rules:

* no app change required
* no transport yet
* app remains mock-only
* app must not access owner stores directly
* app must not infer records from debug output
* app must not infer records from packets
* app must not infer records from MAC addresses
* app must not infer records from Registry
* app must not infer records from providers
* app must not infer records from logs
* future live app still reads only through `Cyber32Api` or an approved transport wrapper

## 14. Safety Invariants

The permanent safety rules remain:

* no fake nodes
* no fake capabilities
* no fake `CAP_TEMPERATURE`
* no fake sensor values
* no fake diagnostics
* no private Registry arrays exposed
* no packet parsing in API
* no HAL calls from API reads
* no Driver calls from API reads
* no Device calls from API reads
* no Module calls from API reads
* no Service calls from API reads
* no Logic calls from API reads
* no Runtime calls from API reads
* no provider selection from API reads
* no command execution
* no `src/main.cpp` changes in this milestone
* no app transport
* no WebServer/HTTP/JSON/BLE/WiFi

## 15. Open Questions

Open questions before real records:

* final public owner module name
* whether Core context is needed before real records
* whether API should receive owner references or use accessors
* when provisional API statics must be removed
* how public owner reset is triggered
* how discovery bridge is allowed to write nodes
* how provider bridge is allowed to write capabilities
* how value and diagnostics stores receive updates
* whether public stores should support remove or only disable/hide initially
* how owner lifetime is tested without `src/main.cpp` changes
* whether reset should be public, internal, or test-only initially
* whether mutable accessors should be hidden behind internal bridge APIs

## 16. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.38 - PublicOwnerStore Skeleton Plan
```

Purpose:

Plan an empty-by-default `PublicOwnerStore` skeleton that can eventually own `NodeDirectory` and `CapabilityDirectory` before real records are added.

Alternative:

```text
Milestone 10.38 - NodeDirectory Controlled Add Path Plan
```

But the recommended path is `PublicOwnerStore` first, because real records should not be added directly into provisional API statics.

## 17. Stop Conditions

Stop future work if it tries to:

* add real records before lifetime/creation path approval
* create fake nodes
* create fake capabilities
* create fake `CAP_TEMPERATURE`
* create fake sensor values
* create fake diagnostics
* parse packets in API
* read Registry arrays from API
* call HAL from API reads
* call Drivers from API reads
* call Devices from API reads
* call Modules from API reads
* call Services from API reads
* call Logic from API reads
* call Runtime from API reads
* trigger provider selection
* execute commands
* modify `src/main.cpp` without explicit milestone approval
* add live transport
* connect Minimal App directly to owners
* add WebServer/HTTP/JSON/BLE/WiFi before transport approval
