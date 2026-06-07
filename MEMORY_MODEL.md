# Cyber32 Memory Model

This document defines the Cyber32 v1 memory model for ESP32.

Cyber32 v1 targets ESP32 only. The implementation must be small, bounded, and resistant to heap fragmentation.

The architecture remains fixed:

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

## Goals

- Keep memory usage predictable.
- Avoid heap fragmentation.
- Keep Registry and metadata bounded.
- Keep Dashboard and API buffers small.
- Preserve enough free heap for WiFi and Arduino internals.
- Fail safely when memory is exhausted.

## Static Allocation Policy

Cyber32 v1 prefers static allocation.

Use static allocation for:

- Registry record tables
- Module records
- Device records
- Capability records
- Service records
- Runtime task slots
- Event queue entries
- PNP metadata read buffer
- API request and response buffers
- Dashboard response buffers
- Telemetry sample buffers
- Logic rule and variable slots

Static allocation rules:

- All core table sizes must be defined by compile-time constants.
- Table limits must match `ESP32_V1_SCOPE.md`.
- Static objects must have clear ownership.
- Static buffers must have explicit maximum lengths.
- Static buffers must be reused instead of recreated.

Recommended v1 static limits:

```text
CYBER32_MAX_MODULES       8
CYBER32_MAX_DEVICES       16
CYBER32_MAX_CAPABILITIES  48
CYBER32_MAX_SERVICES      8
CYBER32_MAX_RULES         16
CYBER32_MAX_FLOWS         4
CYBER32_MAX_VARIABLES     16
CYBER32_MAX_EVENTS        32
CYBER32_MAX_TASKS         16
```

## Dynamic Allocation Policy

Dynamic allocation is allowed only when bounded, short-lived, and justified.

Allowed dynamic allocation:

- temporary parsing scratch space during setup
- short-lived API parsing objects within a request
- library internals that cannot be avoided

Not allowed:

- unbounded allocation per loop iteration
- long-lived heap strings for registry records
- dynamic creation of modules, devices, services, or capabilities beyond fixed slots
- dynamic event queue growth
- dynamic task creation during normal operation
- heap allocation in timing-critical control paths

Rules:

- Allocate during boot when possible.
- Avoid allocation after system state becomes `READY`.
- Free temporary allocations before leaving setup or request scope.
- If allocation fails, return an error and keep the system safe.
- Do not retry allocation in a tight loop.

## Ownership Rules

Every memory object must have a single owner.

Ownership model:

```text
HAL owns HAL driver state.
Drivers own hardware-specific driver state.
Devices own device runtime state.
Modules own module runtime state.
PNP owns temporary discovery and metadata buffers.
Registry owns registered object records.
Runtime owns task, lifecycle, event, and scheduler state.
Services own service-specific state.
Logic owns rules, flows, variables, and bindings.
API owns request/response buffers during request handling.
Dashboard owns only its minimal rendering buffers.
```

Borrowing rules:

- Higher layers may read lower-layer data through documented interfaces.
- Borrowed pointers must not outlive the owner.
- Registry references should use IDs or indexes, not raw pointers, across layers.
- API and Dashboard must copy only the small response data they need.
- Logic must store capability IDs or registry indexes, not pointers to mutable provider internals.

Mutation rules:

- Only the owner may mutate owned state.
- Other layers request changes through the owner API.
- Registry records are updated only through Registry operations.
- Runtime lifecycle state is updated only through Runtime operations.

## Registry Storage Model

Registry must use fixed-size tables for v1.

Tables:

```text
ModuleRecord modules[CYBER32_MAX_MODULES]
DeviceRecord devices[CYBER32_MAX_DEVICES]
CapabilityRecord capabilities[CYBER32_MAX_CAPABILITIES]
ServiceRecord services[CYBER32_MAX_SERVICES]
MetadataRecord metadata[CYBER32_MAX_MODULES + CYBER32_MAX_DEVICES + CYBER32_MAX_SERVICES]
```

Registry records should store:

- compact IDs
- status enums
- type enums
- indexes to related records
- capability IDs
- metadata indexes

Registry records should not store:

- long descriptions
- large metadata blobs
- dashboard layout data
- raw JSON metadata
- large strings copied from module metadata

Recommended fixed field sizes:

```text
id:              32 bytes
name:            32 bytes
type:            enum or 16 bytes
capability_id:   32 bytes
metadata_index:  uint16 or int16
record_index:    uint16 or int16
status:          enum
```

Lookup rules:

- Prefer lookup by ID or index.
- Avoid linear scans in high-frequency control paths.
- Linear scans are acceptable for setup, Dashboard display, and API listing.
- Cache resolved capability indexes for Logic after validation.

## Metadata Storage Model

Metadata is split into minimal runtime metadata and optional extended metadata.

Runtime metadata:

- required for boot
- compact
- stored in fixed records
- kept in RAM only if needed

Extended metadata:

- human descriptions
- documentation URLs
- dashboard hints
- manufacturer text
- calibration details larger than v1 runtime needs

Extended metadata should remain in module EEPROM, flash, or be read on demand.

Maximum v1 metadata sizes:

```text
Level 1 passive ID payload:        <= 16 bytes
Level 1 resolved metadata record:  <= 256 bytes
Level 2 I2C metadata payload:      <= 512 bytes
Level 3 EEPROM metadata payload:   <= 1024 bytes
metadata read buffer:              <= 1024 bytes
metadata string field:             <= 32 bytes
```

Metadata buffer rules:

- Use one shared PNP metadata read buffer.
- Validate length before reading.
- Validate magic, schema version, and checksum.
- Reject metadata that exceeds buffer limits.
- Do not expose capabilities from invalid metadata.
- Do not keep raw metadata in RAM after registration unless required.

## String Handling Rules

Strings are a major fragmentation risk on ESP32.

Rules:

- Avoid long-lived Arduino `String` objects.
- Prefer fixed-size `char` arrays for stored IDs and names.
- Prefer enums for repeated types and states.
- Store capability IDs as fixed-size ASCII strings or compact enum IDs.
- Store display names separately from logic IDs.
- Never bind Logic to display strings.
- Do not build large JSON strings in memory.
- Stream API responses where possible, or use bounded response buffers.

String size limits:

```text
capability ID:       32 bytes
module ID:           32 bytes
device ID:           32 bytes
service ID:          32 bytes
display name:        32 bytes
short description:   96 bytes, optional
API path segment:    48 bytes
```

String safety:

- Always reserve space for null terminators.
- Truncate display text safely when needed.
- Reject truncated IDs.
- Treat IDs as case-sensitive.
- Use ASCII for IDs.

## Buffer Management

All buffers must have explicit owners and maximum sizes.

Recommended v1 buffers:

```text
PNP metadata buffer:        1024 bytes
API request body buffer:    1024 bytes
API response buffer:        4096 bytes
Dashboard render buffer:    4096 bytes
Telemetry sample buffer:    2048 bytes
Event queue payload total:  4096 bytes
Log line buffer:            256 bytes
Driver scratch buffer:      256 bytes per active driver class
```

Rules:

- Reuse shared buffers when operations cannot overlap.
- Do not allocate per event.
- Do not allocate per telemetry sample.
- Do not allocate per dashboard widget.
- Do not store full API responses after sending.
- Do not store telemetry history in RAM by default.

Overflow behavior:

- Reject oversized API requests.
- Reject oversized metadata.
- Drop or coalesce low-priority telemetry events.
- Preserve safety events.
- Report buffer exhaustion through system state or diagnostics.

## Heap Usage Limits

Cyber32 v1 must preserve free heap for WiFi, Arduino internals, network buffers, and emergency handling.

Minimum free heap targets:

```text
after boot without WiFi clients: >= 60 KB
after WiFi starts:               >= 45 KB
under API/Dashboard load:        >= 35 KB
emergency minimum:               >= 25 KB
```

If free heap falls below emergency minimum:

1. Reject new API requests.
2. Disable optional Dashboard updates.
3. Reduce telemetry frequency.
4. Prevent PNP rescans.
5. Enter `WARNING` or `ERROR` state depending on severity.
6. Keep actuator safety behavior available.

Debug builds should expose:

```text
free_heap
minimum_free_heap
largest_free_block
heap_warning_count
allocation_failure_count
```

## Fragmentation Avoidance Strategy

ESP32 heap fragmentation must be treated as a design constraint.

Strategy:

- Prefer static allocation.
- Allocate long-lived objects at boot.
- Avoid creating and destroying variable-sized objects repeatedly.
- Avoid long-lived `String`.
- Avoid large temporary JSON documents.
- Avoid per-request heap growth.
- Reuse buffers.
- Keep Dashboard responses small.
- Keep telemetry payloads small.
- Use fixed-size pools for events and tasks.
- Disable optional features when memory budget is tight.

Request handling strategy:

```text
receive request into bounded buffer
validate size
parse only required fields
execute command or query
write bounded response
release temporary parser state
```

PNP parsing strategy:

```text
read metadata into shared buffer
validate length and checksum
extract compact fields
register compact records
discard raw metadata buffer
```

## RAM Budget Per Layer

Target v1 RAM budget:

| Layer | Target RAM | Notes |
|---|---:|---|
| HAL | 4 KB | Interface state, timers, bus handles. |
| Drivers | 8 KB | Small driver state and scratch buffers. |
| Devices | 6 KB | Device state and capability links. |
| Modules | 4 KB | Module state and device indexes. |
| PNP | 6 KB | Shared metadata buffer and validation scratch. |
| Registry | 16 KB | Fixed tables for modules, devices, capabilities, services, metadata. |
| Runtime | 10 KB | Scheduler, lifecycle, tasks, event queue. |
| Services | 12 KB | Managers, telemetry, storage, network state. |
| Logic | 8 KB | Rules, flows, variables, resolved capability bindings. |
| API | 12 KB | Request and response buffers. |
| Dashboard | 8 KB | Minimal page rendering buffers. |
| Logging/Diagnostics | 2 KB | Serial log buffer and counters. |
| Safety reserve | 40 KB minimum free heap | Required after boot. |

Total Cyber32 target before reserve:

```text
approximately 96 KB RAM
```

This budget is a ceiling, not a goal. Lower usage is better.

## Layer-Specific Notes

### HAL

HAL must avoid storing high-level objects. It owns only platform and bus state.

### Drivers

Drivers must avoid unbounded buffers. Any driver requiring large buffers must document them.

### Devices

Devices should store state, status, and capability indexes. They should not copy metadata blobs.

### Modules

Modules should store identity, status, device indexes, and capability indexes. Long metadata stays out of module RAM.

### PNP

PNP may use temporary buffers during discovery. PNP must release or reuse those buffers after registration.

### Registry

Registry is fixed-size and index-based. It must reject new records when full.

### Runtime

Runtime owns fixed task and event pools. It must not grow queues dynamically.

### Services

Services must have explicit buffers and must be disableable by compile-time feature flags where possible.

### Logic

Logic stores validated rules and resolved capability IDs/indexes. Logic does not store module names.

### API

API uses bounded request and response buffers. Oversized requests are rejected.

### Dashboard

Dashboard is minimal and should avoid large templates, asset buffers, or client session state.

## Failure Behavior

Memory failure must be safe.

Rules:

- Failed allocation must not crash the system.
- Failed registration must leave the module unavailable.
- Failed metadata parsing must reject the module.
- Failed API buffer allocation must reject the request.
- Failed Dashboard rendering must return a minimal error page or no page.
- Logic must not execute rules with unresolved capabilities.
- Actuators must remain or return to safe state.

## Review Requirement

Any feature that increases memory usage must document:

- static memory added
- possible heap memory added
- buffer sizes
- maximum object counts affected
- failure behavior when memory is unavailable

If the feature cannot stay within this memory model, it is not part of ESP32 v1.
