# Cyber32 Event Model

This document defines how Cyber32 v1 communicates internally between layers using events.

Cyber32 v1 targets ESP32 only. The event model must be lightweight, bounded, and safe for ESP32 RAM limits.

## Purpose

Events announce that something changed.

Events are not storage. Events are not commands by default. Events are not large data containers.

Registry stores state. Event Bus announces changes.

## Architecture Context

The fixed Cyber32 layer order remains:

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

Events may move information upward or laterally through documented consumers, but events must not create hidden dependencies that bypass the architecture.

## What An Event Is

An event is a compact record that describes a state change, status change, value update, error, or lifecycle transition.

An event should answer:

```text
what happened
where it happened
when it happened
how important it is
which object changed
```

An event should not contain:

- large metadata blobs
- raw Dashboard/UI data
- large API responses
- long strings
- full registry records
- full telemetry history
- heap-owned payloads

## Event Record Fields

Every event record must fit in a fixed-size slot.

Required fields:

```text
event_id
category
priority
timestamp_ms
source_layer
source_id
target_id
status
value_type
value
```

Field definitions:

### event_id

Compact event identifier.

Example:

```text
EVT_MODULE_DISCOVERED
```

### category

High-level event family.

Example:

```text
EVT_MODULE_*
```

### priority

Delivery importance.

Allowed values:

```text
critical
high
normal
low
```

### timestamp_ms

System uptime timestamp in milliseconds.

### source_layer

Layer that emitted the event.

Allowed values:

```text
hal
drivers
devices
modules
pnp
registry
runtime
services
logic
api
dashboard
```

### source_id

Optional ID for the emitting object.

Examples:

```text
module:module-power-001
device:device-battery-001
service:power-manager
```

### target_id

Optional ID for the object that changed.

Events may reference:

- `module_id`
- `device_id`
- `capability_id`
- `service_id`

### status

Compact status code.

Examples:

```text
available
unavailable
registered
updated
failed
warning
error
```

### value_type

Type of the optional compact value.

Allowed values:

```text
none
boolean
integer
float
enum
id
error_code
```

### value

Small inline value.

Examples:

```text
true
42
19.5
CAP_BATTERY_LEVEL
ERR_METADATA_INVALID
```

The value must be small enough to fit inside the fixed event slot. Large values belong in Registry, Services, or telemetry buffers, not the event.

## Recommended Fixed Event Slot

Cyber32 v1 should target an event slot no larger than 128 bytes.

Recommended logical shape:

```text
event_id:      uint16 or fixed compact enum
category:      uint8
priority:      uint8
timestamp_ms:  uint32
source_layer:  uint8
source_id:     compact ID reference or 32-byte fixed string
target_id:     compact ID reference or 32-byte fixed string
status:        uint8
value_type:    uint8
value:         16-byte inline union
```

String IDs are allowed in documentation and early implementation, but compact numeric IDs or indexes are preferred after Registry assigns them.

## Event ID Naming Rules

Event IDs must:

- start with `EVT_`
- use uppercase ASCII letters, numbers, and underscores
- include a category prefix
- describe the change that occurred
- avoid module names
- avoid manufacturer names
- avoid Dashboard/UI-only terminology

Correct:

```text
EVT_CAPABILITY_REGISTERED
EVT_DEVICE_LOST
EVT_MODULE_DISCOVERED
EVT_ERROR_RAISED
```

Incorrect:

```text
EVT_GPS_MODULE_READY
EVT_ACME_BOARD_CLICKED
EVT_DASHBOARD_CARD_OPENED
```

## Allowed Event Categories

Required categories:

```text
EVT_BOOT_*
EVT_DEVICE_*
EVT_MODULE_*
EVT_CAPABILITY_*
EVT_PNP_*
EVT_REGISTRY_*
EVT_RUNTIME_*
EVT_SERVICE_*
EVT_ERROR_*
```

Allowed v1 event IDs:

```text
EVT_BOOT_STARTED
EVT_BOOT_LAYER_READY
EVT_BOOT_READY
EVT_BOOT_FAILED

EVT_DEVICE_REGISTERED
EVT_DEVICE_READY
EVT_DEVICE_UPDATED
EVT_DEVICE_LOST
EVT_DEVICE_ERROR

EVT_MODULE_DISCOVERED
EVT_MODULE_IDENTIFIED
EVT_MODULE_REGISTERED
EVT_MODULE_READY
EVT_MODULE_REMOVED
EVT_MODULE_UNSUPPORTED

EVT_CAPABILITY_REGISTERED
EVT_CAPABILITY_AVAILABLE
EVT_CAPABILITY_UNAVAILABLE
EVT_CAPABILITY_VALUE_UPDATED
EVT_CAPABILITY_ERROR

EVT_PNP_SCAN_STARTED
EVT_PNP_SCAN_COMPLETED
EVT_PNP_METADATA_READ
EVT_PNP_METADATA_INVALID
EVT_PNP_VALIDATION_FAILED

EVT_REGISTRY_RECORD_ADDED
EVT_REGISTRY_RECORD_UPDATED
EVT_REGISTRY_RECORD_REMOVED
EVT_REGISTRY_FULL

EVT_RUNTIME_STARTED
EVT_RUNTIME_TASK_STARTED
EVT_RUNTIME_TASK_FAILED
EVT_RUNTIME_OVERLOAD

EVT_SERVICE_STARTED
EVT_SERVICE_STOPPED
EVT_SERVICE_DEGRADED
EVT_SERVICE_ERROR

EVT_ERROR_RAISED
EVT_ERROR_RECOVERED
EVT_ERROR_FATAL
```

New event IDs must be documented before use.

## Which Layers May Emit Events

Allowed emitters:

| Layer | May Emit | Notes |
|---|---|---|
| HAL | Yes | Hardware readiness, bus errors, timing faults. |
| Drivers | Yes | Driver ready, read failure, timeout, device communication issue. |
| Devices | Yes | Device state and value changes. |
| Modules | Yes | Module state changes. |
| PNP | Yes | Discovery, metadata, validation, registration requests. |
| Registry | Yes | Record added, updated, removed, full. |
| Runtime | Yes | Scheduler, lifecycle, task, overload events. |
| Services | Yes | Service state, telemetry, power, network, storage events. |
| Logic | Yes | Rule fired, flow state, logic error. |
| API | Limited | API error, command accepted/rejected, client state if needed. |
| Dashboard | No for v1 internal Event Bus | Dashboard/UI-only interactions must not enter the core event bus. |

Dashboard may consume events through API or service projections, but Dashboard click/UI events are not Cyber32 core events in v1.

## Which Layers May Consume Events

Allowed consumers:

| Layer | May Consume | Notes |
|---|---|---|
| HAL | No | HAL should stay low-level. |
| Drivers | Limited | Only driver-relevant hardware/runtime events if needed. |
| Devices | Limited | Device-relevant driver/runtime events. |
| Modules | Limited | Module-relevant device/PNP events. |
| PNP | Yes | Discovery, metadata, registry, error events. |
| Registry | Limited | Registration requests and removal/update events only. |
| Runtime | Yes | Lifecycle, scheduling, overload, service events. |
| Services | Yes | Device, module, capability, runtime, error events. |
| Logic | Yes | Capability events and selected system events. |
| API | Yes | To expose state changes and errors. |
| Dashboard | Indirect only | Through API, not direct Event Bus access in v1. |

Logic rule:

```text
Logic reacts to capability events, not module names.
```

For example, Logic may consume `EVT_CAPABILITY_VALUE_UPDATED` for `CAP_BATTERY_LEVEL`. It must not consume `EVT_MODULE_*` to target a specific named module for automation decisions.

## Event Queue Limits

Cyber32 v1 uses a fixed-size queue.

Limits:

```text
maximum event queue entries: 32
maximum event slot size:     128 bytes
maximum event payload:       16 bytes inline
dynamic allocation:          not allowed
queue growth:                not allowed
```

Memory target:

```text
32 entries * 128 bytes = 4096 bytes
```

This matches the ESP32 v1 memory model.

## Overflow Behavior

No unbounded queues are allowed.

If the event queue is full:

1. Preserve `critical` events.
2. Preserve safety-related `high` events.
3. Drop or coalesce `low` telemetry-style updates.
4. Coalesce repeated `EVT_CAPABILITY_VALUE_UPDATED` events for the same capability.
5. Increment an overflow counter.
6. Emit or retain one `EVT_RUNTIME_OVERLOAD` or `EVT_ERROR_RAISED` event if possible.

Overflow must not allocate memory.

Overflow counters:

```text
event_overflow_count
event_dropped_low_count
event_coalesced_count
event_critical_preserved_count
```

If overflow persists, Runtime should enter `WARNING` or `ERROR` depending on severity.

## Priority Rules

Priority order:

```text
critical
high
normal
low
```

Critical examples:

- emergency stop
- fatal error
- watchdog recovery
- unsafe actuator state
- brownout warning if available

High examples:

- device lost
- capability unavailable
- module removed
- registry full
- service degraded

Normal examples:

- module discovered
- capability registered
- service started
- boot layer ready

Low examples:

- periodic telemetry value update
- signal strength update
- dashboard polling hint

Priority rules:

- Critical events must be processed before lower priority events.
- Low events may be dropped or coalesced.
- Critical events should not be dropped unless the system is already unrecoverable.
- Event handlers must stay short.

## Delivery Rules

Cyber32 v1 uses in-process delivery through a bounded Event Bus.

Rules:

- Delivery is best-effort except for critical safety events.
- Event handlers must not block.
- Event handlers must not allocate memory.
- Event handlers must not perform long I/O.
- Event handlers may update owned state or request work from Runtime.
- Event handlers must not mutate Registry directly unless they are Registry-owned handlers.
- Event Bus must not store long-term state.
- Event Bus must not become a second Registry.

Registry rule:

```text
Registry stores current state.
Event Bus announces state changes.
```

Consumers that need current state must query Registry or the owning Service, not inspect old events.

## Timing Rules

Events must not break control timing.

Targets:

```text
event publish target:       <= 1 ms
single handler target:      <= 2 ms
event pump per loop target: <= 5 ms
critical event latency:     as soon as possible
```

Rules:

- Event processing should be bounded per loop.
- Long work must be converted into Runtime tasks.
- PNP scans must not flood the event queue.
- Telemetry updates must be rate-limited.
- API/Dashboard traffic must not starve event processing.
- Control and safety events must have priority over UI and telemetry events.

## Event Data Rules

Events may reference:

- `module_id`
- `device_id`
- `capability_id`
- `service_id`

Events must not contain:

- large metadata blobs
- full module metadata
- full registry records
- raw Dashboard HTML
- user interface layout state
- large telemetry history

If more data is needed:

1. Put current state in Registry or the owning Service.
2. Emit an event with the compact ID.
3. Let consumers query the owner for details.

## Examples

### Module Discovered

```text
event_id: EVT_MODULE_DISCOVERED
category: EVT_MODULE_*
priority: normal
timestamp_ms: 1250
source_layer: pnp
source_id: pnp:discovery
target_id: module:module-unknown-001
status: discovered
value_type: enum
value: pnp_level_2
```

Meaning:

PNP detected a possible Level 2 module. Metadata has not necessarily been validated yet.

### Capability Registered

```text
event_id: EVT_CAPABILITY_REGISTERED
category: EVT_CAPABILITY_*
priority: normal
timestamp_ms: 1680
source_layer: registry
source_id: registry:capabilities
target_id: capability:CAP_POSITION
status: registered
value_type: id
value: module-navigation-001
```

Meaning:

Registry registered `CAP_POSITION` and associated it with a provider. Consumers that need details should query Registry.

### Device Lost

```text
event_id: EVT_DEVICE_LOST
category: EVT_DEVICE_*
priority: high
timestamp_ms: 42100
source_layer: devices
source_id: device:device-distance-001
target_id: device:device-distance-001
status: unavailable
value_type: error_code
value: ERR_DEVICE_TIMEOUT
```

Meaning:

A device stopped responding. Related capabilities should become unavailable through Registry or owning Service updates.

### Error Raised

```text
event_id: EVT_ERROR_RAISED
category: EVT_ERROR_*
priority: high
timestamp_ms: 43000
source_layer: runtime
source_id: runtime:scheduler
target_id: service:telemetry-manager
status: error
value_type: error_code
value: ERR_EVENT_QUEUE_OVERFLOW
```

Meaning:

Runtime raised an error. Consumers should query Runtime diagnostics for counters and details.

### Battery Value Updated

```text
event_id: EVT_CAPABILITY_VALUE_UPDATED
category: EVT_CAPABILITY_*
priority: low
timestamp_ms: 52000
source_layer: services
source_id: service:power-manager
target_id: capability:CAP_BATTERY_LEVEL
status: updated
value_type: float
value: 19.5
```

Meaning:

The battery level changed. Logic may react to `CAP_BATTERY_LEVEL`, for example:

```text
IF CAP_BATTERY_LEVEL < 20
THEN CAP_SEND_NOTIFICATION
```

Logic must not bind this rule to a specific Battery Module name.

## Safety Rules

- No unbounded queues.
- No dynamic memory allocation for events in v1.
- Events must use compact IDs.
- Events must not contain large metadata blobs.
- Events must not contain Dashboard/UI-only data.
- Events must be safe for ESP32 RAM limits.
- Event Bus must not become a second Registry.
- Registry stores state; Event Bus only announces changes.
- Logic must react to capability events, not module names.

## V1 Success Criteria

The Cyber32 v1 Event Model is successful when:

1. Events are published without heap allocation.
2. Queue memory is fixed and bounded.
3. Overflow behavior is predictable.
4. Safety and error events are prioritized.
5. Registry remains the source of truth.
6. Logic reacts to capability IDs.
7. Dashboard/UI data stays outside the core Event Bus.
