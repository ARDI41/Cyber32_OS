# Safe Mode Architecture Plan

## Goal

Design Safe Mode for Cyber32 before any real actuator hardware is introduced.

This document is planning only.

No source code is modified by this document.

## Inputs Reviewed

- `RuntimeState`
- `ServoService`
- `CAP_SERVO_POSITION`
- `MILESTONE_3_ARCHITECTURE_AUDIT.md`
- `MILESTONE_3_2_RUNTIME_GATING_PLAN.md`
- `ACTUATOR_SAFETY_POLICY.md`

## Scope

Safe Mode is a Runtime-controlled system state that prevents unsafe actuator behavior.

Included:

- `SAFE_MODE` state
- entry conditions
- exit conditions
- actuator behavior in Safe Mode
- Registry behavior
- Runtime responsibilities
- Service responsibilities
- API visibility
- validation requirements
- future motor implications

Excluded:

- real servo hardware
- real PWM
- motor implementation
- WiFi
- WebServer
- Dashboard implementation
- OTA
- cloud
- Mobile Studio

## 1. SAFE_MODE State

Current `RuntimeState`:

```text
BOOTING
INITIALIZING
DISCOVERING
REGISTERING
STARTING
READY
RUNNING
ERROR_STATE
```

Required future state:

```text
SAFE_MODE
```

Recommended placement:

```text
BOOTING
INITIALIZING
DISCOVERING
REGISTERING
STARTING
READY
RUNNING
SAFE_MODE
ERROR_STATE
```

Meaning:

```text
Runtime is active, but unsafe actuator commands are blocked.
Only explicitly allowed safety actions may be accepted.
```

Safe Mode differs from `ERROR_STATE`.

`SAFE_MODE` means:

- system can still run bounded tasks
- API can report status
- Registry remains readable
- Services may perform safe actions
- unsafe commands are rejected

`ERROR_STATE` means:

- system has entered an error condition
- normal operation is not available
- recovery may require restart or manual intervention

## 2. SAFE_MODE Entry Conditions

Runtime owns Safe Mode state transitions.

Safe Mode may be entered when:

- repeated watchdog reset is detected
- actuator command failure repeats
- driver failure affects actuator capability
- module metadata is invalid for an actuator
- actuator capability becomes unavailable during operation
- Runtime overload persists
- configuration is invalid
- brownout recovery requires conservative behavior
- API or system safety command requests safe mode
- validation harness explicitly sets Safe Mode for tests

Initial v1 simulated entry conditions:

```text
manual validation transition
driver failure validation transition
runtime overload placeholder
```

Future real hardware entry conditions:

```text
watchdog boot loop
brownout recovery
motor driver fault
servo driver fault
power output fault
unsafe configuration
```

Safe Mode entry must:

1. set Runtime state to `SAFE_MODE`
2. block unsafe actuator commands
3. preserve Registry readability
4. store or expose latest compact safety/error state
5. avoid dynamic allocation
6. avoid blocking loops

## 3. SAFE_MODE Exit Conditions

Safe Mode exit must be explicit and validated.

Allowed exits:

```text
SAFE_MODE -> READY
SAFE_MODE -> RUNNING
SAFE_MODE -> ERROR_STATE
```

Exit to `READY` or `RUNNING` requires:

- Runtime confirms no active fatal error
- Services report safe readiness
- actuator capabilities are available or intentionally disabled
- command slots are empty or finalized
- watchdog/brownout recovery checks have passed
- configuration is valid
- Registry has current state

Exit to `ERROR_STATE` occurs when:

- fault cannot be cleared
- repeated failure persists
- required actuator safe action cannot be confirmed
- Registry cannot store required safety state

Safe Mode must not exit automatically after safety-critical actuator faults unless the recovery rule explicitly allows it.

For real hardware, manual recovery may be required after:

- repeated watchdog reset
- brownout during actuator operation
- motor fault
- actuator metadata mismatch
- invalid safety configuration

## 4. Actuator Behavior In SAFE_MODE

### Global Rules

In Safe Mode:

- unsafe actuator commands are rejected
- API cannot bypass Services
- Logic cannot bypass Services
- Dashboard cannot bypass API
- Registry stores state only
- Runtime coordinates state only
- Services own command policy
- Devices execute only validated safe actions

### CAP_SERVO_POSITION

Current implemented command:

```text
set_position
```

Safe Mode behavior:

```text
set_position is blocked
```

Future safe commands may be added only after documentation:

```text
hold
disable
move_to_safe_position
```

Allowed in Safe Mode after future documentation:

- hold current position if safe
- disable output if supported
- move to configured safe position only if that configuration is validated

Blocked in Safe Mode:

- arbitrary `set_position`
- sweep
- auto-center
- any command based on display name or module name

Blocked command result:

```text
CommandState::FAILED
accepted = false
executed = false
error_code = ERR_CAPABILITY_UNAVAILABLE until a precise error exists
payload remains previous safe/valid value
Device is not called
```

Recommended future error:

```text
ERR_ACTUATOR_BLOCKED
```

or:

```text
ERR_RUNTIME_SAFE_MODE
```

### CAP_MOTOR_CONTROL Future

Safe Mode behavior for future motor support must be stricter than servo.

Allowed:

```text
stop
brake
disable_output
```

Blocked:

```text
set_speed
forward
reverse
turn
steering motion
```

Motor emergency commands should remain available even in Safe Mode if the provider can safely accept them.

## 5. Registry Behavior

Registry stores facts and state only.

Registry may store:

- Runtime state summary if a runtime-state record is later added
- capability availability
- latest capability payload
- latest command state
- latest compact error code
- safe mode reason code when schema exists

Registry must not:

- enter Safe Mode
- exit Safe Mode
- execute safe actions
- validate actuator policy
- call Services
- call Devices
- call Drivers

For `CAP_SERVO_POSITION`, Registry should preserve:

- latest valid payload
- latest failed command state for blocked Safe Mode command
- compact error code

Blocked command state example:

```text
capability_id = CAP_SERVO_POSITION
command_state = FAILED
timestamp_ms = now_ms
registry_result = OK
error_code = ERR_CAPABILITY_UNAVAILABLE
value_float = requested_position
value_int = 0
```

## 6. Runtime Responsibilities

Runtime owns Safe Mode transition coordination.

Runtime may:

- set state to `SAFE_MODE`
- expose current Runtime state
- continue bounded event processing
- continue bounded task scheduling where safe
- coordinate watchdog/brownout recovery
- prevent unsafe normal operation from resuming automatically

Runtime must not:

- validate servo position ranges
- decide servo command policy
- call servo Device directly
- call servo Driver directly
- store command history
- replace Registry
- expose API

Runtime should continue to process:

- safety state updates
- Registry/event announcements
- API status visibility if available
- safe command tasks if documented

Runtime may pause or deprioritize:

- nonessential telemetry
- nonessential display/audio work
- normal actuator command tasks

## 7. Service Responsibilities

Services own Safe Mode command policy.

`ServoService` must:

- read Runtime state
- reject `set_position` when Runtime is `SAFE_MODE`
- reject commands before `READY`
- reject commands in `ERROR_STATE`
- update command state in Registry
- leave payload unchanged for blocked commands
- avoid calling Device for blocked commands

Future `MotorService` must:

- preserve stop/brake/disable actions
- reject motion commands
- fail safe on provider errors
- update Registry command state

Services must not:

- set Runtime state except through a documented Runtime API if later approved
- expose external API
- run Logic
- allocate memory dynamically
- store long command history

## 8. API Visibility

API must expose Safe Mode state through machine-readable status.

Existing internal API can already expose:

```text
runtime_state
```

When `SAFE_MODE` is added, `GET /system/status` or internal equivalent should report:

```text
runtime_state = SAFE_MODE
ok = true or degraded depending final API policy
latest_error_code = safe mode reason if available
```

Command API behavior in Safe Mode:

```text
command request -> API -> Service -> rejected by Service
```

API response:

```text
ok = false
command_state = FAILED
accepted = false
executed = false
error_code = ERR_CAPABILITY_UNAVAILABLE or future safe-mode error
```

API command-state read:

```text
getServoCommandState()
```

should expose the latest blocked command result.

API must not:

- bypass Services
- call Device
- call Driver
- decide actuator safety policy
- use module names as command targets

## 9. Validation Requirements

Validation must run without real hardware and without modifying `src/main.cpp`.

Required validation:

1. Add `RuntimeState::SAFE_MODE`.
2. Runtime can enter `SAFE_MODE`.
3. API system status exposes `SAFE_MODE`.
4. `CAP_SERVO_POSITION set_position` is rejected in `SAFE_MODE`.
5. Rejected command has `CommandState::FAILED`.
6. Rejected command has `accepted == false`.
7. Rejected command has `executed == false`.
8. Rejected command has compact non-`none` error code.
9. Device is not called for Safe Mode blocked command.
10. Servo payload remains previous valid value.
11. Registry command state stores failed command.
12. API command-state read exposes failed command.
13. Runtime does not know `CAP_SERVO_POSITION`.
14. ServoService owns policy.
15. Existing blocked Runtime-state tests still pass.
16. Existing `READY` and `RUNNING` command tests still pass.
17. Temperature and distance validation still pass.

Suggested validation sequence:

```text
Runtime READY
command CAP_SERVO_POSITION to 45.0 -> success
verify payload 45.0
Runtime SAFE_MODE
command CAP_SERVO_POSITION to 90.0 -> rejected
verify command state FAILED
verify payload still 45.0
Runtime RUNNING
command CAP_SERVO_POSITION to 90.0 -> success
verify payload 90.0
```

## 10. Future Motor Implications

`CAP_MOTOR_CONTROL` is more safety-critical than `CAP_SERVO_POSITION`.

Safe Mode design must support:

- emergency stop
- brake
- disable output
- reject normal movement
- preserve emergency path under Runtime overload

Motor implementation must not begin until Safe Mode supports:

- explicit `SAFE_MODE` Runtime state
- Service-level command filtering
- command priority policy
- stop/brake/disable action policy
- failure-mode validation
- watchdog recovery behavior

Future motor Safe Mode examples:

```text
Runtime SAFE_MODE
CAP_MOTOR_CONTROL set_speed 50 -> rejected
CAP_MOTOR_CONTROL stop -> accepted if provider available
CAP_MOTOR_CONTROL brake -> accepted if provider available
```

Motor support must also define what happens if provider is unavailable:

```text
Registry marks capability unavailable
Service rejects commands
Runtime remains or enters SAFE_MODE
API exposes machine-readable error
```

## Stop Conditions Before Real Hardware

Stop before real actuator hardware if:

- `SAFE_MODE` is not represented in Runtime state.
- ServoService does not block `set_position` in Safe Mode.
- blocked Safe Mode commands reach Device.
- blocked Safe Mode commands change payload state.
- Registry cannot store blocked command state.
- API cannot expose Safe Mode status.
- API bypasses Service.
- Runtime owns servo policy.
- EventBus stores state instead of announcing changes.
- dynamic allocation is introduced.
- Arduino `String` is introduced.
- STL containers are introduced.
- real `Servo.h` or PWM appears before Safe Mode validation passes.
- motor implementation starts before Safe Mode policy is validated.

## Success Criteria

Safe Mode architecture is ready for implementation when:

- Runtime has a clear `SAFE_MODE` state.
- Runtime owns Safe Mode transition coordination.
- Services own Safe Mode command policy.
- Registry stores Safe Mode-related state only.
- API exposes Runtime state and command outcomes.
- `CAP_SERVO_POSITION set_position` is blocked in Safe Mode.
- future motor behavior is constrained before implementation.
- validation can prove blocked commands do not reach Device.
- no real hardware has been added.
