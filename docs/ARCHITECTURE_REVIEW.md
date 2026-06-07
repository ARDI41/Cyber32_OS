# Cyber32 Architecture Review

This review covers the current Cyber32 documentation set, including the root architecture documents and the README files under `src`, `include`, `lib`, and `test`.

Cyber32 v1 targets ESP32 only. This review intentionally does not optimize for Raspberry Pi, Linux, Android, desktop simulation, or other future hosts.

## Executive Summary

Cyber32 has a strong and consistent core idea: a capability-first robotics OS where Logic operates on stable capability IDs instead of concrete module names. The fixed layer order is clear and should remain unchanged:

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
-> Dashboard
```

The main architectural risk is not the concept. The risk is scope pressure. The documentation describes a large OS-like platform, but ESP32 v1 has limited RAM, limited flash, no full operating system, Arduino/PlatformIO constraints, WiFi instability risks, and tight timing requirements. Cyber32 v1 should therefore implement the architecture as small contracts and static/lightweight managers, not as a heavy dynamic runtime.

The next documentation priority is to define v1 scope limits: what is required on ESP32 now, what is deferred, and what each layer means in a microcontroller environment.

## Reviewed Documentation Areas

Reviewed documentation includes:

- Root architecture and schema docs: `ARCHITECTURE.md`, `AI_RULES.md`, `CAPABILITY_SCHEMA.md`, `CAPABILITY_CATALOG.md`, `MODULE_METADATA_SCHEMA.md`, `REGISTRY_SCHEMA.md`, `DEVICE_CLASSIFICATION.md`, `PNP_DISCOVERY_FLOW.md`, `BOOT_SEQUENCE.md`
- Layer README files under `src/hal`, `src/drivers`, `src/devices`, `src/modules`, `src/pnp`, `src/registry`, `src/runtime`, `src/services`, `src/logic`, `src/api`, and `src/dashboard`
- Supporting docs under `src/core`, `src/platform`, `src/protocols`, `src/sdk`, `src/standards`, `src/utils`, `include`, `lib`, and `test`

## Architecture Understanding

Cyber32 is intended to work upward from hardware abstraction to user-facing control.

HAL abstracts ESP32 hardware interfaces. Drivers use HAL to talk to concrete hardware. Devices combine drivers into usable Cyber32 objects. Modules group devices into plug-and-play units. PNP discovers modules, reads metadata, validates compatibility, and registers them. Registry stores modules, devices, services, capabilities, and metadata. Runtime manages lifecycle and scheduling. Services coordinate system-level concerns. Logic binds to capabilities only. API exposes the system. Dashboard presents and controls it.

The central design rule is correct and should be protected:

```text
Logic uses capability IDs.
Logic does not depend on module names.
```

## Contradictions

### 1. Capability examples sometimes use names instead of IDs

Some older README files describe capabilities as human names such as `Position`, `Speed`, `Battery Level`, or `Temperature`. The newer standards correctly require stable IDs such as `CAP_POSITION`, `CAP_SPEED`, and `CAP_BATTERY_LEVEL`.

Risk: developers may accidentally bind Logic or API payloads to display names.

Recommendation: all future examples should use `CAP_*` IDs first and optionally show display names separately.

### 2. PNP Level 3 naming differs between documents

The current standard says:

```text
Level 3: Smart module EEPROM
```

The Cyber32 Bus README still describes Level 3 as `Memory / ID Chip`.

Risk: unclear hardware target for v1 module metadata.

Recommendation: standardize wording to `Smart module EEPROM` and describe allowed physical implementations separately.

### 3. Architecture claims hardware agnostic, but v1 target is ESP32 only

The architecture documents correctly say Cyber32 should remain hardware agnostic. The user requirement now clarifies that Cyber32 v1 targets ESP32 only.

This is not a true contradiction if handled carefully:

- Architecture should remain portable in design.
- Implementation scope should be ESP32-only for v1.

Risk: premature abstraction for future platforms may consume ESP32 resources and delay a working v1.

Recommendation: add a v1 scope document that says ESP32 is the only implementation target, while interfaces must avoid hardcoding ESP32 details above HAL/Drivers.

### 4. Dashboard is documented as a full UI, but ESP32 may not host a full Dashboard

Dashboard docs describe pages, widgets, frontend, backend, assets, logic builder, OTA views, and real-time updates. On ESP32 v1, a full dashboard may be too large if hosted directly on-device.

Risk: flash/RAM pressure and unstable WiFi service if the device serves a heavy UI.

Recommendation: for v1, define Dashboard as a minimal ESP32-hosted control/status UI or API-facing external client. Defer rich dashboard assets and complex logic builder UI.

### 5. Runtime sounds OS-like, but ESP32 Arduino is not a full operating system

Runtime docs include scheduler, tasks, lifecycle, resources, and state machine. ESP32 under Arduino/PlatformIO has loop-based execution plus FreeRTOS underneath, but Cyber32 should not assume desktop process semantics.

Risk: implementing an overly dynamic runtime can cause fragmentation, timing jitter, watchdog resets, and complex failure modes.

Recommendation: define v1 Runtime as cooperative scheduling plus explicit state transitions, with bounded memory and no unbounded dynamic task creation.

### 6. API examples imply HTTP features before security and resource limits are defined

REST and WebSocket docs describe broad control and telemetry. Security docs are conceptual but not yet tied to API behavior.

Risk: exposed control endpoints on ESP32 AP mode can be unsafe or unstable.

Recommendation: define a v1 API surface with authentication expectations, rate limits, command validation, and safe failure responses.

## Duplicated Responsibilities

### 1. Core Registry and top-level Registry overlap

`src/core/registry` and `src/registry` both describe registry responsibilities.

Possible interpretation:

- `src/core/registry` is the internal primitive.
- `src/registry` is the system-level registry domain and collections.

Risk: two registry owners may emerge.

Recommendation: document one owner. Prefer `src/registry` as the system registry layer and `src/core/registry` as shared primitive/interfaces only.

### 2. Device Manager and Device Registry overlap

Device Registry stores Device records and state. Device Manager manages Device objects, searches devices, and tracks lifecycle/events.

Risk: state may split between Registry and Device Manager.

Recommendation: Registry stores facts; Device Manager performs operations and lifecycle coordination. Device Manager should not become a second registry.

### 3. Module Manager and Module Registry overlap

The same issue exists for modules.

Recommendation: Registry stores module records; Module Manager coordinates module lifecycle and service-facing behavior.

### 4. Runtime Lifecycle and Service/Manager lifecycle overlap

Runtime Lifecycle manages component states. Device Manager and Module Manager also describe lifecycle tracking.

Risk: multiple components may attempt to start/stop the same object.

Recommendation: Runtime owns lifecycle state transitions. Managers request transitions and observe results.

### 5. PNP Registration and Registry registration overlap

PNP Registration docs say Registration creates entries in Registry. Registry docs say Registry registers objects.

Recommendation: PNP initiates registration; Registry stores and indexes records. Registry should validate record shape but not discover hardware.

### 6. Protocols and PNP metadata overlap

Protocols define message/metadata transport. PNP defines discovery and metadata workflow.

Recommendation: Protocols define wire format and transport framing. PNP decides when and why to read metadata.

### 7. Dashboard Backend and API overlap

Dashboard Backend uses API and aggregates data. API exposes Cyber32 functionality.

Risk: backend may grow business logic that belongs in Services or API.

Recommendation: Dashboard Backend should remain an adapter/cache/session layer. It should not own system decisions.

## Missing Layers or Concepts

These are not requests to change the fixed layer order. They are missing concepts that should be documented within the existing layers.

### 1. ESP32 v1 scope profile

Missing: a document defining Cyber32 v1 limits for ESP32.

Should include:

- supported ESP32 boards
- Arduino/PlatformIO assumptions
- RAM/flash budgets
- enabled layers for v1
- deferred features
- maximum module count
- maximum capability count
- maximum metadata size
- WiFi/AP mode expectations

### 2. Memory ownership model

Missing: rules for static allocation, dynamic allocation, string ownership, buffers, registry storage, metadata storage, and telemetry buffers.

ESP32 v1 needs this before implementation.

### 3. Error model

Missing: consistent errors for HAL, Drivers, Devices, PNP, Registry, Runtime, Services, Logic, and API.

Needed fields:

```text
code
layer
severity
message
recoverable
timestamp
source_id
```

### 4. Event model

Event Bus exists, but event shape, delivery rules, queue limits, overflow behavior, and priority are not defined.

ESP32 v1 must avoid unbounded event queues.

### 5. Capability value model

Capability IDs are defined, but value payload shapes are not.

Examples needing definition:

- `CAP_POSITION` object fields
- `CAP_MOTOR_CONTROL` command payload
- `CAP_TELEMETRY_STREAM` payload
- units and precision
- stale value handling
- unavailable capability behavior

### 6. Configuration model

Config docs describe responsibility, but config schema, defaults, persistence, validation, and safe rollback are not defined.

### 7. Security minimum for v1

Security Manager is conceptual. ESP32 v1 needs a minimum security baseline.

Examples:

- AP password rules
- local-only vs network control
- command authorization
- OTA trust rules
- metadata trust rules

### 8. Safe boot and recovery model

Boot sequence exists, but safe boot, recovery triggers, rollback, brownout behavior, and repeated crash handling are not defined.

### 9. Timing model

HAL Time, Scheduler, and Runtime mention timing, but no timing budget exists.

Needed:

- control loop period
- sensor polling limits
- WiFi/API handling budget
- PNP scan interval
- watchdog feeding policy
- blocking-call policy

### 10. Persistence model

Storage Manager exists, but the project needs a v1 persistence policy:

- what goes into flash
- what goes into EEPROM/module EEPROM
- what is volatile
- write frequency limits
- wear leveling expectations

### 11. Build/profile model

PlatformIO is present, but no Cyber32 build profiles are documented.

Needed:

- debug vs release
- serial logging levels
- dashboard enabled/disabled
- WiFi enabled/disabled
- PNP scan enabled/disabled
- memory diagnostics enabled/disabled

### 12. Test strategy for architecture boundaries

Tests are only described by generic PlatformIO text. There is no Cyber32 test strategy.

Needed:

- capability schema validation tests
- metadata validation tests
- registry lookup tests
- logic cannot bind module names
- boot sequence state tests
- ESP32 memory budget checks

## Future Scalability Risks

### 1. Registry growth

If Registry stores many strings, metadata blobs, and object records dynamically, ESP32 RAM will disappear quickly.

Mitigation: use fixed-size records, IDs, string tables, and bounded counts for v1.

### 2. Capability catalog growth

The catalog can expand fast. Without versioning and deprecation rules, modules and Logic flows may break.

Mitigation: add catalog versioning and compatibility checks.

### 3. Metadata size growth

Level 2 and Level 3 metadata can become too rich. Dashboard hints, descriptions, calibration, security data, and documentation URLs should not all be loaded into RAM.

Mitigation: define minimal boot metadata and optional extended metadata.

### 4. Service count growth

Services are useful, but too many always-on managers can overload ESP32.

Mitigation: v1 services should be compile-time selectable and lazy where possible.

### 5. Event storm risk

Telemetry, module hotplug, WiFi reconnects, sensor polling, and dashboard updates can generate many events.

Mitigation: bounded queues, event coalescing, and rate limits.

### 6. Dashboard complexity

A rich dashboard served by ESP32 can compete with control timing, WiFi stability, and memory.

Mitigation: v1 dashboard should be minimal and static, with optional external dashboard later.

### 7. Logic complexity

Flows, rules, variables, triggers, and actions can become a full automation engine.

Mitigation: v1 should support a small rule subset and compile/validate flows before execution.

### 8. Hot-plug complexity

Hot-plug is valuable, but real-time removal and replacement can be difficult on ESP32 buses.

Mitigation: v1 can support boot-time discovery first, then controlled rescan, then true hot-plug later.

### 9. Protocol expansion

CAN, UART, I2C, and Network are documented. Supporting all deeply in v1 would spread effort thin.

Mitigation: choose a v1 primary path, likely I2C metadata plus local direct drivers.

### 10. AI-ready scope creep

AI-ready should mean machine-readable contracts and validation, not onboard AI inference for v1.

Mitigation: keep AI features as documentation/schema/tooling support for now.

## ESP32-Specific Risks and Constraints

### Limited RAM

ESP32 RAM is limited, especially when WiFi, WebServer, JSON parsing, strings, and dynamic allocation are used.

Risks:

- registry records consume heap
- metadata parsing fragments memory
- WebSocket buffers compete with telemetry buffers
- `String` usage fragments heap over time
- dashboard assets consume memory

Recommendations:

- define maximum modules, devices, capabilities, and services for v1
- prefer fixed-size buffers and static allocation
- avoid storing full metadata descriptions in RAM
- keep Dashboard minimal
- avoid unbounded JSON parsing
- define memory budget per layer

### Limited Flash Storage

ESP32 flash must hold firmware, filesystem assets, configuration, possible OTA partitions, and metadata caches.

Risks:

- full Dashboard assets may exceed practical space
- OTA requires partition planning
- logs and telemetry can wear flash
- catalog and schema strings can grow

Recommendations:

- define v1 partition strategy before OTA work
- store only minimal UI assets on-device
- make logging ring-buffered and optional
- keep schemas compact in firmware
- avoid writing telemetry frequently to flash

### No Full Operating System

Arduino on ESP32 runs on top of FreeRTOS, but Cyber32 should not assume process isolation, filesystems, or desktop service semantics.

Risks:

- Runtime docs may imply too much dynamic scheduling
- blocking calls can starve control loops
- one crash can take down the whole system

Recommendations:

- model Runtime as cooperative scheduling plus bounded tasks
- define non-blocking requirements for Drivers and Services
- define watchdog and recovery behavior
- avoid per-module dynamic task creation in v1

### Arduino/PlatformIO Constraints

The current project uses PlatformIO with Arduino framework.

Risks:

- library APIs may block
- C++ exceptions and RTTI may be undesirable
- heap fragmentation from Arduino `String`
- dependency size can grow silently
- build flags and partition tables are currently not documented

Recommendations:

- define allowed C++ subset for v1
- define logging and memory diagnostics compile flags
- document dependency approval rules
- document partition table expectations
- keep platform-specific code below HAL/Drivers

### WiFi Stability

ESP32 WiFi can be unstable under heavy load, weak signal, AP+STA mode, or frequent reconnects.

Risks:

- dashboard/API traffic affects control timing
- reconnect loops flood events
- WebSocket clients consume memory
- AP mode with weak security is unsafe

Recommendations:

- rate-limit network events
- cap WebSocket clients
- keep control loops independent from WiFi availability
- define WiFi recovery state machine
- define safe behavior when WiFi drops

### Hardware Timing

Robotics control depends on timing. WiFi, I2C scans, storage writes, and blocking drivers can disturb timing.

Risks:

- servo/motor jitter
- missed sensor reads
- watchdog resets
- unsafe actuator behavior during stalls

Recommendations:

- define timing budgets per loop
- isolate safety-critical actions from dashboard/API handling
- avoid long blocking scans during active motion
- define emergency stop path with minimal dependencies
- define driver timeout policy

### I2C Device Discovery Limits

I2C discovery is useful but limited.

Risks:

- address conflicts
- slow bus scans
- devices that lock the bus
- devices that do not respond until powered or initialized
- no universal metadata standard
- unsafe scanning while devices are active

Recommendations:

- define allowed I2C metadata address ranges
- support address conflict reporting
- perform full scans at boot, not continuously
- provide controlled rescan command
- define bus reset behavior
- keep Level 2 metadata small and deterministic

### EEPROM / Module Metadata Limits

Smart module EEPROM is a good Level 3 target, but EEPROM size, write endurance, speed, and data integrity are limited.

Risks:

- metadata too large for cheap EEPROMs
- partial/corrupt metadata
- slow reads at boot
- write wear from configuration changes
- no trust/authentication for metadata

Recommendations:

- define binary or compact metadata format for v1
- include magic, version, length, checksum, and schema version
- distinguish read-only identity from writable config
- avoid frequent writes
- define maximum metadata size
- define fallback behavior for corrupt metadata

### Runtime Performance

The documented Runtime includes scheduler, tasks, resources, lifecycle, and state machine. That can be too heavy if implemented generically.

Risks:

- too many virtual calls or dynamic allocations
- task queues grow under load
- event dispatch becomes expensive
- telemetry and dashboard updates dominate CPU

Recommendations:

- start with fixed task slots
- use simple state enums
- keep lifecycle transitions explicit
- make telemetry sampling configurable
- measure loop time and free heap continuously in debug builds

### Safe Boot and Recovery

Safe boot is not yet documented deeply enough for robotics.

Risks:

- actuators move unexpectedly at boot
- corrupted config prevents startup
- bad module metadata blocks boot
- repeated crash loop after OTA
- brownout leaves unsafe state

Recommendations:

- define boot-safe actuator defaults
- validate config before applying
- quarantine invalid modules
- provide safe mode with PNP/API/Dashboard minimal
- define OTA rollback before OTA implementation
- define brownout and watchdog recovery states

## Layer-by-Layer Review Notes

### HAL

The HAL docs are clean and appropriately low-level. For ESP32 v1, HAL should expose only the interfaces actually needed by v1 hardware.

Risk: implementing every HAL category too early.

Recommendation: start with GPIO, PWM, I2C, UART, time, and storage if needed by v1.

### Drivers

Driver responsibilities are mostly clear. The docs correctly keep drivers away from Logic and Dashboard.

Risk: drivers may become blocking or allocate memory internally without policy.

Recommendation: add driver timing, timeout, and memory rules.

### Devices

Devices are well-defined as driver-composing objects. Device category docs are useful.

Risk: Devices may duplicate Registry state or Service policy.

Recommendation: Devices should expose state and capabilities, not own global coordination.

### Modules

Modules are well-positioned as PNP units.

Risk: module docs may encourage module-specific Logic examples.

Recommendation: examples should always translate module functionality into capability IDs.

### PNP

PNP flow is now clear.

Risk: true hot-plug may be too ambitious for v1 ESP32.

Recommendation: v1 should prioritize boot-time discovery and controlled rescans before continuous hot-plug.

### Registry

Registry is central and necessary.

Risk: registry can become a RAM-heavy database.

Recommendation: v1 Registry should use fixed-size tables and compact IDs.

### Runtime

Runtime is important but can become too OS-like.

Recommendation: v1 Runtime should be a small cooperative scheduler/lifecycle coordinator, not a general-purpose runtime.

### Services

Services are well categorized.

Risk: too many always-on services for ESP32.

Recommendation: define v1 required services and optional compile-time services.

### Logic

Logic is architecturally strong because it is capability-first.

Risk: full flows/rules/variables engine is too large for v1.

Recommendation: implement only a minimal validated rule/action subset first.

### API

API is useful but broad.

Risk: REST plus WebSocket plus Dashboard can overload ESP32.

Recommendation: v1 API should be minimal, rate-limited, and safe-by-default.

### Dashboard

Dashboard is valuable but likely too broad for ESP32-hosted v1.

Recommendation: v1 Dashboard should be a minimal status/control page or external client using API.

## Priority Recommendations

### Highest Priority

1. Create an ESP32 v1 scope document.
2. Define memory budgets and maximum object counts.
3. Define safe boot and recovery behavior.
4. Define capability payload shapes for the first catalog.
5. Define PNP metadata size and binary/compact format limits.

### Medium Priority

1. Clarify Registry vs Manager responsibilities.
2. Clarify Core Registry vs Registry layer.
3. Define event queue limits and event payload shape.
4. Define API v1 minimal endpoints and security behavior.
5. Define build profiles and PlatformIO settings.

### Lower Priority

1. Expand SDK docs.
2. Expand Dashboard page/widget docs.
3. Add rich telemetry history.
4. Add advanced hot-plug.
5. Add multi-platform support beyond ESP32.

## V1 Scope Recommendation

Cyber32 v1 should prove the architecture with a small ESP32-native vertical slice:

```text
HAL
-> one or two Drivers
-> one or two Devices
-> one Module metadata path
-> PNP boot-time registration
-> compact Registry
-> simple Runtime loop
-> minimal Services
-> capability-first Logic rule
-> small API
-> minimal Dashboard
```

The goal is not to implement every documented subsystem immediately. The goal is to prove that a capability-first ESP32 robotics OS can boot safely, discover or register capabilities, run simple logic, expose state, and recover predictably.

## Final Assessment

Cyber32's documentation is coherent enough to proceed, but it needs an ESP32 v1 constraint layer before implementation. The current architecture should remain fixed, but its v1 implementation must be deliberately small.

The most important design discipline is this:

```text
Keep the architecture complete.
Keep the ESP32 v1 implementation bounded.
Keep Logic capability-first.
```
