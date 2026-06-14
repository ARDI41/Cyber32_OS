# Milestone 3.4 Safe Mode Management Plan

## Goal

Design explicit Safe Mode management before implementation.

This document is planning only.

No source code is modified by this document.

## Inputs Reviewed

- `Runtime`
- `RuntimeState`
- `SAFE_MODE_ARCHITECTURE_PLAN.md`
- Milestone 3.3 implementation state
- `ServoService` Runtime gating

## Current State

`RuntimeState` now includes:

```text
SAFE_MODE
```

`ServoService` currently blocks normal `CAP_SERVO_POSITION setPosition` commands when Runtime state is `SAFE_MODE`.

Runtime currently exposes:

```text
setState(RuntimeState state)
state() const
```

Milestone 3.4 should add explicit Safe Mode management without making Runtime own actuator policy.

## 1. Runtime enterSafeMode() Behavior

Recommended Runtime API:

```text
bool enterSafeMode(const char* reason_code)
```

Minimal first implementation may be:

```text
bool enterSafeMode()
```

Behavior:

1. Set Runtime state to `RuntimeState::SAFE_MODE`.
2. Preserve bounded Runtime operation.
3. Preserve EventBus processing.
4. Preserve Registry readability.
5. Record compact reason later when a runtime-state/error schema exists.
6. Return `true` if Runtime entered or was already in `SAFE_MODE`.

Allowed callers:

- validation harness
- future watchdog/recovery coordinator
- future system safety service
- future API/system command only through a documented Service or Runtime management path

Runtime must not:

- decide servo command policy
- inspect actuator command payloads
- call actuator Devices
- call actuator Drivers
- execute safe actions directly
- allocate memory

Idempotency:

```text
enterSafeMode() while already in SAFE_MODE returns true and leaves state unchanged.
```

## 2. Runtime exitSafeMode() Behavior

Recommended Runtime API:

```text
bool exitSafeMode(RuntimeState target_state)
```

Allowed target states:

```text
READY
RUNNING
ERROR_STATE
```

Behavior:

1. Verify current state is `SAFE_MODE`.
2. Verify target state is allowed.
3. Transition to target state only if checks pass.
4. Return `true` on transition.
5. Return `false` for blocked transitions.

Initial minimal implementation may only validate allowed targets.

Future implementation should require:

- no active fatal error
- command slots finalized
- actuator Services report safe readiness
- Registry state available
- watchdog/brownout recovery checks complete
- configuration valid

Runtime must not ask actuator-specific questions such as:

```text
is servo angle safe?
is motor speed safe?
```

Those remain Service responsibilities.

## 3. Runtime isSafeMode() Behavior

Recommended Runtime API:

```text
bool isSafeMode() const
```

Behavior:

```text
return state_ == RuntimeState::SAFE_MODE
```

Purpose:

- lightweight API visibility
- Service gating readability
- validation clarity

`isSafeMode()` must not:

- change state
- emit events
- query Registry
- inspect Services

## 4. Allowed Exit Targets

Allowed:

```text
SAFE_MODE -> READY
SAFE_MODE -> RUNNING
SAFE_MODE -> ERROR_STATE
```

`READY`:

Use when the system is safe but normal repeated Runtime loop operation has not resumed yet.

`RUNNING`:

Use when the system has passed recovery validation and normal operation may resume.

`ERROR_STATE`:

Use when Safe Mode cannot recover or required safe-state confirmation fails.

## 5. Blocked Exit Cases

Blocked target states:

```text
BOOTING
INITIALIZING
DISCOVERING
REGISTERING
STARTING
SAFE_MODE as an exit target
```

Blocked when:

- current state is not `SAFE_MODE`
- target state is not allowed
- fatal error is active in future error model
- watchdog boot loop is active
- configuration invalid
- actuator safe readiness checks fail
- Registry cannot store required state

Initial implementation may block only invalid target states and non-Safe-Mode current state.

Future implementation must add recovery checks before real hardware.

## 6. Registry And EventBus Implications

### Registry

Registry stores state only.

Future Registry may store:

- latest Runtime state
- safe mode reason code
- safe mode entry timestamp
- safe mode exit timestamp
- latest safety error code

Registry must not:

- enter Safe Mode
- exit Safe Mode
- decide recovery
- call Services
- call Devices
- execute actuator commands

No Registry storage is required for the first explicit Runtime API phase unless a runtime-state record is separately planned.

### EventBus

EventBus announces transitions only.

Future events:

```text
EVT_RUNTIME_SAFE_MODE_ENTERED
EVT_RUNTIME_SAFE_MODE_EXITED
EVT_RUNTIME_STATE_CHANGED
```

Initial implementation may reuse existing runtime/error events only if already available.

Event payloads must be compact:

- event ID
- timestamp
- source layer
- target state or compact value
- optional compact reason code

EventBus must not store Safe Mode state.

## 7. API Visibility Later

Existing API system status exposes:

```text
runtime_state
```

After explicit Safe Mode management exists, API should expose:

```text
runtime_state = SAFE_MODE
```

Later API additions may expose:

```text
safe_mode_active
safe_mode_reason_code
safe_mode_since_ms
```

API rules:

- API may read Runtime state.
- API may read Registry safety state if added.
- API must not decide actuator policy.
- API must not call Devices.
- API must not call Drivers.
- API must not bypass Services for actuator commands.

Safe Mode management commands through API are future work and must be separately documented.

## 8. Validation Checks

Validation must not modify `src/main.cpp`.

Required checks for explicit Safe Mode management:

1. Runtime begins in `BOOTING`.
2. `enterSafeMode()` sets state to `SAFE_MODE`.
3. `isSafeMode()` returns true in `SAFE_MODE`.
4. Calling `enterSafeMode()` while already in `SAFE_MODE` succeeds and remains stable.
5. `exitSafeMode(READY)` succeeds.
6. `exitSafeMode(RUNNING)` succeeds.
7. `exitSafeMode(ERROR_STATE)` succeeds.
8. `exitSafeMode(BOOTING)` fails.
9. `exitSafeMode(INITIALIZING)` fails.
10. `exitSafeMode(DISCOVERING)` fails.
11. `exitSafeMode(REGISTERING)` fails.
12. `exitSafeMode(STARTING)` fails.
13. `exitSafeMode(SAFE_MODE)` fails as an exit target.
14. Servo commands remain blocked while Runtime is `SAFE_MODE`.
15. Servo commands work after valid exit to `RUNNING`.
16. Runtime does not inspect `CAP_SERVO_POSITION`.
17. Service still owns command validation.
18. Existing temperature and distance validation still pass.

Suggested sequence:

```text
runtime.enterSafeMode()
assert runtime.isSafeMode()
servo command 90.0 -> rejected
runtime.exitSafeMode(RUNNING)
servo command 90.0 -> accepted
runtime.enterSafeMode()
runtime.exitSafeMode(BOOTING) -> rejected
assert runtime.isSafeMode()
```

## 9. Stop Conditions Before Real Hardware

Stop before real actuator hardware if:

- Runtime has no explicit `enterSafeMode()`.
- Runtime has no explicit `exitSafeMode()`.
- Runtime has no explicit `isSafeMode()`.
- Safe Mode exit accepts boot/discovery/registration states.
- Safe Mode can be exited without validation after safety-critical faults.
- Runtime owns actuator policy.
- Services cannot block actuator commands in Safe Mode.
- API can bypass Services.
- Registry executes commands.
- EventBus stores state instead of announcing transitions.
- servo command reaches Device during Safe Mode.
- real `Servo.h` or PWM is introduced before Safe Mode management validation passes.
- motor implementation starts before Safe Mode management is validated.
- dynamic allocation is introduced.
- Arduino `String` is introduced.
- STL containers are introduced.

## Recommended Phase Order

### Phase 1 - Runtime Safe Mode API

Add:

```text
enterSafeMode()
exitSafeMode(RuntimeState target_state)
isSafeMode() const
```

No Registry or EventBus behavior yet.

### Phase 2 - Validation

Validate entry, exit, idempotency, blocked exits, and servo command blocking.

### Phase 3 - Runtime State Event Plan

Document and add compact Runtime state-change events if needed.

### Phase 4 - API Visibility

Expose Safe Mode state through internal API status.

### Phase 5 - Recovery Preconditions

Define Service readiness and error-state checks before real hardware.

## Success Criteria

Milestone 3.4 design is complete when:

- Runtime owns Safe Mode state transitions.
- Runtime does not own actuator policy.
- Services still own command validation.
- API visibility is planned without bypassing Services.
- Registry/EventBus roles remain state storage and announcement only.
- validation can prove Safe Mode entry and exit behavior.
- no real hardware is introduced.
