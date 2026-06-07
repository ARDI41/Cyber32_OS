# Cyber32 First Vertical Slice Plan

This document defines the smallest possible Cyber32 implementation that proves the architecture works end-to-end.

This is not a demo.

This is an architecture validation slice.

This plan follows:

- `ARCHITECTURE.md`
- `REGISTRY_IMPLEMENTATION_PLAN.md`
- `RUNTIME_IMPLEMENTATION_PLAN.md`
- `CAPABILITY_PAYLOAD_SCHEMA.md`
- `COMMAND_DISPATCH_CONTRACT.md`
- `API_V1_CONTRACT.md`
- `EVENT_MODEL.md`
- `ERROR_MODEL.md`
- `ESP32_V1_SCOPE.md`

## Scope

The slice includes:

```text
HAL
Driver
Device
Module
PNP
Registry
Runtime
Service
Logic
API
```

The slice uses exactly one capability:

```text
CAP_TEMPERATURE
```

The slice uses one simulated temperature provider.

No real sensor is required.

## Explicitly Out Of Scope

Not included:

- Dashboard
- Mobile Studio
- OTA
- cloud
- WiFi requirement
- actuator control
- servo
- motor
- real temperature sensor
- WebSocket
- hot-plug
- persistent storage beyond optional boot counters

API may be represented through serial, local in-process request handling, or minimal HTTP only if already available. WiFi is not required for this slice.

## Goal

The slice proves:

- a simulated module is discovered
- it is registered
- it exposes `CAP_TEMPERATURE`
- Runtime updates it
- Registry stores it
- Logic queries it
- API returns it
- Events announce updates
- no layer violations occur

Critical rule:

```text
Logic queries CAP_TEMPERATURE.
Logic never knows the module name.
```

## Architecture Path

The vertical path is:

```text
HAL
-> Driver
-> Device
-> Module
-> PNP
-> Registry
-> Runtime
-> Service
-> Logic
-> API
```

No layer may skip the layer immediately responsible for its contract.

## Single Capability

Capability:

```text
CAP_TEMPERATURE
```

Payload schema:

```text
capability_id: CAP_TEMPERATURE
schema_version: 1
timestamp_ms
available
stale
value
unit: degree_celsius
quality
error_code
```

Valid range:

```text
-40.0 to 125.0
```

Recommended precision:

```text
0.1 degree_celsius
```

## Component Definitions

### HAL

Purpose:

Provide minimal time access for the simulated provider.

Required HAL contract:

```text
read uptime milliseconds
```

HAL does not know:

- temperature
- module name
- Registry
- Logic
- API

### Driver

Purpose:

Provide simulated temperature readings through a Driver interface.

Driver behavior:

```text
initialize simulated temperature source
return deterministic or simple changing temperature value
return read status
```

Example value:

```text
22.4 degree_celsius
```

Driver does not:

- register capabilities
- write Registry
- run Logic
- expose API

### Device

Purpose:

Wrap the simulated Driver as a Cyber32 Device.

Device identity:

```text
device_id: device-sim-temperature-001
device_type: sensor
```

Device exposes:

```text
CAP_TEMPERATURE
```

Device responsibilities:

- call Driver read
- validate read result range
- produce compact temperature payload
- report unavailable/error state if Driver read fails

Device does not:

- perform PNP discovery
- own Registry storage
- run Logic

### Module

Purpose:

Represent the simulated temperature provider as a Cyber32 Module.

Module identity:

```text
module_id: module-sim-temperature-001
module_type: sensing
metadata_level: 1
```

Module contains:

```text
device-sim-temperature-001
CAP_TEMPERATURE
```

Module name is display metadata only.

Logic must not know or use:

```text
module-sim-temperature-001
Simulated Temperature Module
```

### PNP

Purpose:

Simulate discovery of one known module.

PNP level:

```text
Level 1: Passive identification
```

PNP output:

```text
validated module metadata for module-sim-temperature-001
```

PNP responsibilities:

- detect simulated module
- identify metadata
- validate metadata
- initiate Registry registration

PNP does not:

- store Registry state
- execute Runtime
- run Logic

### Registry

Purpose:

Store compact state and facts.

Registry stores:

- module record
- device record
- capability record
- latest temperature payload
- latest error state

Registry does not:

- discover module
- read Driver
- run Logic
- expose Dashboard decisions

### Runtime

Purpose:

Coordinate startup, scheduled update, event processing, Service update, Logic evaluation, and API servicing.

Runtime uses:

- fixed task slot for temperature update
- bounded event processing
- no dynamic task creation
- no dynamic allocation after boot

Runtime does not:

- store system facts
- replace Registry
- implement Logic

### Service

Purpose:

Own temperature update policy for this slice.

Service name:

```text
temperature-service
```

Service responsibilities:

- query Registry for `CAP_TEMPERATURE` provider
- request Device read
- validate payload freshness/update policy
- update Registry latest payload
- emit capability update event

Service does not:

- expose external API directly
- run Logic rule decisions

### Logic

Purpose:

Prove Logic queries capability state by capability ID.

Logic query:

```text
read CAP_TEMPERATURE.value from Registry
```

Minimal rule:

```text
IF CAP_TEMPERATURE.available == true
THEN logic_status = temperature_seen
```

Optional threshold rule:

```text
IF CAP_TEMPERATURE.value > 40.0
THEN logic_status = temperature_high
```

Logic does not:

- know module name
- know device name
- call Driver
- call HAL

### API

Purpose:

Expose `CAP_TEMPERATURE` externally using API contract.

Required conceptual endpoints:

```text
GET /system/status
GET /capabilities
GET /capabilities/CAP_TEMPERATURE
GET /capabilities/CAP_TEMPERATURE/state
```

API does not:

- call Device directly
- call Driver directly
- bypass Registry
- own state

## Boot Sequence

Step-by-step:

```text
1. Power on
2. Runtime enters BOOTING
3. Initialize minimal HAL time
4. Initialize Event Bus fixed queue
5. Initialize Registry fixed tables
6. Initialize simulated temperature Driver
7. Prepare simulated temperature Device
8. Prepare simulated temperature Module definition
9. Runtime enters DISCOVERING
10. PNP discovers simulated module
11. PNP reads static Level 1 metadata
12. PNP validates metadata
13. Runtime enters REGISTERING
14. PNP initiates Registry registration
15. Registry stores module record
16. Registry stores device record
17. Registry stores capability record
18. Registry marks CAP_TEMPERATURE available
19. Runtime enters STARTING
20. Start temperature Service
21. Resolve Logic binding to CAP_TEMPERATURE
22. Start minimal API surface
23. Runtime enters READY/RUNNING
```

## Registration Sequence

PNP metadata:

```text
module_id: module-sim-temperature-001
metadata_level: 1
name: Simulated Temperature Module
module_type: sensing
devices:
  - device_id: device-sim-temperature-001
    device_type: sensor
    capabilities:
      - CAP_TEMPERATURE
capabilities:
  - CAP_TEMPERATURE
```

Registration order:

```text
validate module metadata
reserve module slot
register metadata reference
register device slot
register capability slot
link device to module
link capability to device
mark capability available
emit registration events
```

## Registry Records Created

### Module Record

```text
module_id: module-sim-temperature-001
module_type: sensing
status: available
pnp_level: 1
device_count: 1
capability_count: 1
latest_error_index: none
```

### Device Record

```text
device_id: device-sim-temperature-001
device_type: sensor
status: available
module_index: <module index>
capability_count: 1
latest_error_index: none
```

### Capability Record

```text
capability_id: CAP_TEMPERATURE
category: sensors
kind: sensor
data_type: float
access: read
source: device
owner_index: <device index>
available: true
stale: false
latest_error_index: none
```

### Latest Payload State

Initial state before first read:

```text
capability_id: CAP_TEMPERATURE
schema_version: 1
timestamp_ms: 0
available: true
stale: true
value: null
unit: degree_celsius
quality: stale
error_code: none
```

After first update:

```text
capability_id: CAP_TEMPERATURE
schema_version: 1
timestamp_ms: <now>
available: true
stale: false
value: 22.4
unit: degree_celsius
quality: valid
error_code: none
```

### Latest Error State

Normal:

```text
latest_error_index: none
```

## Events Emitted

Boot and registration:

```text
EVT_BOOT_STARTED
EVT_PNP_SCAN_STARTED
EVT_MODULE_DISCOVERED
EVT_MODULE_IDENTIFIED
EVT_REGISTRY_RECORD_ADDED
EVT_CAPABILITY_REGISTERED
EVT_CAPABILITY_AVAILABLE
EVT_PNP_SCAN_COMPLETED
EVT_RUNTIME_STARTED
EVT_SERVICE_STARTED
EVT_BOOT_READY
```

Runtime update:

```text
EVT_CAPABILITY_VALUE_UPDATED
```

Error path:

```text
EVT_CAPABILITY_ERROR
EVT_ERROR_RAISED
```

Events must be compact and must not contain full metadata or full Registry records.

## Runtime Behavior

Runtime fixed tasks:

```text
temperature_update_task
logic_evaluation_task
api_service_task
event_pump_task
```

Task count:

```text
4 fixed task slots
```

No dynamic task creation.

No dynamic memory allocation after boot.

Update loop:

```text
process critical events
run temperature_update_task if due
update Registry payload through Service
emit EVT_CAPABILITY_VALUE_UPDATED
run logic_evaluation_task if due
service API request if present
process remaining events within budget
```

Temperature update period:

```text
1000 ms
```

## Logic Query Path

Logic flow:

```text
Logic rule references CAP_TEMPERATURE
-> Runtime schedules Logic evaluation
-> Logic queries Registry for CAP_TEMPERATURE
-> Registry returns latest payload
-> Logic reads available/stale/value/error_code
-> Logic evaluates rule
```

Logic must not access:

```text
module-sim-temperature-001
device-sim-temperature-001
Simulated Temperature Module
temperature Driver
HAL
```

Expected Logic result:

```text
logic_status: temperature_seen
last_temperature: 22.4
```

## API Responses

### GET /system/status

Response:

```text
ok: true
schema_version: 1
request_id: none
timestamp_ms: <now>
data:
  system_state: READY
  runtime_state: RUNNING
  safe_mode: false
  degraded: false
  capability_count: 1
  module_count: 1
  device_count: 1
  service_count: 1
  latest_error_code: none
```

### GET /capabilities

Response:

```text
ok: true
schema_version: 1
request_id: none
timestamp_ms: <now>
data:
  capabilities:
    - capability_id: CAP_TEMPERATURE
      name: Temperature
      category: sensors
      kind: sensor
      access: read
      available: true
      stale: false
      unit: degree_celsius
```

### GET /capabilities/CAP_TEMPERATURE

Response:

```text
ok: true
schema_version: 1
request_id: none
timestamp_ms: <now>
data:
  capability_id: CAP_TEMPERATURE
  name: Temperature
  category: sensors
  kind: sensor
  data_type: float
  access: read
  source: device
  available: true
  stale: false
  owner_ref: device:<compact>
```

### GET /capabilities/CAP_TEMPERATURE/state

Response:

```text
ok: true
schema_version: 1
request_id: none
timestamp_ms: <now>
data:
  capability_id: CAP_TEMPERATURE
  schema_version: 1
  timestamp_ms: <sample_time>
  available: true
  stale: false
  value: 22.4
  unit: degree_celsius
  quality: valid
  error_code: none
```

API must expose capability state from Registry.

API must not call Device or Driver directly.

## Error Path

Simulated error:

```text
temperature Driver read fails or times out
```

Error flow:

```text
Driver returns read failure
-> Device maps failure to ERR_DEVICE_TIMEOUT
-> Service marks CAP_TEMPERATURE unavailable or stale
-> Registry stores latest payload error state
-> Registry stores latest error state
-> Event Bus emits EVT_CAPABILITY_ERROR
-> Event Bus emits EVT_ERROR_RAISED
-> Logic sees CAP_TEMPERATURE.available == false or error_code
-> API returns error state
```

Registry payload on error:

```text
capability_id: CAP_TEMPERATURE
schema_version: 1
timestamp_ms: <now>
available: false
stale: false
value: null
unit: degree_celsius
quality: unavailable
error_code: ERR_DEVICE_TIMEOUT
```

API state response on error:

```text
ok: true
schema_version: 1
request_id: none
timestamp_ms: <now>
data:
  capability_id: CAP_TEMPERATURE
  schema_version: 1
  timestamp_ms: <sample_time>
  available: false
  stale: false
  value: null
  unit: degree_celsius
  quality: unavailable
  error_code: ERR_DEVICE_TIMEOUT
```

Logic behavior on error:

```text
Logic must not treat unavailable as zero.
Logic must not query module name.
Logic may set logic_status = temperature_unavailable.
```

## Layer Violation Checks

The slice fails if:

- Logic references module or device name
- API calls Device directly
- Registry executes behavior
- Runtime stores Registry facts as source of truth
- Service bypasses Registry for exposed state
- Event contains full metadata blob
- PNP exposes capability before validation
- any dynamic task is created
- any actuator capability appears
- Dashboard, Mobile Studio, OTA, cloud, WiFi, servo, or motor are required

## Minimal Test Checklist

Required checks:

1. Boot reaches `READY/RUNNING`.
2. PNP discovers exactly one simulated module.
3. Registry contains exactly one module.
4. Registry contains exactly one device.
5. Registry contains exactly one capability.
6. Capability ID is exactly `CAP_TEMPERATURE`.
7. Registry stores latest temperature payload.
8. Event Bus emits `EVT_CAPABILITY_REGISTERED`.
9. Event Bus emits `EVT_CAPABILITY_VALUE_UPDATED`.
10. Logic query uses `CAP_TEMPERATURE`.
11. Logic does not access module ID.
12. API returns `CAP_TEMPERATURE`.
13. Error path returns `ERR_DEVICE_TIMEOUT`.
14. No actuator code path is used.

## Success Criteria

The first vertical slice is successful when:

```text
A simulated module is discovered,
registered,
exposes CAP_TEMPERATURE,
Runtime updates it,
Registry stores it,
Logic queries it,
API returns it,
without any layer violations.
```

This proves Cyber32's architecture end-to-end with the smallest useful capability-first path.
