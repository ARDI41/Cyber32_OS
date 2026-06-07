# Cyber32 Runtime Implementation Plan

This document defines the Cyber32 v1 Runtime implementation for ESP32.

This plan follows:

- `ARCHITECTURE.md`
- `BOOT_SEQUENCE.md`
- `EVENT_MODEL.md`
- `MEMORY_MODEL.md`
- `ERROR_MODEL.md`
- `REGISTRY_IMPLEMENTATION_PLAN.md`
- `ESP32_V1_SCOPE.md`

Cyber32 v1 targets ESP32 only.

## Purpose

Runtime coordinates system execution.

Runtime is responsible for lifecycle, scheduling, task execution, event processing, timing, watchdog coordination, and controlled degraded/recovery behavior.

Runtime does not store system facts.

Runtime does not replace Registry.

Runtime does not implement Logic.

## Architecture Position

The fixed Cyber32 architecture remains:

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

Registry stores state and facts.

Runtime coordinates execution using Registry, Events, Services, and Logic through documented contracts.

## Runtime Responsibilities

Runtime is responsible for:

- system execution coordination
- boot state transitions
- component lifecycle transitions
- cooperative scheduling
- bounded task execution
- bounded event processing
- watchdog-aware loop behavior
- degraded mode coordination
- safe mode entry
- recovery coordination
- timing diagnostics
- runtime error reporting

Runtime owns:

- Runtime state machine
- task slots
- scheduler state
- event pump budget
- lifecycle transition coordinator
- runtime timing counters
- watchdog interaction state

Runtime does not own:

- module facts
- device facts
- capability facts
- service facts
- capability payload state
- latest error state for registered objects
- Logic rules
- Dashboard decisions

Those belong to Registry, Services, Logic, API, and Dashboard according to their contracts.

## Runtime State Machine

Cyber32 v1 Runtime uses a compact state machine.

Top-level states:

```text
BOOTING
INITIALIZING
DISCOVERING
REGISTERING
STARTING
READY
RUNNING
DEGRADED
SAFE_MODE
RECOVERY
ERROR
SHUTDOWN
```

State transitions must be explicit and bounded.

Invalid transitions must raise:

```text
ERR_RUNTIME_INVALID_TRANSITION
```

If this code is not implemented in v1, use:

```text
ERR_RUNTIME_OVERLOAD
```

or a documented Runtime error code until the catalog expands.

## Boot States

### BOOTING

System has started but layers are not initialized.

Allowed work:

- initialize minimal diagnostics
- set actuator-safe defaults
- initialize HAL prerequisites

Not allowed:

- run Logic
- accept API commands
- expose Dashboard control

### INITIALIZING

Core layer setup is in progress.

Allowed work:

- initialize HAL
- initialize Drivers
- prepare Devices
- prepare Modules
- initialize Event Bus
- initialize Registry tables

### DISCOVERING

PNP discovery is active.

Allowed work:

- boot-time PNP scan
- passive ID detection
- I2C metadata read
- optional EEPROM metadata read

Rules:

- discovery must be bounded
- invalid modules must not block boot
- active motion must not occur during discovery

### REGISTERING

Validated PNP records are being registered.

Allowed work:

- call Registry registration APIs
- store compact records
- emit Registry and Capability events

Rules:

- partial failure must leave records unavailable
- full tables must return `ERR_REGISTRY_FULL`

### STARTING

Runtime starts Services, Logic, API, and Dashboard in safe order.

Allowed work:

- start required Services
- resolve Logic capability bindings
- start minimal API
- start minimal Dashboard

## Normal Operation States

### READY

The system completed startup and is ready to run.

Allowed work:

- accept API commands
- update Services
- evaluate Logic
- serve Dashboard
- process events

### RUNNING

The normal update loop is active.

Allowed work:

- scheduled task execution
- event processing
- service updates
- capability payload updates
- Logic evaluation
- API servicing
- Dashboard servicing

`READY` and `RUNNING` may be represented by one runtime state in v1 if memory or code simplicity requires it.

## Degraded States

### DEGRADED

The system is operational but one or more capabilities, devices, modules, services, or resources are unavailable.

Examples:

- WiFi unavailable
- optional module missing
- storage write failed
- telemetry reduced
- non-critical service degraded

Rules:

- safety-critical features must remain safe
- API must expose degraded state
- Dashboard must display degraded state
- Logic must see capability unavailable/stale/error state

## Safe Mode States

### SAFE_MODE

Safe mode is used when normal operation is unsafe.

Safe mode behavior:

- actuator outputs disabled or safe
- optional PNP rescans disabled
- optional Dashboard features disabled
- API commands restricted
- Logic execution restricted or disabled
- recovery diagnostics exposed if safe

Triggers:

- repeated watchdog reset
- fatal error
- invalid config with no safe default
- safety-critical actuator failure
- repeated boot failure

Fatal errors must enter safe mode.

## Recovery States

### RECOVERY

Recovery attempts to return from degraded or safe state to normal operation.

Allowed work:

- reload safe defaults
- retry required service startup
- clear recoverable errors after verification
- mark unavailable modules as unavailable
- expose recovery state through API

Not allowed:

- hide active fatal errors
- re-enable actuator outputs without validation
- clear repeated boot errors before recovery completes

### ERROR

The system encountered an unrecovered error.

Behavior:

- preserve safe actuator state
- reject unsafe API commands
- keep diagnostics available if possible
- allow restart or safe recovery only

### SHUTDOWN

Controlled shutdown or restart state.

Behavior:

- stop scheduled tasks
- stop services
- flush safe minimal state if allowed
- keep actuator outputs safe

## Runtime Lifecycle

Runtime coordinates lifecycle for components.

Lifecycle states:

```text
CREATED
INITIALIZED
STARTING
RUNNING
DEGRADED
STOPPING
STOPPED
ERROR
RECOVERY
DISABLED
```

Lifecycle rules:

- Runtime owns lifecycle transitions.
- Managers and Services may request transitions.
- Registry stores latest state facts.
- Events announce lifecycle changes.
- Invalid transitions raise Runtime errors.

Component types with lifecycle:

- Runtime tasks
- Services
- Modules
- Devices
- API surface
- Dashboard surface

## Startup Flow

Cyber32 v1 startup follows the boot sequence:

```text
Power on
-> set safe actuator defaults
-> initialize Event Bus
-> initialize Registry fixed tables
-> initialize HAL
-> initialize Drivers
-> prepare Devices
-> prepare Modules
-> run PNP discovery
-> register accepted objects in Registry
-> start Runtime scheduler
-> start Services
-> bind Logic to capabilities
-> start API
-> start Dashboard
-> enter READY/RUNNING
```

Runtime must not mark the system ready until:

- Registry is initialized
- required Services are started or marked degraded
- Logic has resolved required capability bindings or marked rules disabled
- API is safe to accept commands
- actuators are in known safe state

## Update Loop

Cyber32 v1 uses a cooperative update loop.

Conceptual loop:

```text
while running:
  read time
  feed or check watchdog policy
  process critical events
  run due Runtime tasks within budget
  update Services within budget
  evaluate Logic within budget
  service API within budget
  service Dashboard within budget
  process remaining events within budget
  update timing diagnostics
```

Rules:

- each loop iteration must be bounded
- no unbounded blocking calls
- no dynamic memory allocation after boot
- no dynamic task creation
- API and Dashboard must not starve safety or control work
- low-priority telemetry may be skipped under load

## Scheduler Model

Cyber32 v1 uses a lightweight cooperative scheduler.

Allowed:

- fixed task slots
- periodic tasks
- one-shot tasks using fixed slots
- priority bands
- task enable/disable
- bounded execution budget

Not allowed:

- unbounded task creation
- heap-allocated tasks
- desktop-style service threads
- long blocking tasks
- task queues that grow dynamically

Task priority bands:

```text
critical
high
normal
low
```

Recommended execution order:

```text
critical safety tasks
high lifecycle/service tasks
normal sensor/service updates
low telemetry/dashboard tasks
```

## Task Model

Task record fields:

```text
task_id
state
priority
period_ms
next_run_ms
last_run_ms
timeout_ms
owner_layer
owner_id
run_count
fail_count
enabled
```

Maximum task count:

```text
CYBER32_MAX_TASKS = 16
```

Task states:

```text
CREATED
WAITING
RUNNING
COMPLETED
FAILED
CANCELLED
DISABLED
```

Rules:

- task slots are fixed
- tasks are registered during boot or setup
- tasks are enabled/disabled at runtime
- tasks must return quickly
- timeout failures raise Runtime errors
- repeated task failures may degrade a Service or enter safe mode

## Event Processing Model

Runtime pumps the bounded Event Bus.

Event limits:

```text
maximum event queue entries: 32
maximum event slot size: 128 bytes
maximum event payload: 16 bytes inline
dynamic allocation: not allowed
```

Runtime event responsibilities:

- process critical events first
- enforce per-loop event processing budget
- route events to registered consumers
- count dropped/coalesced events
- raise overload errors when needed
- prevent event handling from blocking

Overflow behavior:

- preserve critical events
- preserve safety-related high events
- coalesce repeated low-priority capability value updates
- drop low-priority telemetry events if required
- emit or retain `EVT_RUNTIME_OVERLOAD` or `EVT_ERROR_RAISED`

Runtime must not use Event Bus as storage.

Consumers needing current state must query Registry or owning Services.

## Registry Interaction

Runtime uses Registry for facts and current state.

Runtime may query:

- registered Services
- registered Devices
- registered Modules
- available Capabilities
- latest error state
- latest payload state

Runtime may request Registry updates only through documented Registry APIs.

Runtime must not:

- store duplicate Registry facts
- own capability payload state
- own module/device/service facts
- perform hardware discovery

Registry stores state. Runtime coordinates execution.

## Service Interaction

Runtime starts, stops, updates, and monitors Services.

Services may request:

- task registration during boot
- lifecycle transitions
- event subscriptions
- degraded state reporting

Runtime may:

- call service update hooks
- enforce service timing budgets
- mark service lifecycle state
- raise service timeout errors
- coordinate recovery attempts

Runtime must not implement Service policy.

Examples:

- Power Manager decides power policy.
- Telemetry Manager decides telemetry sampling.
- Storage Manager decides persistence behavior.
- Runtime decides when each update may run.

## Watchdog Interaction

ESP32 v1 Runtime must be watchdog-aware.

Rules:

- main loop must not block indefinitely
- long operations must be split into bounded steps
- watchdog reset cause must be detected when possible
- repeated watchdog reset must trigger recovery or safe mode
- watchdog recovery state must be exposed through Registry/API

Watchdog-related errors:

```text
ERR_WATCHDOG_RESET
ERR_WATCHDOG_BOOT_LOOP
```

Watchdog-related events:

```text
EVT_ERROR_RAISED
EVT_ERROR_FATAL
EVT_RUNTIME_OVERLOAD
```

Repeated watchdog resets:

```text
first reset: record and continue with caution
repeated reset: enter RECOVERY or SAFE_MODE
boot loop: enter SAFE_MODE
```

## Timing Model

Runtime timing must be bounded.

Recommended v1 targets:

```text
main loop target:              <= 20 ms
event publish target:          <= 1 ms
single event handler target:   <= 2 ms
event pump per loop target:    <= 5 ms
single task target:            <= 5 ms
service update target:         <= 5 ms per service
API processing target:         <= 10 ms per loop slice
Dashboard processing target:   <= 10 ms per loop slice
PNP boot scan target:          <= 1000 ms total
```

Rules:

- safety and control work take priority over API/Dashboard
- PNP scanning must not run during active motion unless explicitly safe
- telemetry may be rate-limited under load
- Dashboard updates may be skipped under load
- Runtime overload raises `ERR_RUNTIME_OVERLOAD`

## Memory Limits

Runtime uses bounded memory.

Recommended v1 Runtime budget:

```text
Runtime total:              <= 10 KB RAM
task table:                 <= 2 KB
event processing state:     <= 1 KB
lifecycle state table:      <= 2 KB
timing diagnostics:         <= 1 KB
watchdog/recovery state:    <= 1 KB
reserve/alignment:          <= 3 KB
```

Global related limits:

```text
CYBER32_MAX_TASKS  16
CYBER32_MAX_EVENTS 32
event slot size    128 bytes
```

Rules:

- no dynamic task creation
- no dynamic memory allocation after boot
- no unbounded queues
- no large strings
- no large event payloads
- no stored API or Dashboard buffers inside Runtime

## ESP32 Constraints

Runtime must respect ESP32 constraints:

- limited RAM
- limited flash
- Arduino/PlatformIO framework
- no full operating system
- WiFi can block or destabilize timing
- I2C scans may block
- storage writes may block
- watchdog resets are possible
- heap fragmentation must be avoided

Implementation rules:

- use fixed arrays
- avoid `std::vector`, `std::map`, and unbounded containers
- avoid Arduino `String` in Runtime state
- avoid blocking network calls in Runtime
- avoid long I2C scans during normal operation
- avoid flash writes in timing-critical paths
- keep Runtime code small

## Error Handling

Runtime raises compact error codes.

Runtime errors include:

```text
ERR_RUNTIME_OVERLOAD
ERR_WATCHDOG_RESET
ERR_WATCHDOG_BOOT_LOOP
ERR_REGISTRY_FULL
ERR_CAPABILITY_UNAVAILABLE
```

Rules:

- recoverable errors may move system to `DEGRADED`
- fatal errors must move system to `SAFE_MODE`
- repeated boot errors must move system to `RECOVERY` or `SAFE_MODE`
- safety-critical actuator errors must fail safe
- Runtime must emit `EVT_ERROR_*` for error state changes
- Registry stores latest error state

## Degraded Operation

Runtime enters degraded operation when non-fatal failures exist.

Examples:

- optional service failed
- optional module missing
- telemetry overflow
- WiFi unavailable
- storage write failed
- stale non-critical capability

Degraded behavior:

- continue safety-critical operation
- reduce optional work
- expose degraded state through API/Dashboard
- keep Logic capability-first
- allow recovery attempts

## Safe Mode Operation

Safe mode is for unsafe or fatal conditions.

Safe mode behavior:

- disable actuator outputs unless explicitly safe
- stop or reject motion commands
- disable optional Dashboard features
- disable optional PNP rescans
- restrict API commands
- keep diagnostic API available if safe
- prevent Logic from executing unsafe actions

Safe mode exits only after recovery validation.

## Recovery Operation

Recovery attempts to return to normal operation.

Recovery may:

- reload safe config
- retry Service startup
- mark missing capabilities unavailable
- clear recoverable errors after verification
- restart API/Dashboard surfaces
- request controlled PNP rescan while idle

Recovery must not:

- clear active fatal errors automatically
- re-enable unsafe actuator outputs without validation
- hide repeated watchdog resets
- bypass Registry state

## Example Flow

Required example:

```text
Boot
-> PNP
-> Registry
-> Runtime Ready
-> Service Update
-> Logic Evaluation
-> API Update
```

Expanded flow:

```text
Power on
-> Runtime enters BOOTING
-> safe actuator defaults applied
-> Event Bus initialized
-> Registry fixed tables initialized
-> HAL initialized
-> Drivers initialized
-> Devices prepared
-> Modules prepared
-> Runtime enters DISCOVERING
-> PNP detects module metadata
-> PNP validates metadata
-> Runtime enters REGISTERING
-> Registry stores module/device/capability records
-> Registry emits EVT_CAPABILITY_REGISTERED
-> Runtime enters STARTING
-> Services start
-> Logic resolves capability IDs
-> API starts
-> Dashboard starts
-> Runtime enters READY/RUNNING
-> Service Update reads device/provider state
-> Registry updates latest capability payload
-> Event Bus announces EVT_CAPABILITY_VALUE_UPDATED
-> Logic Evaluation reads CAP_* payloads from Registry
-> API Update exposes current state
```

Key rule:

```text
Logic evaluates CAP_* capabilities.
Logic does not know module names.
```

## v1 Limitations

Cyber32 v1 Runtime does not support:

- preemptive Cyber32 task scheduling
- unbounded dynamic task creation
- desktop-style service processes
- long-running blocking tasks
- full hot-plug during active motion
- large event histories
- distributed runtime nodes
- multi-core runtime policy
- dynamic plugin execution
- onboard AI inference scheduling

These are v2+ considerations.

## Success Criteria

Cyber32 v1 Runtime is successful when:

1. It coordinates boot to READY/RUNNING.
2. It uses bounded memory.
3. It uses fixed task slots.
4. It processes bounded event slices.
5. It performs no dynamic memory allocation after boot.
6. It never replaces Registry as state storage.
7. It never implements Logic rules.
8. It coordinates Services without owning Service policy.
9. It handles watchdog reset and boot loop recovery.
10. It enters safe mode for fatal errors.
