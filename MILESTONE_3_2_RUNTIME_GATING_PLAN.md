# Milestone 3.2 Runtime Gating Plan

## Goal

Design how actuator commands are blocked unless Runtime is in an allowed state.

This document is planning only.

No source code is modified by this document.

## Inputs Reviewed

- `RuntimeState`
- `ServoService`
- `CAP_SERVO_POSITION`
- `ACTUATOR_SAFETY_POLICY.md`
- `MILESTONE_3_ARCHITECTURE_AUDIT.md`
- `MILESTONE_3_1_IMPLEMENTATION_PLAN.md`

## Scope

Milestone 3.2 defines Runtime-state gating for actuator commands before real hardware exists.

Included:

- allowed Runtime states
- blocked Runtime states
- ServoService gating responsibility
- safe Runtime-state access pattern
- command failure behavior when Runtime is not ready
- error code mapping
- validation checks
- stop conditions before real hardware

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

## Architecture Rule

Runtime provides system execution state.

ServoService owns actuator command policy.

API must route commands through ServoService.

Runtime must not:

- inspect servo position values
- validate servo command ranges
- decide servo command policy
- call Device
- call Driver
- own command state

ServoService must not:

- expose external API
- run Logic
- bypass Registry state updates
- allocate memory dynamically

## Current RuntimeState

Current enum:

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

Current gap:

```text
SAFE_MODE
DEGRADED
RECOVERY
```

These states are required before real actuator hardware, but they are not required to design the first gating step.

## Allowed Runtime States

Normal `CAP_SERVO_POSITION` commands are allowed only when Runtime is:

```text
READY
RUNNING
```

Meaning:

- Registry has completed registration.
- Services are initialized.
- Runtime is no longer booting.
- Normal actuator policy may be evaluated by ServoService.

Allowed command:

```text
set_position
```

Only after ServoService also validates:

- position range
- timeout range
- capability availability
- pending command slot availability
- safe mode not active
- Device attached

## Blocked Runtime States

Normal `CAP_SERVO_POSITION` commands must be blocked when Runtime is:

```text
BOOTING
INITIALIZING
DISCOVERING
REGISTERING
STARTING
ERROR_STATE
```

Rationale:

- Boot and initialization states cannot guarantee actuator safety.
- Discovery and registration states cannot guarantee provider correctness.
- Starting state may not have all Services ready.
- Error state must fail safe.

Future blocked states:

```text
DEGRADED
SAFE_MODE
RECOVERY
```

Future exception:

```text
SAFE_MODE may allow hold, disable, or configured safe position only.
```

The current first servo command is only `set_position`, so it must be blocked in safe mode.

## ServoService Gating Responsibility

ServoService owns the gating decision.

ServoService should validate Runtime state before accepting or executing a command.

Recommended validation order:

1. Registry pointer attached.
2. Device pointer attached.
3. Runtime state available.
4. Runtime state is `READY` or `RUNNING`.
5. Safe mode flag is not active.
6. Command position is within `0.0F` to `180.0F`.
7. Timeout is valid.
8. Pending command slot is free.
9. Capability state is available if required by the current Service contract.
10. Command is accepted or rejected.

ServoService must reject blocked-state commands before calling Device.

## Safe Runtime-State Access

Preferred implementation:

```text
ServoService::attachRuntime(Runtime* runtime)
```

or:

```text
ServoService::begin(Registry* registry, SimServoDevice* device, Runtime* runtime)
```

Preferred for compatibility:

```text
attachRuntime(Runtime* runtime)
```

Reason:

- preserves existing `begin()` callers
- keeps Runtime optional during staged migration
- avoids broad validation harness churn

ServoService may read:

```text
runtime->state()
```

ServoService must not:

- call `runtime->update()`
- register Runtime tasks itself
- change Runtime state
- store Runtime facts in Registry
- make Runtime own servo policy

Alternative minimal option:

```text
ServoService::setRuntimeState(RuntimeState state)
```

This is simpler for tests but weaker architecturally because the state can become stale. Prefer attaching Runtime if available.

## Command Failure Behavior

When Runtime is not ready:

```text
state = CommandState::FAILED
accepted = false
executed = false
Device is not called
Registry command state is updated
Servo payload remains previous safe/valid state
```

Command-state record:

```text
capability_id = CAP_SERVO_POSITION
command_state = FAILED
timestamp_ms = now_ms
registry_result = RegistryResult::OK if command-state write succeeds
error_code = runtime-not-ready error mapping
value_float = requested position
value_int = 0
```

API response:

```text
ok = false
command_state = FAILED
accepted = false
executed = false
error_code = runtime-not-ready error mapping
```

Payload state:

```text
CAP_SERVO_POSITION latest payload is unchanged
```

Device safety rule:

```text
Device must not be called for blocked Runtime states.
```

## Error Code Mapping

Current available compact error IDs:

```text
ERR_DEVICE_TIMEOUT
ERR_REGISTRY_FULL
ERR_CAPABILITY_UNAVAILABLE
```

For the first gating implementation, use:

```text
ERR_CAPABILITY_UNAVAILABLE
```

Meaning:

```text
actuator command is unavailable because Runtime state blocks it
```

Recommended future canonical error IDs:

```text
ERR_RUNTIME_NOT_READY
ERR_ACTUATOR_BLOCKED
ERR_COMMAND_INVALID
ERR_RUNTIME_OVERLOAD
```

Do not add new error IDs until the error catalog phase explicitly approves them.

## Safe Mode Blocking

Safe mode is required by policy but not currently represented by `RuntimeState`.

Plan:

1. Runtime-state gating handles current states now.
2. Add `SAFE_MODE` to `RuntimeState` before real hardware.
3. ServoService blocks arbitrary `set_position` when Runtime is `SAFE_MODE`.
4. Future safe commands may be introduced only after documentation:

```text
hold
disable
move_to_safe_position
```

Until those commands exist, safe mode blocks all servo position commands.

## Validation Checks

Validation must use public interfaces and must not modify `src/main.cpp`.

Required checks:

1. Runtime `READY` allows valid `CAP_SERVO_POSITION` command.
2. Runtime `RUNNING` allows valid `CAP_SERVO_POSITION` command.
3. Runtime `BOOTING` rejects valid position command.
4. Runtime `INITIALIZING` rejects valid position command.
5. Runtime `DISCOVERING` rejects valid position command.
6. Runtime `REGISTERING` rejects valid position command.
7. Runtime `STARTING` rejects valid position command.
8. Runtime `ERROR_STATE` rejects valid position command.
9. Rejected command returns `CommandState::FAILED`.
10. Rejected command has `accepted == false`.
11. Rejected command has `executed == false`.
12. Rejected command exposes compact error code.
13. Rejected command writes latest command state to Registry.
14. API `getServoCommandState()` exposes the failed command state.
15. Servo payload remains previous valid value after blocked command.
16. Device is not called for blocked command.
17. API still calls Service, not Device or Driver.
18. Runtime does not know `CAP_SERVO_POSITION`.
19. Existing temperature and distance validation still pass.

Suggested direct validation sequence:

```text
set Runtime READY
command 45.0 -> success
verify payload 45.0
set Runtime STARTING
command 90.0 -> failed
verify command state failed
verify payload still 45.0
set Runtime RUNNING
command 90.0 -> success
verify payload 90.0
```

## Implementation Notes For Future Phase

Recommended Service helper:

```text
bool runtimeAllowsActuatorCommands() const
```

Allowed:

```text
RuntimeState::READY
RuntimeState::RUNNING
```

Blocked:

```text
all other states
```

If Runtime is not attached:

```text
block commands by default
```

Reason:

```text
default-safe actuator policy
```

This changes current simulated behavior and should be introduced in its own implementation phase with validation updates.

## Stop Conditions Before Real Hardware

Stop before real servo hardware if:

- ServoService accepts commands without Runtime state access.
- Runtime state is missing or stale.
- missing Runtime attachment allows commands by default.
- commands are accepted in `BOOTING`.
- commands are accepted in `INITIALIZING`.
- commands are accepted in `DISCOVERING`.
- commands are accepted in `REGISTERING`.
- commands are accepted in `STARTING`.
- commands are accepted in `ERROR_STATE`.
- safe mode does not exist or cannot block arbitrary movement.
- blocked command reaches Device.
- blocked command changes servo payload.
- API bypasses ServoService.
- Runtime owns servo policy.
- Registry executes commands.
- real `Servo.h` or PWM is added before gating validation passes.
- dynamic allocation is introduced.
- Arduino `String` is introduced.
- STL containers are introduced.

## Success Criteria

Milestone 3.2 design is satisfied when:

- `READY` and `RUNNING` are the only allowed normal actuator command states.
- ServoService owns the Runtime gating decision.
- Runtime provides state but does not own servo policy.
- API still routes commands through ServoService.
- blocked commands fail compactly.
- blocked commands do not call Device.
- blocked commands do not change servo payload state.
- command-state API exposes blocked command results.
- no real hardware is introduced.
