# Cyber32 v1 Implementation Order

This document defines the exact implementation order for Cyber32 v1.

This document does not introduce new architecture.

It uses the existing Cyber32 architecture documents and `FIRST_VERTICAL_SLICE_PLAN.md`.

Goal:

```text
Provide a practical development roadmap from empty repository to a fully working first vertical slice.
```

The first vertical slice uses exactly one capability:

```text
CAP_TEMPERATURE
```

The provider is simulated. No real sensor is required.

## 1. Implementation Principles

### Architecture-First

Implementation must follow the fixed architecture:

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

For the first vertical slice, Dashboard is not implemented.

### Capability-First

All behavior above Registry must use capability IDs.

The first vertical slice uses:

```text
CAP_TEMPERATURE
```

Logic must not know module names.

API must expose capability state by capability ID.

### Documentation-First

Implementation must follow existing contracts.

If a required contract is missing or contradictory, stop and update/review documentation before coding further.

### Small Vertical Slices

Implement the shortest complete path.

Do not implement broad subsystems before the first slice works end-to-end.

### No Layer Violations

Stop immediately if:

- Logic references a module name
- API calls Device directly
- Registry executes behavior
- Runtime becomes Registry
- Services bypass Registry for exposed state
- PNP exposes capability before validation

### ESP32 v1 Constraints

Implementation must respect:

- fixed-size memory
- no dynamic allocation after boot
- bounded Registry
- bounded Event Bus
- bounded task count
- compact payloads
- no Dashboard
- no Mobile Studio implementation
- no OTA
- no cloud
- no actuator control
- no WiFi requirement for the first slice

## 2. Development Phases

Exact implementation order:

```text
Phase 0  - Project Skeleton
Phase 1  - Core IDs and Shared Types
Phase 2  - Event Bus
Phase 3  - Registry
Phase 4  - Runtime
Phase 5  - HAL
Phase 6  - Simulated Driver
Phase 7  - Device
Phase 8  - Module
Phase 9  - PNP Discovery
Phase 10 - Registry Registration
Phase 11 - Temperature Service
Phase 12 - Logic
Phase 13 - API
Phase 14 - Vertical Slice Validation
```

## Phase 0 - Project Skeleton

### Purpose

Create the minimal folder and file structure needed for implementation without adding behavior.

### Files/Folders Affected

Expected areas:

```text
src/core
src/hal
src/drivers
src/devices
src/modules
src/pnp
src/registry
src/runtime
src/services
src/logic
src/api
```

### Dependencies

None.

### Completion Criteria

- Build still succeeds or fails only because no implementation entry points exist yet.
- No architecture layer is skipped.
- No feature code is added outside its layer.

### Validation Checks

- Folder structure matches architecture.
- No Dashboard, OTA, cloud, servo, motor, or real sensor code is added.

### Expected Outputs

- Empty or minimal implementation skeleton.
- Clear file locations for later phases.

## Phase 1 - Core IDs and Shared Types

### Purpose

Define compact shared constants and types used across the first slice.

### Files/Folders Affected

Expected areas:

```text
src/core/interfaces
src/core/system_state
src/registry
src/utils/validation
```

### Dependencies

- Phase 0

### Completion Criteria

Defines v1 constants for:

```text
CAP_TEMPERATURE
ERR_DEVICE_TIMEOUT
ERR_REGISTRY_FULL
ERR_CAPABILITY_UNAVAILABLE
EVT_CAPABILITY_REGISTERED
EVT_CAPABILITY_VALUE_UPDATED
```

Defines minimal enums for:

```text
status
availability
stale state
event priority
runtime state
error code
payload type
```

### Validation Checks

- IDs are compact and stable.
- Capability ID starts with `CAP_`.
- Error IDs start with `ERR_`.
- Event IDs start with `EVT_`.
- No module names appear in Logic-facing constants.

### Expected Outputs

- Shared type definitions.
- Shared ID constants.

## Phase 2 - Event Bus

### Purpose

Implement bounded event communication.

### Files/Folders Affected

Expected areas:

```text
src/core/event_bus
src/runtime
```

### Dependencies

- Phase 1

### Completion Criteria

- Fixed event queue exists.
- Maximum event count is bounded.
- Events can be published without dynamic allocation.
- Events can be processed in bounded slices.
- Overflow counters exist.

### Validation Checks

- No dynamic allocation.
- No unbounded queues.
- Event payload is compact.
- Event Bus does not store state as Registry.

### Expected Outputs

- `EVT_BOOT_STARTED` can be emitted.
- `EVT_CAPABILITY_REGISTERED` can be emitted.
- `EVT_CAPABILITY_VALUE_UPDATED` can be emitted.

## Phase 3 - Registry

### Purpose

Implement fixed-size Registry tables.

### Files/Folders Affected

Expected areas:

```text
src/registry
src/registry/modules
src/registry/devices
src/registry/capabilities
src/registry/services
src/registry/metadata
```

### Dependencies

- Phase 1
- Phase 2

### Completion Criteria

Registry can store:

```text
1 module
1 device
1 capability
1 service
latest CAP_TEMPERATURE payload
latest error state
```

Registry rejects full tables with:

```text
ERR_REGISTRY_FULL
```

### Validation Checks

- No dynamic allocation.
- Fixed-size slots only.
- Registry does not execute behavior.
- Registry emits or requests compact events for changes.

### Expected Outputs

- Empty Registry initializes.
- Capability lookup by `CAP_TEMPERATURE` works after registration.
- Latest payload storage slot exists.

## Phase 4 - Runtime

### Purpose

Implement minimal cooperative Runtime.

### Files/Folders Affected

Expected areas:

```text
src/runtime
src/runtime/scheduler
src/runtime/tasks
src/runtime/lifecycle
src/runtime/state_machine
```

### Dependencies

- Phase 1
- Phase 2
- Phase 3

### Completion Criteria

Runtime supports:

```text
BOOTING
INITIALIZING
DISCOVERING
REGISTERING
STARTING
READY
RUNNING
ERROR
```

Runtime supports fixed task slots for:

```text
temperature_update_task
logic_evaluation_task
api_service_task
event_pump_task
```

### Validation Checks

- No dynamic task creation.
- No dynamic memory allocation after boot.
- Runtime does not store Registry facts.
- Runtime does not implement Logic.

### Expected Outputs

- Runtime can transition through boot states.
- Runtime can process event queue.
- Runtime can call scheduled task callbacks.

## Phase 5 - HAL

### Purpose

Implement minimal HAL needed for the simulated slice.

### Files/Folders Affected

Expected areas:

```text
src/hal
src/hal/time
```

### Dependencies

- Phase 1
- Phase 4

### Completion Criteria

HAL provides:

```text
uptime milliseconds
```

### Validation Checks

- HAL does not know Registry.
- HAL does not know Logic.
- HAL does not expose API.
- HAL does not know module name.

### Expected Outputs

- Runtime can read uptime through HAL/time contract.

## Phase 6 - Simulated Driver

### Purpose

Implement one simulated temperature Driver.

### Files/Folders Affected

Expected areas:

```text
src/drivers/sensors
```

### Dependencies

- Phase 1
- Phase 5

### Completion Criteria

Driver can:

```text
initialize
return simulated temperature value
return read failure for error-path test
```

### Validation Checks

- Driver does not register capabilities.
- Driver does not write Registry.
- Driver does not run Logic.
- Driver does not expose API.

### Expected Outputs

- Simulated read returns `22.4 degree_celsius`.
- Simulated failure returns a compact failure status.

## Phase 7 - Device

### Purpose

Wrap simulated Driver as a Cyber32 Device.

### Files/Folders Affected

Expected areas:

```text
src/devices/sensors
```

### Dependencies

- Phase 1
- Phase 6

### Completion Criteria

Device:

```text
device_id: device-sim-temperature-001
device_type: sensor
capability: CAP_TEMPERATURE
```

Device can produce a compact `CAP_TEMPERATURE` payload.

### Validation Checks

- Device does not perform PNP discovery.
- Device does not own Registry storage.
- Device does not run Logic.
- Device maps Driver failure to `ERR_DEVICE_TIMEOUT`.

### Expected Outputs

- Valid temperature payload.
- Error payload path on simulated failure.

## Phase 8 - Module

### Purpose

Define simulated temperature Module.

### Files/Folders Affected

Expected areas:

```text
src/modules/sensing
```

### Dependencies

- Phase 1
- Phase 7

### Completion Criteria

Module metadata exists:

```text
module_id: module-sim-temperature-001
metadata_level: 1
module_type: sensing
device: device-sim-temperature-001
capability: CAP_TEMPERATURE
```

### Validation Checks

- Module name is display metadata only.
- Logic cannot access module name.
- Module does not register itself directly.

### Expected Outputs

- Static module metadata object or equivalent compact representation.

## Phase 9 - PNP Discovery

### Purpose

Implement simulated Level 1 discovery.

### Files/Folders Affected

Expected areas:

```text
src/pnp
src/pnp/discovery
src/pnp/identification
src/pnp/compatibility
```

### Dependencies

- Phase 1
- Phase 2
- Phase 8

### Completion Criteria

PNP can:

```text
detect simulated module
identify static metadata
validate metadata
produce registration request
```

### Validation Checks

- PNP does not store Registry state.
- PNP does not run Logic.
- PNP does not expose capability before validation.
- PNP emits compact discovery events.

### Expected Outputs

- `EVT_MODULE_DISCOVERED`
- validated registration request for `module-sim-temperature-001`

## Phase 10 - Registry Registration

### Purpose

Connect PNP registration request to Registry.

### Files/Folders Affected

Expected areas:

```text
src/pnp/registration
src/registry
```

### Dependencies

- Phase 3
- Phase 9

### Completion Criteria

Registry stores:

```text
module-sim-temperature-001
device-sim-temperature-001
CAP_TEMPERATURE
```

Capability becomes available.

### Validation Checks

- Registration fails cleanly if duplicate.
- Registration fails cleanly if table full.
- Registry emits registration events.
- Registry stores no large metadata blobs.

### Expected Outputs

- `EVT_REGISTRY_RECORD_ADDED`
- `EVT_CAPABILITY_REGISTERED`
- `EVT_CAPABILITY_AVAILABLE`

## Phase 11 - Temperature Service

### Purpose

Implement Service that updates `CAP_TEMPERATURE`.

### Files/Folders Affected

Expected areas:

```text
src/services
src/services/telemetry_manager
```

or a minimal v1 service location:

```text
src/services/temperature_service
```

### Dependencies

- Phase 3
- Phase 4
- Phase 7
- Phase 10

### Completion Criteria

Service can:

```text
resolve CAP_TEMPERATURE provider through Registry
request Device read
update Registry latest payload
emit value update event
handle read failure
```

### Validation Checks

- Service does not expose API directly.
- Service does not run Logic decisions.
- Service uses Registry for exposed state.
- Service maps read failure to `ERR_DEVICE_TIMEOUT`.

### Expected Outputs

- `CAP_TEMPERATURE.value = 22.4`
- `EVT_CAPABILITY_VALUE_UPDATED`

## Phase 12 - Logic

### Purpose

Implement minimal capability-first Logic query.

### Files/Folders Affected

Expected areas:

```text
src/logic
src/logic/rules
```

### Dependencies

- Phase 3
- Phase 4
- Phase 11

### Completion Criteria

Logic can evaluate:

```text
IF CAP_TEMPERATURE.available == true
THEN logic_status = temperature_seen
```

Optional:

```text
IF CAP_TEMPERATURE.value > 40.0
THEN logic_status = temperature_high
```

### Validation Checks

- Logic queries Registry by `CAP_TEMPERATURE`.
- Logic does not know module ID.
- Logic does not know device ID.
- Logic does not call Driver or HAL.
- Logic does not treat unavailable as zero.

### Expected Outputs

- `logic_status = temperature_seen`
- error-path status `temperature_unavailable`

## Phase 13 - API

### Purpose

Expose first slice through v1 API contract.

### Files/Folders Affected

Expected areas:

```text
src/api
src/api/internal
src/api/rest
```

### Dependencies

- Phase 3
- Phase 4
- Phase 11
- Phase 12

### Completion Criteria

API exposes:

```text
GET /system/status
GET /capabilities
GET /capabilities/CAP_TEMPERATURE
GET /capabilities/CAP_TEMPERATURE/state
```

API may be implemented through minimal local request handling first. WiFi is not required for the first slice.

### Validation Checks

- API never calls Device directly.
- API never calls Driver directly.
- API never owns state.
- API serializes Registry state.
- API exposes `CAP_TEMPERATURE`.

### Expected Outputs

- API status shows one module, one device, one capability.
- API capability state returns temperature payload.
- API error state returns `ERR_DEVICE_TIMEOUT` on simulated failure.

## Phase 14 - Vertical Slice Validation

### Purpose

Validate the full architecture path end-to-end.

### Files/Folders Affected

Expected areas:

```text
test
src/app
```

### Dependencies

- Phases 0 through 13

### Completion Criteria

The full path works:

```text
Boot
-> PNP discovers simulated module
-> Registry registers module/device/CAP_TEMPERATURE
-> Runtime runs
-> Service updates temperature
-> Registry stores payload
-> Event Bus announces update
-> Logic queries CAP_TEMPERATURE
-> API returns CAP_TEMPERATURE
```

### Validation Checks

Required checks:

```text
one simulated module discovered
one device registered
one capability registered
CAP_TEMPERATURE available
Runtime READY/RUNNING
temperature Service updates payload
EVT_CAPABILITY_VALUE_UPDATED emitted
Logic query uses CAP_TEMPERATURE
API returns CAP_TEMPERATURE state
Driver failure maps to ERR_DEVICE_TIMEOUT
no layer violations
```

### Expected Outputs

- Successful serial or test output showing vertical slice status.
- API response examples match `FIRST_VERTICAL_SLICE_PLAN.md`.
- Error-path test passes.

## 4. First Vertical Slice Mapping

| Architecture Layer | Slice Implementation | Capability |
|---|---|---|
| HAL | minimal time HAL | supports timestamping |
| Driver | simulated temperature Driver | produces temperature value |
| Device | simulated temperature Device | exposes `CAP_TEMPERATURE` |
| Module | simulated sensing Module | contains temperature Device |
| PNP | simulated Level 1 discovery | validates module metadata |
| Registry | fixed tables | stores `CAP_TEMPERATURE` |
| Runtime | cooperative scheduler | coordinates update/evaluation/API |
| Service | temperature Service | updates payload |
| Logic | minimal rule/query | reads `CAP_TEMPERATURE` |
| API | minimal v1 endpoints | returns `CAP_TEMPERATURE` |

## 5. Stop Conditions

Implementation must stop and architecture must be reviewed if:

- layer violation is detected
- dynamic allocation is introduced in Registry, Event Bus, or Runtime
- dynamic task creation is introduced
- Registry becomes executor
- Event Bus becomes state store
- Runtime stores Registry facts as source of truth
- Logic depends on module names
- Logic depends on device names
- API bypasses Services
- API calls Device directly
- Service exposes external API directly
- PNP exposes capability before validation
- Dashboard is introduced
- Mobile Studio implementation is introduced
- OTA is introduced
- cloud is introduced
- servo, motor, or actuator logic is introduced
- WiFi becomes required for the first slice
- `CAP_TEMPERATURE` is not the only capability
- unavailable value is treated as zero
- metadata blobs are stored in RAM

## 6. Definition Of Done

Cyber32 First Vertical Slice is complete when:

```text
one simulated module discovered
one capability registered
Runtime running
Registry storing state
Logic querying CAP_TEMPERATURE
API returning CAP_TEMPERATURE
events emitted
no architecture violations
```

Detailed Definition of Done:

1. System boots into `READY/RUNNING`.
2. PNP discovers exactly one simulated module.
3. Registry stores exactly one module record.
4. Registry stores exactly one device record.
5. Registry stores exactly one capability record.
6. Capability ID is exactly `CAP_TEMPERATURE`.
7. Service updates `CAP_TEMPERATURE` payload.
8. Registry stores latest temperature payload.
9. Event Bus emits registration and value update events.
10. Logic queries `CAP_TEMPERATURE`.
11. Logic does not know module name.
12. API returns `CAP_TEMPERATURE`.
13. Error path maps simulated read failure to `ERR_DEVICE_TIMEOUT`.
14. No Dashboard, Mobile Studio, OTA, cloud, WiFi requirement, servo, motor, or actuator feature is introduced.

## Final Rule

Focus only on the shortest path to a working first vertical slice.

Do not redesign architecture.

Do not add new features.

Do not introduce Dashboard.

Do not introduce Mobile Studio implementation.

Do not introduce OTA.

Do not introduce cloud.
