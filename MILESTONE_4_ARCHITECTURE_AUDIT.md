# Milestone 4 Architecture Audit

## Goal

Review the implemented `CAP_MOTOR_CONTROL` slice against Cyber32 architecture and safety rules.

This document is an audit only.

No source code is modified by this document.

## Scope Reviewed

- `CAP_MOTOR_CONTROL`
- `SimMotorDriver`
- `SimMotorDevice`
- `SimMotorModule`
- PNP discovery and registration
- Registry payload and command-state usage
- `MotorService`
- internal API motor command path
- Runtime task integration
- `SAFE_MODE` blocking
- validation harness

## Passes

### Capability-First Targeting

`CAP_MOTOR_CONTROL` is represented as a canonical capability ID and is used across the slice for:

- module metadata
- PNP discovery
- PNP registration
- Registry capability state
- Registry command state
- Service payload updates
- API state reads
- API command routing
- validation checks

No Logic behavior has been added for motor control, so no Logic code depends on module IDs, device IDs, driver classes, or friendly names.

### Driver Layer

`SimMotorDriver` remains a simulated Driver only.

Passes:

- no Registry access
- no EventBus access
- no Runtime access
- no API access
- no module-name dependency
- no real motor hardware
- no PWM
- no H-bridge code
- no dynamic allocation
- no Arduino `String`
- no STL containers

The Driver stores only initialized state, failure mode, current direction, and current speed percent.

### Device Layer

`SimMotorDevice` wraps `SimMotorDriver` and exposes `CAP_MOTOR_CONTROL` payloads.

Passes:

- Device does not register capabilities.
- Device does not write Registry.
- Device does not publish events.
- Device does not run Logic.
- Device does not expose API.
- Device does not know module names.
- Device does not include real motor hardware, PWM, or H-bridge control.

The Device encodes motor state compactly:

```text
value_float = speed_percent
value_int = MotorDirection
unit = "percent"
```

Unavailable/error payloads use compact state with `ERR_DEVICE_TIMEOUT`.

### Module Layer

`SimMotorModule` is metadata-only.

Passes:

- exposes module ID, module type, metadata level, display name, device ID, device type, and capability ID
- does not register itself
- does not write Registry
- does not publish events
- does not run Logic
- does not expose API
- does not call Device methods
- does not touch hardware

The friendly display name remains metadata only.

### PNP Discovery

PNP discovery supports the simulated motor module through `discoverSimulatedMotorModule()`.

Passes:

- reads module metadata only
- validates required fields are non-null and non-empty
- validates metadata level `1`
- validates `CAP_` prefix
- publishes `EVT_MODULE_DISCOVERED` only if EventBus is attached
- does not write Registry
- does not call Driver
- does not call Device behavior
- does not run Logic
- does not expose API

EventBus integration remains optional.

### PNP Registration

PNP registration now supports:

- `CAP_TEMPERATURE`
- `CAP_DISTANCE`
- `CAP_SERVO_POSITION`
- `CAP_MOTOR_CONTROL`

Passes:

- uses Registry public result APIs
- uses `RegistryWriteResult` indexes for module/device ownership
- does not write Registry arrays directly
- does not call Driver
- does not call Device behavior
- does not run Logic
- does not expose API
- preserves existing temperature, distance, and servo behavior

The motor capability is registered as:

```text
category = "actuators"
kind = "actuator"
data_type = PayloadValueType::FLOAT
access = "read_write"
unit = "percent"
```

### Registry Ownership

Registry continues to store facts and state only.

Passes:

- stores motor module record
- stores motor device record
- stores `CAP_MOTOR_CONTROL` capability record
- stores latest motor payload state
- stores latest motor command state through `CommandStateRecord`
- emits capability registration and value update events through existing Registry behavior
- does not execute motor commands
- does not validate motor direction or speed policy
- does not call Service, Device, Driver, Logic, API, or Runtime

### MotorService Ownership

`MotorService` owns motor command validation and policy.

Passes:

- validates Runtime state before normal command execution
- blocks commands when Runtime is missing
- allows commands only in `READY` and `RUNNING`
- blocks commands in `BOOTING`, `INITIALIZING`, `DISCOVERING`, `REGISTERING`, `STARTING`, `ERROR_STATE`, and `SAFE_MODE`
- validates direction/speed before calling Device
- rejects `STOP` unless `speed_percent == 0.0F`
- rejects speed below `0.0F` or above `100.0F`
- calls `SimMotorDevice` only after validation/gating succeeds
- updates Registry through `updateCapabilityPayloadWithResult()`
- writes latest command state to Registry
- does not run Logic
- does not expose API transport
- does not publish events directly
- does not know module names
- does not include real motor hardware, PWM, or H-bridge control

### API Command Path

The internal API exposes motor state and command request support without external transport.

Passes:

- `getMotorControlState()` reads Registry state using `getCapabilityPayloadWithResult(CAP_MOTOR_CONTROL)`
- `commandMotorControl()` calls `MotorService::setMotor()`
- `commandMotorStop()` calls `MotorService::stop()`
- `getMotorCommandState()` reads Registry command state for `CAP_MOTOR_CONTROL`
- API does not call `SimMotorDevice`
- API does not call `SimMotorDriver`
- API does not call HAL
- API does not own canonical state
- no JSON
- no HTTP
- no WiFi/WebServer

The command path satisfies the current internal rule:

```text
API command -> MotorService -> SimMotorDevice -> SimMotorDriver
```

### Runtime Task Integration

Runtime remains scheduler-only for motor state updates.

Passes:

- Runtime task `task.motor_service.update_state` calls `MotorService::updateState()`
- Runtime does not inspect `CAP_MOTOR_CONTROL`
- Runtime does not inspect motor direction
- Runtime does not inspect speed
- Runtime does not validate motor safety policy
- Runtime does not call Device or Driver directly

### SAFE_MODE Blocking

Implemented `MotorService` gating blocks normal motor commands in `SAFE_MODE`.

Passes:

- `SAFE_MODE` command attempts fail
- Device is not called for blocked commands
- Registry payload remains unchanged after blocked commands
- command state is written as failed
- validation verifies blocked `SAFE_MODE` motor command behavior

### Validation Harness

The validation harness now covers:

- simulated motor Driver initialization
- simulated motor Device initialization
- motor module PNP discovery
- motor module/device/capability registration
- Registry counts of 4 modules, 4 devices, and 4 capabilities
- Runtime-driven motor state update
- API motor state read
- initial motor state `STOP / 0.0F`
- valid `FORWARD / 50.0F` command
- command-state read after successful command
- stop command
- invalid `FORWARD / 999.0F` command
- payload preservation after invalid command
- `SAFE_MODE` command blocking

The validation harness does not modify `src/main.cpp`.

### Memory And Allocation Rules

The implemented motor slice follows the current ESP32 v1 memory policy.

Passes:

- fixed objects
- pointer references only
- no heap allocation
- no Arduino `String`
- no STL containers
- compact payload structures
- compact command request/result structures
- latest command state only, no command history

## Warnings

### Command Execution Is Not Yet Runtime-Queued

Motor state update is Runtime-scheduled, but motor commands currently execute synchronously through:

```text
API -> MotorService -> SimMotorDevice -> SimMotorDriver
```

This preserves the required Service-owned policy boundary and avoids API-to-Device bypass, but it is not the final bounded command coordination model for real hardware.

Before real motor hardware, motor commands should be coordinated through a bounded Runtime command task or equivalent bounded execution slot.

### SAFE_MODE Blocks Stop In Current Implementation

The current implementation blocks `commandMotorStop()` in `SAFE_MODE` because `MotorService::stop()` uses the same Runtime gating as normal motion commands.

This matches the Milestone 4 Phase 9 validation requirement that `SAFE_MODE` blocks motor commands, but it differs from the motor architecture plan's future safety target where emergency stop should remain available in `SAFE_MODE`.

Before real hardware, `SAFE_MODE` must distinguish:

```text
blocked: forward, reverse
allowed: emergency stop if provider is available
```

### Timeout Field Is Not Enforced

`MotorCommandRequest` and `ApiMotorCommandRequest` include `timeout_ms`, but timeout bounds and deadline enforcement are not implemented.

This is acceptable for the synchronous simulated slice, but must be resolved before real motor hardware or external transport.

### Motor Capability Record Is Less Specific Than The Plan

The implementation registers motor as:

```text
kind = "actuator"
data_type = PayloadValueType::FLOAT
```

The motor architecture plan expected:

```text
kind = "motor"
data_type = PayloadValueType::OBJECT
```

The compact `FLOAT + value_int` representation is consistent with the current `CapabilityPayload` storage model and ESP32 memory constraints, but docs and registration should be reconciled before more actuator types are added.

### Error Codes Are Coarse

Invalid motor commands and Runtime-blocked commands currently use `ERR_CAPABILITY_UNAVAILABLE`.

This is compact and uses existing canonical IDs, but it does not distinguish:

- invalid speed
- invalid direction
- Runtime not ready
- actuator blocked in safe mode
- command timeout

More precise canonical error IDs are needed before external API transport or real hardware.

### Direct Validation Path Still Calls Service Updates Directly

`runOnce()` calls `MotorService::updateState()` directly, while `runOnceWithRuntime()` uses the Runtime task.

This is intentional because the direct validation path is retained for comparison and compatibility, but the Runtime-controlled path should be considered the architecture validation path.

## Architecture Violations

No hard architecture violations were found in the implemented simulated motor slice.

Specifically:

- API commands go through `MotorService`, not Device or Driver.
- `MotorService` owns direction and speed validation.
- Runtime remains scheduler-only.
- Registry stores state only.
- command state is stored in Registry and readable through API.
- PNP discovery does not write Registry.
- PNP registration uses Registry public result APIs.
- no real hardware/PWM/H-bridge code is present.
- no dynamic allocation, Arduino `String`, or STL containers were introduced in the motor slice.

The warnings above are readiness gaps, not current hard violations.

## Safety Gaps Before Real Hardware

The simulated motor slice is not ready for real motor hardware.

Required before real motor hardware:

1. HAL motor/PWM abstraction with default disabled outputs.
2. Driver-level output disable behavior.
3. Boot-time guarantee that motor outputs remain stopped.
4. `SAFE_MODE` emergency stop support.
5. Distinction between blocked motion and allowed stop.
6. Runtime-queued bounded command execution.
7. Timeout validation and enforcement.
8. Watchdog-triggered stop/disable policy.
9. Brownout recovery stop/disable policy.
10. Driver failure-mode validation.
11. Module-loss validation.
12. Capability-unavailable validation.
13. Runtime overload policy for motor commands.
14. More precise actuator command error codes.
15. Validation that invalid direction never reaches Device.
16. Validation that invalid speed never reaches Device.
17. Validation that unavailable motor state is never treated as stopped.
18. Real hardware tests only with motor power isolated and load removed.

## Recommended Fixes

1. Add a bounded motor command coordination plan.

Target:

```text
API or Logic
-> MotorService request/accept
-> Runtime command task
-> MotorService execution
-> Device
-> Driver
-> Registry payload update
-> Registry command-state update
```

2. Split motor Runtime gating into normal motion and emergency stop policy.

Target:

```text
READY/RUNNING: forward, reverse, stop
SAFE_MODE: stop only
other states: blocked unless explicitly documented recovery action
```

3. Standardize motor capability metadata.

Decide whether the v1 compact representation should remain:

```text
data_type = PayloadValueType::FLOAT
value_float = speed_percent
value_int = direction
```

or migrate docs/code toward:

```text
data_type = PayloadValueType::OBJECT
```

For ESP32 v1, the compact representation is likely the better implementation choice, but the schema documents should say so explicitly.

4. Add precise command error IDs.

Recommended future IDs:

```text
ERR_COMMAND_INVALID
ERR_RUNTIME_NOT_READY
ERR_ACTUATOR_BLOCKED
ERR_COMMAND_TIMEOUT
ERR_ACTUATOR_UNAVAILABLE
```

5. Add validation for Driver failure mode.

Validation should prove:

- failed Driver command writes failed command state
- Registry payload remains safe or unavailable
- API response is compact and machine-readable
- previous valid payload is not replaced with an unsafe command value

6. Add validation for `REVERSE`.

The current motor validation covers `FORWARD`, stop, invalid speed, and `SAFE_MODE` blocking.

Add `REVERSE` validation before real hardware.

## Next Steps

Recommended next milestone:

```text
Milestone 4.1 - Bounded Motor Command Safety
```

Suggested phases:

1. Document motor command coordination and emergency stop behavior.
2. Add precise motor command error codes.
3. Add Runtime command task or bounded pending-command slot.
4. Allow emergency stop in `SAFE_MODE` while blocking forward/reverse.
5. Enforce timeout bounds.
6. Add Driver failure-mode validation.
7. Add reverse command validation.
8. Reconcile motor payload schema docs with compact implementation.
9. Audit again before any HAL/PWM/H-bridge work.

Do not implement real motor hardware until these gaps are closed.
