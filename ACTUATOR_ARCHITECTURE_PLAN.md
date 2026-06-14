# Cyber32 Actuator Architecture Plan

## Goal

Design actuator support before implementation.

Primary actuator capabilities:

```text
CAP_SERVO_POSITION
CAP_MOTOR_CONTROL
```

This document is planning only.

No source code is modified by this plan.

## Scope

This plan defines:

- command model
- command payload schema
- command validation
- command permissions
- command result model
- Runtime interaction
- Service responsibilities
- Logic responsibilities
- Registry responsibilities
- Event model
- safety rules
- failure handling

Out of scope for this plan:

- implementation code
- WiFi
- WebServer
- Dashboard implementation
- Mobile Studio implementation
- real actuator drivers
- servo/motor hardware control
- command queue implementation

## Architecture Rules

Actuator command flow must preserve the fixed Cyber32 architecture:

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

Mandatory command path:

```text
Logic or API
-> Service
-> Runtime
-> Device
-> Driver
-> HAL
```

State and result path:

```text
Device
-> Service
-> Registry
-> EventBus
-> Logic / API / Dashboard
```

Rules:

- Registry stores state only.
- Registry never executes commands.
- Runtime schedules and coordinates execution only.
- Runtime never owns actuator policy.
- Services own command policy and validation.
- Devices execute validated commands.
- Drivers perform hardware-specific operations through HAL.
- Logic and API request actuator behavior by capability ID only.
- Dashboard may request actuator behavior only through API.

## Capability Definitions

### CAP_SERVO_POSITION

Capability ID:

```text
CAP_SERVO_POSITION
```

Direction:

```text
read-write
```

Datatype:

```text
float for read state
object for command request
```

Canonical unit:

```text
degree
```

Valid command range:

```text
position_degree: 0.0 to 180.0
duration_ms: 0 to 10000
timeout_ms: 0 to 10000
```

Safety profile:

```text
position actuator
potentially safety-critical depending on attached mechanism
```

Default safe state:

```text
disabled / no movement before Runtime READY
```

### CAP_MOTOR_CONTROL

Capability ID:

```text
CAP_MOTOR_CONTROL
```

Direction:

```text
write
```

Datatype:

```text
object
```

Canonical units:

```text
speed_percent: percent
duration_ms: millisecond
```

Valid command range:

```text
mode: stop, coast, brake, set_speed
speed_percent: -100.0 to 100.0
direction: forward, reverse, left, right, none
duration_ms: 0 to 60000
timeout_ms: 0 to 60000
```

Safety profile:

```text
safety-critical motion actuator
```

Default safe state:

```text
stopped / disabled / brake if supported
```

## Command Model

A command is a bounded request targeting a capability ID.

Command fields:

```text
request_id
capability_id
schema_version
command
params
source_type
source_id
timeout_ms
timestamp_ms
```

Allowed source types:

```text
logic
api
service
system
```

Command state machine:

```text
request
accepted
executing
completed
failed
timed_out
cancelled
```

Allowed transitions:

```text
request -> accepted
request -> failed
request -> cancelled

accepted -> executing
accepted -> failed
accepted -> timed_out
accepted -> cancelled

executing -> completed
executing -> failed
executing -> timed_out
executing -> cancelled
```

Final states:

```text
completed
failed
timed_out
cancelled
```

Command acknowledgement is not completion.

`accepted` means:

```text
Service validated the command and Runtime can attempt execution.
```

`completed` means:

```text
Device execution succeeded and Service stored the final state in Registry.
```

## Command Payload Schema

Command payloads must be compact, fixed-shape, and ESP32-safe.

General command envelope:

```text
capability_id
schema_version
request_id
command
params
timeout_ms
timestamp_ms
```

Rules:

- no dynamic allocation
- no Arduino `String`
- no STL containers
- no unbounded text
- no nested arbitrary JSON internally
- no Dashboard-only fields
- no module names as command targets

### CAP_SERVO_POSITION Command Payload

Supported command:

```text
set_position
```

Payload:

```text
capability_id = CAP_SERVO_POSITION
schema_version = 1
request_id
command = set_position
params.position_degree
params.duration_ms
timeout_ms
timestamp_ms
```

Parameter rules:

```text
position_degree: 0.0 to 180.0
duration_ms: 0 to 10000
timeout_ms: 0 to 10000
```

Optional future commands:

```text
hold
disable
move_to_safe_position
```

These must be documented before implementation.

### CAP_MOTOR_CONTROL Command Payload

Supported commands:

```text
stop
brake
coast
set_speed
```

Payload:

```text
capability_id = CAP_MOTOR_CONTROL
schema_version = 1
request_id
command
params.mode
params.speed_percent
params.direction
params.duration_ms
timeout_ms
timestamp_ms
```

Parameter rules:

```text
mode: stop, coast, brake, set_speed
speed_percent: -100.0 to 100.0
direction: forward, reverse, left, right, none
duration_ms: 0 to 60000
timeout_ms: 0 to 60000
```

Safety priority:

```text
stop and brake commands must outrank normal motion commands.
```

## Command Validation

Services validate actuator commands before Runtime schedules execution.

Common validation checks:

1. `capability_id` exists in Registry.
2. Capability is available.
3. Capability access supports write or read-write.
4. Payload `schema_version` is supported.
5. Command name is valid for the capability.
6. Parameters are within documented ranges.
7. Units are canonical.
8. `request_id` is present and bounded.
9. `timeout_ms` is within capability limits.
10. Source is permitted.
11. Runtime state allows actuator commands.
12. Safe mode policy allows command.
13. Device provider is available.
14. Command does not violate actuator safety policy.

Validation failure result:

```text
command_state = failed
error_code = canonical compact error
safe_action = capability-specific safe fallback
```

Required validation errors:

```text
ERR_CAPABILITY_UNAVAILABLE
ERR_CONFIG_INVALID
ERR_API_UNAUTHORIZED
ERR_DEVICE_TIMEOUT
ERR_RUNTIME_OVERLOAD
```

Future command-specific errors may be added only after updating the canonical error model.

## Command Permissions

Command permissions are enforced by API and Services.

### Source Permissions

| Source | May Request Commands | Notes |
|---|---|---|
| `logic` | Yes | Capability-based rules only. |
| `api` | Yes | Requires size validation and optional auth. |
| `service` | Limited | Internal safe actions only. |
| `system` | Yes | Safe stop, recovery, watchdog, safe mode actions. |
| `dashboard` | Indirect only | Must go through API. |

### CAP_SERVO_POSITION Permissions

Allowed before Runtime `READY`:

```text
none, except internal safe disable/hold if configured
```

Allowed in `READY/RUNNING`:

```text
set_position if Service validation passes
hold
disable
```

Allowed in safe mode:

```text
hold
disable
move_to_safe_position only if configured safe
```

Blocked in safe mode:

```text
arbitrary set_position
```

### CAP_MOTOR_CONTROL Permissions

Allowed before Runtime `READY`:

```text
internal stop/brake/disable only
```

Allowed in `READY/RUNNING`:

```text
stop
brake
coast
set_speed if Service validation passes
```

Allowed in safe mode:

```text
stop
brake
disable output
```

Blocked in safe mode:

```text
set_speed
forward/reverse motion commands
```

## Command Result Model

Registry stores latest compact command state.

Recommended command result fields:

```text
capability_id
request_id
schema_version
command_state
timestamp_ms
started_at_ms
completed_at_ms
error_code
result_code
safe_action
last_value_type
last_value_float
last_value_int
```

Command result states:

```text
accepted
executing
completed
failed
timed_out
cancelled
```

Result codes should be compact enums, not strings.

Error codes remain canonical `ERR_*` IDs.

Registry must not store:

- large command payload history
- Dashboard text
- raw JSON
- heap-owned command data
- long diagnostic messages

## Runtime Interaction

Runtime coordinates execution using bounded task slots.

Runtime may:

- reserve command task slot
- run command task callback
- enforce deadline checks
- detect overload
- coordinate safe mode transitions
- emit Runtime task events

Runtime must not:

- parse command policy
- validate servo/motor ranges
- decide whether a motor command is safe
- call API
- call Dashboard
- store command history
- allocate command tasks dynamically

Runtime command task input:

```text
fixed command execution context
callback
timeout/deadline
enabled state
```

Runtime scheduling priority:

1. emergency stop/brake/disable tasks
2. safety recovery tasks
3. normal actuator tasks
4. sensor/service updates
5. API/Dashboard/telemetry work

No unbounded command queue is allowed.

## Service Responsibilities

Actuator Services own command policy.

Likely owners:

```text
MotionService
ServoService
MotorService
DeviceManagerService
```

For v1, a single Motion or Device Manager service may own both actuator capabilities if kept compact.

Service responsibilities:

1. Receive command request by capability ID.
2. Validate capability exists in Registry.
3. Validate capability availability.
4. Validate access mode.
5. Validate source permission.
6. Validate payload schema.
7. Validate numeric ranges.
8. Enforce Runtime state restrictions.
9. Enforce safe mode restrictions.
10. Select active provider.
11. Request Runtime task execution.
12. Interpret Device execution result.
13. Update Registry command state.
14. Emit or cause command-related events.
15. Fail safe on errors.

Service must not:

- expose external API directly
- run Logic decisions
- bypass Runtime for actuator execution
- store large command history
- allocate command memory dynamically
- know Dashboard controls

## Logic Responsibilities

Logic may request actuator commands through capability IDs only.

Correct:

```text
IF CAP_DISTANCE.value < 0.30
THEN CAP_MOTOR_CONTROL command stop
```

Incorrect:

```text
IF Front Distance Module < 30cm
THEN Left Motor Driver stop
```

Logic responsibilities:

- inspect capability state from Registry
- respect unavailable/stale/error state
- request commands through Service-facing capability command API
- handle accepted/failed/timed-out command outcomes when required

Logic must not:

- call Devices
- call Drivers
- call HAL
- bypass Services
- depend on module names
- depend on device IDs
- force unsafe commands during safe mode

## Registry Responsibilities

Registry stores facts and state only.

Registry may store:

- actuator capability records
- availability state
- stale state
- latest read state where applicable
- latest command state
- latest command error code
- latest command result code
- compact request ID
- timestamps

Registry must not:

- execute commands
- validate actuator policy
- call Devices
- call Drivers
- call Services
- schedule Runtime tasks
- store full command payloads
- store Dashboard UI state

Registry updates:

```text
command accepted
command executing
command completed
command failed
command timed_out
command cancelled
capability unavailable
capability error
```

## Event Model

Events announce command and actuator state changes.

Recommended event IDs to use initially:

```text
EVT_CAPABILITY_VALUE_UPDATED
EVT_CAPABILITY_ERROR
EVT_SERVICE_ERROR
EVT_RUNTIME_TASK_STARTED
EVT_RUNTIME_TASK_FAILED
EVT_ERROR_RAISED
```

Future command event IDs may be added if documented first:

```text
EVT_COMMAND_ACCEPTED
EVT_COMMAND_EXECUTING
EVT_COMMAND_COMPLETED
EVT_COMMAND_FAILED
EVT_COMMAND_TIMED_OUT
EVT_COMMAND_CANCELLED
```

Event rules:

- events reference `capability_id`
- events may reference compact `request_id`
- events do not contain full command payloads
- events do not contain Dashboard UI data
- events do not store current state
- Registry remains source of truth
- EventBus remains bounded transport

Priority rules:

```text
CAP_MOTOR_CONTROL stop/brake/disable: critical
CAP_SERVO_POSITION disable/hold safe: high
normal servo position command: normal
normal motor set_speed command: high or normal depending policy
display/audio non-safety command: low or normal
```

## Safety Rules

### Global Actuator Safety

1. Motion outputs must never activate before Runtime `READY`.
2. Safe mode blocks unsafe motion.
3. Watchdog recovery starts with safe actuator defaults.
4. Brownout recovery starts with safe actuator defaults.
5. Invalid metadata prevents actuator exposure.
6. Invalid configuration prevents actuator enable.
7. Module loss marks actuator capabilities unavailable.
8. API cannot bypass Services.
9. Logic cannot bypass Services.
10. Dashboard cannot bypass API.
11. Registry never executes commands.

### CAP_SERVO_POSITION Safety

Boot:

```text
PWM disabled or held safe
no sweep
no arbitrary movement
safe neutral only if explicitly configured and validated
```

Normal:

```text
position range enforced
duration bounded
timeout bounded
provider available
Runtime READY/RUNNING
```

Safe mode:

```text
arbitrary movement blocked
hold/disable allowed
configured safe position allowed only if validated
```

Failure:

```text
driver timeout -> disable or hold safe
capability unavailable -> reject position commands
runtime overload -> reject low-priority position updates
```

### CAP_MOTOR_CONTROL Safety

Boot:

```text
outputs stopped/off
motion commands blocked
internal stop/disable allowed
```

Normal:

```text
speed range enforced
direction validated
duration bounded
timeout bounded
provider available
Runtime READY/RUNNING
```

Safe mode:

```text
set_speed blocked
forward/reverse motion blocked
stop/brake/disable allowed
```

Failure:

```text
driver timeout -> stop/brake/disable
capability unavailable -> reject motion commands
runtime overload -> preserve stop/brake/disable
watchdog reset -> stopped/off until recovery
```

## Failure Handling

### Validation Failure

Result:

```text
command_state = failed
error_code = ERR_CONFIG_INVALID or ERR_CAPABILITY_UNAVAILABLE
safe_action = none or safe fallback
```

Registry:

```text
store latest failed command state
```

Events:

```text
EVT_CAPABILITY_ERROR
EVT_ERROR_RAISED when needed
```

### Capability Unavailable

Result:

```text
command_state = failed
error_code = ERR_CAPABILITY_UNAVAILABLE
```

Action:

```text
reject command
do not call Device except allowed safe stop if provider can accept it
```

### Device Timeout

Result:

```text
command_state = timed_out or failed
error_code = ERR_DEVICE_TIMEOUT
```

Action:

```text
servo: hold/disable safe
motor: stop/brake/disable
```

### Runtime Overload

Result:

```text
command_state = failed or timed_out
error_code = ERR_RUNTIME_OVERLOAD
```

Action:

```text
preserve emergency stop/brake/disable
reject low-priority actuator commands
enter DEGRADED or SAFE_MODE if persistent
```

### Watchdog Reset

Result:

```text
ERR_WATCHDOG_RESET
```

Action:

```text
actuators default safe
motion blocked until Runtime READY
repeated reset enters SAFE_MODE
```

### Module Loss

Result:

```text
ERR_CAPABILITY_UNAVAILABLE
```

Action:

```text
mark module/device/capability unavailable
reject actuator commands
fail safe if output can still be controlled
```

## ESP32 Memory Limits

Recommended v1 actuator command limits:

```text
max active command slots: 4 to 8
max request_id length: 16 bytes
max command payload size: 128 bytes
max command history: latest state only
dynamic allocation: not allowed
```

Storage model:

```text
fixed command slots
fixed request_id buffers or compact IDs
inline numeric params
compact command enum
compact command state enum
compact result enum
canonical error_code pointer or compact error enum
```

Avoid:

- JSON inside Runtime/Registry
- heap-owned command params
- large strings
- unbounded queues
- command logs in RAM

## Implementation Prerequisites

Before implementing actuators, Cyber32 should have:

1. Registry result APIs stable.
2. Registry command state schema defined.
3. Command request/result structs defined.
4. Runtime command task model defined.
5. Event priority behavior improved or documented as limited.
6. Error IDs added for config, runtime overload, unauthorized API, watchdog, driver init failure.
7. Safe mode and Runtime state transition behavior implemented or explicitly stubbed.
8. Service command validation contract implemented.
9. Device execution safety contract implemented.
10. Validation harness extended with non-hardware simulated actuator tests only.

## Stop Conditions

Stop actuator implementation if:

- API calls Device directly
- Logic calls Device directly
- Runtime validates servo/motor policy
- Registry executes commands
- Dashboard bypasses API
- EventBus stores command state
- dynamic allocation is introduced
- motor output can activate before Runtime `READY`
- servo can move before Runtime `READY`
- safe mode cannot block motion
- command failure cannot be represented compactly

## Recommended Implementation Order

1. Define command shared types.
2. Define command result/status types.
3. Extend Registry records for latest command state.
4. Add result/event IDs needed for command failures.
5. Add Runtime fixed command task coordination.
6. Add simulated servo Driver and Device with no hardware output.
7. Add simulated servo Service validation.
8. Add API command request method without WiFi/WebServer.
9. Add Logic command request path by capability ID.
10. Validate safe rejection before Runtime `READY`.
11. Validate safe mode rejection.
12. Only then consider real actuator hardware.

## Success Criteria

Actuator architecture is ready for implementation when:

1. Commands use capability IDs only.
2. Services own validation and policy.
3. Runtime coordinates execution without owning policy.
4. Devices execute only validated commands.
5. Registry stores command state only.
6. Events announce command state compactly.
7. API and Logic cannot bypass Services.
8. Safety rules are enforceable in code.
9. Failure states map to compact error/result codes.
10. No actuator can move before Runtime `READY`.
