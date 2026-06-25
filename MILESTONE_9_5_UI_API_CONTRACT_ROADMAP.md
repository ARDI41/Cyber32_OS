# Milestone 9.5 - UI API Contract Roadmap

## 1. Purpose

Milestone 9.5 defines the roadmap for future internal Cyber32 API contracts used by UI clients and higher-level integrations.

The purpose is to prepare bounded, ESP32-safe API response and request structures before any Dev Panel, Minimal App, Mission Control, Dashboard, Cloud Bridge, Marketplace, or AI Assistant implementation begins.

All future UI clients must use the same Cyber32 API layer.

## 2. Scope

This milestone is documentation only.

The roadmap covers internal API contract expansion for:

- system visibility
- node visibility
- capability visibility
- provider and diagnostics visibility
- project placeholders
- Logic Builder placeholders
- template metadata placeholders
- command request structures
- read-only validation and compile validation

The roadmap does not implement any of these phases yet.

## 3. Non-Goals

Milestone 9.5 does not create headers, source files, API structs, API methods, WebServer handlers, JSON serialization, UI screens, app projects, cloud integrations, marketplace integrations, AI integrations, or command execution behavior.

Non-goals:

- No WebServer
- No HTTP
- No JSON
- No app implementation
- No Dashboard
- No Cloud implementation
- No Marketplace implementation
- No AI implementation
- No Runtime architecture change
- No Registry ownership change
- No Service policy bypass
- No packet ingestion
- No provider ingestion
- No provisioning implementation
- No persistence implementation

## 4. Future UI Clients

Future UI clients include:

- Dev Panel
- Minimal App
- Mission Control
- Dashboard
- Cloud Bridge
- Marketplace
- AI Assistant

Each client is a user of the Cyber32 API, not a privileged architecture layer.

UI clients may display system state, request safe commands, show diagnostics, guide setup, compose projects, build logic rules, and present templates, but they must not access hardware or internal storage directly.

## 5. API-First Rule

All UI clients communicate only through Cyber32 API contracts.

Correct flow:

```text
UI -> Cyber32 API -> Services / Runtime / Logic -> Capability Router -> Provider
```

Forbidden flows:

```text
UI -> Driver
UI -> Device
UI -> Module
UI -> HAL
UI -> Registry array
UI -> wireless internals
```

API rules:

- API reads Registry only where already supported by public Registry APIs.
- API never calls Drivers.
- API never calls Devices.
- API never calls HAL.
- API never parses wireless packets.
- API never writes Registry arrays directly.
- API does not choose providers unless an explicit future API method delegates to Registry public selection APIs.
- API does not own Service policy.
- API does not own Runtime scheduling.

## 6. Capability-First Rule

Cyber32 API contracts must expose stable capability concepts before hardware-specific details.

UI clients should bind to:

- `CAP_TEMPERATURE`
- `CAP_DISTANCE`
- `CAP_SERVO_POSITION`
- `CAP_MOTOR_CONTROL`
- `CAP_RELAY_CONTROL`
- future `CAP_*` IDs

UI clients must not bind core behavior to:

- driver class names
- sensor model names
- module IDs
- transport names
- pins
- raw wireless packet fields
- private Registry array layout

Provider, node, MAC, and diagnostics fields may be exposed through read-only diagnostics contracts, but normal capability reads must remain capability-first.

## 7. Safety and Command-Routing Rule

Read contracts and command contracts must be separated.

Read-only API responses may report system state, node state, capability values, provider health, diagnostics, and project metadata.

Command request structures must never directly drive hardware.

Command flow:

```text
UI command request
-> Cyber32 API
-> Service command policy
-> Runtime safety state
-> bounded command record
-> actuator provider / device path
```

Command rules:

- Commands go through Services only.
- API never bypasses Services for actuator behavior.
- API never calls Drivers, Devices, or HAL directly.
- API command structures must be bounded.
- Motor and motion commands must preserve Emergency Stop and safe-state policy.
- Future command requests must include enough fields for validation, rejection, diagnostics, and auditability.

## 8. Phase-By-Phase Roadmap

### Phase 1 - API Contract Inventory and Naming Rules

Goal: inventory existing API structs and define naming rules before adding more contracts.

Deliverables:

- list existing API response structs
- list existing API methods
- identify naming patterns
- define suffix rules such as `State`, `Diagnostic`, `Summary`, `Request`, and `Result`
- define bounded field rules
- define error-code and result-code conventions

Rules:

- documentation and review first
- no behavior changes
- no source implementation unless a later phase explicitly requests it

### Phase 2 - System API Structs Only

Goal: define bounded system-level response structs.

Candidate contracts:

- system identity summary
- firmware/build summary
- runtime state summary
- mode summary
- memory/storage summary placeholders
- uptime summary

Rules:

- structs only
- read-only
- no WebServer/HTTP/JSON
- no Runtime behavior change

### Phase 3 - Node API Structs Only

Goal: define bounded node-level response structs for wired and wireless nodes.

Candidate contracts:

- node diagnostic
- node summary
- node identity
- node discovery placeholder
- node health placeholder

Fields should prepare for:

- `node_id`
- source MAC for wireless nodes
- provider type
- allow/trust state where applicable
- last seen
- status
- diagnostics

Rules:

- structs only
- no discovery behavior yet
- no allowlist mutation yet
- no packet parsing

### Phase 4 - Capability API Structs Only

Goal: define bounded capability-level response structs for UI capability cards and project views.

Candidate contracts:

- capability state
- capability summary
- capability catalog entry
- capability availability
- capability value card response

Fields should prepare for:

- `capability_id`
- value type
- value
- unit
- availability
- stale state
- last update
- active provider summary

Rules:

- capability-first
- no driver or device details in normal capability reads
- no provider selection changes

### Phase 5 - Provider / Diagnostics API Alignment

Goal: align existing provider diagnostics, wireless node diagnostics, and wireless security diagnostics with future UI needs.

Existing areas:

- provider diagnostics API
- wireless node diagnostics API
- wireless security diagnostics API

Alignment topics:

- provider summary naming
- node diagnostics naming
- security diagnostic naming
- active provider visibility
- accepted/rejected packet counters
- stale/lost provider display

Rules:

- read-only
- no counter reset
- no provider mutation
- no canonical payload mutation
- no wireless packet exposure

### Phase 6 - Project API Placeholders

Goal: reserve bounded placeholder contracts for future Project-oriented UI.

Candidate contracts:

- project summary
- project detail placeholder
- project page summary
- project widget placeholder
- project notes placeholder
- project safety status placeholder

Rules:

- placeholders only
- no storage implementation
- no project persistence
- no app implementation

### Phase 7 - Logic Builder Placeholders

Goal: reserve bounded placeholder contracts for future drag-and-drop Logic Builder.

Candidate contracts:

- logic rule summary
- trigger block placeholder
- condition block placeholder
- action block placeholder
- safety block placeholder
- timer block placeholder
- notification block placeholder

Rules:

- capability-first only
- no arbitrary scripts
- no free-form hardware access
- no Logic runtime implementation
- no command execution bypass

### Phase 8 - Template Metadata Placeholders

Goal: reserve bounded placeholder contracts for future project templates and Template Scripts.

Candidate contracts:

- template summary
- required capability list placeholder
- optional capability list placeholder
- template safety requirements
- template page/widget metadata
- template logic metadata

Rules:

- templates are metadata and project starters
- templates are not privileged code
- templates bind to `CAP_*` IDs
- templates never call Drivers, Devices, HAL, Registry arrays, or transport internals

### Phase 9 - Command Request Structures

Goal: define bounded request structures for future UI commands.

Candidate contracts:

- capability command request
- actuator command request
- safe stop command request
- motor command request
- relay command request
- logic enable/disable request placeholder
- pairing accept/block request placeholder

Rules:

- commands go through Services only
- command requests must be validated before execution
- no API-to-driver path
- no API-to-device path
- no API-to-HAL path
- no direct Registry array writes
- actuator commands must preserve safety policy

### Phase 10 - Read-Only Validation / Compile Validation

Goal: validate that contract structs compile and do not change behavior.

Validation expectations:

- structs are bounded
- structs are C++11 compatible
- no dynamic allocation
- no Arduino `String`
- no STL containers
- normal capability reads remain unchanged
- diagnostics reads remain read-only
- command structs do not execute commands
- no WebServer/HTTP/JSON introduced

## 9. Validation Expectations

Each implementation phase after this roadmap must include validation appropriate to its scope.

For struct-only phases:

- compile validation
- size/boundedness review where useful
- field default behavior if constructors or init helpers exist
- no behavior regression

For read-only API method phases:

- valid read
- missing record
- invalid ID
- out-of-range index
- Registry not attached
- read does not mutate state
- normal capability reads unchanged

For command request phases:

- invalid command rejected
- unsafe command rejected
- missing Service rejected
- Runtime unsafe state rejected
- safe command accepted only through Service policy
- no direct Driver/Device/HAL calls

Build rule:

- PlatformIO build must pass before milestone completion.
- If `pio` is unavailable in the current shell, the limitation must be reported explicitly.

## 10. Stop Conditions

Stop implementation and return to architecture review if any of these occur:

- WebServer is added during contract phases
- HTTP is added during contract phases
- JSON is added during contract phases
- app UI code is added during contract phases
- Dashboard implementation begins during contract phases
- Cloud implementation begins during contract phases
- Marketplace implementation begins during contract phases
- AI implementation begins during contract phases
- API calls Drivers
- API calls Devices
- API calls HAL
- API parses wireless packets
- API writes Registry arrays directly
- Runtime architecture changes
- Registry ownership changes
- Service policy is bypassed
- command requests execute without Services
- STL containers are introduced
- Arduino `String` is introduced
- heap allocation is introduced
- contracts become unbounded
- Logic becomes provider-aware, node-aware, or transport-aware

## 11. Recommended First Implementation Phase

Recommended first implementation phase:

```text
Milestone 9.5 Phase 1 - API contract inventory and naming rules
```

Reason:

Cyber32 already has multiple API response types for capability reads, provider diagnostics, wireless node diagnostics, and wireless security diagnostics. Before adding new structs, the project should inventory existing contracts, align names, identify reusable result/error patterns, and document field conventions.

This keeps future Dev Panel, Minimal App, Mission Control, Dashboard, Cloud Bridge, Marketplace, and AI Assistant work API-first, capability-first, bounded, and consistent.

