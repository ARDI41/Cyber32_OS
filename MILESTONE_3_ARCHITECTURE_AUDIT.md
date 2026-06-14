# Milestone 3 Architecture Audit

## Goal

Review the implemented `CAP_SERVO_POSITION` actuator slice against the Cyber32 architecture.

This document is an audit only.

No source code is modified by this document.

## Scope Reviewed

- `CAP_SERVO_POSITION`
- `SimServoDriver`
- `SimServoDevice`
- `SimServoModule`
- PNP discovery and registration
- Registry state usage
- `ServoService`
- internal API command path
- validation harness
- actuator safety rules

## Passes

### Capability-First Targeting

`CAP_SERVO_POSITION` is represented as a canonical capability ID.

The servo slice uses capability IDs for:

- PNP module metadata
- Registry capability registration
- payload state updates
- API state reads
- API command request routing
- validation checks

No servo Logic behavior has been implemented yet, so there is no Logic dependency on module names, device IDs, or driver classes.

### Driver Layer

`SimServoDriver` remains a simulated Driver only.

Passes:

- no Registry access
- no EventBus access
- no Runtime access
- no API access
- no module-name dependency
- no `Servo.h`
- no real PWM or hardware control
- no dynamic allocation
- no Arduino `String`
- no STL containers

The Driver stores only initialized state, failure mode, and current simulated position.

### Device Layer

`SimServoDevice` wraps `SimServoDriver` and exposes `CAP_SERVO_POSITION` payloads.

Passes:

- Device does not register capabilities.
- Device does not write Registry.
- Device does not publish events.
- Device does not run Logic.
- Device does not expose API.
- Device does not know module names.
- Device does not include real servo hardware control.

The Device performs a final device-level safety check through the Driver range validation path and returns unavailable/error payloads on failure.

### Module Layer

`SimServoModule` is metadata-only.

Passes:

- exposes module ID, module type, metadata level, display name, device ID, device type, and capability ID
- does not register itself
- does not write Registry
- does not publish events
- does not run Logic
- does not expose API
- does not call Device methods

The friendly display name remains metadata only.

### PNP Discovery

PNP discovery now supports the simulated servo module through `discoverSimulatedServoModule()`.

Passes:

- reads module metadata only
- validates required fields are present
- validates metadata level
- validates `CAP_` prefix
- publishes `EVT_MODULE_DISCOVERED` only if EventBus is attached
- does not write Registry
- does not call Driver
- does not call Device behavior
- does not run Logic
- does not expose API

EventBus integration remains optional.

### PNP Registration

PNP registration supports:

- `CAP_TEMPERATURE`
- `CAP_DISTANCE`
- `CAP_SERVO_POSITION`

Passes:

- uses Registry public result APIs
- uses `RegistryWriteResult` indexes for module and device ownership
- does not write Registry arrays directly
- does not call Driver
- does not call Device behavior
- does not run Logic
- does not expose API
- preserves temperature and distance behavior

The servo capability is registered as:

```text
category = "actuators"
kind = "actuator"
data_type = PayloadValueType::FLOAT
access = "read_write"
unit = "degree"
```

### Registry Ownership

The Registry continues to store facts and state only.

Passes:

- stores module/device/capability records
- stores latest `CAP_SERVO_POSITION` payload
- emits capability registration and value update events through existing Registry behavior
- does not execute servo commands
- does not validate servo command policy
- does not call Device, Driver, Service, Logic, API, or Runtime

### ServoService Ownership

`ServoService` owns first-slice command validation and command execution policy.

Passes:

- validates position range before calling Device
- rejects invalid `position_degrees`
- calls `SimServoDevice` only after validation
- updates Registry through `updateCapabilityPayloadWithResult()`
- returns compact command state through `ServoCommandResult`
- stores last Registry result and command state locally
- does not run Logic
- does not expose API transport
- does not publish events directly when Registry already emits payload update events
- does not know module names
- does not include real servo hardware control

### API Command Path

The internal API exposes servo state and command request support without external transport.

Passes:

- `getServoPositionState()` reads Registry state using `getCapabilityPayloadWithResult(CAP_SERVO_POSITION)`
- `commandServoPosition()` calls `ServoService::setPosition()`
- API does not call `SimServoDevice`
- API does not call `SimServoDriver`
- API does not call HAL
- API does not own canonical state
- no JSON
- no HTTP
- no WiFi/WebServer

This satisfies the required rule:

```text
API command -> Service -> Device -> Driver
```

### Runtime Responsibility

Runtime remains scheduler-only in the implemented validation path.

Passes:

- Runtime task `task.servo_service.update_state` calls `ServoService::updateState()`
- Runtime does not inspect `CAP_SERVO_POSITION`
- Runtime does not validate servo position ranges
- Runtime does not call Device or Driver directly
- Runtime does not own command policy

### Validation Harness

The validation harness now covers:

- simulated servo Driver initialization
- simulated servo Device initialization
- servo module PNP discovery
- servo module/device/capability registration
- Registry counts of 3 modules, 3 devices, 3 capabilities
- Runtime-driven servo state update
- API servo state read
- valid command `45.0F`
- invalid command `999.0F`
- preservation of previous valid state after invalid command

The validation harness does not modify `src/main.cpp`.

### Memory And Allocation Rules

The implemented servo slice follows the current ESP32 v1 memory policy.

Passes:

- fixed objects
- pointer references only
- no heap allocation
- no Arduino `String`
- no STL containers
- no unbounded queues
- compact payload and command result structures

## Warnings

### Runtime Does Not Yet Coordinate Servo Commands

The validation harness runs servo state updates through a Runtime task, but API servo commands currently call `ServoService::setPosition()` directly.

This is acceptable for the current internal validation phase, but it is not the final command dispatch model.

Target architecture says command execution should flow:

```text
API or Logic
-> Service
-> Runtime
-> Device
-> Driver
```

Current implemented command flow is:

```text
API
-> ServoService
-> SimServoDevice
-> SimServoDriver
```

This is a controlled first-slice shortcut and must be closed before real servo hardware.

### ServoService Does Not Yet Enforce Runtime READY/RUNNING

The actuator safety policy says motion outputs must not activate before Runtime `READY`.

Current `ServoService::setPosition()` validates range and attachment, but does not receive or inspect Runtime state.

This is safe only because the Driver is simulated and has no hardware output.

Before real hardware, `ServoService` must block arbitrary position commands unless Runtime is in an allowed state.

### Safe Mode Is Not Implemented

The current slice has no safe mode state or safe mode command restrictions.

The actuator safety policy requires:

- arbitrary servo movement blocked in safe mode
- hold/disable allowed
- configured safe position allowed only if validated

This is a readiness gap, not a current layer violation.

### Command State Is Not Stored In Registry

`ServoService` stores `last_command_state_` locally, and API returns command results directly.

The command dispatch contract says Registry should store latest compact command state when command-state storage exists.

Current Registry stores latest servo payload state, not latest command state.

Before external API transport or real hardware, add a bounded Registry command-state model.

### Timeout Field Is Accepted But Not Enforced

`ServoCommandRequest` and `ApiServoCommandRequest` include `timeout_ms`.

Current implementation does not enforce timeout bounds or deadlines.

Before real actuator control, Services and Runtime must enforce bounded command timeouts.

### Error Codes Are Coarse

Invalid command range currently maps to `ERR_CAPABILITY_UNAVAILABLE`.

This is compact and uses existing IDs, but semantically coarse.

The error model should add or enable more precise command validation errors before external command exposure, such as:

```text
ERR_CONFIG_INVALID
ERR_COMMAND_INVALID
ERR_RUNTIME_NOT_READY
ERR_RUNTIME_OVERLOAD
ERR_ACTUATOR_BLOCKED
```

Only canonical error IDs should be added.

### Capability Access String Differs From Earlier Planning Text

The implemented servo registration uses:

```text
access = "read_write"
```

Some planning text uses:

```text
read-write
```

This should be standardized before more command capabilities are added.

### Servo Module Type Differs From Planning Alternative

The implemented module type is:

```text
actuator
```

Some planning text listed `motion` as an option.

This is acceptable because the user explicitly required `module_type = "actuator"` for the implemented phase, but the classification docs should eventually standardize actuator module categories.

## Architecture Violations

No hard architecture violations were found in the implemented simulated slice.

The following are not current violations because the slice is still internal and simulated, but they must not be carried into real actuator hardware:

- API command path currently bypasses Runtime command scheduling.
- Servo commands are not gated by Runtime `READY/RUNNING`.
- Safe mode restrictions are not implemented.
- Registry does not yet store latest command state.
- Timeout behavior is not enforced.

## Recommended Fixes

1. Add a bounded command execution task for servo commands.

Target:

```text
API
-> ServoService request/accept
-> Runtime task
-> ServoService execution callback
-> Device
-> Driver
-> Registry update
```

2. Extend `ServoService::begin()` or add attachment methods for Runtime state access without making Runtime own policy.

The Service should be able to reject commands when Runtime is not `READY` or `RUNNING`.

3. Define compact command-state storage in Registry.

Store only latest command facts:

```text
capability_id
command_state
timestamp_ms
error_code
compact result code
latest value
```

Do not store full command payloads or histories.

4. Add timeout validation and enforcement.

Service should validate timeout range.

Runtime should enforce deadlines when command tasks are scheduled.

5. Standardize access strings.

Choose one canonical form:

```text
read_write
```

or:

```text
read-write
```

Then update docs and registration code consistently.

6. Add precise actuator command error codes.

Recommended future additions:

```text
ERR_CONFIG_INVALID
ERR_RUNTIME_OVERLOAD
ERR_RUNTIME_NOT_READY
ERR_COMMAND_INVALID
ERR_ACTUATOR_BLOCKED
```

7. Add simulated failure-mode validation.

Validation should cover:

- driver failure mode
- command failure path
- Registry unavailable payload update
- API failed command response
- state remains safe after failed command

8. Add safe mode validation before real hardware.

Validation should prove arbitrary servo commands are blocked when Runtime enters safe mode.

## Actuator Readiness Gaps

The simulated actuator slice is useful for architecture validation, but not ready for real servo hardware.

Required before real servo hardware:

1. HAL servo/PWM abstraction with safe defaults.
2. Driver disable/detach behavior.
3. Boot-time no-pulse/no-sweep guarantee.
4. Runtime `READY/RUNNING` command gating.
5. Safe mode implementation.
6. Watchdog recovery behavior.
7. Brownout recovery behavior.
8. Runtime command task scheduling for servo commands.
9. Timeout enforcement.
10. Registry command-state storage.
11. More precise actuator error IDs.
12. Emergency safe action for servo failure.
13. Validation for driver failure and module loss.
14. Validation that invalid commands never reach Device.
15. Validation that unavailable capability is never treated as zero degrees.
16. Real hardware tests on non-load-bearing servo setup only.

## Next Steps

Recommended next milestone:

```text
Milestone 3.1 - Bounded Servo Command Coordination
```

Suggested phases:

1. Define Registry command-state record schema.
2. Add compact command result/error codes if approved.
3. Add Servo command pending slot to `ServoService`.
4. Add Runtime servo command execution task context.
5. Change API command path to request through Service and complete through Runtime task.
6. Add Runtime state gating to `ServoService`.
7. Add timeout validation.
8. Add validation for failure mode and blocked command paths.
9. Audit again before any real servo HAL or Driver work.

Do not implement real servo hardware until these gaps are closed.
