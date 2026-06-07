# Cyber32 Registry Implementation Plan

This document defines how the Cyber32 v1 Registry is implemented on ESP32 using fixed-size, memory-bounded tables.

This plan follows:

- `ARCHITECTURE.md`
- `REGISTRY_SCHEMA.md`
- `MEMORY_MODEL.md`
- `EVENT_MODEL.md`
- `CAPABILITY_PAYLOAD_SCHEMA.md`
- `ERROR_MODEL.md`
- `ESP32_V1_SCOPE.md`

Cyber32 v1 targets ESP32 only.

## Purpose

Registry is the central store of current system facts and state.

Registry answers:

```text
what modules exist
what devices exist
what capabilities exist
what services exist
what payload state is latest
what error state is latest
```

Registry does not discover, control, decide, render, or execute behavior.

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

PNP initiates registration.

Runtime and Services may query Registry.

Logic queries capabilities only.

API exposes Registry state through documented contracts.

Dashboard displays Registry/API state but does not own Registry decisions.

## Registry Responsibilities

Registry is responsible for:

- storing module records
- storing device records
- storing service records
- storing capability records
- storing compact metadata references
- storing latest capability payload state
- storing latest error state
- providing lookup by ID
- providing lookup by capability ID
- tracking availability and stale state
- rejecting duplicate or invalid records
- rejecting new records when fixed tables are full
- emitting Registry and Error events when state changes

## What Registry Stores

Registry stores compact facts:

```text
module records
device records
service records
capability records
metadata references
latest payload state
latest error state
status fields
relationship indexes
```

Registry records should store IDs, compact enums, indexes, status, timestamps, and small payload values.

Registry may store:

- `module_id`
- `device_id`
- `service_id`
- `capability_id`
- provider indexes
- owner indexes
- latest payload value
- availability flag
- stale flag
- latest `error_code`
- timestamp

## What Registry Must Not Do

Registry must not:

- discover hardware
- scan buses
- read module EEPROM directly
- parse large metadata blobs after registration
- run Logic
- execute Actions
- control HAL
- call Drivers directly
- own Dashboard decisions
- render UI
- store Dashboard-only data
- store full telemetry history
- store large metadata blobs in RAM
- dynamically grow tables
- allocate memory dynamically in v1

Registry stores facts/state only.

## Fixed-Size Table Design

Cyber32 v1 Registry uses fixed-size tables.

No dynamic allocation is allowed in v1 Registry.

Recommended tables:

```text
modules[CYBER32_MAX_MODULES]
devices[CYBER32_MAX_DEVICES]
capabilities[CYBER32_MAX_CAPABILITIES]
services[CYBER32_MAX_SERVICES]
errors[CYBER32_MAX_ERRORS]
metadata_refs[CYBER32_MAX_METADATA_REFS]
```

Recommended limits:

```text
CYBER32_MAX_MODULES       8
CYBER32_MAX_DEVICES       16
CYBER32_MAX_CAPABILITIES  48
CYBER32_MAX_SERVICES      8
CYBER32_MAX_ERRORS        16
CYBER32_MAX_METADATA_REFS 32
```

Slot state:

```text
empty
reserved
registered
available
unavailable
removed
disabled
error
```

Each table entry must have a small status field so unavailable records do not require removal or memory movement.

## Table Record Shapes

### Module Slot

Stores:

```text
module_id
module_type
status
pnp_level
metadata_ref_index
first_device_index
device_count
first_capability_index
capability_count
latest_error_index
created_at_ms
updated_at_ms
```

### Device Slot

Stores:

```text
device_id
device_type
status
module_index
metadata_ref_index
first_capability_index
capability_count
latest_error_index
created_at_ms
updated_at_ms
```

### Capability Slot

Stores:

```text
capability_id
category
kind
data_type
access
source
owner_type
owner_index
status
available
stale
payload_index_or_inline_payload
latest_error_index
created_at_ms
updated_at_ms
```

### Service Slot

Stores:

```text
service_id
service_type
status
capability_indexes
capability_count
latest_error_index
created_at_ms
updated_at_ms
```

### Error Slot

Stores:

```text
error_code
severity
recoverable
source_layer
source_id_ref
target_id_ref
state
retry_count
safe_action
timestamp_ms
```

### Metadata Reference Slot

Stores:

```text
metadata_id
owner_type
owner_index
schema_version
metadata_level
checksum
storage_location
offset_or_address
length
```

Metadata reference slots do not store large metadata blobs.

## Maximum Records

Cyber32 v1 Registry maximums:

| Record Type | Maximum |
|---|---:|
| Modules | 8 |
| Devices | 16 |
| Capabilities | 48 |
| Services | 8 |
| Active/latest errors | 16 |
| Metadata references | 32 |

These are v1 implementation limits, not permanent architecture limits.

## Compact ID Strategy

Registry accepts stable string IDs at the boundary, but stores compact references internally where possible.

Boundary IDs:

```text
CAP_TEMPERATURE
module-sim-temp-001
device-sim-temp-001
service:telemetry-manager
```

Internal references:

```text
module_index
device_index
capability_index
service_index
metadata_ref_index
error_index
```

Rules:

- IDs must be stable.
- IDs must be ASCII.
- IDs must fit fixed field limits.
- Capability IDs must start with `CAP_`.
- Error codes must start with `ERR_`.
- Event IDs must start with `EVT_`.
- Logic stores or resolves capability IDs only.
- Registry may cache resolved indexes for speed.

Recommended fixed string limits:

```text
module_id:      32 bytes
device_id:      32 bytes
service_id:     32 bytes
capability_id:  32 bytes
metadata_id:    32 bytes
error_code:     32 bytes at API boundary, compact enum internally
```

## Lookup Rules

Registry must support:

```text
find module by module_id
find device by device_id
find service by service_id
find capability by capability_id
find capabilities by owner
find devices by capability_id
find modules by capability_id
find latest payload by capability_id
find latest error by target ID
```

Lookup behavior:

- Return explicit status.
- Return `ERR_CAPABILITY_UNAVAILABLE` when a capability exists but is unavailable.
- Return `ERR_REGISTRY_FULL` only when insertion fails due to no free slot.
- Return a not-found error for missing IDs.
- Never return module display names as Logic bindings.

Performance rules:

- Linear scans are acceptable in v1 for setup, API lists, and Dashboard views.
- Logic should resolve capability IDs to indexes during validation.
- High-frequency paths should use cached capability indexes.

## Registration Rules

PNP initiates registration.

Registry registration order:

```text
validate module record
reserve module slot
register metadata reference
register devices
register capabilities
link relationships by indexes
mark module available or degraded
emit events
```

Rules:

- PNP must validate metadata before registration.
- Registry must validate record shape again before storing.
- Registry must reject records with invalid IDs.
- Registry must reject capabilities with invalid `CAP_` IDs.
- Registry must reject records exceeding fixed limits.
- Registry must leave partial registration in a safe unavailable state if registration fails.
- Registry must not expose capabilities until registration is complete enough to be safe.

On success:

```text
EVT_REGISTRY_RECORD_ADDED
EVT_CAPABILITY_REGISTERED
```

On failure:

```text
ERR_REGISTRY_FULL
ERR_REGISTRY_DUPLICATE_ID
ERR_METADATA_INVALID
EVT_ERROR_RAISED
```

## Duplicate Handling

Registry must detect duplicates before reserving new slots.

Duplicate keys:

- duplicate `module_id`
- duplicate `device_id`
- duplicate `service_id`
- duplicate capability provider conflict where policy disallows multiple providers
- duplicate metadata owner reference

Rules:

- Exact duplicate IDs are rejected.
- Re-registration of the same physical module may update the existing slot if explicitly allowed by PNP.
- Multiple providers for the same capability ID are allowed only if Registry can track provider ownership and Services/Logic can resolve the active provider.
- If multiple providers are present and no selection policy exists, mark capability selection as ambiguous or degraded.

Error:

```text
ERR_REGISTRY_DUPLICATE_ID
```

Event:

```text
EVT_ERROR_RAISED
```

## Unavailable And Stale Handling

Registry stores current availability and stale state for capabilities and owners.

Unavailable means the provider cannot currently supply the capability.

Stale means the last value exists but is older than the freshness window.

Rules:

- Do not remove records immediately when a module disappears.
- Mark module, devices, and capabilities unavailable.
- Preserve last known payload only if safe.
- Mark stale separately from unavailable.
- Logic must treat unavailable and stale safety-critical capabilities conservatively.
- API must expose `available` and `stale`.
- Dashboard may translate these states for humans.

Events:

```text
EVT_CAPABILITY_UNAVAILABLE
EVT_CAPABILITY_VALUE_UPDATED
EVT_DEVICE_LOST
EVT_MODULE_REMOVED
```

Errors:

```text
ERR_CAPABILITY_UNAVAILABLE
ERR_DEVICE_TIMEOUT
```

## Error State Storage

Registry stores latest error state for registered objects.

Registry may store latest error for:

- module
- device
- capability
- service

Registry does not store full logs.

Error slots are fixed:

```text
CYBER32_MAX_ERRORS = 16
```

Rules:

- Store compact `error_code`.
- Store severity and safe action.
- Store timestamp.
- Store target reference.
- Replace or reuse old cleared slots when needed.
- Do not allocate memory for error messages.
- Do not store stack traces.

When no error slot is available:

```text
ERR_REGISTRY_FULL
EVT_ERROR_RAISED
```

## Capability Payload Storage

Registry stores latest capability payload state.

For ESP32 v1, payloads must be compact.

Registry stores:

```text
capability_id
schema_version
timestamp_ms
available
stale
value_type
small inline value or payload slot
unit enum
quality enum
error_code
```

Allowed payload forms:

- inline boolean
- inline integer
- inline float
- compact enum
- small fixed object for known schemas

Not allowed:

- arbitrary nested JSON
- large strings
- full telemetry history
- full Dashboard display data
- heap-owned payload objects

For complex payloads such as `CAP_POSITION`, Registry should use a fixed struct slot with known fields.

For command capabilities such as `CAP_DISPLAY_TEXT`, Registry may store last command status and small last requested state, not full display buffers.

## Event Emission Rules

Registry emits events when Registry-owned state changes.

Allowed Registry events:

```text
EVT_REGISTRY_RECORD_ADDED
EVT_REGISTRY_RECORD_UPDATED
EVT_REGISTRY_RECORD_REMOVED
EVT_REGISTRY_FULL
```

Registry may also trigger capability events:

```text
EVT_CAPABILITY_REGISTERED
EVT_CAPABILITY_AVAILABLE
EVT_CAPABILITY_UNAVAILABLE
EVT_CAPABILITY_VALUE_UPDATED
EVT_CAPABILITY_ERROR
```

Registry may announce errors:

```text
EVT_ERROR_RAISED
EVT_ERROR_RECOVERED
EVT_ERROR_FATAL
```

Rules:

- Events must be compact.
- Events must reference IDs or indexes.
- Events must not contain full registry records.
- Events must not contain large metadata blobs.
- Events must not contain Dashboard-only data.
- Event Bus does not store Registry state.

## Memory Budget Estimate

Target Registry RAM budget from `MEMORY_MODEL.md`:

```text
Registry: <= 16 KB RAM
```

Estimated v1 budget:

| Table | Count | Approx Slot Size | Estimate |
|---|---:|---:|---:|
| Modules | 8 | 96 bytes | 768 bytes |
| Devices | 16 | 96 bytes | 1536 bytes |
| Capabilities | 48 | 128 bytes | 6144 bytes |
| Services | 8 | 96 bytes | 768 bytes |
| Errors | 16 | 128 bytes | 2048 bytes |
| Metadata references | 32 | 64 bytes | 2048 bytes |
| Index/cache overhead | bounded | - | 1024 bytes |
| Alignment/reserve | bounded | - | 1664 bytes |

Estimated total:

```text
approximately 16 KB
```

If actual structs exceed this budget, reduce optional fields or maximum counts before adding dynamic allocation.

## ESP32 Constraints

Registry implementation must respect:

- limited RAM
- limited flash
- WiFi heap pressure
- Arduino/PlatformIO runtime behavior
- no full operating system
- watchdog sensitivity
- no unbounded queues
- no heap-heavy runtime parsing

Rules:

- Prefer fixed arrays.
- Avoid Arduino `String` in stored records.
- Avoid `std::vector`, `std::map`, or unbounded containers in Registry.
- Avoid dynamic allocation after boot.
- Avoid large JSON documents.
- Keep lookups predictable.
- Keep event emission non-blocking.
- Keep Registry operations short.

## v1 Limitations

Cyber32 v1 Registry does not support:

- unlimited modules
- unlimited capabilities
- dynamic table growth
- persistent full Registry database
- full metadata blob storage in RAM
- multi-provider capability arbitration beyond simple policy
- rich historical error logs
- telemetry history
- dynamic plugin records
- dashboard layout storage
- cloud sync
- distributed multi-node registry

These may be considered for v2+ only after ESP32 v1 is stable.

## Example: Simulated Temperature Module

Scenario:

A simulated module registers `CAP_TEMPERATURE`.

PNP receives validated metadata:

```text
module_id: module-sim-temp-001
metadata_level: 1
module_type: sensing
devices:
  - device_id: device-sim-temp-001
    device_type: sensor
    capabilities:
      - CAP_TEMPERATURE
capabilities:
  - CAP_TEMPERATURE
```

Registry stores module:

```text
module_id: module-sim-temp-001
module_type: sensing
status: available
pnp_level: 1
device_count: 1
capability_count: 1
latest_error_index: none
```

Registry stores device:

```text
device_id: device-sim-temp-001
device_type: sensor
status: available
module_index: <module-sim-temp-001 index>
capability_count: 1
latest_error_index: none
```

Registry stores capability:

```text
capability_id: CAP_TEMPERATURE
category: sensors
kind: sensor
data_type: float
access: read
source: device
owner_index: <device-sim-temp-001 index>
available: true
stale: false
latest_error_index: none
```

Registry stores latest payload:

```text
capability_id: CAP_TEMPERATURE
schema_version: 1
timestamp_ms: 2500
available: true
stale: false
value_type: float
value: 22.4
unit: degree_celsius
quality: valid
error_code: none
```

Registry stores latest error state:

```text
latest_error_index: none
```

Registry emits:

```text
EVT_REGISTRY_RECORD_ADDED for module
EVT_REGISTRY_RECORD_ADDED for device
EVT_CAPABILITY_REGISTERED for CAP_TEMPERATURE
EVT_CAPABILITY_AVAILABLE for CAP_TEMPERATURE
EVT_CAPABILITY_VALUE_UPDATED for CAP_TEMPERATURE
```

Logic query:

```text
find capability CAP_TEMPERATURE
read latest payload value
evaluate rule using CAP_TEMPERATURE.value
```

Logic does not know or depend on:

```text
module-sim-temp-001
device-sim-temp-001
Simulated Temperature Module
```

Correct Logic:

```text
IF CAP_TEMPERATURE.value > 40.0
THEN CAP_DISPLAY_TEXT "High temperature"
```

Incorrect Logic:

```text
IF Simulated Temperature Module > 40.0
THEN Display Module shows warning
```

## Success Criteria

Cyber32 v1 Registry is successful when:

1. It uses fixed-size tables only.
2. It performs no dynamic allocation.
3. It stores compact facts and latest state.
4. It rejects full tables with `ERR_REGISTRY_FULL`.
5. It emits `EVT_REGISTRY_*` or `EVT_ERROR_*` when needed.
6. It stores latest capability payload state.
7. It stores latest compact error state.
8. It does not store large metadata blobs in RAM.
9. Runtime and Services can query it safely.
10. Logic can query capabilities without knowing module names.
