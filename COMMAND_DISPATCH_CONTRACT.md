# Cyber32 Command Dispatch Contract

This document defines how Cyber32 v1 command capabilities are executed.

Cyber32 v1 targets ESP32 only.

Command capabilities include:

- `CAP_MOTOR_CONTROL`
- `CAP_SERVO_POSITION`
- `CAP_DISPLAY_TEXT`
- `CAP_AUDIO_OUTPUT`

## Purpose

Command dispatch defines the safe path from a command request to device execution.

All command execution must flow through Services.

Registry stores state only.

Registry never executes commands.

Runtime coordinates execution.

Runtime never owns command policy.

Services own command policy and validation.

Devices perform execution.

Logic and API request commands through capability IDs only.

Dashboard must not directly control Devices.

## Architecture Context

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

Command dispatch must not bypass this architecture.

## Command Ownership

Ownership rules:

| Responsibility | Owner |
|---|---|
| Current capability state | Registry |
| Command policy | Service |
| Command validation | Service |
| Command scheduling/coordination | Runtime |
| Command execution | Device |
| Hardware-specific behavior | Driver |
| Hardware access | HAL |
| Command request from automation | Logic |
| Command request from external client | API |
| Human display/control surface | Dashboard through API |

Registry is never the executor.

Runtime is never the policy owner.

API is never allowed to call Devices directly.

Dashboard is never allowed to call Devices directly.

## Command State Machine

Every command request moves through a bounded state machine.

States:

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
accepted -> cancelled
accepted -> timed_out
accepted -> failed

executing -> completed
executing -> failed
executing -> timed_out
executing -> cancelled

completed -> final
failed -> final
timed_out -> final
cancelled -> final
```

Invalid transitions must raise a Runtime or Service error.

Final states:

```text
completed
failed
timed_out
cancelled
```

## Command Request

A command request is a compact payload targeting a capability ID.

Common request fields:

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

Allowed `source_type` values:

```text
logic
api
service
system
```

Rules:

- `capability_id` is required.
- `request_id` must fit a fixed-size field.
- `params` must match the capability payload schema.
- `timeout_ms` must be bounded.
- No dynamic allocation is allowed.
- No large strings are allowed.
- Dashboard requests must enter through API.

## Command Flow

The command flow is fixed:

```text
Logic or API
-> Service
-> Runtime
-> Device
-> Driver
-> HAL
```

State and notifications flow back:

```text
Device
-> Service
-> Registry
-> Events
-> Runtime / Logic / API / Dashboard
```

Complete flow:

```text
command requested by capability ID
-> Service validates policy and payload
-> Service accepts or rejects command
-> Runtime schedules execution using fixed task slot
-> Device executes command
-> Driver/HAL perform hardware interaction
-> Device returns result
-> Service interprets result and policy outcome
-> Registry stores latest command state
-> Event Bus announces state change
-> API/Logic/Dashboard observe result through Registry/API/events
```

## Validation Flow

Services validate commands before execution.

Validation checks:

- capability exists
- capability is available
- capability access allows write or execute
- payload schema version is supported
- command name is valid
- params are valid
- numeric ranges are valid
- units are canonical
- source is authorized
- Runtime state allows command
- safe mode rules allow command
- provider Device is available
- timeout is within limits

Validation failure:

```text
state: failed
error_code: validation-specific error
safe_action: none or capability-specific fallback
```

Possible validation errors:

```text
ERR_CAPABILITY_UNAVAILABLE
ERR_CONFIG_INVALID
ERR_API_UNAUTHORIZED
ERR_LOGIC_BINDING_FAILED
ERR_DEVICE_UNAVAILABLE
```

If a more specific command validation error is needed, it must be added to the canonical error catalog before implementation.

## Execution Flow

Devices execute commands only after Service validation and Runtime coordination.

Execution steps:

```text
Service creates accepted command record
Runtime reserves fixed task slot
Runtime marks command executing
Device receives bounded command payload
Device calls Driver through its interface
Driver uses HAL
Device returns compact result
Service maps result to command state
Registry stores latest command state
Events announce result
```

Execution rules:

- Device must not receive unvalidated API payloads.
- Device must not receive Dashboard commands directly.
- Device must not allocate memory for command execution.
- Device must fail safe on safety-critical command errors.
- Driver timeouts must map to command timeout or failure.

## Acknowledgement Flow

Command acknowledgement is separate from command completion.

Acknowledgement states:

```text
accepted
failed
cancelled
```

`accepted` means:

- Service validated the request.
- Runtime can attempt execution.
- The command has not necessarily completed.

`completed` means:

- Device execution returned success.
- Service accepted the result.
- Registry stored latest command state.

API acknowledgement:

```text
request received
-> validated by Service
-> accepted response includes request_id and state accepted
-> later status can be read through API
```

Logic acknowledgement:

```text
Logic requests command
-> Service accepts or rejects
-> Logic may continue, wait, or react to events depending on rule type
```

## Timeout Handling

Every command must have a bounded timeout.

Timeout fields:

```text
timeout_ms
started_at_ms
deadline_ms
```

Rules:

- Service defines maximum timeout per capability.
- Runtime checks deadlines.
- Device and Driver should enforce lower-level timeouts.
- Timeout must not block the main loop.
- Timed-out safety-critical actuator commands must fail safe.

Timeout state:

```text
timed_out
```

Timeout errors:

```text
ERR_DEVICE_TIMEOUT
ERR_RUNTIME_OVERLOAD
ERR_CAPABILITY_UNAVAILABLE
```

Timeout events:

```text
EVT_ERROR_RAISED
EVT_CAPABILITY_ERROR
EVT_RUNTIME_TASK_FAILED
```

## Error Handling

Errors are compact and stable.

Command errors may come from:

- validation failure
- capability unavailable
- service policy rejection
- Runtime overload
- task timeout
- Device timeout
- Driver failure
- HAL failure
- safe mode rejection
- unauthorized API request

Error rules:

- Store latest command error in Registry.
- Emit `EVT_ERROR_RAISED` when needed.
- Include `error_code` in command status.
- Use safe fallback for actuator errors.
- Fatal command errors must enter safe mode if safety policy requires it.

Common errors:

```text
ERR_CAPABILITY_UNAVAILABLE
ERR_DEVICE_TIMEOUT
ERR_DRIVER_INIT_FAILED
ERR_RUNTIME_OVERLOAD
ERR_API_UNAUTHORIZED
ERR_WATCHDOG_RESET
```

## Event Generation

Command dispatch emits compact events.

Recommended events:

```text
EVT_CAPABILITY_VALUE_UPDATED
EVT_CAPABILITY_ERROR
EVT_RUNTIME_TASK_STARTED
EVT_RUNTIME_TASK_FAILED
EVT_SERVICE_ERROR
EVT_ERROR_RAISED
```

Future command-specific events may be added only if documented.

Event rules:

- Events must reference `request_id` or `capability_id` compactly.
- Events must not contain full command payloads.
- Events must not contain Dashboard/UI-only data.
- Event Bus does not store command state.
- Registry stores latest command state.

## Registry Interaction

Registry stores command state only.

Registry may store:

```text
capability_id
latest_request_id
command_state
timestamp_ms
last_error_code
last_result_code
small_last_value
```

Registry must not:

- execute commands
- validate command policy
- call Devices
- call Drivers
- store large command payloads
- store Dashboard UI state

Registry updates:

```text
request accepted
executing
completed
failed
timed_out
cancelled
```

## Runtime Interaction

Runtime coordinates command execution.

Runtime may:

- reserve fixed task slot
- run command task within budget
- enforce task timeout
- emit Runtime task events
- report overload
- coordinate safe mode restrictions

Runtime must not:

- decide command policy
- parse Dashboard UI requests
- call API directly for command execution
- store command facts long-term
- allocate memory dynamically

## Service Interaction

Services own command policy and validation.

Service responsibilities:

- resolve capability provider through Registry
- validate payload schema
- validate command parameters
- enforce authorization/policy
- enforce safe mode restrictions
- choose active provider if supported
- request Runtime execution
- interpret Device result
- update Registry command state
- emit command-related events

Service examples:

| Capability | Likely Service Owner |
|---|---|
| `CAP_MOTOR_CONTROL` | Motion or Device Manager service |
| `CAP_SERVO_POSITION` | Motion or Device Manager service |
| `CAP_DISPLAY_TEXT` | Display or Device Manager service |
| `CAP_AUDIO_OUTPUT` | Audio or Device Manager service |

If a dedicated domain service does not exist in v1, Device Manager may own command policy for that capability.

## Device Interaction

Devices perform execution.

Device responsibilities:

- accept validated compact command payloads
- map command to Driver calls
- enforce device-level safety
- enforce driver timeout
- report compact result
- report errors

Devices must not:

- accept API payloads directly
- accept Dashboard requests directly
- decide user authorization
- update Registry directly unless explicitly owned by a Service path
- allocate command memory dynamically

## API Interaction

API may request commands through capability IDs only.

Allowed API shape:

```text
POST /capabilities/{capability_id}/command
```

Request body:

```text
request_id
schema_version
command
params
timeout_ms
```

API responsibilities:

- authenticate/authorize request
- validate request size
- validate capability ID format
- forward command request to owning Service
- return accepted/rejected response
- expose command status through API

API must not:

- call Devices directly
- call Drivers directly
- call HAL directly
- bypass Services
- store canonical command state

## Logic Interaction

Logic may request commands through capability IDs only.

Correct:

```text
IF CAP_BATTERY_LEVEL.value < 20.0
THEN CAP_AUDIO_OUTPUT mode beep
```

Incorrect:

```text
IF Battery Module is low
THEN Buzzer Board beep
```

Logic responsibilities:

- reference capability IDs
- provide command payload matching schema
- respect unavailable/stale/error state
- handle accepted/failed/timed-out results if rule requires it

Logic must not:

- reference module names
- reference device names
- call Devices directly
- bypass Services

## Dashboard Interaction

Dashboard may request commands only through API.

Dashboard flow:

```text
Dashboard
-> API
-> Service
-> Runtime
-> Device
```

Dashboard must not:

- control Devices directly
- bypass API
- store canonical command state
- rely on display names as command IDs

Dashboard may:

- translate command labels for humans
- display accepted/executing/completed/failed states
- show error messages from machine-readable error codes

## Capability-Specific Notes

### CAP_MOTOR_CONTROL

Safety-critical.

Rules:

- stop/brake commands must be prioritized
- unavailable motor capability must reject motion commands
- timeout must fail safe
- safe mode must reject motion except safe stop if allowed

### CAP_SERVO_POSITION

Potentially safety-critical depending on attached mechanism.

Rules:

- position range must be validated
- duration and timeout must be bounded
- unavailable servo capability must reject position commands
- safe mode behavior must follow actuator safety policy

### CAP_DISPLAY_TEXT

Not safety-critical by default.

Rules:

- text length must be bounded
- display commands may be dropped or failed in low-memory conditions
- Registry stores status, not display buffer

### CAP_AUDIO_OUTPUT

May be notification or warning output.

Rules:

- frequency, duration, and volume must be bounded
- stop command should be allowed when available
- audio output must not block Runtime loop

## ESP32 v1 Limits

Command dispatch must be memory-bounded.

Limits:

```text
maximum active command slots: 8
maximum command payload size: 128 bytes
maximum request_id length: 16 bytes
maximum command params string field: 64 bytes
maximum command timeout: capability-specific, bounded
dynamic allocation: not allowed
```

Rules:

- command slots are fixed
- command payloads are copied into fixed slots or referenced by compact validated data
- no heap allocation after boot
- no unbounded queues
- no long blocking execution
- no large command history

## Example Command Flow

Example: API requests servo movement.

```text
POST /capabilities/CAP_SERVO_POSITION/command
request_id: req-001
schema_version: 1
command: set_position
params.position_degree: 90.0
params.duration_ms: 500
timeout_ms: 1000
```

Flow:

```text
API validates request size and capability ID
-> API forwards request to Service
-> Service validates CAP_SERVO_POSITION is available
-> Service validates payload range and policy
-> Service marks command accepted
-> Registry stores latest command state accepted
-> Runtime reserves fixed task slot
-> Runtime marks command executing
-> Device executes servo position command
-> Driver writes PWM through HAL
-> Device returns success
-> Service marks command completed
-> Registry stores latest command state completed
-> Event Bus announces command-related capability update
-> API exposes completed status
```

Logic never references the servo module name.

Dashboard never calls the servo Device directly.

## Success Criteria

Cyber32 v1 Command Dispatch is successful when:

1. Commands are requested only through capability IDs.
2. API and Logic never bypass Services.
3. Dashboard never controls Devices directly.
4. Services validate policy and payloads.
5. Runtime coordinates bounded execution.
6. Devices execute validated commands.
7. Registry stores command state only.
8. Events announce command changes.
9. No dynamic allocation is used.
10. Safety-critical actuator errors fail safe.
