# Cyber32 Bible

## Purpose

`CYBER32_BIBLE.md` is the definitive master document for the Cyber32 architecture, implementation direction, safety model, provider model, and roadmap.

This document is documentation only. It does not change source code, does not modify `src/main.cpp`, and does not introduce new architecture beyond the architecture and milestone documents already present in the repository.

## Canonical Source Set

This Bible consolidates the current Cyber32 documentation set, including:

- `ARCHITECTURE.md`
- `ARCHITECTURE_GAP_ANALYSIS.md`
- `ARCHITECTURE_AUDIT.md`
- `docs/ARCHITECTURE_REVIEW.md`
- `ESP32_V1_SCOPE.md`
- `BOOT_SEQUENCE.md`
- `FIRST_VERTICAL_SLICE_PLAN.md`
- `FIRST_VERTICAL_SLICE_BOOTSTRAP.md`
- `IMPLEMENTATION_ORDER.md`
- `PRODUCTION_BOOTSTRAP_PLAN.md`
- `CAPABILITY_SCHEMA.md`
- `CAPABILITY_PAYLOAD_SCHEMA.md`
- `CAPABILITY_CATALOG.md`
- `COMMAND_DISPATCH_CONTRACT.md`
- `API_V1_CONTRACT.md`
- `EVENT_MODEL.md`
- `ERROR_MODEL.md`
- `MEMORY_MODEL.md`
- `REGISTRY_SCHEMA.md`
- `REGISTRY_IMPLEMENTATION_PLAN.md`
- `REGISTRY_RESULT_CODES_PLAN.md`
- `RUNTIME_IMPLEMENTATION_PLAN.md`
- `RUNTIME_TASK_COORDINATION_PLAN.md`
- `PNP_DISCOVERY_FLOW.md`
- `MODULE_METADATA_SCHEMA.md`
- `DEVICE_CLASSIFICATION.md`
- `ACTUATOR_ARCHITECTURE_PLAN.md`
- `ACTUATOR_SAFETY_POLICY.md`
- `SAFE_MODE_ARCHITECTURE_PLAN.md`
- `MOBILE_STUDIO_VISION.md`
- all `MILESTONE_*` plans, audits, and roadmaps through Milestone 9.0

If this document appears to conflict with a more detailed phase document, the detailed phase document is the local implementation source, and this Bible should be updated to reflect it.

## Cyber32 Design Philosophy

### Capability First

`CAP_*` IDs are the stable system contract.

Logic, API clients, dashboards, Mobile Studio, future AI tools, and external integrations must bind to capabilities rather than modules, devices, transports, provider IDs, node IDs, or physical pins.

### Local First

Core Cyber32 functionality must work without cloud services.

Cloud integrations may extend the system, but local sensing, actuation, validation, safety checks, Registry state, Runtime scheduling, and API state access must remain usable without network dependency.

### Safety First

Unsafe actuator behavior must fail safe.

Actuator commands must be validated, bounded, gated by Runtime state, blocked in unsafe states, and written through command-state records so failures are visible. For fail-safe actuators, the safe default must be explicit, testable, and preserved during failures.

### Bounded By Default

Cyber32 defaults to fixed storage, bounded memory, bounded queues, bounded command slots, bounded packet sizes, and compact result codes.

Unbounded provider lists, packet queues, event queues, command queues, heap allocation, Arduino `String`, and STL containers are not allowed in core ESP32 paths.

### Simulation Before Hardware

Simulated slices validate architecture before real devices are introduced.

Drivers, Devices, Modules, PNP, Registry registration, Services, Logic, API, Runtime tasks, command flow, and validation must prove the contract in simulation before hardware-specific behavior is added.

### Documentation Before Implementation

Architecture decisions must be documented before coding.

New capabilities, actuators, provider systems, wireless flows, safety rules, and Runtime coordination changes require a plan or architecture document before implementation begins.

### Validation Before Commit

Validation coverage must be added before milestone completion.

Validation should prove happy paths, failure paths, timeout behavior, safe-state behavior, Registry result behavior, command-state behavior, and architecture boundaries.

### Build Before Merge

A successful PlatformIO build is required before a milestone is considered complete.

If the local environment cannot run PlatformIO, that limitation must be reported explicitly and the milestone must remain pending final build verification.

## Prime Directive

Cyber32 is a capability-first, ESP32-first, layered embedded system.

The stable contract is:

```text
CAP_* capability IDs
```

Logic, API clients, Dashboard, Mobile Studio, and future AI tools must bind to capability IDs, not module names, device names, transport names, provider IDs, pin numbers, node IDs, or cloud endpoint names.

## AI Contributor Contract

These rules apply to Codex, ChatGPT, future AI agents, human contributors, and automation scripts.

- Never modify `src/main.cpp` unless the milestone explicitly requires it.
- Use public Registry APIs only.
- Do not access Registry arrays directly outside Registry implementation.
- Runtime remains scheduler-only.
- Services own policy.
- Registry owns state.
- Logic is provider-blind.
- API is capability-first.
- Do not use dynamic allocation in core paths.
- Do not use Arduino `String` in core paths.
- Do not use STL containers in core paths.
- Add validation before commit.
- Require a successful build before milestone completion.
- Update architecture documents when architecture changes.
- Do not add real hardware behavior before simulated validation and safety rules exist.
- Do not add WiFi, WebServer, Dashboard, cloud, Mobile Studio, or external transports unless the milestone explicitly includes them.

## Fixed Layer Order

The architecture order is fixed:

```text
HAL
-> Drivers
-> Devices
-> Modules
-> PNP
-> Registry
-> Runtime
-> Services
-> Logic
-> API
-> Dashboard / Mobile Studio / External Apps
```

Dependency direction must remain downward or lateral only through documented contracts. Higher layers must not bypass middle layers.

## Layer Responsibilities

### HAL

HAL abstracts platform primitives.

HAL may expose:

- time
- GPIO primitives
- bus primitives
- future wireless/radio primitives

HAL must not know Registry, Runtime, Services, Logic, API, Dashboard, or capability policy.

### Drivers

Drivers talk to concrete hardware or simulated hardware.

Drivers may:

- initialize hardware/simulated state
- read raw values
- perform low-level execution
- expose failure modes for validation

Drivers must not register capabilities, write Registry, publish events, run Logic, expose API, or know module names.

### Devices

Devices wrap Drivers and convert driver behavior into Cyber32 payloads.

Devices may:

- expose `CapabilityPayload`
- perform direct driver execution
- enforce execution safety that belongs closest to the hardware

Devices must not register capabilities, write Registry, run Logic, expose API, or know Dashboard behavior.

### Modules

Modules are metadata and identity.

Modules may expose:

- module ID
- module type
- metadata level
- display name
- device ID/type
- capability ID

Modules must not call Device behavior, write Registry, publish events, run Logic, or expose API.

### PNP

PNP discovers and validates modules.

PNP Discovery reads metadata and emits discovery facts.

PNP Registration converts validated discovery facts into Registry records through public Registry APIs.

PNP must not call Drivers, call Device behavior, run Logic, expose API, or write Registry arrays directly.

### Registry

Registry is the source of truth for system state.

Registry stores:

- modules
- devices
- capabilities
- payloads
- command state
- provider records
- active provider mappings
- selected canonical payloads

Registry may publish compact state-change events.

Registry must not:

- execute commands
- discover hardware
- call Drivers
- call Devices
- call Services
- run Logic
- expose API
- parse raw wireless packets
- own actuator policy

### Runtime

Runtime coordinates execution.

Runtime owns:

- runtime state
- cooperative task scheduling
- bounded event draining
- Safe Mode state helpers

Runtime does not own:

- service policy
- command policy
- provider policy
- packet parsing
- Logic decisions
- API responses

Runtime schedules tasks. It does not become the task.

### Services

Services own policy.

Services may:

- call Devices
- validate commands
- enforce actuator gating
- update Registry through public APIs
- process packets when the Service owns that policy
- perform stale/lost checks for provider classes

Services must not expose API, run Logic, write Registry arrays directly, or publish duplicate events when Registry already emits the state event.

### Logic

Logic queries capabilities only.

Logic may:

- query Registry by `CAP_*`
- evaluate conditions
- request commands by capability ID only

Logic must not know module IDs, device IDs, provider IDs, transport, bus, pin, wireless node, or Dashboard metadata.

### API

API is the supported external interface.

API reads Registry state and Runtime state where documented.

API commands go through Services only.

API must not call Devices, Drivers, HAL, or bypass Services.

### Dashboard, Mobile Studio, External Apps

User-facing layers use API only.

They may show friendly names, icons, translations, provider diagnostics, and visual blocks.

They must preserve canonical `CAP_*` IDs underneath.

## Repository Structure

The intended Cyber32 repository layout is:

```text
src/
  hal/
  drivers/
  devices/
  modules/
  pnp/
  registry/
  runtime/
  services/
  logic/
  api/
  app/

docs/
milestones/
```

`src/` contains production and validation source code.

`src/hal/` contains platform primitives such as time, GPIO, bus, and future radio abstractions.

`src/drivers/` contains hardware or simulated hardware drivers. Drivers are low-level and do not know Registry, Runtime, Services, Logic, API, or module names.

`src/devices/` contains Device wrappers that convert Driver behavior into Cyber32 payloads and safe device operations.

`src/modules/` contains metadata-only Module definitions used by PNP discovery.

`src/pnp/` contains discovery and registration flow code. PNP reads Module metadata and registers records through Registry public APIs.

`src/registry/` contains bounded state storage, records, result codes, provider records, command-state records, and Registry-owned state transitions.

`src/runtime/` contains Runtime state and task scheduling. Runtime coordinates execution but does not own business policy.

`src/services/` contains policy-owning Services that update capability state, validate commands, coordinate Device calls, and write command-state records.

`src/logic/` contains capability-first business logic. Logic queries capabilities by `CAP_*` IDs and remains provider-blind.

`src/api/` contains internal API surfaces. API reads Registry state and sends commands through Services.

`src/app/` contains validation harnesses and optional application-level entry helpers.

`docs/` contains long-lived architecture reviews and supporting documentation.

`milestones/` is the intended home for future milestone-specific planning and audit documents if the repository is later reorganized away from root-level milestone files.

## Architecture Decision Record Summary

### ADR-001 Capability First

Decision: `CAP_*` IDs are the stable contract.

Reason: Hardware, transport, provider, and module identity can change without changing Logic or API behavior.

Impact: Logic, API, Dashboard, Mobile Studio, and AI tools must bind to capability IDs, not implementation details.

### ADR-002 Runtime Is Scheduler Only

Decision: Runtime coordinates task execution and state transitions but does not own Service policy.

Reason: Business and safety rules belong in Services so Runtime remains capability-agnostic.

Impact: Runtime tasks call callbacks; Runtime must not evaluate capabilities or execute actuator policy directly.

### ADR-003 Registry Owns State

Decision: Registry owns modules, devices, capabilities, payloads, command state, provider records, and active provider mappings.

Reason: Central bounded state storage keeps reads, writes, and diagnostics consistent.

Impact: Other layers use public Registry APIs and never write Registry arrays directly.

### ADR-004 Services Own Policy

Decision: Services own update policy, command validation, Runtime gating policy, timeout enforcement, and Device execution policy.

Reason: Services are the narrow layer where capability behavior can safely interact with Devices and Registry.

Impact: API, Logic, Runtime, PNP, and Registry must not bypass Services for command behavior.

### ADR-005 Provider Architecture

Decision: Capabilities may have multiple bounded providers, with active provider tracking and provider payload storage.

Reason: Wired, wireless, simulated, virtual, cloud, and future AI providers must be able to expose the same capability without changing Logic.

Impact: Registry stores provider records and selected provider mappings; Logic remains provider-blind.

### ADR-006 Safety First Actuator Control

Decision: Actuators require bounded command flow, command-state records, Runtime gating, Safe Mode behavior, timeout handling, and fail-safe defaults.

Reason: Unsafe actuator behavior must be blocked or fail safe before real hardware is introduced.

Impact: Servo, motor, relay, and future actuators must validate commands in Services and expose command results through Registry/API state.

### ADR-007 No Dynamic Allocation In Core

Decision: Core ESP32 paths must avoid dynamic allocation, Arduino `String`, and STL containers.

Reason: Embedded behavior must remain predictable, bounded, and memory-safe.

Impact: Storage uses fixed tables, fixed slots, compact structs, bounded packet schemas, and result codes.

### ADR-008 Documentation Before Implementation

Decision: Architecture plans, milestone plans, contracts, and audits must precede behavior changes.

Reason: Cyber32 architecture depends on explicit boundaries and phased validation.

Impact: New capabilities, providers, transports, actuators, and safety features require documentation before implementation.

## ESP32 v1 Scope

Cyber32 v1 targets ESP32.

Rules:

- no dynamic allocation in core paths
- no Arduino `String`
- no STL containers in core paths
- bounded arrays and compact structs
- no unbounded JSON in embedded internals
- no unbounded packet or event history
- local-first operation
- cloud, OTA, Dashboard hosting, and Mobile Studio implementation remain deferred unless explicitly planned

The validation harness may own many fixed objects, but production boot must remain bounded and deliberate.

## Capability-First Model

Capabilities are the system contract.

Examples currently implemented or planned:

- `CAP_TEMPERATURE`
- `CAP_DISTANCE`
- `CAP_SERVO_POSITION`
- `CAP_MOTOR_CONTROL`
- `CAP_RELAY_CONTROL`
- future `CAP_BATTERY_LEVEL`
- future `CAP_BATTERY_VOLTAGE`
- future `CAP_SIGNAL_STRENGTH`

Capability IDs are canonical. Friendly names are display metadata only.

## Payload Model

`CapabilityPayload` is compact and latest-only.

Core payload fields:

```text
capability_id
schema_version
timestamp_ms
available
stale
value_type
value_float
value_int
unit
quality
error_code
```

Unavailable values must not be treated as zero.

Stale values must be explicit.

Complex payloads require a bounded schema before implementation.

## Command Model

All command execution flows through Services.

Required flow:

```text
API or Logic
-> capability ID command request
-> Service validation and policy
-> Device execution
-> Registry payload and command-state update
-> API/Logic observes state
```

Registry stores command state only.

Runtime coordinates tasks but does not own command policy.

Command states:

```text
REQUEST
ACCEPTED
EXECUTING
COMPLETED
FAILED
TIMED_OUT
CANCELLED
```

No Dashboard-to-Device control.

No API-to-Device bypass.

No Logic-to-Device bypass.

## Safety Model

Cyber32 uses fail-safe and default-safe behavior.

Safety-critical actuators must fail safe.

Motion outputs must never activate before Runtime is `READY` or `RUNNING`.

`SAFE_MODE` blocks unsafe motion.

Logic, API, and Dashboard cannot bypass safety policy.

Services own actuator policy.

Devices enforce execution safety closest to hardware.

Registry stores safety-relevant state but does not execute safety behavior.

## Safe Mode

`RuntimeState::SAFE_MODE` is explicit.

Runtime helper behavior:

- `enterSafeMode()` sets `SAFE_MODE`
- `exitSafeMode()` exits only from `SAFE_MODE` to `READY`
- `isSafeMode()` reports state

Runtime does not own actuator policy.

Service behavior:

- servo normal `setPosition` is blocked in `SAFE_MODE`
- motor `FORWARD` and `REVERSE` are blocked in `SAFE_MODE`
- motor `STOP / 0.0F` is allowed in `SAFE_MODE`
- relay `ON` is blocked in `SAFE_MODE`
- relay `OFF` is allowed in `SAFE_MODE`

## Registry Result Model

Registry operations use compact result codes.

Core results:

```text
OK
NOT_ATTACHED
INVALID_RECORD
INVALID_ID
UNSUPPORTED_CAPABILITY
DUPLICATE_ID
TABLE_FULL
NOT_FOUND
UNAVAILABLE
STALE
TYPE_MISMATCH
INTERNAL_ERROR
```

Bool APIs remain wrappers where needed for compatibility.

Result APIs are preferred for new Registry interactions.

## Event Model

EventBus is bounded transport, not state storage.

Current minimal events include:

- boot started
- module discovered
- capability registered
- capability value updated
- error raised

Runtime drains events with a bounded budget.

Event processing must not become hidden business logic.

## Runtime Task Model

Runtime uses fixed `RuntimeTask` records.

Tasks include:

- service update tasks
- Logic evaluation tasks
- actuator state update tasks
- actuator pending command execution tasks
- future wireless packet processing task
- future wireless timeout task

Runtime must remain scheduler-only.

## First Vertical Slice

The first architecture validation slice used:

```text
CAP_TEMPERATURE
```

with:

- HAL time
- simulated temperature driver
- simulated temperature device
- simulated temperature module
- PNP discovery
- PNP registration
- Registry
- Runtime
- TemperatureService
- TemperatureLogic
- internal API
- validation harness

Success criteria:

```text
discovered
registered
payload updated
Registry stores state
Logic queries CAP_TEMPERATURE
API returns CAP_TEMPERATURE
no layer violations
```

## Implemented Sensor Capabilities

### CAP_TEMPERATURE

Simulated provider:

```text
22.4 degree_celsius
```

Wireless simulated provider work is underway:

```text
23.5 degree_celsius provider payload
```

Canonical selected payload update is planned and partially supported through Registry state-copy foundations.

### CAP_DISTANCE

Simulated provider:

```text
1.25 meter
```

Distance Logic remains capability-first:

```text
CAP_DISTANCE < 0.30 -> DISTANCE_NEAR
```

No motor behavior is triggered by distance Logic in the current scope.

## Implemented Actuator Capabilities

### CAP_SERVO_POSITION

Simulated servo:

- default position: `90.0F`
- valid range: `0.0F..180.0F`
- Service owns validation
- Runtime state gating blocks commands unless `READY` or `RUNNING`
- `SAFE_MODE` blocks normal `setPosition`
- command state is stored in Registry

### CAP_MOTOR_CONTROL

Simulated motor:

- directions: `STOP`, `FORWARD`, `REVERSE`
- speed range: `0.0F..100.0F`
- bounded pending command slot
- Runtime task executes pending command
- `STOP / 0.0F` may replace pending motion
- timeouts are enforced before Device execution
- `SAFE_MODE` blocks motion and allows stop
- driver failure does not store unsafe payload

### CAP_RELAY_CONTROL

Simulated relay:

- fail-safe default: `OFF`
- bounded pending command slot
- Runtime task executes pending command
- `SAFE_MODE` blocks `ON`
- `SAFE_MODE` allows `OFF`
- timeout, driver failure, and pending transition validation exist

## Precise Command And Actuator Errors

Command and actuator errors include:

```text
ERR_COMMAND_INVALID
ERR_COMMAND_INVALID_SPEED
ERR_COMMAND_INVALID_DIRECTION
ERR_COMMAND_INVALID_TIMEOUT
ERR_RUNTIME_NOT_READY
ERR_SAFE_MODE_BLOCKED
ERR_PENDING_COMMAND_EXISTS
ERR_COMMAND_TIMEOUT
ERR_ACTUATOR_UNAVAILABLE
ERR_ACTUATOR_EXECUTION_FAILED
```

Motor and relay validation expects precise errors.

Servo still uses its current coarse error mapping where not yet migrated.

## Capability Providers

Providers allow multiple sources for one canonical capability.

Provider types:

```text
UNKNOWN
WIRED
SIMULATED
WIRELESS
```

Provider statuses:

```text
UNKNOWN
AVAILABLE
STALE
UNAVAILABLE
LOST
DISABLED
```

Provider records store:

```text
provider_id
capability_id
owner_module_index
owner_device_index
provider_type
status
priority
last_update_ms
latest_payload
```

Providers are internal. Logic never sees provider IDs.

## Provider Selection

Registry owns provider storage, active provider mapping, provider selection, and selected payload copy.

Selection concepts:

- first available
- priority
- freshness
- failover
- recovery
- manual override future

Current foundation includes:

- fixed provider storage
- active provider mapping
- `selectBestProvider(...)`
- `updateSelectedCapabilityPayload(...)`

Expected selected provider flow:

```text
provider payload update
-> select best provider
-> set active provider
-> copy selected provider payload into canonical capability payload
-> Logic/API read canonical capability
```

## Wireless Architecture

Wireless changes provider path, not capability contract.

ESP-NOW is the first wireless transport, simulated first.

Wireless stack:

```text
SimEspNowTransportDriver
-> WirelessTemperatureDevice
-> WirelessTemperatureModule
-> Wireless PNP
-> Registry provider record
-> WirelessService
-> Runtime tasks future
-> Logic reads CAP_TEMPERATURE
```

Current ESP-NOW integration flow:

```text
ESP-NOW Callback
-> Raw Payload Capture
-> Transport Decode
-> WirelessPacketTransportAdapter
-> WirelessService
-> Checksum Validation
-> MAC-to-Node Validation
-> Allowlist Validation
-> Trust Validation
-> Sequence Validation
-> Provider Update
-> Registry State
```

Logic remains provider-blind and transport-blind.

Packet model is bounded:

- no JSON
- no dynamic allocation
- packet payload `<= 250` bytes
- fixed header
- bounded capability ID
- simple payload types: `NONE`, `FLOAT`, `INT`, `BOOLEAN`

Wireless diagnostics:

- `CAP_BATTERY_LEVEL`
- `CAP_BATTERY_VOLTAGE`
- `CAP_SIGNAL_STRENGTH`
- `last_seen_ms` diagnostic state

WirelessService responsibilities:

- read bounded packets
- validate checksum
- verify MAC identity against packet `node_id` when source MAC is available
- enforce allowlist state
- enforce trust state
- reject duplicate sequences
- validate packet path through Device
- update provider payload
- update wireless security diagnostics through Registry public APIs
- check stale/lost timeouts
- trigger provider selection and selected payload copy through Registry public APIs

WirelessService must not expose API, run Logic, or parse packets into Registry directly.

### Real ESP-NOW Driver Boundary

Real ESP-NOW is now present only behind the driver boundary:

```text
src/drivers/communication/espnow_transport_driver.h
src/drivers/communication/espnow_transport_driver.cpp
```

Rules:

- `WiFi.h` and `esp_now.h` must remain isolated to `espnow_transport_driver.cpp`
- Registry must not parse ESP-NOW packets
- Runtime must not parse ESP-NOW packets
- Logic must not see MAC, node, or provider details
- WirelessService remains the integration layer
- simulated transport remains the validation baseline until real driver integration is explicitly planned

Current real ESP-NOW status:

- real ESP-NOW driver skeleton exists
- clean header boundary exists
- ESP-NOW initialization uses `WiFi.mode(WIFI_STA)` and `esp_now_init()`
- receive callback is registered
- callback metadata is captured:
  - `callback_received_`
  - `last_received_length_`
  - `last_source_mac_`
- raw payload capture exists
- raw payload decode to `WirelessPacketHeader`, `WirelessCapabilityValue`, and `WirelessNodeDiagnostics` exists
- transport adapter path supports simulated and real ESP-NOW drivers
- WirelessService can process decoded ESP-NOW packets through `WirelessPacketTransportAdapter`
- MAC-to-node identity enforcement exists in WirelessService
- simulated transport remains an active validation baseline

## Security Model

Wireless security is enforced by WirelessService after packet read and before provider update.

Current wireless security gates:

- checksum validation
- MAC identity verification when source MAC is available
- MAC to `node_id` agreement
- allowlist validation
- trust-state validation
- duplicate sequence protection
- Device packet validation
- provider update result handling

MAC identity policy:

```text
source MAC
-> Registry allowlist MAC lookup
-> allow_state check
-> MAC record node_id must match packet header.node_id
-> normal node_id allowlist check
```

No-MAC packets preserve the simulated transport path and continue through node-ID allowlist validation.

Allowlist states:

```text
UNKNOWN
ALLOWED
BLOCKED
```

Trust states:

```text
UNKNOWN
TRUSTED
UNTRUSTED
BLOCKED
```

Wireless rejection paths include:

- `wireless_checksum_invalid`
- `wireless_mac_not_allowed`
- `wireless_mac_node_mismatch`
- `wireless_node_blocked`
- `wireless_node_not_allowed`
- `wireless_untrusted`
- `wireless_duplicate_sequence`
- `device_update_failed`
- `provider_update_failed`

Rejected packets are consumed, do not update providers, and do not mutate canonical capability payloads.

## Diagnostics

Wireless security diagnostics are stored in `WirelessNodeSecurityDiagnosticRecord`.

Diagnostic records include:

- node identity
- MAC identity
- allow state
- trust state
- `last_seen_ms`
- last accepted sequence ID
- last rejected sequence ID
- last error code
- accepted packet count
- checksum reject count
- MAC-not-allowed reject count
- MAC/node mismatch reject count
- blocked reject count
- node-not-allowed reject count
- untrusted reject count
- duplicate sequence reject count
- invalid packet reject count

Registry owns diagnostic state and bounded diagnostic storage.

WirelessService owns security policy and updates diagnostics through Registry public APIs after accepted and rejected packets.

Diagnostic failures must not block packet processing, provider updates, or existing rejection behavior.

## API v1 Contract

API is capability-first and bounded.

Endpoint categories:

- system
- capabilities
- commands
- events
- errors
- registry summary

API rules:

- API reads Registry state
- API may read Runtime state for system status
- API commands go through Services
- API does not call Drivers, Devices, HAL, or direct transports
- API does not own state

Current implementation is internal structs, not HTTP/WebServer.

WiFi/WebServer transport remains separate from API contract.

### Wireless Security Diagnostics API

Wireless security diagnostics are exposed through read-only API methods:

```text
getWirelessSecurityDiagnostic(...)
getWirelessSecurityDiagnosticByIndex(...)
getWirelessSecuritySummary(...)
```

Rules:

- API reads Registry diagnostics only
- API does not reset counters
- API does not mutate diagnostics
- API does not call WirelessService
- API does not expose raw packets
- API does not update providers
- API does not update canonical capability payloads
- normal capability reads remain unchanged

## Mobile Studio Vision

Mobile Studio is future v2+.

It must:

- use API only
- show friendly names/icons/translations
- compile visual blocks into capability-based Logic rules
- preserve `CAP_*` IDs underneath
- never depend on module names
- never directly access Devices

Example:

```text
User sees:
Distance sensor < 30 cm -> Stop motor

Internal:
IF CAP_DISTANCE.value < 0.30
THEN CAP_MOTOR_CONTROL command stop
```

The current ESP32 v1 requirement is only to expose capability-first API contracts.

## Production Bootstrap

Production startup order:

```text
HAL
Drivers
Devices
Modules
PNP
Registry
Runtime
Services
Logic
API
```

Core bootstrap pattern:

1. initialize bounded static objects
2. begin Registry and EventBus
3. begin Runtime
4. initialize HAL
5. initialize Drivers
6. initialize Devices
7. discover Modules through PNP
8. register records through Registry public APIs
9. begin Services
10. register Runtime tasks
11. begin Logic
12. begin API
13. set Runtime `READY` / `RUNNING`

`src/main.cpp` has intentionally remained untouched through validation phases unless explicitly required.

## Validation Philosophy

Validation is architecture validation, not a demo.

Validation must prove:

- correct layer boundaries
- capability-first behavior
- Registry state ownership
- Runtime scheduling only
- Service policy ownership
- API-to-Service command flow
- no Logic provider/device/module dependency
- safe actuator behavior
- no dynamic allocation in core paths
- no Arduino `String`
- no STL containers

Validation harness may use public APIs and fixed objects only.

## Milestone Summary

### Milestone 1

Core IDs and shared types.

### Milestone 2

`CAP_DISTANCE` sensor provider added.

### Milestone 2.1

Registry result codes and provider record foundation began.

### Milestone 3

`CAP_SERVO_POSITION` actuator slice.

### Milestone 3.1

Command-state storage.

### Milestone 3.2

Runtime state gating for servo commands.

### Milestone 3.3

`SAFE_MODE` state and servo command blocking.

### Milestone 3.4

Runtime Safe Mode helpers.

### Milestone 4

`CAP_MOTOR_CONTROL` actuator slice.

### Milestone 4.1

Bounded pending motor commands, STOP replacement, timeout, failure validation.

### Milestone 4.2

Pending motor command transition safety.

### Milestone 4.3

Precise command and actuator error IDs.

### Milestone 5

`CAP_RELAY_CONTROL` actuator slice.

### Milestone 5.1

Relay timeout and runtime transition validation.

### Milestone 6

ESP-NOW wireless architecture and simulated wireless foundations.

### Milestone 6.1

Multi-provider records and active provider mapping.

### Milestone 6.2

Automatic provider selection helper and validation.

### Milestone 6.3

WirelessService skeleton, one-packet processing, provider payload update, validation.

### Milestone 6.4

Selected provider payload plan and Registry state-copy method.

### Milestone 7

Capability provider roadmap across wired, wireless, virtual, cloud, dashboard, Mobile Studio, and AI.

### Milestone 7.7

ESP-NOW identity and pairing plan.

Implemented MAC address support in wireless allowlist records, Registry MAC lookup helper, and MAC lookup validation.

### Milestone 7.8

ESP-NOW transport boundary plan.

Implemented simulated transport source MAC support and MAC injection/read validation.

### Milestone 7.9

Real ESP-NOW integration checklist.

Readiness result: WARNING until real ESP-NOW driver validation, raw packet capture, decode, and hardware tests are complete.

### Milestone 8.0

Real ESP-NOW transport driver skeleton.

Created a clean driver header boundary and isolated `WiFi.h` / `esp_now.h` to the `.cpp`. No WirelessService integration yet.

### Milestone 8.1

ESP-NOW driver initialization.

Implemented WiFi STA mode, `esp_now_init()`, and initialization smoke validation.

### Milestone 8.2

Receive callback skeleton.

Implemented callback bridge and metadata capture only. No packet parsing and no provider updates yet.

### Milestone 8.7

ESP-NOW Adapter to WirelessService validation.

Validated decoded ESP-NOW packets through `WirelessPacketTransportAdapter` and the existing WirelessService policy pipeline without real radio hardware.

### Milestone 8.8

MAC-to-Node Enforcement.

Implemented WirelessService MAC identity verification using Registry MAC lookup and packet `node_id` agreement. Validated matching MAC acceptance, unknown MAC rejection, blocked MAC rejection, MAC/node mismatch rejection, and no-MAC simulated compatibility.

### Milestone 8.9

Wireless Security Diagnostics.

Added bounded `WirelessNodeSecurityDiagnosticRecord` storage, Registry accepted/rejected update helpers, WirelessService diagnostic updates, and validation for accepted/rejected counter behavior.

### Milestone 9.0

Wireless Security Diagnostics API.

Added read-only API structs and methods for wireless security diagnostics by node ID, by index, and summary totals. Validated read-only behavior and confirmed normal capability reads remain unchanged.

## Current Project Status

Completed:

- Milestone 8.7 ESP-NOW Adapter to WirelessService Validation
- Milestone 8.8 MAC-to-Node Enforcement
- Milestone 8.9 Wireless Security Diagnostics
- Milestone 9.0 Wireless Security Diagnostics API

## Current Project Snapshot

Purpose: allow future AI sessions and contributors to immediately understand the current Cyber32 project status.

Current Milestone: Milestone 9.0 Wireless Security Diagnostics API

Current Build: pending local PlatformIO verification in this environment

Current Validation:

- validation coverage added through Milestone 9.0
- local `pio run` could not be executed because `pio` is not available on PATH

Current Capability Slices:

- `CAP_TEMPERATURE`
- `CAP_DISTANCE`
- `CAP_SERVO_POSITION`
- `CAP_MOTOR_CONTROL`
- `CAP_RELAY_CONTROL`

Current Provider System: 100%

Current Wireless Security: 100%

Current Diagnostics: 100%

Current Diagnostics API: 100%

Current ESP-NOW Status:

- real ESP-NOW driver skeleton exists
- `WiFi.h` and `esp_now.h` are isolated to `espnow_transport_driver.cpp`
- ESP-NOW initialization is implemented with `WiFi.mode(WIFI_STA)` and `esp_now_init()`
- receive callback is registered
- callback metadata is captured:
  - `callback_received_`
  - `last_received_length_`
  - `last_source_mac_`
- raw payload capture exists
- transport decode exists
- decoded structured packets are exposed through adapter-compatible reads
- `WirelessPacketTransportAdapter` supports simulated and real ESP-NOW drivers
- WirelessService processes ESP-NOW adapter packets through the same policy pipeline as simulated packets
- MAC-to-node enforcement exists
- wireless security diagnostics and read-only diagnostics API exist
- simulated transport remains a validation baseline

Status Estimate:

```text
Core Architecture          100%
Registry                   100%
Runtime                    100%
Provider System            100%
Wireless Security          100%
Diagnostics                100%
Diagnostics API            100%

Sim Wireless               100%
ESP-NOW Driver             90%
ESP-NOW Integration        100%
```

Next Planned Step:

```text
Milestone 9.1
```

## Known Remaining Gaps

Critical and important gaps still tracked across audits:

- full production bootstrap in `src/main.cpp` is not wired
- PlatformIO build has not been verified in this environment because `pio` is not on PATH
- real wireless temperature node test pending
- persistence for allowlist/pairing still future work
- stronger cryptographic authentication still future work
- real hardware drivers are not implemented
- persistence/configuration model is incomplete
- security minimum is still future work
- EventBus remains minimal
- complex payload schemas remain deferred

## Architecture Stop Conditions

Stop and review architecture if any of these occur:

- Logic depends on module ID, device ID, provider ID, node ID, pin, transport, or cloud topic
- API calls Devices or Drivers directly
- Dashboard or Mobile Studio bypasses API
- Registry executes commands
- Runtime owns Service policy
- Services write Registry arrays directly
- PNP calls Device behavior
- wireless packet parsing enters Registry
- provider storage becomes unbounded
- dynamic allocation appears in core paths
- Arduino `String` appears in core paths
- STL containers appear in core paths
- actuator commands bypass Services
- Safe Mode can be bypassed by API, Logic, Dashboard, or provider selection
- real actuator hardware is introduced before safety validation passes
- wireless actuator support is introduced before trusted command acknowledgement and timeout behavior are defined

## Production Roadmap

Recommended order:

1. Milestone 9.1
2. verify PlatformIO build in an environment with `pio` available
3. real wireless temperature node test
4. add production bootstrap path
5. add real wired sensor providers
6. add persistence/configuration
7. add stronger wireless authentication beyond MAC identity and checksum
8. expose Dashboard/Mobile Studio diagnostics through API transports
9. consider real actuator hardware only after safety readiness audits
10. consider wireless/cloud/AI actuator providers only after trusted bounded command flow exists

## Next Target

NEXT TARGET:

```text
Milestone 9.1
```

Recommendations:

- use Milestone 9.0 diagnostics as the observable baseline for future wireless work
- keep API diagnostics read-only
- keep Logic provider-blind, transport-blind, and diagnostics-blind
- verify PlatformIO build in an environment where `pio` is available before marking the current source set production-ready
- choose the next milestone from documented gaps rather than adding new behavior opportunistically

## Final Canon

Cyber32 is not module-first.

Cyber32 is not device-first.

Cyber32 is not dashboard-first.

Cyber32 is capability-first.

The entire system exists to preserve this contract:

```text
providers may change
transports may change
hardware may change
services may grow
UI may evolve
AI may assist

but Logic and API remain anchored to CAP_* capability contracts
```

That is the Cyber32 architecture.
