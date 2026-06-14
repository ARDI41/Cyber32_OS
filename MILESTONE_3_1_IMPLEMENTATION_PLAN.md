# Milestone 3.1 Implementation Plan

## Goal

Design bounded servo command coordination before any real servo hardware is introduced.

This document is planning only.

No source code is modified by this document.

## Inputs Reviewed

- `ServoService`
- `Runtime`
- `Registry`
- `CommandState`
- `CAP_SERVO_POSITION`
- `ACTUATOR_ARCHITECTURE_PLAN.md`
- `CAP_SERVO_POSITION_VERTICAL_SLICE_PLAN.md`
- `MILESTONE_3_ARCHITECTURE_AUDIT.md`

## Scope

Milestone 3.1 closes the simulated actuator safety gaps found in Milestone 3.

Included:

- bounded command-state storage
- one pending servo command slot
- Runtime-coordinated servo command execution
- Runtime `READY/RUNNING` command gating
- safe mode command blocking design
- timeout validation
- timeout enforcement
- failure-mode handling
- validation additions

Excluded:

- real servo hardware
- real PWM
- motor logic
- WiFi
- WebServer
- Dashboard
- OTA
- cloud
- Mobile Studio
- unbounded command queues

## Architecture Rule

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

Milestone 3.1 target command flow:

```text
API or future Logic
-> ServoService request/validation
-> Runtime task scheduling
-> ServoService execution callback
-> SimServoDevice
-> SimServoDriver
-> Registry state update
-> EventBus announces state changes
```

Runtime must coordinate execution without owning servo command policy.

Registry must store state without executing commands.

## 1. Registry Command-State Record

Add a compact fixed-shape command-state record before external command transport or real hardware.

Recommended record:

```text
CommandStateRecord
- const char* capability_id
- CommandState command_state
- uint32_t request_id
- uint32_t timestamp_ms
- uint32_t started_at_ms
- uint32_t completed_at_ms
- const char* error_code
- RegistryResult registry_result
- PayloadValueType value_type
- float value_float
- int32_t value_int
```

Rules:

- one latest command-state record per command capability is enough for v1
- no command history
- no full command payload storage
- no JSON
- no Dashboard-only text
- no dynamic allocation
- no Arduino `String`
- no STL containers

Initial Registry limit:

```text
MAX_COMMAND_STATES = 8
```

For the current slice, only one command-state slot is required:

```text
CAP_SERVO_POSITION
```

Registry APIs to plan:

```text
registerCommandState(const CommandStateRecord& record)
updateCommandState(const char* capability_id, const CommandStateRecord& record)
getCommandState(const char* capability_id, CommandStateRecord& out_record) const
```

Result-returning versions should be preferred:

```text
RegistryWriteResult registerCommandStateWithResult(...)
RegistryResult updateCommandStateWithResult(...)
RegistryResult getCommandStateWithResult(...) const
```

Registry must not:

- validate servo policy
- execute commands
- call Service
- call Device
- call Driver
- schedule Runtime tasks

## 2. Servo Pending-Command Slot

`ServoService` should own exactly one pending servo command slot in Milestone 3.1.

Recommended pending command structure:

```text
ServoPendingCommand
- bool occupied
- uint32_t request_id
- float position_degrees
- uint32_t requested_at_ms
- uint32_t timeout_ms
- uint32_t deadline_ms
- CommandState state
- const char* error_code
```

Rules:

- one slot only
- no queue
- no dynamic allocation
- no string-owned request IDs
- reject new normal command when slot is occupied
- command slot is cleared only after final state is stored

Command acceptance flow:

```text
API or future Logic calls ServoService request method
-> ServoService validates payload and policy
-> if valid and slot free, pending slot is filled
-> command state becomes ACCEPTED
-> Registry latest command state is updated
-> Runtime task executes later
```

If a command is invalid:

```text
state = FAILED
accepted = false
executed = false
Device is not called
Registry command state records failure if command-state storage exists
```

If a command is valid but slot is occupied:

```text
state = FAILED
error_code = ERR_RUNTIME_OVERLOAD or compact equivalent
Device is not called
```

## 3. Runtime Command Task Model

Runtime should coordinate servo command execution using an existing fixed `RuntimeTask`.

Task ID:

```text
task.servo_command.execute
```

Recommended context:

```text
ServoCommandTaskContext
- ServoService* service
- uint32_t now_ms
- bool ran
- bool last_result
- CommandState last_command_state
- RegistryResult last_registry_result
- const char* last_error
```

Execution:

```text
Runtime update
-> task callback runs
-> callback calls ServoService::executePendingCommand(now_ms)
-> ServoService updates Device and Registry
```

Runtime may:

- schedule the fixed task
- call the task callback
- pass current time through context
- enforce bounded per-update task execution through existing task model

Runtime must not:

- know `CAP_SERVO_POSITION`
- inspect servo angle
- validate servo range
- decide safe mode servo policy
- call Device
- call Driver
- store command facts

The existing state-update task may remain:

```text
task.servo_service.update_state
```

Milestone 3.1 adds a separate command execution task:

```text
task.servo_command.execute
```

## 4. Runtime READY/RUNNING Gating

Servo command requests must be blocked unless Runtime state allows actuator movement.

Allowed states for normal `set_position`:

```text
READY
RUNNING
```

Blocked states:

```text
BOOTING
INITIALIZING
DISCOVERING
REGISTERING
STARTING
ERROR_STATE
```

Current `RuntimeState` does not include `SAFE_MODE`, `DEGRADED`, or `RECOVERY`.

Milestone 3.1 may use `ERROR_STATE` as the only blocked error state for now, but must document the future requirement to add:

```text
DEGRADED
SAFE_MODE
RECOVERY
```

Service owns the gating decision.

Implementation direction:

```text
ServoService attaches Runtime or receives RuntimeState through a small method.
ServoService checks state before accepting command.
Runtime does not decide whether position is safe.
```

Rejected command response:

```text
state = FAILED
accepted = false
executed = false
error_code = ERR_CAPABILITY_UNAVAILABLE or future ERR_RUNTIME_NOT_READY
```

Before real hardware, a canonical `ERR_RUNTIME_NOT_READY` or equivalent should be added.

## 5. Safe Mode Command Blocking

Safe mode is not currently represented in `RuntimeState`.

Milestone 3.1 must define the contract even if implementation uses a stub.

Safe mode policy for `CAP_SERVO_POSITION`:

Allowed:

```text
hold
disable
configured safe position only if validated
```

Blocked:

```text
arbitrary set_position
```

Current first command is only:

```text
set_position
```

Therefore in safe mode:

```text
set_position must be rejected
```

Initial implementation options:

1. Add future `RuntimeState::SAFE_MODE` before real hardware.
2. Add a `ServoService` safety flag such as `setSafeMode(bool enabled)` for validation only.
3. Keep safe mode documented as blocked work, but do not permit real hardware until state exists.

Preferred before real hardware:

```text
Add SAFE_MODE to RuntimeState and validate command rejection.
```

## 6. Timeout Validation

`ServoCommandRequest.timeout_ms` must be validated before a command is accepted.

Recommended limits:

```text
MIN_TIMEOUT_MS = 1
MAX_TIMEOUT_MS = 10000
```

Validation:

```text
timeout_ms == 0 may mean use default timeout
default timeout = 1000
timeout_ms > 10000 fails validation
```

Recommended command acceptance behavior:

```text
if timeout_ms == 0:
    effective_timeout_ms = 1000
else if timeout_ms <= 10000:
    effective_timeout_ms = timeout_ms
else:
    reject command
```

Invalid timeout result:

```text
state = FAILED
accepted = false
executed = false
error_code = future ERR_COMMAND_INVALID or ERR_CONFIG_INVALID
```

Until more precise errors exist, use the existing compact error code chosen by the current error model.

## 7. Timeout Enforcement

Runtime and Service share timeout enforcement responsibilities.

Service owns:

- command deadline calculation
- command timeout policy
- final command state
- Registry command-state update

Runtime owns:

- bounded task execution
- calling the command task
- preventing unbounded processing

Recommended Service deadline:

```text
deadline_ms = requested_at_ms + effective_timeout_ms
```

Execution behavior:

```text
if now_ms > deadline_ms before execution:
    state = TIMED_OUT
    accepted = true
    executed = false
    Device is not called
    Registry command state updated
    latest servo payload remains previous valid value
```

If Device execution fails:

```text
state = FAILED
error_code = ERR_DEVICE_TIMEOUT
Registry payload updated with unavailable/error payload if provided by Device
```

If Registry update fails:

```text
state = FAILED
error_code = ERR_CAPABILITY_UNAVAILABLE
registry_result stores exact RegistryResult
```

Timeout enforcement must not block Runtime.

## 8. Failure-Mode Handling

Failure scenarios to handle before real hardware:

### Driver Failure Mode

Setup:

```text
SimServoDriver::setFailureMode(true)
```

Expected:

```text
ServoService command execution fails
CommandState = FAILED
error_code = ERR_DEVICE_TIMEOUT
Registry payload becomes unavailable or remains previous valid state based on documented policy
Device does not report success
```

Preferred simulated policy:

```text
Registry payload should record unavailable/error payload from Device.
Last valid position may be retained only if a separate stale-last-known-value model exists.
```

### Invalid Position

Setup:

```text
position_degrees = -1.0 or 999.0
```

Expected:

```text
CommandState = FAILED
accepted = false
executed = false
Device is not called
Registry servo payload remains previous valid state
```

### Runtime Not Ready

Setup:

```text
RuntimeState = STARTING or BOOTING
```

Expected:

```text
CommandState = FAILED
accepted = false
executed = false
Device is not called
Registry servo payload remains safe previous state
```

### Safe Mode

Setup:

```text
RuntimeState = SAFE_MODE or equivalent safety flag
```

Expected:

```text
set_position rejected
Device is not called
Registry command state records blocked/failure state
```

### Pending Slot Occupied

Setup:

```text
one accepted command remains pending
second normal command is requested
```

Expected:

```text
second command rejected
first command remains pending
Device is not called for second command
```

## 9. Validation Additions

Extend validation harness without modifying `src/main.cpp`.

Required validation cases:

1. Registry command-state record can be registered for `CAP_SERVO_POSITION`.
2. API command request is accepted into ServoService pending slot.
3. Runtime command task executes the pending command.
4. Device position changes only when Runtime command task runs.
5. Registry payload updates after command execution.
6. Registry command state becomes `COMPLETED` after successful command.
7. Invalid position is rejected before Device call.
8. Invalid position does not change Registry payload.
9. Timeout larger than maximum is rejected.
10. Expired pending command becomes `TIMED_OUT`.
11. Driver failure mode produces `FAILED`.
12. Runtime blocked state rejects command.
13. Safe mode or safety flag rejects `set_position`.
14. Pending slot occupied rejects second command.
15. API still does not call Device or Driver.
16. Runtime still does not know `CAP_SERVO_POSITION`.
17. Logic still has no servo command behavior in this milestone.
18. Temperature and distance validation still pass.

Expected task count after adding command task:

```text
existing validation tasks + task.servo_command.execute
```

The validation harness may call API command methods, but command execution should complete only after Runtime task update.

## 10. Stop Conditions Before Real Hardware

Stop before real servo hardware if any of these remain true:

- API command path calls Device or Driver directly.
- Runtime validates servo angle or owns command policy.
- Registry executes commands.
- ServoService accepts commands before Runtime `READY/RUNNING`.
- Safe mode cannot block arbitrary `set_position`.
- Timeout is accepted but not enforced.
- Invalid commands can reach Device.
- Driver failure leaves command state ambiguous.
- Registry cannot store latest command state.
- EventBus stores command state instead of announcing changes.
- Dynamic allocation is introduced.
- Arduino `String` is introduced.
- STL containers are introduced.
- Real `Servo.h` or PWM output is added before simulated command coordination passes.
- `src/main.cpp` is modified to run real actuator behavior before validation is complete.

## Recommended Milestone 3.1 Phases

### Phase 1 - Command-State Schema

Add shared command-state record types and Registry result APIs.

No ServoService behavior change yet.

### Phase 2 - Registry Command-State Storage

Add fixed command-state table.

Store latest state only.

### Phase 3 - Servo Pending Slot

Add bounded pending command slot to `ServoService`.

Split request/accept from execution.

### Phase 4 - Runtime Command Task

Add validation harness task context:

```text
task.servo_command.execute
```

Runtime remains scheduler only.

### Phase 5 - API Request Path Migration

Change internal API command method to submit a request to `ServoService` instead of immediate execution.

Execution completes through Runtime task.

### Phase 6 - Runtime State Gating

Block commands unless Runtime is `READY` or `RUNNING`.

### Phase 7 - Timeout Validation And Enforcement

Validate `timeout_ms`.

Mark expired commands as `TIMED_OUT`.

### Phase 8 - Failure And Safe Mode Validation

Add driver failure, invalid command, pending slot, and safe mode validation.

### Phase 9 - Architecture Audit

Create a follow-up audit before any real servo hardware work.

## Success Criteria

Milestone 3.1 is complete when:

- servo commands are accepted through capability-first API/Service path
- command execution is coordinated by Runtime task
- Service owns all command policy
- Runtime owns scheduling only
- Registry stores latest command state and latest servo payload
- invalid commands never reach Device
- commands are blocked outside `READY/RUNNING`
- safe mode can block arbitrary servo movement
- timeout validation and timeout outcome exist
- driver failure maps to compact command failure
- validation proves all of the above
- no real servo hardware has been added
