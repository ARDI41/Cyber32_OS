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
- all `MILESTONE_*` plans, audits, and roadmaps through Milestone 9.3

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

## Cyber32 Foundation Principles

Cyber32 is not merely a Home Automation platform, Robot OS, IoT platform, or Smart Home controller.

Cyber32 is a Universal Capability Platform.

Its purpose is to create a capability-based operating system capable of controlling any physical or virtual system through stable capability contracts.

The foundation is:

```text
Hardware may change.
Drivers may change.
Wireless transports may change.
Cloud may change.
UI may change.
AI may change.

Capability contracts remain stable.
Logic remains capability-first forever.
```

### Capability First

Capabilities are the stable contract between the physical world and Cyber32 behavior.

This exists so hardware can be replaced without rewriting Logic, UI, automations, dashboards, Mini App flows, cloud integrations, or AI-generated projects.

### API First

Every UI and external client must communicate through the Cyber32 API.

This exists so Dev Panel, Mini App, Mission Control, Dashboard, Cloud, Marketplace assets, and future tools share one bounded access model instead of creating fragile direct hardware paths.

### Core First

Core is the brain of the Cyber32 ecosystem.

This exists so Registry, Runtime, Services, Logic, API, Device Management, OTA, diagnostics, and future Cloud bridge remain centralized, auditable, and safe.

### Local First

Cyber32 must work locally without cloud services.

This exists so homes, robots, labs, schools, farms, and field devices keep functioning when internet access is unavailable or intentionally disabled.

### Documentation Before Implementation

Architecture decisions must be documented before implementation.

This exists so milestones remain intentional, future AI sessions inherit context, and contributors do not accidentally turn temporary hardware-test paths into permanent architecture.

### Simulation Before Hardware

Simulated slices validate architecture before real hardware is introduced.

This exists so Registry contracts, Runtime behavior, Services, Logic, API, safety, diagnostics, and provider selection are proven before hardware noise enters the system.

### Safety Before Features

Safety comes before convenience, speed, novelty, or demo value.

This exists because Cyber32 can control motors, relays, robots, cameras, alarms, and future actuators. Unsafe commands must fail safe.

### Simple Nodes, Intelligent Core

Sensor Nodes and Module Nodes should stay simple.

This exists so nodes can be cheap, bounded, reliable capability providers while Core owns policy, identity, orchestration, diagnostics, OTA strategy, and future cloud bridging.

## Cyber32 Identity

Every Cyber32 Core owns a permanent identity.

Core identity is the root identity for the ecosystem.

Core identity includes:

- Core UUID
- Friendly Name
- Owner
- Hardware Revision
- Firmware Version
- Pairing State
- Provisioning State
- License State future
- Manufacturing Information future
- Cloud Identity future

Nodes also own identities, but node identity never replaces Core identity.

Node identities describe capability providers attached to, paired with, or trusted by a Core. Core identity is the anchor for Projects, permissions, Mini App pairing, cloud bridging, OTA policy, diagnostics, and future marketplace licensing.

## Projects

Cyber32 organizes systems as Projects.

A Project represents an entire installation.

Example hierarchy:

```text
Project
-> Rooms / Zones
-> Nodes
-> Capabilities
-> Logic
-> Dashboards
-> Templates
```

Projects remain capability-first.

Projects may later support:

- Export
- Import
- Backup
- Restore
- Version History
- Sharing

Projects must describe stable capabilities and desired behavior rather than fragile hardware assumptions wherever possible.

## Custom Project Pages and App Builder

Cyber32 Mini App and future Mission Control must allow users to create custom project pages.

A custom project page is a user-created view inside a Project.

Examples:

- Greenhouse Overview
- Robot Rover Control
- Garage Security
- Weather Station
- Pet Feeder
- Workshop Dashboard
- Energy Monitor

Custom pages may contain:

- capability value cards
- buttons
- sliders
- charts
- device map widgets
- camera view widgets
- motor control widgets
- logic status widgets
- notification widgets
- project notes
- safety status blocks

Custom pages are UI composition only.

They must never talk directly to Drivers, Devices, Modules, HAL, Registry arrays, or wireless internals.

Correct flow:

```text
Custom Page Widget -> Cyber32 API -> Services / Runtime / Logic -> Capability Router -> Provider
```

Forbidden flows:

```text
Custom Page Widget -> Driver
Custom Page Widget -> Module
Custom Page Widget -> HAL
```

Custom pages must bind to `CAP_*` capability IDs.

Examples:

```text
Temperature Card -> CAP_TEMPERATURE
Servo Slider -> CAP_SERVO
Motor Stop Button -> CAP_DRIVE or CAP_MOTOR_CONTROL
Camera View -> CAP_CAMERA
Speaker Button -> CAP_AUDIO_OUTPUT
```

Projects must be easy to follow.

Each Project may contain:

- overview page
- device map
- capability list
- logic rules
- custom pages
- logs
- diagnostics
- timeline
- notifications
- project notes
- templates used
- safety status

### Mini App Builder

Mini App Builder is a future no-code composition tool that allows a user to create simple custom views without programming.

Builder modes:

- Add Page
- Add Widget
- Bind Widget to Capability
- Choose Layout
- Save as Template
- Share Template later

Widget types:

- Value Card
- Toggle Button
- Slider
- Emergency Stop Button
- Chart
- Camera Feed
- Audio Button
- Device Map
- Logic Rule Status
- Notification List
- Timeline
- Text / Notes Block
- Project Checklist

Safety rule:

Any widget that can trigger actuator behavior must go through API command policy and Services.

Motor widgets must include visible stop or safe controls where appropriate.

Emergency Stop must be globally available in Motor Lab, Robot Control, and any custom page that controls motion.

Custom pages are not plugins with privileged access.

They are declarative UI layouts stored as Project configuration.

Future storage may support:

- export
- import
- backup
- restore
- share
- marketplace distribution

All shared custom pages must remain capability-based and API-first.

## Drag & Drop Logic Builder and Template Scripts

Cyber32 Mini App, Mission Control, and future Dashboard must support drag-and-drop logic building.

Logic Builder must be capability-first and API-first.

Users should be able to build automations visually using blocks:

- Trigger Block
- Condition Block
- Action Block
- Safety Block
- Delay / Timer Block
- Notification Block
- Project Block

Examples:

```text
WHEN CAP_MOTION == true
THEN CAP_AUDIO_OUTPUT = beep

WHEN CAP_DISTANCE < 20cm
THEN CAP_MOTOR_CONTROL = stop

WHEN CAP_TEMPERATURE > 30
THEN CAP_FAN = on
```

All drag-and-drop blocks must compile into deterministic Cyber32 logic rules.

They must not become free-form scripts that bypass Runtime, Services, Registry, API, or safety policy.

### Template Scripts

Template Scripts are prebuilt capability-first logic recipes included with Cyber32.

Template Scripts are project starters, not privileged code.

### Temperature Monitor Template

Required capabilities:

- `CAP_TEMPERATURE`

Optional capabilities:

- `CAP_DISPLAY`
- `CAP_NOTIFICATION`

Rules:

```text
IF CAP_TEMPERATURE > threshold THEN notify
IF CAP_TEMPERATURE unavailable THEN show warning
```

### Robot Rover Template

Required capabilities:

- `CAP_DRIVE` or `CAP_MOTOR_CONTROL`
- `CAP_DISTANCE`

Optional capabilities:

- `CAP_CAMERA`
- `CAP_AUDIO_OUTPUT`

Rules:

- `IF CAP_DISTANCE < safe_distance THEN stop`
- Emergency Stop always available
- manual drive controls must go through Services

### Smart Guard Template

Required capabilities:

- `CAP_MOTION`

Optional capabilities:

- `CAP_CAMERA`
- `CAP_AUDIO_OUTPUT`
- `CAP_NOTIFICATION`

Rules:

```text
IF CAP_MOTION == true THEN camera capture
IF CAP_MOTION == true THEN audio alert
IF CAP_MOTION == true THEN notification
```

### CNC / Motion Template

Required capabilities:

- future `CAP_AXIS_X`
- future `CAP_AXIS_Y`
- Emergency Stop

Optional capabilities:

- future `CAP_AXIS_Z`

Rules:

- must run only in safe bounded mode
- no unvalidated G-code execution in early architecture
- movement commands must go through Services and safety policy

### Greenhouse Template

Required capabilities:

- `CAP_TEMPERATURE`

Optional capabilities:

- `CAP_HUMIDITY`
- `CAP_LIGHT`
- `CAP_WATER_LEVEL`
- `CAP_RELAY_CONTROL`

Rules:

```text
IF temperature too high THEN fan on
IF soil/moisture low THEN pump on only within safe limits
```

### Pet Feeder Template

Required capabilities:

- `CAP_SERVO` or `CAP_MOTOR_CONTROL`

Optional capabilities:

- `CAP_CAMERA`
- `CAP_WEIGHT`
- `CAP_NOTIFICATION`

Rules:

- scheduled feed action
- jam detection future
- manual feed button through Services

### Energy Monitor Template

Required capabilities:

- future `CAP_POWER`

Optional capabilities:

- `CAP_VOLTAGE`
- `CAP_CURRENT`
- `CAP_RELAY_CONTROL`

Rules:

- show usage
- notify abnormal usage
- actuator control only through Services

### Weather Station Template

Required capabilities:

- `CAP_TEMPERATURE`

Optional capabilities:

- `CAP_HUMIDITY`
- `CAP_PRESSURE`
- `CAP_WIND`
- `CAP_RAIN`

Rules:

- display current values
- record timeline
- notify threshold changes

Template Script rules:

- templates are project starters, not privileged code
- templates must bind to `CAP_*` IDs
- templates must declare required and optional capabilities
- templates must declare safety requirements
- templates must declare UI widgets
- templates may create custom project pages
- templates may create logic rules
- templates may create notifications
- templates may create diagnostics views
- templates must never directly call Drivers, Devices, HAL, Registry arrays, or transport internals

## Permission Model

Cyber32 will use a role-based permission model.

Future roles include:

- Owner
- Administrator
- Developer
- Family
- Operator
- Guest
- Read Only

Permission checks occur above the API and never inside Drivers or Devices.

Drivers and Devices must remain hardware-facing implementation layers. They do not know users, roles, ownership, or UI permissions.

Permission policy belongs in API-facing, Service-facing, or future identity layers that can make decisions before commands or state-changing operations reach lower layers.

## Notification Engine

Cyber32 will include a Notification Engine.

Future notifications include:

- Battery Low
- Node Offline
- Motion Detected
- Temperature Alert
- OTA Finished
- System Warning
- Safety Warning

Notifications are generated by Services and Runtime events.

UI only displays notifications. UI must not become the source of truth for notification state, safety state, node health, or provider health.

## Plugin Architecture

Cyber32 will support plugins in future architecture phases.

Future plugin types include:

- Logic Plugins
- Dashboard Plugins
- Visualization Plugins
- AI Plugins
- Project Templates
- Future Driver Packages

Plugins must never bypass the API or capability contracts.

Plugin packages may extend behavior, visualization, templates, drivers, AI assistance, or project workflows, but they must preserve the Core-first, API-first, capability-first architecture.

## Mission Control Vision

Mission Control is the future central visual control surface for Cyber32.

Mission Control is not a privileged backdoor. It is a UI client that communicates only through the Cyber32 API.

Future Mission Control modules include:

- System Overview
- Live Device Map
- Capability Health
- Diagnostics
- Logic Builder
- Automation Monitor
- Robot Control
- Security Center
- Energy Monitor
- Project Explorer

Mission Control must remain capability-first and provider-blind unless it is explicitly showing diagnostics through API-approved diagnostic views.

## Architecture Layer Clarification

Cyber32 now recognizes two UI layers.

Developer Layer:

- Dev Panel
- Test Lab
- Motor Lab
- Diagnostics

Customer Layer:

- Mini App
- Mission Control
- Dashboard
- Project Templates

Both layers use exactly the same Cyber32 API.

Neither layer may call Drivers, Devices, Modules, HAL, Registry arrays, or transport internals directly.

## Future Marketplace Vision

Cyber32 Marketplace may distribute:

- Drivers
- Logic Templates
- Projects
- Dashboards
- AI Assistants
- Visual Components
- Plugin Packages
- Educational Content

Marketplace assets never communicate directly with hardware.

Everything goes through the Cyber32 API and stable capability contracts.

## Recent Architecture Decisions

These decisions are authoritative for future work. If they appear to narrow or clarify older text, use this section as the current architecture source rather than deleting milestone history.

### Cyber32 Ecosystem

Cyber32 is an ecosystem made of simple capability providers around a capable Core.

Ecosystem parts:

- Core
- Sensor Nodes
- Module Nodes
- Dev Panel
- Mini App
- future Cloud
- future Dashboard
- future Marketplace

Core is the brain of the system.

Core owns:

- Registry
- Runtime
- Services
- Logic
- API
- Device Management
- OTA
- future Cloud bridge

Sensor Nodes and Module Nodes must stay simple capability providers. They may sense, report, announce, and eventually execute bounded commands, but they must not become a second Core, own system policy, run global Logic, or expose privileged UI paths.

### Core Modes

Core supports these architecture modes:

- Setup Mode
- Developer Mode
- Local Mode
- Remote Mode

Setup Mode is used when the Core needs first-time network setup or recovery access.

Developer Mode enables hardware-test helpers, Test Lab workflows, diagnostics, replay, simulation, and safe command testing.

Local Mode is the normal LAN-first operating mode where UI clients talk to Core over the local Cyber32 API.

Remote Mode is future cloud-assisted access. Core must initiate outbound cloud connections; Cyber32 must not require router port forwarding.

### API Principle

All UI clients communicate only through the Cyber32 API.

This applies to:

- Dev Panel
- Mini App
- future Dashboard
- future Cloud
- future Marketplace clients

Correct flow:

```text
UI -> API -> Runtime / Services / Logic -> Capability Router -> Provider
```

Forbidden flows:

```text
UI -> Driver
UI -> Module
UI -> HAL
```

UI clients must never access Drivers, Devices, Modules, HAL, or Registry arrays directly. The API is the only UI-facing contract.

### Capability-First Rule

Cyber32 logic must depend on capability IDs, not hardware names.

Use:

```text
CAP_TEMPERATURE
CAP_SERVO
CAP_DISTANCE
CAP_DRIVE
```

Do not bind Logic to:

```text
DS18B20
DHT22
ServoMotorDriver
specific pins
specific transport names
```

Hardware is replaceable. Capabilities are stable contracts.

### Existing Hardware Context

Current available test hardware includes:

- Cyber32 Core ESP32
- Sensor Node ESP32
- ESP32-CAM
- Robot Car Kit
- ChatGPT / AI Robot Kit
- speaker
- microphone
- multiple sensors and modules

This hardware is useful for validation, demos, and future templates, but it must not weaken capability-first boundaries.

## Dev Panel, Mini App and Test Lab Strategy

### Dev Panel

Dev Panel runs on Cyber32 Core as an embedded web interface.

Dev Panel is for development and debugging. It is accessed through local IP or the Core setup AP.

Dev Panel must use Cyber32 API only. It must never call Drivers, Devices, Modules, HAL, or Registry arrays directly.

Dev Panel includes:

- System
- Devices
- Capabilities
- Logs
- Diagnostics
- Test Lab
- OTA

### Mini App

Mini App is the future customer and developer mobile app.

Mini App is used for:

- setup
- local control
- remote view
- testing
- future logic building

Initial connection flow:

```text
user powers Core
-> Core starts Setup Mode
-> app finds Core
-> user selects home WiFi
-> Core joins local network
-> app connects locally
```

Future remote mode goes through Cyber32 Cloud.

Remote mode rule:

```text
Core -> outbound cloud connection -> Cloud -> Mini App
```

No router port forwarding is required.

### Test Lab

Test Lab is an official Cyber32 architecture concept.

Test Lab must support:

- inject capability values
- simulate sensors
- simulate motion
- simulate distance
- simulate low battery
- simulate weak signal
- simulate lost node
- replay packets
- trigger discovery
- clear Registry in Developer Mode
- test motor commands safely

Test Lab is a development surface over API and Services. It must not bypass safety, Runtime state, command policy, Registry APIs, or capability-first rules.

### Motor Lab

Motor Lab is the Test Lab area for actuator and motion testing.

Motor Lab includes:

- Servo test
- DC motor test
- Stepper test

Emergency Stop is required for motor testing.

Motor Lab must preserve actuator safety policy. It may help validate commands, bounds, and safe states, but it must not provide an unsafe shortcut around Services or Runtime state.

### Minimal Logic Builder

Minimal Logic Builder is a future no-code IF / THEN logic system.

The model is:

```text
trigger -> condition -> action
```

Logic Builder is capability-based only.

Examples:

```text
IF CAP_MOTION == true THEN CAP_AUDIO_OUTPUT = beep
IF CAP_DISTANCE < 20cm THEN CAP_DRIVE = stop
IF CAP_TEMPERATURE > 30 THEN CAP_FAN = on
```

Logic Builder must not expose Drivers, Modules, HAL, pins, transport IDs, or provider IDs as logic dependencies.

### Fun and Wow Layer

Cyber32 may include a future user experience layer that makes learning, testing, and building feel alive without weakening architecture.

Future concepts:

- Mission Control
- demo missions
- achievements
- project templates
- robot rover demo
- smart guard demo
- first node connected
- first automation
- first robot alive

This layer is UI and experience only. It must use the Cyber32 API and capability contracts.

### Project Templates

Future app templates may include:

- Smart Greenhouse
- Weather Station
- Smart Guard
- Robot Rover
- Pet Feeder
- Garage Monitor

Templates configure capabilities, providers, safe defaults, diagnostics, and suggested UI views. Templates must not hard-code fragile hardware assumptions where capability contracts are available.

## Implementation Impact

Future Codex tasks, ChatGPT sessions, human contributions, and implementation milestones must follow this updated Bible.

Implementation impact:

- Core remains the system brain.
- Sensor Nodes and Module Nodes remain simple capability providers.
- Dev Panel, Mini App, Dashboard, Cloud, and Marketplace clients must use the Cyber32 API only.
- UI must never call Drivers, Devices, Modules, HAL, or Registry arrays directly.
- Test Lab and Motor Lab must preserve safety and capability-first boundaries.
- Logic Builder must depend on `CAP_*` IDs only.
- Cloud access must be Core-initiated outbound access, with no router port forwarding requirement.
- Temporary hardware-test hooks must remain clearly marked and must not become production architecture by accident.
- Future Dev Panel, Mini App, Mission Control, and Dashboard work must support Project-oriented UI structure.
- UI screens should be designed so they can later become user-created custom pages.
- Avoid hard-coded one-purpose screens where a reusable capability widget would be better.
- Future UI and Logic Builder work must treat drag-and-drop blocks, Template Scripts, and custom pages as declarative Project configuration.
- Drag-and-drop blocks, Template Scripts, and custom pages compile into API-safe, capability-first Cyber32 rules.
- They are not arbitrary scripts with hardware access.
- When a future task conflicts with older milestone text, this section and the Recent Architecture Decisions section are authoritative.

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
- ESP-NOW initialization exists behind the driver boundary
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
- first real Sensor Firmware to Cyber32 OS Base Node ESP-NOW receive has been proven on hardware
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

### Milestone 9.1

First Real Wireless Node Preparation.

Milestone 9.1 prepared the Cyber32 OS Base Node and Cyber32 Sensor Firmware path for the first real ESP-NOW `CAP_TEMPERATURE` hardware test.

Completed phases:

- Phase 1 Base Node Hardware Test Plan
- Phase 2 Wireless Temperature Sender Plan
- Phase 3 Real Node Test Checklist
- Phase 4 Base Harness Skeleton Plan
- Phase 5 Base Hardware Test Harness Skeleton
- Phase 6 Wireless Temperature Sender Skeleton
- Phase 7 MAC Setup Plan
- Phase 8 First Real Packet Test Plan
- Phase 9 MAC Discovery Helper Plan
- Phase 10 MAC Discovery Helper
- Phase 11 Base Node MAC Serial Print Helper

Milestone 9.1 established the hardware-test scaffolding for:

- Base Node ESP-NOW receive preparation
- Sensor Firmware one-shot wireless temperature sender preparation
- MAC discovery and manual pairing setup
- first real packet test procedure
- temporary serial visibility for Base Node MAC discovery

Hardware MAC results:

- Base Node MAC: `CC:DB:A7:9F:C3:D4`
- Sender Node MAC: `AC:27:6E:A5:8C:68`

Build status:

- PlatformIO builds were verified locally by the user.
- AI/Codex shell could not run `pio` because `pio` is not available on PATH in this environment.

Milestone 9.1 remained hardware-test focused. It did not introduce Registry ingestion, provider updates, Logic integration, API exposure, Dashboard behavior, provisioning runtime, or NVS storage.

### Milestone 9.2

First Real Sensor Firmware ESP-NOW Receive.

Confirmed the first real wireless hardware test between Cyber32 Sensor Firmware on ESP32-S3 and Cyber32 OS Base Node on ESP32.

Sensor Firmware result:

- packet validation OK
- transport validation OK
- send result: `SEND_OK`
- ESP-NOW TX callback status: `SUCCESS`
- peer MAC: `CC:DB:A7:9F:C3:D4`
- channel: `1`

Base Node result:

- `Cyber32 Base Node MAC: CC:DB:A7:9F:C3:D4`
- `Cyber32 WiFi channel: 1`
- `Cyber32 ESP-NOW RX test started`
- received packet visibility:

```text
Cyber32 ESP-NOW RX
Source MAC: AC:27:6E:A5:8C:68
Length: 60
```

What was proven:

- Sensor Firmware can build, validate, and send a Cyber32 60-byte packet.
- ESP-NOW TX callback confirms delivery success.
- Cyber32 OS Base Node receives the packet through the ESP-NOW receive callback.
- The first complete Sensor Node to Base Node wireless path is proven.

Why it matters:

- Real hardware communication between independent Cyber32 nodes is proven.
- Sensor Firmware and Cyber32 OS are now connected over ESP-NOW.
- The 60-byte Wireless Sensor Protocol v1.0 packet physically reaches the Base Node.
- Next work can move from receive visibility to packet decode and provider ingestion.

Temporary hardware-test changes:

- Base Node uses a temporary ESP-NOW RX boot hook.
- Base Node uses AP+STA mode.
- Base Node SoftAP is fixed to channel `1`.
- Sensor Firmware is fixed to channel `1` for the development test.
- Static Base Node MAC is a development fallback only.
- Production path remains plug-and-play provisioning.

Boundaries:

- No Registry update yet.
- No Provider update yet.
- No Logic, API, or Dashboard integration yet.
- No WirelessService ingestion yet.
- No provisioning runtime yet.
- No NVS storage yet.
- Current result is receive visibility only.

Next recommended milestone:

Milestone 9.3 - Decode incoming Sensor Firmware packet:

- validate magic
- validate protocol version
- validate packet size
- decode `node_id`
- decode `capability_id`
- decode value
- print decoded `CAP_TEMPERATURE` payload
- still do not update Registry or Provider yet

## Milestone 9.3 - Decode Incoming Sensor Firmware Packet

Result: SUCCESS.

Milestone 9.3 proved that Cyber32 OS can decode the first real Sensor Firmware ESP-NOW packet at the hardware-test visibility layer.

Confirmed real hardware decode:

- Source MAC: `AC:27:6E:A5:8C:68`
- Length: `60`
- Magic: OK
- Version: OK
- Packet Type: `1`
- Packet Size: `60`
- Node ID: `1`
- Sequence ID: `427`
- Capability: `CAP_TEMPERATURE`
- Payload Type: FLOAT
- Payload Schema Version: `1`
- Payload Available: `1`
- Temperature: `23.50 C`
- Checksum: OK
- Timestamp: `1278204`
- Error Code: `0`
- Pairing Flags: `0`
- Security Level: `0`

Battery, battery voltage, signal, and diagnostics fields are currently placeholder/raw values until Sensor Firmware fills them with real measurements.

What was proven:

- Sensor Firmware sends a valid 60-byte Cyber32 Sensor Packet v1.0 payload over ESP-NOW.
- Cyber32 OS Base Node receives the payload from the real sender MAC.
- The temporary hardware-test decoder validates packet size, magic, protocol version, and checksum.
- The decoder reads the real node ID, sequence ID, capability ID, payload type, and temperature value.
- The packet reaches Cyber32 OS in a form suitable for the next discovery/provider-ingestion milestones.

Boundaries:

- No Registry update yet.
- No Provider update yet.
- No WirelessService ingestion yet.
- No Logic or API integration yet.
- No Dashboard, Dev Panel, or Mini App behavior yet.
- Provider ingestion remains future work.

Milestone 9.3 remains a hardware-test debug decode milestone only.

## Sensor Model Clarification

Cyber32 supports wired and wireless sensors through the same provider/capability model.

Wired sensor:

- `provider_type = WIRED`
- local driver/device
- no MAC address
- no radio diagnostics

Wireless sensor:

- `provider_type = WIRELESS`
- source MAC
- `node_id`
- `sequence_id`
- battery/signal diagnostics
- accepted/rejected packet diagnostics

The stable UI and Logic contract remains capability-first. Wired and wireless sensors may differ in diagnostics and transport metadata, but both must eventually appear as capability providers.

## Minimal App Readiness

The Minimal App may now begin as a separate project, but it must use the Cyber32 API only.

Initial app targets:

- Node list
- Sensor detail
- Capability value card
- Battery/signal card
- Pairing request screen later
- Diagnostics screen later

Initial visible data model:

- `node_id`
- source MAC
- `capability_id`
- value
- unit
- provider status
- last update
- battery percent future
- battery voltage future
- signal/RSSI future
- diagnostics/error status

Rules:

- App must not talk to Drivers.
- App must not parse ESP-NOW packets.
- App must not access Registry arrays.
- App must use Cyber32 API only.
- App must remain capability-first.

Minimal App work should begin with API contract design or mock data if needed until discovery/provider APIs are ready.

## Plug-And-Play Direction

Manual MAC setup is development-only.

Production plug-and-play should follow this direction:

```text
unknown node packet/announce
-> Base creates pending discovery record
-> API exposes pending sensor
-> Minimal App shows "New sensor found"
-> user accepts
-> Base creates allowlist/trust/provider records
```

Plug-and-play must remain API-first and capability-first. The app must not parse wireless packets or directly create Registry array entries.

## Current Project Status

Completed:

- Milestone 8.7 ESP-NOW Adapter to WirelessService Validation
- Milestone 8.8 MAC-to-Node Enforcement
- Milestone 8.9 Wireless Security Diagnostics
- Milestone 9.0 Wireless Security Diagnostics API
- Milestone 9.1 First Real Wireless Node Preparation
- Milestone 9.2 First Real Sensor Firmware ESP-NOW Receive
- Milestone 9.3 Decode Incoming Sensor Firmware Packet

## Current Project Snapshot

Purpose: allow future AI sessions and contributors to immediately understand the current Cyber32 project status.

Current Milestone: Milestone 9.3 complete

Current Build:

- PlatformIO builds verified locally by the user
- AI/Codex shell could not run `pio` because `pio` is not available on PATH in this environment

Current Validation:

- validation coverage added through Milestone 9.0
- Milestone 9.1 hardware-test scaffolding completed
- hardware receive visibility proven through Milestone 9.2
- real Sensor Firmware packet debug decode proven through Milestone 9.3
- AI/Codex shell build command remains unavailable until `pio` is on PATH

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
- ESP-NOW initialization is implemented behind `EspNowTransportDriver`
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
- first real ESP-NOW receive from Sensor Firmware to Cyber32 OS Base Node is proven
- real Sensor Firmware packet receive and debug decode are proven
- current hardware-test receive packet length is `60`
- decoded hardware-test packet: `CAP_TEMPERATURE`, node `1`, sequence `427`, temperature `23.50 C`, checksum OK
- Base Node MAC: `CC:DB:A7:9F:C3:D4`
- Sender Node MAC: `AC:27:6E:A5:8C:68`
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
Milestone 9.4 / 9.5 - discovery and API readiness for Minimal App
```

## Known Remaining Gaps

Critical and important gaps still tracked across audits:

- full production bootstrap in `src/main.cpp` is not wired
- PlatformIO builds were verified locally by the user; AI/Codex shell cannot run `pio` because it is not on PATH
- real ESP-NOW receive visibility and packet debug decode are proven; provider ingestion is pending
- persistence for allowlist/pairing still future work
- current wireless security includes checksum, MAC-to-node, allowlist, trust, sequence, and diagnostics
- future production security still needs stronger cryptographic authentication
- real hardware drivers are not implemented
- persistence/configuration model is incomplete
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

1. Milestone 9.4 / 9.5 - discovery and API readiness for Minimal App
2. verify PlatformIO build in an environment with `pio` available
3. ingest decoded packet through the planned wireless/provider path
4. add production bootstrap path
5. add plug-and-play provisioning and persistence
6. add real wired sensor providers
7. add stronger wireless authentication beyond MAC identity and checksum
8. expose Dashboard/Mobile Studio diagnostics through API transports
9. consider real actuator hardware only after safety readiness audits
10. consider wireless/cloud/AI actuator providers only after trusted bounded command flow exists

## Next Target

NEXT TARGET:

```text
Milestone 9.4 / 9.5 - discovery and API readiness for Minimal App
```

Recommendations:

- use the Milestone 9.3 hardware decode result as the baseline for discovery and provider-ingestion planning
- keep API diagnostics read-only
- keep Logic provider-blind, transport-blind, and diagnostics-blind
- keep provider ingestion explicit and testable; Milestone 9.3 did not update Registry/provider state
- design Minimal App screens around API contracts and capability/provider data
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

## AI Philosophy

AI is an assistant.

AI never owns architecture.

AI never bypasses safety.

AI may:

- generate capability logic
- explain system behavior
- build projects
- recommend configurations
- help write documentation
- help produce validation plans
- help create API-first UI flows

AI must not:

- replace Runtime
- replace Registry
- replace Services
- replace Safety
- bypass capability contracts
- bypass API boundaries
- silently change architecture
- convert temporary hardware-test code into production behavior without an explicit milestone

Cyber32 architecture remains deterministic.

AI may assist Cyber32, but Cyber32 remains governed by documented architecture, bounded state, explicit validation, and safety-first execution.

## Foundation Summary

This Foundation Summary is authoritative for future architecture.

Cyber32 is not:

- hardware-first
- module-first
- dashboard-first
- cloud-first
- AI-first

Cyber32 is:

- Capability-first
- API-first
- Safety-first
- Core-first
- Documentation-first
- Universal Capability Platform

The stable Cyber32 promise is:

```text
Capability contracts are the center.
Core owns system intelligence.
Nodes provide capabilities.
UI uses API.
Safety gates action.
Documentation guides implementation.
AI assists, but does not govern.
```
