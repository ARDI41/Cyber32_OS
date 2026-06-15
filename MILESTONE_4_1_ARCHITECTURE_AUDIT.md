# Milestone 4.1 Architecture Audit

## Goal

Review Milestone 4.1 bounded motor command safety against Cyber32 architecture and safety rules.

This document is an audit only.

No source code is modified by this document.

## Scope Reviewed

- `CAP_MOTOR_CONTROL`
- `MotorService`
- `MotorPendingCommand`
- pending command slot
- Runtime motor command execution task
- command `ACCEPTED` to executed flow
- timeout handling
- STOP replacement and override
- `SAFE_MODE` stop exception
- Registry payload state
- Registry command state
- API motor command path
- validation harness

## Passes

### Capability-First Command Targeting

Motor command behavior remains centered on `CAP_MOTOR_CONTROL`.

Passes:

- command state records use `CAP_MOTOR_CONTROL`
- payload state records use `CAP_MOTOR_CONTROL`
- API motor state reads use `CAP_MOTOR_CONTROL`
- API motor command-state reads use `CAP_MOTOR_CONTROL`
- validation checks capability state by capability ID
- no motor Logic behavior depends on module names

### API Command Path

API motor commands still request through `MotorService`.

Passes:

- `commandMotorControl()` maps API request into `MotorCommandRequest`
- API does not call `SimMotorDevice`
- API does not call `SimMotorDriver`
- API does not call HAL
- API does not own motor state
- API does not execute motor policy
- no JSON, HTTP, WiFi, or WebServer behavior is introduced

Current bounded path:

```text
API
-> MotorService::setMotor()
-> pending command slot
-> Registry command state ACCEPTED
```

Execution path:

```text
Runtime task
-> MotorService::executePendingCommand()
-> SimMotorDevice
-> SimMotorDriver
-> Registry payload update
-> Registry command state final update
```

### Pending Command Slot

`MotorPendingCommand` is a compact one-slot record owned by `MotorService`.

Fields:

```text
occupied
direction
speed_percent
requested_at_ms
timeout_ms
state
error_code
```

Passes:

- one fixed slot only
- no queue
- no heap allocation
- no STL containers
- no Arduino `String`
- no command history
- cleared after final command state
- initialized to empty safe state

The pending slot is internal execution state, while Registry remains the public source for latest command state.

### Accepted Vs Executed Flow

Milestone 4.1 separates request acceptance from Device execution.

Passes:

- `MotorService::setMotor()` validates and accepts valid requests.
- accepted requests return `CommandState::ACCEPTED`
- accepted requests set `accepted = true`
- accepted requests set `executed = false`
- accepted requests write `ACCEPTED` command state to Registry
- accepted requests do not call Device
- accepted requests do not update motor payload
- payload changes only after `executePendingCommand()`

Validation verifies:

- API `FORWARD / 50.0F` returns `ACCEPTED`
- payload remains `STOP / 0.0F` before Runtime command task
- Runtime command task executes command
- command state becomes `COMPLETED`
- payload becomes `FORWARD / 50.0F`

### Runtime Command Execution Task

The validation harness adds:

```text
task.motor_service.execute_command
```

Passes:

- Runtime task uses fixed context storage
- Runtime invokes a static callback
- callback calls `MotorService::executePendingCommand(now_ms)`
- Runtime does not know `CAP_MOTOR_CONTROL`
- Runtime does not inspect direction
- Runtime does not inspect speed
- Runtime does not validate motor policy
- Runtime does not call Device or Driver

Runtime remains scheduler-only.

### MotorService Policy Ownership

`MotorService` owns motor command safety policy.

Passes:

- validates Runtime state
- validates direction
- validates speed
- validates timeout range
- enforces one-slot pending policy
- owns STOP replacement rule
- owns `SAFE_MODE` stop exception
- re-checks Runtime policy during execution
- checks timeout before Device execution
- writes command state to Registry
- updates payload through Registry public result API

Runtime provides state only.

Registry stores state only.

API routes requests only.

### Device And Driver Call Boundary

Device and Driver are called only during execution, not request acceptance.

Passes:

- `setMotor()` stores pending command and does not call Device
- `executePendingCommand()` is the path that calls `SimMotorDevice::setMotor()`
- timed-out commands return before Device call
- Runtime-blocked commands return before Device call
- pending motion replacement does not call Device
- invalid commands fail before Device call

`MotorService::stop()` remains a direct synchronous stop path from the previous milestone, but Milestone 4.1 validation uses `commandMotorControl(STOP, 0.0F)` for bounded pending STOP behavior.

### Timeout Handling

Timeout enforcement exists before Device execution.

Passes:

- pending command stores `requested_at_ms`
- pending command stores `timeout_ms`
- timeout is checked in `executePendingCommand()` before Device call
- timeout writes `CommandState::TIMED_OUT`
- timeout clears pending slot
- timeout leaves payload unchanged
- timeout returns `false`

Validation verifies:

- `FORWARD / 40.0F` with `timeout_ms = 1` is accepted
- Runtime execution after timeout writes `TIMED_OUT`
- payload remains `STOP / 0.0F`
- no completed motion state is stored

### STOP Replacement And Override

One-slot replacement rules are implemented for STOP.

Passes:

- pending `FORWARD` or `REVERSE` rejects another motion command
- pending motion may be replaced by `STOP / 0.0F`
- replacement does not call Device
- payload remains unchanged until Runtime execution
- Runtime command task executes the replacement STOP
- final payload is `STOP / 0.0F`

Validation verifies:

- `FORWARD / 40.0F` is accepted
- `FORWARD / 60.0F` is rejected while pending exists
- `STOP / 0.0F` replaces pending motion
- Runtime execution completes STOP
- `FORWARD / 40.0F` never reaches payload

### SAFE_MODE Stop Exception

`SAFE_MODE` motor policy now distinguishes normal motion from stop.

Passes:

- `SAFE_MODE` blocks `FORWARD`
- `SAFE_MODE` blocks `REVERSE`
- `SAFE_MODE` allows `STOP / 0.0F`
- blocked motion is rejected at request stage
- blocked motion does not call Device
- blocked motion leaves payload unchanged
- `STOP / 0.0F` is accepted in `SAFE_MODE`
- `STOP / 0.0F` executes through Runtime command task
- successful STOP updates payload to `STOP / 0.0F`

Validation verifies both blocked motion and allowed stop behavior.

### Registry Payload State

Registry continues to store motor state only.

Passes:

- payload state changes only after execution
- payload state is not changed on accepted-but-not-executed command
- payload state is not changed on timed-out command
- payload state is not changed by blocked `SAFE_MODE` motion
- driver failure does not store unsafe requested motion payload
- Registry is updated through public result API

### Registry Command State

Registry stores latest command facts only.

Passes:

- `ACCEPTED` is stored at request acceptance
- `COMPLETED` is stored after successful execution
- `FAILED` is stored after blocked or failed execution
- `TIMED_OUT` is stored after timeout
- command state uses compact fields
- no history is stored
- no command queue is stored
- Registry does not execute commands

### Driver Failure Validation

Validation covers simulated driver failure.

Passes:

- driver failure mode is enabled in validation
- valid `FORWARD / 30.0F` request is accepted
- Runtime command task attempts execution
- command state becomes `FAILED`
- failure mode is cleared after test
- payload does not become `FORWARD / 30.0F`
- payload remains safe `STOP / 0.0F`

### REVERSE Validation

Validation now covers reverse motor command behavior.

Passes:

- Runtime is set to `RUNNING`
- `REVERSE / 25.0F` request is accepted
- Runtime command task executes command
- command state becomes `COMPLETED`
- payload speed becomes `25.0F`
- payload direction becomes `MotorDirection::REVERSE`

### Memory And Allocation Rules

Milestone 4.1 remains ESP32-friendly.

Passes:

- fixed pending slot
- fixed Runtime task contexts
- no dynamic allocation
- no Arduino `String`
- no STL containers
- no queues
- no command history
- no large metadata blobs

### Hardware Boundary

No real motor hardware was introduced.

Passes:

- no PWM
- no H-bridge code
- no motor hardware library
- no HAL motor driver
- no `src/main.cpp` motor behavior
- no WiFi/WebServer/Dashboard changes

## Warnings

### Direct `MotorService::stop()` Still Executes Immediately

`MotorService::stop()` remains a direct synchronous stop path that calls Device immediately.

This existed before Milestone 4.1 and was not changed by the bounded command phases.

The bounded STOP path is now available through:

```text
commandMotorControl(STOP, 0.0F)
-> pending command
-> Runtime command task
```

Before real hardware, decide whether `commandMotorStop()` should also use the pending command path or remain an emergency immediate stop path with explicit safety documentation.

### Replacement Does Not Store CANCELLED For Replaced Command

When STOP replaces pending motion, the latest command state becomes the accepted STOP request.

The replaced motion command is not separately written as `CANCELLED`.

This is acceptable for the latest-state-only Registry model, but it means the system cannot externally observe the replaced command after the STOP acceptance overwrites latest command state.

### Error Codes Remain Coarse

Several distinct outcomes still use `ERR_CAPABILITY_UNAVAILABLE`, including:

- invalid command
- Runtime blocked command
- timeout
- pending-slot unavailable
- safe-mode blocked motion

The behavior is machine-readable by `CommandState`, but error detail is coarse.

Before external transport or hardware, add canonical compact error IDs for command validation and actuator blocking.

### Timeout Failure Does Not Trigger Automatic Stop

Timed-out motion commands do not reach Device and leave payload unchanged.

This is correct for the simulated slice.

Before real hardware, timeout of an active or pending motion command should trigger a documented stop/disable policy if hardware output might already be active.

### Runtime Task Validation Is Harness-Driven

The command task exists in the validation harness, not yet in a production bootstrap or `main.cpp`.

This is acceptable because the user has intentionally kept `src/main.cpp` untouched.

Before hardware, the real ESP32 bootstrap sequence must register the command task in the production startup path.

### STOP Timeout Rules Are Still Strict In Pending Path

The plan allowed future emergency STOP to permit `timeout_ms == 0`.

Current `setMotor()` validation requires timeout `1..10000` for all pending commands, including STOP through `commandMotorControl(STOP, 0.0F)`.

This is safe and bounded, but the final emergency-stop policy should decide whether zero timeout means immediate execution, no timeout, or invalid request.

## Architecture Violations

No hard architecture violations were found in Milestone 4.1 bounded motor command safety.

The implementation satisfies the major architectural checks:

- API commands request through `MotorService`.
- Device and Driver are called by `MotorService` during Runtime task execution.
- Runtime remains scheduler-only.
- `MotorService` owns validation and safety policy.
- Registry stores state only and never executes commands.
- `SAFE_MODE` blocks `FORWARD` and `REVERSE`.
- `SAFE_MODE` allows `STOP / 0.0F`.
- timed-out commands do not reach Device.
- failed driver command does not store unsafe payload.
- STOP can replace pending motion.
- no queue or dynamic allocation was introduced.
- no Arduino `String` or STL containers were introduced.
- no real hardware/PWM/H-bridge was introduced.
- `src/main.cpp` remains untouched.

## Safety Gaps Before Real Hardware

The bounded simulated motor command model is stronger, but still not ready for real motor hardware.

Remaining gaps:

1. Production bootstrap does not yet register motor command task.
2. `commandMotorStop()` direct path needs explicit final policy.
3. No HAL motor/PWM abstraction exists.
4. No physical output disable path exists.
5. No watchdog-triggered motor stop/disable behavior exists.
6. No brownout-triggered motor stop/disable behavior exists.
7. No module-loss motor behavior exists.
8. No capability-unavailable motor recovery behavior exists.
9. No Runtime overload policy for motor commands exists.
10. No precise command error codes exist.
11. No command cancellation visibility beyond latest-state overwrite.
12. No validation for Runtime entering `ERROR_STATE` while command is pending.
13. No validation for Runtime entering `SAFE_MODE` while motion command is pending.
14. No validation for driver failure during `SAFE_MODE` stop.
15. No real hardware safety interlock.
16. No motor power isolation or enable/disable policy.

## Recommended Fixes

1. Decide final stop semantics.

Choose one:

```text
commandMotorStop() remains immediate emergency action
```

or:

```text
commandMotorStop() also requests bounded pending STOP
```

If immediate stop remains, document it as a deliberate emergency exception.

2. Add precise error IDs.

Recommended:

```text
ERR_COMMAND_INVALID
ERR_COMMAND_TIMEOUT
ERR_RUNTIME_NOT_READY
ERR_ACTUATOR_BLOCKED
ERR_PENDING_COMMAND_EXISTS
ERR_ACTUATOR_UNAVAILABLE
```

3. Add pending-state transition validation.

Cover:

- Runtime enters `SAFE_MODE` while pending motion exists
- Runtime enters `ERROR_STATE` while pending command exists
- pending motion is cancelled or failed before execution
- payload remains safe

4. Add production bootstrap plan.

Document object creation and task registration for:

```text
task.motor_service.update_state
task.motor_service.execute_command
```

without modifying `src/main.cpp` yet.

5. Add motor safe-output policy before HAL work.

Define:

- default stopped state
- output disable behavior
- driver fault behavior
- watchdog stop behavior
- brownout stop behavior

6. Re-audit before any real hardware.

No PWM, H-bridge, or motor driver hardware work should begin until the above policies are documented and simulated.

## Next Steps

Recommended next milestone:

```text
Milestone 4.2 - Motor Runtime Recovery And Stop Policy
```

Suggested phases:

1. Document final emergency stop semantics.
2. Add precise command and actuator error IDs.
3. Validate pending command when Runtime transitions to `SAFE_MODE`.
4. Validate pending command when Runtime transitions to `ERROR_STATE`.
5. Add production bootstrap documentation for Runtime command task registration.
6. Add motor safe-output policy document.
7. Audit again before HAL/PWM/H-bridge work.

## Final Assessment

Milestone 4.1 passes architecture review for the simulated ESP32 v1 bounded motor command model.

The implementation now proves the most important safety shape:

```text
API request
-> Service validation
-> bounded pending slot
-> Runtime scheduled execution
-> Service policy re-check
-> Device/Driver execution
-> Registry state update
```

It also proves that:

- accepted commands do not immediately move the motor
- timeout prevents Device execution
- STOP can replace pending motion
- `SAFE_MODE` blocks motion but allows STOP
- driver failure does not store unsafe requested payload

The slice remains simulation-only and hardware-safe.

Do not begin real motor hardware, PWM, or H-bridge work yet.
