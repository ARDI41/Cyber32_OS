# Milestone 4.1 Bounded Motor Command Plan

## Goal

Design bounded `CAP_MOTOR_CONTROL` command execution before any real motor hardware.

This document is planning only.

No source code is modified by this document.

## Inputs Reviewed

- `MILESTONE_4_ARCHITECTURE_AUDIT.md`
- `MILESTONE_4_MOTOR_ARCHITECTURE_PLAN.md`
- `SAFE_MODE_ARCHITECTURE_PLAN.md`
- `MILESTONE_3_4_SAFE_MODE_MANAGEMENT_PLAN.md`

## Scope

Included:

- pending motor command slot
- Runtime command execution task
- accepted vs executed state
- emergency stop behavior
- `SAFE_MODE` stop exception
- timeout enforcement
- command replacement rules
- Registry command-state updates
- validation requirements
- stop conditions before real hardware

Excluded:

- source code changes
- real motor hardware
- PWM
- H-bridge implementation
- WiFi
- WebServer
- Dashboard
- OTA
- cloud
- Mobile Studio

## Current State

The implemented motor slice currently supports:

- `CAP_MOTOR_CONTROL`
- `SERVICE_MOTOR`
- `MotorDirection`
- `SimMotorDriver`
- `SimMotorDevice`
- `SimMotorModule`
- PNP discovery and registration
- Registry payload state
- Registry command-state storage
- `MotorService`
- internal API motor state reads
- internal API motor command calls
- Runtime-scheduled motor state update task
- `SAFE_MODE` blocking for motor commands

Current command flow:

```text
API
-> MotorService
-> SimMotorDevice
-> SimMotorDriver
-> Registry payload update
-> Registry command-state update
```

Target bounded command flow:

```text
API or Logic
-> MotorService request validation
-> pending motor command slot
-> Runtime command execution task
-> MotorService execution
-> SimMotorDevice
-> SimMotorDriver
-> Registry payload update
-> Registry command-state update
```

Runtime remains scheduler/coordinator only.

MotorService remains the owner of motor command policy.

## 1. Pending Motor Command Slot

Cyber32 v1 must use one fixed pending motor command slot.

No queue is introduced in Milestone 4.1.

Proposed compact record:

```text
MotorPendingCommand
- bool occupied
- MotorDirection direction
- float speed_percent
- uint32_t requested_at_ms
- uint32_t timeout_ms
- CommandState state
- const char* error_code
```

Rules:

- stored inside `MotorService`
- exactly one pending command
- no dynamic allocation
- no heap-owned strings
- no command history
- no `std::queue`
- no STL containers
- no Arduino `String`
- no direct Registry table access

The pending slot represents command execution work only.

Registry remains the source of latest public command state.

## 2. Runtime Command Execution Task

Add one Runtime task for motor command execution:

```text
task.motor_service.execute_command
```

Suggested period:

```text
10 ms to 25 ms for simulation
```

Responsibilities:

- Runtime invokes a static callback with a fixed context pointer.
- The callback calls a `MotorService` command execution method.
- Runtime does not inspect capability ID.
- Runtime does not inspect direction.
- Runtime does not inspect speed.
- Runtime does not decide command policy.
- Runtime does not call Device or Driver.

Target callback shape:

```text
void runMotorCommandTask(void* context)
```

Target Service method:

```text
bool executePendingCommand(uint32_t now_ms)
```

Runtime task input context should contain:

```text
MotorService* service
uint32_t now_ms
bool ran
bool last_result
const char* last_error
```

This matches existing bounded Runtime task context style.

## 3. Accepted Vs Executed State

Milestone 4.1 must separate command acceptance from command execution.

### Request Phase

When API or Logic requests a motor command:

1. API or Logic targets `CAP_MOTOR_CONTROL`.
2. API calls `MotorService`, not Device or Driver.
3. MotorService validates:
   - Registry/Device attachment
   - Runtime state
   - direction
   - speed
   - timeout
   - pending slot availability/replacement policy
4. If accepted, MotorService stores command in the pending slot.
5. Registry command state becomes:

```text
command_state = ACCEPTED
accepted = true
executed = false
```

No Device call occurs during acceptance.

### Execution Phase

When Runtime task runs:

1. Runtime invokes the motor command task.
2. MotorService checks the pending slot.
3. MotorService revalidates Runtime state.
4. MotorService checks timeout.
5. MotorService calls Device only if still allowed.
6. Device calls Driver.
7. MotorService updates Registry payload.
8. MotorService updates Registry command state to `COMPLETED`, `FAILED`, or `TIMED_OUT`.
9. MotorService clears the pending slot after final state.

Execution result:

```text
accepted = true
executed = true only if Device command succeeded and Registry payload update succeeded
```

## 4. Emergency Stop Behavior

`STOP / 0.0F` is a safety action, not normal motion.

Emergency stop requirements:

- may replace a pending `FORWARD` or `REVERSE` command
- may be accepted in `READY`
- may be accepted in `RUNNING`
- may be accepted in `SAFE_MODE`
- must force `speed_percent = 0.0F`
- must update Registry payload to stopped when Device succeeds
- must update Registry command state
- must not be delayed behind normal motion commands
- must not require a multi-command queue

Emergency stop is still executed through Service and Device.

API must not call Device or Driver directly for stop.

Logic must not call Device or Driver directly for stop.

## 5. SAFE_MODE Stop Exception

Current Milestone 4 behavior blocks all motor commands in `SAFE_MODE`.

Milestone 4.1 target behavior:

```text
SAFE_MODE allows STOP / 0.0F
SAFE_MODE blocks FORWARD
SAFE_MODE blocks REVERSE
```

Rules:

- MotorService owns this distinction.
- Runtime only reports state.
- API cannot override MotorService.
- Registry stores command result only.
- Device is not called for blocked `FORWARD` or `REVERSE`.
- Device may be called for `STOP / 0.0F` in `SAFE_MODE` if the provider is attached and available.

Blocked `SAFE_MODE` motion command result:

```text
CommandState::FAILED
accepted = false
executed = false
error_code = ERR_CAPABILITY_UNAVAILABLE until precise error exists
payload remains unchanged
```

Allowed `SAFE_MODE` stop result:

```text
CommandState::COMPLETED if Device stop and Registry update succeed
accepted = true
executed = true
error_code = "none"
payload = STOP / 0.0F
```

If Device stop fails in `SAFE_MODE`:

```text
CommandState::FAILED
accepted = true
executed = false
error_code = ERR_DEVICE_TIMEOUT or future actuator unavailable code
Registry payload should become unavailable/error if Device provides unavailable payload
Runtime should remain in SAFE_MODE
```

## 6. Timeout Enforcement

Motor command timeout must be bounded and enforced.

Recommended simulated v1 timeout rules:

```text
minimum timeout_ms = 1
maximum timeout_ms = 10000
```

Rejected timeout values:

```text
timeout_ms == 0 for normal FORWARD/REVERSE
timeout_ms > 10000
```

Emergency stop may allow:

```text
timeout_ms == 0
```

Timeout fields:

```text
requested_at_ms
timeout_ms
deadline_ms = requested_at_ms + timeout_ms
```

Overflow-safe comparison must be used when implemented.

Timeout behavior:

1. If a command is expired before execution, it must not call Device.
2. Registry command state becomes `TIMED_OUT`.
3. Pending slot is cleared.
4. Payload remains unchanged unless a safe fallback action is explicitly executed.
5. For timed-out motion commands, a future policy may request emergency stop.

Initial simulated behavior may simply fail timed-out commands without automatic stop.

Before real hardware, timed-out motion should trigger stop/disable policy.

## 7. Command Replacement Rules

Because v1 has one pending slot, command replacement must be explicit.

### Empty Slot

Any valid command may be accepted if Runtime policy allows it.

### Pending Motion Command

If pending command is `FORWARD` or `REVERSE`:

- `STOP` may replace it.
- another `FORWARD` or `REVERSE` must be rejected with current compact error.
- replacement writes latest command state for the replaced command as `CANCELLED` if supported.

If `CANCELLED` command-state write is deferred, the replacement must at least record the accepted stop command.

### Pending Stop Command

If pending command is `STOP`:

- another `STOP` may replace or refresh it.
- `FORWARD` or `REVERSE` must be rejected until stop completes.

### Completed/Failed/Timed-Out Command

Final command states clear the pending slot.

New commands may then be accepted according to Runtime and safety policy.

### Runtime State Change While Command Is Pending

If Runtime enters `SAFE_MODE` while a motion command is pending:

- pending `FORWARD` or `REVERSE` must not execute.
- command state becomes `FAILED` or `CANCELLED`.
- pending slot is cleared.
- emergency stop may be accepted next.

If Runtime enters `ERROR_STATE`:

- pending command must not execute.
- command state becomes `FAILED`.
- pending slot is cleared.

## 8. Registry Command-State Updates

Registry stores latest command facts only.

It must not store pending queue history.

Command-state update moments:

### Request Accepted

```text
capability_id = CAP_MOTOR_CONTROL
command_state = ACCEPTED
timestamp_ms = now_ms
registry_result = OK
error_code = "none"
value_float = requested speed_percent
value_int = requested MotorDirection
```

### Execution Started

Optional for Milestone 4.1:

```text
command_state = EXECUTING
```

If used, it must be compact and overwritten by final state.

### Execution Completed

```text
command_state = COMPLETED
registry_result = OK
error_code = "none"
value_float = completed speed_percent
value_int = completed MotorDirection
```

### Invalid Request

```text
command_state = FAILED
registry_result = INVALID_RECORD
error_code = ERR_CAPABILITY_UNAVAILABLE until precise error exists
value_float = requested speed_percent
value_int = requested MotorDirection
```

### Runtime Blocked

```text
command_state = FAILED
registry_result = OK
error_code = ERR_CAPABILITY_UNAVAILABLE until precise error exists
value_float = requested speed_percent
value_int = requested MotorDirection
```

### Timeout

```text
command_state = TIMED_OUT
registry_result = OK
error_code = ERR_CAPABILITY_UNAVAILABLE until precise timeout error exists
value_float = requested speed_percent
value_int = requested MotorDirection
```

### Cancelled/Replaced

```text
command_state = CANCELLED
registry_result = OK
error_code = ERR_CAPABILITY_UNAVAILABLE until precise replacement error exists
value_float = replaced speed_percent
value_int = replaced MotorDirection
```

Registry still must not:

- validate motor policy
- execute commands
- inspect Runtime state
- call Services
- call Devices
- call Drivers

## 9. Validation Requirements

Validation must remain simulated and must not modify `src/main.cpp`.

Required validation:

1. Motor command request is accepted without immediate Device execution.
2. Registry command state becomes `ACCEPTED`.
3. Runtime command task executes pending command.
4. Registry command state becomes `COMPLETED`.
5. Payload updates only after Runtime task execution.
6. `FORWARD / 50.0F` succeeds in `READY`.
7. `REVERSE / 25.0F` succeeds in `RUNNING`.
8. Invalid speed fails before pending slot is occupied.
9. Invalid direction fails before pending slot is occupied.
10. Pending motion command rejects another motion command.
11. Pending motion command may be replaced by `STOP`.
12. `STOP` sets payload to `STOP / 0.0F`.
13. `SAFE_MODE` blocks `FORWARD`.
14. `SAFE_MODE` blocks `REVERSE`.
15. `SAFE_MODE` allows `STOP`.
16. Runtime `ERROR_STATE` blocks all motor commands.
17. Expired pending command becomes `TIMED_OUT`.
18. Timed-out command does not call Device.
19. Driver failure writes failed command state.
20. Driver failure does not store unsafe requested speed as payload.
21. API command path still calls MotorService only.
22. Runtime does not know `CAP_MOTOR_CONTROL`.
23. Registry stores state only.
24. Existing temperature, distance, and servo validation still pass.

Suggested validation sequence:

```text
Runtime READY
API command CAP_MOTOR_CONTROL FORWARD 50 -> accepted
verify command state ACCEPTED
verify payload still previous state
Runtime command task update
verify command state COMPLETED
verify payload FORWARD / 50

API command CAP_MOTOR_CONTROL REVERSE 25 -> accepted
Runtime RUNNING
Runtime command task update
verify payload REVERSE / 25

API command CAP_MOTOR_CONTROL FORWARD 999 -> rejected
verify no pending command
verify payload still REVERSE / 25

API command CAP_MOTOR_CONTROL FORWARD 40 -> accepted
API command CAP_MOTOR_CONTROL FORWARD 60 -> rejected
API command CAP_MOTOR_CONTROL STOP 0 -> replaces pending motion
Runtime command task update
verify payload STOP / 0

Runtime SAFE_MODE
API command CAP_MOTOR_CONTROL FORWARD 25 -> rejected
API command CAP_MOTOR_CONTROL STOP 0 -> accepted
Runtime command task update
verify payload STOP / 0
```

## 10. Stop Conditions Before Real Hardware

Stop before real motor hardware if:

- Motor commands execute immediately in API path.
- API calls Device or Driver directly.
- Runtime owns direction/speed policy.
- Registry executes commands.
- EventBus stores command state.
- pending command slot is unbounded or heap-allocated.
- motion commands can execute in `SAFE_MODE`.
- `STOP` cannot execute in `SAFE_MODE`.
- invalid speed reaches Device.
- invalid direction reaches Device.
- timed-out command reaches Device.
- command replacement rules are undocumented or unvalidated.
- Registry command state cannot represent `ACCEPTED`, `EXECUTING`, `COMPLETED`, `FAILED`, `TIMED_OUT`, and `CANCELLED`.
- failed command stores unsafe requested motion as payload state.
- unavailable motor state is treated as stopped.
- dynamic allocation is introduced.
- Arduino `String` is introduced.
- STL containers are introduced.
- real PWM is introduced.
- H-bridge code is introduced.
- `src/main.cpp` is modified for motor hardware before simulated validation passes.

## Recommended Phase Order

### Phase 1 - Plan And Audit

Create this bounded command plan.

No source changes.

### Phase 2 - Pending Command Types

Add compact pending command record support local to `MotorService`.

No Runtime behavior change yet.

### Phase 3 - Request/Accept API

Change motor API command request path so it requests command acceptance through `MotorService`, without immediate Device execution.

### Phase 4 - Runtime Command Task

Add `task.motor_service.execute_command`.

Runtime schedules only.

### Phase 5 - Execution Method

Add `MotorService::executePendingCommand(now_ms)`.

Service owns policy and Device calls.

### Phase 6 - SAFE_MODE Stop Exception

Allow `STOP / 0.0F` in `SAFE_MODE`.

Continue blocking `FORWARD` and `REVERSE`.

### Phase 7 - Timeout Enforcement

Validate and enforce bounded timeout rules.

### Phase 8 - Replacement Rules

Implement one-slot replacement rules, with `STOP` priority.

### Phase 9 - Validation

Extend validation for acceptance, execution, timeout, replacement, `SAFE_MODE` stop, and failure mode.

### Phase 10 - Audit

Audit again before HAL, PWM, H-bridge, or real motor work.

## Success Criteria

Milestone 4.1 is ready for implementation when:

- one pending motor command slot is defined
- Runtime command task behavior is defined
- API request and Runtime execution phases are separated
- `ACCEPTED` and `COMPLETED` are observably distinct
- emergency stop replacement is documented
- `SAFE_MODE` allows stop and blocks motion
- timeout behavior is bounded
- command replacement rules are explicit
- Registry command-state updates are defined
- validation requirements are complete
- no real hardware is introduced
