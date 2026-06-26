# Milestone 10.3 - Minimal App Transport Boundary Plan

## 1. Scope

Milestone 10.3 defines the future transport boundary between the Minimal App and Cyber32 Core.

This milestone plans transport boundary options only. It does not choose a final transport implementation and does not implement any transport code.

The internal `Cyber32Api` remains the source contract. Any future transport must be an adapter over `Cyber32Api`, not a replacement for `Cyber32Api`.

This plan covers:

- transport boundary principles
- candidate transport options
- recommended first app-development approach
- future transport adapter contract
- serialization boundary
- first Minimal App milestones
- future real-data path
- security and trust notes
- stop conditions

## 2. Non-Goals

Non-goals:

- no WebServer implementation
- no HTTP implementation
- no JSON implementation
- no BLE implementation
- no WiFi transport implementation
- no dashboard transport implementation
- no cloud transport implementation
- no app implementation
- no Dashboard
- no Cloud
- no Marketplace
- no AI implementation
- no packet parsing
- no provider selection
- no Registry direct access
- no Runtime architecture changes
- no Registry ownership changes
- no `src/main.cpp` changes

## 3. Core Principle

`Cyber32Api` is the internal truth.

Future transport adapters may expose API data later, but they must not become a second API, a direct Registry access layer, or a hardware control path.

Transport adapters must not:

- access Registry arrays directly
- parse packets
- parse ESP-NOW payloads
- call Drivers
- call Devices
- call HAL
- call Services directly from UI paths
- choose providers
- mutate Runtime
- mutate Registry
- mutate capability payloads
- invent data

Correct future structure:

```text
Minimal App
-> Transport Adapter
-> Cyber32 API
-> Core OS layers
```

Wrong future structure:

```text
Minimal App
-> HTTP/BLE/JSON handler
-> Registry/private arrays/Drivers/HAL/Packets
```

Transport adapters are protocol surfaces only. They translate between external clients and approved `Cyber32Api` calls.

## 4. Candidate Transport Options

### A. Serial Debug Transport

Description:

A simple serial protocol for early desktop and development testing.

Pros:

- good for early desktop/dev testing
- no WiFi dependency
- easy to inspect manually
- useful for validating API serialization ideas before networking
- keeps mobile networking out of the first experiment

Cons:

- limited for mobile use
- requires USB or serial bridge access
- not suitable for normal customer setup
- lower ergonomics for phone-first workflows

Boundary requirement:

Serial debug transport must still wrap `Cyber32Api` only. It must not parse packets, access Registry arrays, call Drivers, or expose hardware internals.

### B. BLE Transport

Description:

A future local phone-friendly setup and control transport.

Pros:

- good for setup and local phone connection
- useful for provisioning and pairing later
- does not require the Core to already be on local WiFi
- can support local-first onboarding

Cons:

- lower bandwidth than WiFi
- more constrained payload sizes
- requires careful state and pairing design
- not ideal for high-frequency dashboard data

Boundary requirement:

BLE transport must wrap `Cyber32Api` only. BLE characteristics, messages, or services must not read Registry arrays, parse packets, choose providers, or call hardware.

### C. WiFi Local HTTP Transport

Description:

A future local network API surface for web/mobile clients.

Pros:

- convenient for web clients and mobile clients
- familiar development model
- can align with setup AP and local network modes
- easier to test with standard tools once approved

Cons:

- should not be embedded until the API contract is approved
- WebServer, HTTP, and JSON can accidentally become architecture shortcuts
- requires security, pairing, and local network exposure review
- can grow quickly into Dashboard behavior if not bounded

Boundary requirement:

WebServer, HTTP, and JSON must be adapter layers only. They must call approved `Cyber32Api` read or command methods and must not become direct Registry, Driver, Device, HAL, packet, or provider-selection paths.

### D. WebSocket / Local Realtime Transport

Description:

A future realtime local transport for dashboards, live updates, or Mission Control.

Pros:

- good for future live dashboard behavior
- efficient for push-style updates
- useful for status streams, logs, or capability changes later

Cons:

- higher complexity
- requires connection lifecycle handling
- not needed for the first empty-state Minimal App
- can encourage event streaming before ownership and security are ready

Boundary requirement:

Realtime transport must still expose approved API views or future approved event contracts. It must not stream raw packets, private Registry state, or provider internals.

### E. External Bridge / Companion Device

Description:

A future architecture where app transport runs outside the Cyber32 Core firmware.

Pros:

- possible later if Cyber32 Core should remain lean
- useful if richer app transport is better hosted on a companion device
- can keep WebServer, HTTP, JSON, and higher-level UI protocol code out of the constrained Core path
- may simplify experimentation without destabilizing Core OS

Cons:

- adds another component to deploy and trust
- requires bridge-to-Core protocol definition
- may increase setup complexity
- still needs security and pairing design

Boundary requirement:

The bridge must still treat `Cyber32Api` as the source contract. It must not become a privileged backdoor to Registry arrays, Drivers, Devices, HAL, packets, or provider selection.

## 5. Recommended First App-Development Approach

Recommended first approach:

- create Minimal App as a separate project
- use mocked Cyber32 API response models first
- match existing C++ API structs semantically
- implement empty-state screens first
- do not require Core OS transport yet
- later add a transport adapter once the boundary is approved

Rationale:

System API, Node API, and Capability API now have safe read-only empty-state contracts. The app can be designed against those contracts before Core OS exposes any network transport.

This allows UI development to begin while Core OS remains stable, API-first, capability-first, and free of premature WebServer, HTTP, JSON, BLE, or app-specific behavior.

The first Minimal App should validate:

- API-shaped models
- empty node-list state
- empty capability-list state
- missing node detail state
- missing capability detail state
- system summary display
- diagnostics empty-state handling
- no fake sensor values

## 6. Future Transport Adapter Contract

A future transport adapter must expose only approved API views.

### System Views

Allowed System views:

- system summary
- identity
- firmware
- runtime
- modes
- memory

Source methods:

- `getSystemSummary(...)`
- `getSystemIdentity(...)`
- `getSystemFirmware(...)`
- `getSystemRuntime(...)`
- `getSystemModes(...)`
- `getSystemMemory(...)`

### Node Views

Allowed Node views:

- node list
- node detail
- node power
- node signal
- node diagnostics
- node capabilities

Source methods:

- `getNodeList(...)`
- `getNodeSummary(...)`
- `getNodeIdentity(...)`
- `getNodeStatus(...)`
- `getNodePower(...)`
- `getNodeSignal(...)`
- `getNodeDiagnostics(...)`
- `getNodeCapabilities(...)`

### Capability Views

Allowed Capability views:

- capability list
- capability detail
- value
- availability
- provider info
- quality

Source methods:

- `getCapabilityList(...)`
- `getCapabilitySummary(...)`
- `getCapabilityIdentity(...)`
- `getCapabilityValue(...)`
- `getCapabilityAvailability(...)`
- `getCapabilityProviderInfo(...)`
- `getCapabilityQuality(...)`

Transport adapters must preserve:

- `ok` fields
- compact `error_code` values
- empty-list semantics
- `node_not_found` semantics
- `capability_not_found` semantics
- deterministic default semantics
- bounded list counts
- read-only behavior

## 7. Serialization Boundary

Serialization may be needed later, but this milestone does not choose or implement it.

Future serialization options:

### JSON

Pros:

- easy for web/mobile clients
- human-readable
- widely supported

Cons:

- larger payloads
- may tempt WebServer-first architecture
- must be carefully bounded on ESP32

### Compact Binary

Pros:

- small payloads
- efficient on constrained devices
- can map closely to bounded structs

Cons:

- less human-readable
- requires versioning and tooling
- harder to debug manually

### CBOR-like Compact Format

Pros:

- more structured than raw binary
- smaller than JSON
- can preserve typed fields

Cons:

- additional parser/encoder complexity
- must remain bounded
- may be premature for the first app milestone

### Line-Based Debug Format

Pros:

- simple for serial/dev tools
- easy to inspect manually
- good for early experiments

Cons:

- not ideal for mobile app production
- weaker typed structure
- can become ad hoc if not constrained

Serialization rules:

- serialization must be outside Core logic
- serialization must not become packet parsing
- serialization must not expose private Registry state
- serialization must preserve error codes and empty-state semantics
- serialization must preserve bounded counts
- zero defaults on failed reads must not be serialized as valid sensor readings
- provider metadata must remain diagnostic metadata
- command serialization must wait for approved command API milestones

## 8. Minimal App First Milestone Recommendation

The Minimal App may start before Core transport exists by using mock data matching API contracts.

First app screens:

- connection/system screen
- empty node list screen
- empty capability list screen
- capability detail empty-state
- node detail empty-state
- diagnostics empty-state

Mock data should include:

- successful system summary
- empty node list
- empty capability list
- `node_not_found` detail response
- `capability_not_found` detail response
- unavailable provider metadata
- no fake sensor readings

The app should be designed so mock data can later be replaced by live transport-backed API responses without changing screen semantics.

## 9. Future Real-Data Path

Future real-data path:

```text
Internal Cyber32Api already exists
-> transport adapter later exposes approved API views
-> public node owner later fills node list
-> public capability owner later fills capability list
-> canonical capability value read path later fills value cards
-> app switches data source from mock to live API
```

The app should remain unchanged except for transport/data source switching from mock to live API.

Future owner-backed milestones should add:

- public node owner
- discovery owner
- pending pairing API
- node-to-capability mapping owner
- public capability enumeration owner
- Registry-backed safe capability list
- canonical capability value read path
- provider health/freshness owner
- battery/signal diagnostics owner
- friendly-name storage
- owner-backed source MAC metadata

## 10. Security and Trust Notes

Future transport security requirements are conceptual in this milestone.

Security principles:

- local-first operation must not depend on cloud
- read-only screens are safer than command screens
- pairing/trust must be required before remote control later
- remote mode must not require router port forwarding
- Core should initiate outbound cloud connections for future remote access
- command paths must go through Services only in future approved command API milestones
- transport adapters must not create command shortcuts
- transport adapters must not expose raw packets or private internal state

Read-only transport can be planned before command transport, but even read-only transport must preserve Core boundaries and avoid leaking sensitive or unstable internals.

Future command transport requires separate documented milestones for:

- command request contracts
- permission model
- pairing/trust checks
- Runtime readiness checks
- safety gates
- Service routing
- diagnostics and error visibility

## 11. Stop Conditions

Stop future implementation and return to architecture review if work tries to:

- add WebServer now
- add HTTP now
- add JSON now
- add BLE now
- add WiFi app transport now
- put app code into Core OS
- parse packets in transport
- parse ESP-NOW payloads in transport
- access Registry arrays directly
- call Drivers from transport
- call Devices from transport
- call HAL from transport
- select providers in transport
- mutate Runtime from transport
- mutate Registry from transport
- mutate capability payloads from transport
- display deterministic defaults as real data
- modify `src/main.cpp`
- add command execution through transport before command API approval

## 12. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.4 - Minimal App Separate Project Architecture Plan
```

Purpose:

Define the folder/project structure, app technology choice, mock API model, first screens, and development flow for a separate Minimal App project.

Recommended scope:

- documentation only unless explicitly approved otherwise
- keep Minimal App outside Core OS
- define mock API models matching Cyber32 API contracts
- define first screen architecture
- define empty-state test data
- defer live transport implementation

