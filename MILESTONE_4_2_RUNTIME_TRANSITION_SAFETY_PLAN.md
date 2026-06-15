# Milestone 4.2 Runtime Transition Safety Plan

## Goal

Define behavior when Runtime state changes while `CAP_MOTOR_CONTROL` commands are pending.

This document is planning only.

No source code is modified by this document.

## Inputs Reviewed

- `MILESTONE_4_1_ARCHITECTURE_AUDIT.md`
- `MILESTONE_4_1_BOUNDED_MOTOR_COMMAND_PLAN.md`
- `SAFE_MODE_ARCHITECTURE_PLAN.md`

## Scope

Included:

- pending command plus `SAFE_MODE` transition
- pending command plus `ERROR_STATE` transition
- pending command plus Runtime reset
- pending command plus capability unavailable
- pending command plus driver failure before execution
- required command-state transitions
- Registry expectations
- Runtime responsibilities
- MotorService responsibilities
- validation requirements
- state transition tables
- expected command states
- expected payload states
- stop conditions before implementation

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

## Current Model

Milestone 4.1 introduced a bounded pending command model:

```text
API
-> MotorService request validation
-> MotorPendingCommand slot
-> Registry command state ACCEPTED
-> Runtime command task
-> MotorService::executePendingCommand()
-> Device/Driver execution if still safe
-> Registry payload and command-state update
```

Runtime remains scheduler-only.

MotorService owns motor policy.

Registry stores state only.

## 1. Pending Command + SAFE_MODE Transition

### Policy

When Runtime enters `SAFE_MODE` while a motor command is pending:

- pending `FORWARD` must not execute
- pending `REVERSE` must not execute
- pending `STOP / 0.0F` may execute
- payload must remain unchanged for blocked motion
- command state must become final before the slot is cleared

### Transition Table

| Pending command | Runtime transition | Device call allowed | Final command state | Payload state |
|---|---|---:|---|---|
| `FORWARD / speed` | `READY/RUNNING -> SAFE_MODE` | no | `FAILED` or future `CANCELLED` | unchanged |
| `REVERSE / speed` | `READY/RUNNING -> SAFE_MODE` | no | `FAILED` or future `CANCELLED` | unchanged |
| `STOP / 0.0F` | `READY/RUNNING -> SAFE_MODE` | yes | `COMPLETED` if Device succeeds, otherwise `FAILED` | `STOP / 0.0F` on success |

### Expected Command States

Initial simulated v1 should use:

```text
motion pending + SAFE_MODE = FAILED
stop pending + SAFE_MODE = COMPLETED if provider succeeds
```

Future implementation may use:

```text
motion pending + SAFE_MODE = CANCELLED
```

Only add `CANCELLED` behavior if validation and API exposure are updated together.

### Expected Payload States

Motion pending then `SAFE_MODE`:

```text
payload remains previous valid state
```

STOP pending then `SAFE_MODE`:

```text
payload becomes STOP / 0.0F if Device stop succeeds
```

## 2. Pending Command + ERROR_STATE Transition

### Policy

`ERROR_STATE` blocks all pending motor commands, including STOP, unless a future explicitly documented emergency path exists outside normal command execution.

For simulated v1:

- no pending command executes in `ERROR_STATE`
- Device is not called
- command state becomes `FAILED`
- pending slot is cleared
- payload remains unchanged

### Transition Table

| Pending command | Runtime transition | Device call allowed | Final command state | Payload state |
|---|---|---:|---|---|
| `FORWARD / speed` | any allowed state -> `ERROR_STATE` | no | `FAILED` | unchanged |
| `REVERSE / speed` | any allowed state -> `ERROR_STATE` | no | `FAILED` | unchanged |
| `STOP / 0.0F` | any allowed state -> `ERROR_STATE` | no for v1 | `FAILED` | unchanged |

### Rationale

`ERROR_STATE` means the system cannot assume actuator providers are safe to command.

`SAFE_MODE` is where documented safe actions such as STOP may still run.

## 3. Pending Command + Runtime Reset

### Policy

Runtime reset clears in-memory pending commands because Cyber32 v1 uses fixed RAM state with no command queue persistence.

After reset:

- pending command slot is empty
- payload state starts from boot/registration state until refreshed
- Registry command state may show the last pre-reset command only if Registry memory survives, but no command may be resumed from it
- no pending command is replayed
- no Device call occurs because of a pre-reset pending command

### Reset Table

| Condition before reset | After reset pending slot | Device call on boot | Command state after boot | Payload state after boot |
|---|---|---:|---|---|
| pending `FORWARD` | empty | no replay | latest reset/error state if available | initial/stale until update |
| pending `REVERSE` | empty | no replay | latest reset/error state if available | initial/stale until update |
| pending `STOP` | empty | no replay | latest reset/error state if available | initial/stale until update |

### Required Boot Rule

No pending motor command may survive reboot as executable intent.

Real hardware must default to stopped/disabled before Runtime reaches `READY`.

## 4. Pending Command + Capability Unavailable

### Policy

If `CAP_MOTOR_CONTROL` becomes unavailable while a command is pending:

- pending command must not execute
- Device is not called if unavailability is known before execution
- command state becomes `FAILED`
- pending slot is cleared
- payload should be updated to unavailable if Service can obtain or construct a bounded unavailable payload

### Transition Table

| Pending command | Capability state before execution | Device call allowed | Final command state | Payload state |
|---|---|---:|---|---|
| `FORWARD / speed` | unavailable | no | `FAILED` | unavailable or previous safe state |
| `REVERSE / speed` | unavailable | no | `FAILED` | unavailable or previous safe state |
| `STOP / 0.0F` | unavailable | no if provider absent | `FAILED` | unavailable or previous safe state |

### Payload Priority

Preferred payload result:

```text
available = UNAVAILABLE
value_type = NONE
error_code = ERR_CAPABILITY_UNAVAILABLE or ERR_DEVICE_TIMEOUT
```

If unavailable payload cannot be safely generated, preserve previous payload and store failed command state.

## 5. Pending Command + Driver Failure Before Execution

### Policy

If Driver failure mode or driver fault occurs before execution:

- request may already be `ACCEPTED`
- Runtime command task calls MotorService execution
- MotorService calls Device only if Runtime and timeout policy allow it
- Device/Driver failure returns failed execution
- Registry command state becomes `FAILED`
- unsafe requested motion must not become payload state
- pending slot is cleared

### Transition Table

| Pending command | Driver state at execution | Device/Driver result | Final command state | Payload state |
|---|---|---|---|---|
| `FORWARD / speed` | failed | command fails | `FAILED` | previous safe state or unavailable |
| `REVERSE / speed` | failed | command fails | `FAILED` | previous safe state or unavailable |
| `STOP / 0.0F` | failed | stop fails | `FAILED` | previous safe state or unavailable |

### Required Safety Rule

Never store requested motion as payload unless Driver execution succeeds and Registry payload update succeeds.

## 6. Required Command-State Transitions

### Request Accepted

```text
REQUEST -> ACCEPTED
```

Registry command state:

```text
capability_id = CAP_MOTOR_CONTROL
command_state = ACCEPTED
value_float = requested speed_percent
value_int = requested MotorDirection
error_code = "none"
```

### Runtime Executes Successfully

```text
ACCEPTED -> COMPLETED
```

Registry command state:

```text
command_state = COMPLETED
registry_result = OK
error_code = "none"
```

Payload:

```text
updated to executed direction/speed
```

### Runtime Blocks Due To SAFE_MODE Motion

```text
ACCEPTED -> FAILED
```

Device call:

```text
no
```

Payload:

```text
unchanged
```

### Runtime Blocks Due To ERROR_STATE

```text
ACCEPTED -> FAILED
```

Device call:

```text
no
```

Payload:

```text
unchanged
```

### Timeout

```text
ACCEPTED -> TIMED_OUT
```

Device call:

```text
no
```

Payload:

```text
unchanged
```

### Driver Failure

```text
ACCEPTED -> FAILED
```

Payload:

```text
previous safe state or unavailable
```

### STOP Replacement

Current latest-state-only model:

```text
ACCEPTED motion -> ACCEPTED stop -> COMPLETED stop
```

Optional future observable replacement:

```text
ACCEPTED motion -> CANCELLED motion -> ACCEPTED stop -> COMPLETED stop
```

## 7. Registry Expectations

Registry stores facts and state only.

Registry may store:

- latest motor payload state
- latest motor command state
- latest compact error code in payload or command state

Registry must not:

- execute pending commands
- clear pending slots
- inspect Runtime state
- validate motor direction
- validate motor speed
- choose SAFE_MODE behavior
- call MotorService
- call Device
- call Driver

### Payload State Expectations

| Scenario | Payload result |
|---|---|
| accepted but not executed | unchanged |
| completed motion | executed direction/speed |
| completed stop | `STOP / 0.0F` |
| timed out before execution | unchanged |
| SAFE_MODE blocks motion | unchanged |
| ERROR_STATE blocks command | unchanged |
| driver failure | previous safe state or unavailable |
| capability unavailable | unavailable if safe to write, otherwise previous safe state |

### Command State Expectations

| Scenario | Command state |
|---|---|
| accepted request | `ACCEPTED` |
| successful execution | `COMPLETED` |
| timeout | `TIMED_OUT` |
| runtime blocked | `FAILED` |
| capability unavailable | `FAILED` |
| driver failure | `FAILED` |
| replaced motion | overwritten by accepted STOP, or future `CANCELLED` |

## 8. Runtime Responsibilities

Runtime may:

- expose current `RuntimeState`
- enter `SAFE_MODE`
- enter `ERROR_STATE`
- reset/start from boot state
- schedule motor command execution task
- schedule motor state update task

Runtime must not:

- know `CAP_MOTOR_CONTROL`
- inspect pending motor direction
- inspect pending motor speed
- decide whether STOP is safe
- decide whether motion is safe
- call MotorService policy methods except scheduled task callback
- call Device
- call Driver
- store command history

Runtime transition behavior should be observable only through state and scheduled task execution.

MotorService reacts to Runtime state.

## 9. MotorService Responsibilities

MotorService owns:

- pending command slot
- command validation
- timeout checks
- Runtime-state gating
- SAFE_MODE stop exception
- ERROR_STATE blocking
- command replacement policy
- command-state writes
- payload updates through Registry public API
- clearing pending command after final state

MotorService must not:

- change Runtime state
- bypass Registry public APIs
- expose external API
- run Logic
- allocate memory dynamically
- use Arduino `String`
- use STL containers

## 10. Validation Requirements

Validation must remain simulated and must not modify `src/main.cpp`.

Required validation:

1. Pending `FORWARD` then Runtime enters `SAFE_MODE`.
2. Runtime command task runs.
3. Command state becomes `FAILED`.
4. Device is not called.
5. Payload remains unchanged.
6. Pending `REVERSE` then Runtime enters `SAFE_MODE`.
7. Command state becomes `FAILED`.
8. Pending `STOP` then Runtime enters `SAFE_MODE`.
9. Command executes and payload becomes `STOP / 0.0F`.
10. Pending `FORWARD` then Runtime enters `ERROR_STATE`.
11. Command state becomes `FAILED`.
12. Payload remains unchanged.
13. Pending `STOP` then Runtime enters `ERROR_STATE`.
14. Command state becomes `FAILED` for v1.
15. Runtime reset clears pending slot.
16. No command is replayed after reset.
17. Capability unavailable before execution fails pending command.
18. Driver failure before execution fails pending command.
19. Unsafe requested payload is never stored after failure.
20. Existing Milestone 4.1 validations still pass.

### Suggested Validation Sequences

#### SAFE_MODE With Pending Motion

```text
Runtime READY
command FORWARD / 40 -> ACCEPTED
verify payload unchanged
runtime.enterSafeMode()
Runtime command task update
expect command state FAILED
expect payload unchanged
```

#### SAFE_MODE With Pending STOP

```text
Runtime RUNNING
command FORWARD / 20 -> execute successfully
command STOP / 0 -> ACCEPTED
runtime.enterSafeMode()
Runtime command task update
expect command state COMPLETED
expect payload STOP / 0
```

#### ERROR_STATE With Pending Motion

```text
Runtime READY
command REVERSE / 25 -> ACCEPTED
runtime.setState(ERROR_STATE)
Runtime command task update
expect command state FAILED
expect payload unchanged
```

#### Runtime Reset With Pending Command

```text
Runtime READY
command FORWARD / 30 -> ACCEPTED
simulate Runtime/Service begin reset
expect pending slot empty
expect no Device execution
```

#### Capability Unavailable Before Execution

```text
Runtime READY
command FORWARD / 30 -> ACCEPTED
mark CAP_MOTOR_CONTROL unavailable or simulate provider loss
Runtime command task update
expect command state FAILED
expect payload unavailable or previous safe state
```

#### Driver Failure Before Execution

```text
Runtime READY
command FORWARD / 30 -> ACCEPTED
enable simulated driver failure mode
Runtime command task update
expect command state FAILED
expect payload not FORWARD / 30
clear failure mode
```

## State Transition Tables

### Runtime Transition While Pending

| From Runtime | To Runtime | Pending command | Expected command state | Expected payload |
|---|---|---|---|---|
| `READY` | `SAFE_MODE` | `FORWARD` | `FAILED` or future `CANCELLED` | unchanged |
| `RUNNING` | `SAFE_MODE` | `REVERSE` | `FAILED` or future `CANCELLED` | unchanged |
| `READY` | `SAFE_MODE` | `STOP` | `COMPLETED` if provider succeeds | `STOP / 0.0F` |
| `RUNNING` | `ERROR_STATE` | `FORWARD` | `FAILED` | unchanged |
| `READY` | `ERROR_STATE` | `REVERSE` | `FAILED` | unchanged |
| `RUNNING` | `ERROR_STATE` | `STOP` | `FAILED` in v1 | unchanged |
| any | reset | any pending | no execution after reset | initial/stale until refreshed |

### Failure Conditions While Pending

| Condition | Pending command | Device called | Command state | Payload |
|---|---|---:|---|---|
| timeout | any | no | `TIMED_OUT` | unchanged |
| capability unavailable | any | no if known first | `FAILED` | unavailable or unchanged |
| driver failure | motion | yes, fails | `FAILED` | previous safe or unavailable |
| driver failure | stop | yes, fails | `FAILED` | previous safe or unavailable |
| SAFE_MODE motion | `FORWARD`/`REVERSE` | no | `FAILED` | unchanged |
| SAFE_MODE stop | `STOP` | yes | `COMPLETED` or `FAILED` | stopped on success |
| ERROR_STATE | any | no | `FAILED` | unchanged |

## Stop Conditions Before Implementation

Stop before implementing Milestone 4.2 if:

- Runtime would need to inspect motor direction or speed.
- Runtime would need to know `CAP_MOTOR_CONTROL`.
- Registry would need to execute or clear commands.
- EventBus would be used as command-state storage.
- pending command storage would require a queue.
- dynamic allocation would be introduced.
- Arduino `String` would be introduced.
- STL containers would be introduced.
- SAFE_MODE motion could reach Device.
- ERROR_STATE command could reach Device.
- Runtime reset could replay a pending command.
- timed-out command could reach Device.
- failed driver command could store requested unsafe motion as payload.
- `src/main.cpp` would need modification before validation exists.
- real motor hardware, PWM, or H-bridge work would be required.

## Success Criteria

Milestone 4.2 is ready for implementation when:

- pending command behavior is defined for `SAFE_MODE`
- pending command behavior is defined for `ERROR_STATE`
- pending command behavior is defined for Runtime reset
- capability unavailable behavior is defined
- driver failure behavior is defined
- command-state transitions are explicit
- payload expectations are explicit
- Registry remains state-only
- Runtime remains scheduler/state coordinator only
- MotorService remains policy owner
- validation requirements are complete
- no real hardware is introduced
