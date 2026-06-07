# Cyber32 Actuator Safety Policy

This document defines mandatory safety behavior for all Cyber32 actuator capabilities.

Cyber32 v1 targets ESP32 only.

This policy applies during:

- boot
- initialization
- normal operation
- degraded mode
- safe mode
- recovery mode
- watchdog reset
- brownout
- module loss
- capability unavailable state
- communication timeout
- runtime overload
- fatal errors

This policy follows:

- `ARCHITECTURE.md`
- `ERROR_MODEL.md`
- `CAPABILITY_PAYLOAD_SCHEMA.md`
- `COMMAND_DISPATCH_CONTRACT.md`
- `RUNTIME_IMPLEMENTATION_PLAN.md`
- `REGISTRY_IMPLEMENTATION_PLAN.md`
- `ESP32_V1_SCOPE.md`

## 1. Safety Principles

### Fail-Safe Philosophy

Safety-critical actuators must fail safe.

Fail-safe means that loss of control, loss of communication, invalid configuration, invalid metadata, runtime overload, or watchdog recovery must result in the safest practical output state.

For motion outputs, fail-safe means:

```text
stop motion
disable output when possible
reject unsafe commands
preserve emergency stop path
```

### Default-Safe Philosophy

Actuators must default to safe inactive state.

Default-safe applies:

- at power-on
- before HAL initialization
- before Driver initialization
- before Device initialization
- during PNP discovery
- during Registry registration
- before Runtime `READY`
- during safe mode
- after watchdog reset
- after brownout

Motion outputs must never activate before Runtime `READY`.

### Capability-First Safety Model

Safety policy is applied through capability IDs.

Correct:

```text
CAP_MOTOR_CONTROL is disabled until Runtime READY.
```

Incorrect:

```text
Left Motor Module is disabled until ready.
```

Logic, API, and Dashboard must request actuator behavior through capability IDs and must pass through Services.

## 2. Safety Ownership

### HAL

HAL responsibilities:

- provide safe low-level defaults when possible
- keep pins inactive until configured
- avoid unintended pulses during setup
- expose hardware errors upward

HAL must not:

- decide actuator policy
- run Logic
- accept API or Dashboard commands

### Drivers

Driver responsibilities:

- initialize hardware into safe state
- expose stop/disable behavior where supported
- enforce driver-level timeouts
- report driver failures

Drivers must not:

- decide user authorization
- bypass Services
- execute unvalidated API payloads

### Devices

Device responsibilities:

- perform validated actuator execution
- enforce device-level safety bounds
- reject unsafe values
- fail safe on execution errors
- map Driver/HAL failures into compact errors

Devices enforce execution safety.

Devices must not:

- accept Dashboard commands directly
- accept API commands directly
- bypass Service policy

### Services

Service responsibilities:

- own actuator command policy
- validate command payloads
- enforce safe mode restrictions
- enforce Runtime state restrictions
- decide whether command is allowed
- request Runtime coordination
- update Registry state after outcomes

Services own actuator policy.

### Runtime

Runtime responsibilities:

- coordinate safety transitions
- coordinate boot, degraded, safe, recovery, and error states
- schedule bounded command execution
- enforce overload behavior
- coordinate watchdog recovery behavior

Runtime coordinates safety transitions.

Runtime must not:

- own command policy
- replace Registry
- execute actuator commands directly as policy

### Registry

Registry responsibilities:

- store actuator capability availability
- store latest payload state
- store latest command state
- store latest error state

Registry stores state only.

Registry must never execute commands.

### Logic

Logic responsibilities:

- request commands through capability IDs
- react to capability state, stale state, and error state
- respect safety policy outcomes

Logic cannot bypass safety policy.

### API

API responsibilities:

- accept command requests by capability ID only
- authenticate/authorize if enabled
- forward validated-size requests to Services
- expose machine-readable command and error state

API cannot bypass safety policy.

API cannot call Devices directly.

### Dashboard

Dashboard responsibilities:

- display actuator state
- display safety state
- request commands through API only
- translate errors for humans

Dashboard cannot bypass safety policy.

Dashboard cannot call Devices directly.

## 3. Actuator Categories

### CAP_MOTOR_CONTROL

Category:

```text
safety-critical motion actuator
```

Default safe state:

```text
stopped / disabled / brake if supported
```

Safety priority:

```text
highest actuator priority
```

Allowed emergency action:

```text
stop
brake
disable_output
```

### CAP_SERVO_POSITION

Category:

```text
position actuator
```

Default safe state:

```text
no movement before READY
neutral position only if explicitly configured safe
detach/disable output if safer for the mechanism
```

Safety priority:

```text
high when attached to motion or load-bearing mechanism
normal when display-only or non-hazardous
```

Allowed emergency action:

```text
hold
disable_output
move_to_safe_position only if configured safe
```

### CAP_AUDIO_OUTPUT

Category:

```text
notification actuator
```

Default safe state:

```text
silent
```

Safety priority:

```text
low, except warning tones may be preserved
```

Allowed emergency action:

```text
stop_audio
play_warning_tone if policy allows
```

### CAP_DISPLAY_TEXT

Category:

```text
information actuator
```

Default safe state:

```text
blank or boot status if display is initialized
```

Safety priority:

```text
low, except safety messages may be preserved
```

Allowed emergency action:

```text
show_safe_mode
show_error
clear_display
```

### CAP_POWER_OUTPUT_ENABLE

Category:

```text
power actuator
```

Default safe state:

```text
off / disabled
```

Safety priority:

```text
critical if output can power actuators or external loads
high otherwise
```

Allowed emergency action:

```text
disable_output
keep_required_system_power only if explicitly configured
```

## 4. Boot Safety

Motion outputs must never activate before Runtime `READY`.

### Boot State Table

| Capability | Power-On | Before Init | Discovery | Registration | Before Runtime READY |
|---|---|---|---|---|---|
| `CAP_MOTOR_CONTROL` | stopped/off | stopped/off | blocked | blocked | blocked |
| `CAP_SERVO_POSITION` | disabled/no movement | disabled/no movement | blocked | blocked | blocked unless safe neutral explicitly configured |
| `CAP_AUDIO_OUTPUT` | silent | silent | silent | silent | silent or boot beep only if explicitly allowed |
| `CAP_DISPLAY_TEXT` | blank | blank | optional boot text | optional status text | status text allowed |
| `CAP_POWER_OUTPUT_ENABLE` | off | off | off | off | off unless required for system boot |

### CAP_MOTOR_CONTROL Boot Policy

Before Runtime `READY`:

- reject all motion commands
- allow only safe stop/disable internal actions
- keep outputs stopped/off
- report unavailable or blocked state through Registry/API if queried

### CAP_SERVO_POSITION Boot Policy

Before Runtime `READY`:

- reject position movement commands
- do not sweep or auto-center unless configured safe
- keep PWM disabled if that is safest
- if a neutral position is required, it must be documented as safe for the mechanism

### CAP_AUDIO_OUTPUT Boot Policy

Before Runtime `READY`:

- remain silent by default
- optional boot beep is allowed only when non-blocking and configured
- warning tone is allowed in safe mode if policy permits

### CAP_DISPLAY_TEXT Boot Policy

Before Runtime `READY`:

- display status text only if initialized safely
- display must not block boot or Runtime timing
- display failure must not block safe boot

### CAP_POWER_OUTPUT_ENABLE Boot Policy

Before Runtime `READY`:

- outputs default off
- outputs that power actuators remain disabled
- system-required power rails may remain enabled only if explicitly configured as required and safe

## 5. Safe Mode Behavior

Safe mode prevents unsafe actuator behavior.

### Safe Mode Table

| Capability | Allowed Actions | Blocked Actions | Emergency Actions | Recovery Requirement |
|---|---|---|---|---|
| `CAP_MOTOR_CONTROL` | stop, brake, disable output | set speed, forward, reverse, steering motion | stop/brake/disable | Runtime recovery and Service validation |
| `CAP_SERVO_POSITION` | hold, disable, configured safe position | arbitrary movement | disable or safe position | configured safe target and Device validation |
| `CAP_AUDIO_OUTPUT` | stop audio, warning tone if allowed | long/nonessential playback | stop audio | Service validation |
| `CAP_DISPLAY_TEXT` | show safe/error status, clear | nonessential UI updates if overloaded | show safe mode | display available |
| `CAP_POWER_OUTPUT_ENABLE` | disable output, keep required system rail | enable external load or actuator power | disable output | explicit Service validation |

Safe mode rules:

- unsafe motion commands are rejected
- API commands are restricted
- Logic actions are restricted
- Dashboard may request only allowed safe actions through API
- Registry stores blocked/failed command state
- Events announce safety-relevant changes

## 6. Watchdog Behavior

### First Watchdog Reset

On first watchdog reset:

- boot with actuator-safe defaults
- record `ERR_WATCHDOG_RESET`
- keep motion blocked until Runtime `READY`
- expose watchdog state through Registry/API
- enter `RECOVERY` or cautious boot behavior if configured

Actuator states:

| Capability | First Watchdog Reset State |
|---|---|
| `CAP_MOTOR_CONTROL` | stopped/off |
| `CAP_SERVO_POSITION` | disabled/no movement, or configured safe neutral only |
| `CAP_AUDIO_OUTPUT` | silent |
| `CAP_DISPLAY_TEXT` | boot/recovery text if available |
| `CAP_POWER_OUTPUT_ENABLE` | off unless system-required |

### Repeated Watchdog Reset

On repeated watchdog reset:

- raise `ERR_WATCHDOG_RESET`
- consider `ERR_WATCHDOG_BOOT_LOOP`
- enter `RECOVERY` or `SAFE_MODE`
- disable optional services
- block unsafe actuator commands

Actuator states:

| Capability | Repeated Watchdog Reset State |
|---|---|
| `CAP_MOTOR_CONTROL` | stopped/off, commands blocked except stop |
| `CAP_SERVO_POSITION` | disabled or safe configured state |
| `CAP_AUDIO_OUTPUT` | silent or warning tone if allowed |
| `CAP_DISPLAY_TEXT` | safe mode/recovery text if available |
| `CAP_POWER_OUTPUT_ENABLE` | off for external loads |

### Watchdog Boot Loop

On watchdog boot loop:

- raise `ERR_WATCHDOG_BOOT_LOOP`
- enter `SAFE_MODE`
- reject unsafe commands
- require recovery validation before normal operation

Actuator states:

| Capability | Watchdog Boot Loop State |
|---|---|
| `CAP_MOTOR_CONTROL` | stopped/off |
| `CAP_SERVO_POSITION` | disabled/no movement unless configured safe |
| `CAP_AUDIO_OUTPUT` | silent by default |
| `CAP_DISPLAY_TEXT` | safe mode text if available |
| `CAP_POWER_OUTPUT_ENABLE` | off except required system power |

## 7. Communication Failure

Communication failure includes:

- module disappears
- capability unavailable
- driver timeout
- I2C timeout
- service unavailable

### Module Disappears

Behavior:

- mark module unavailable
- mark related devices unavailable
- mark related capabilities unavailable
- stop or disable safety-critical actuator outputs
- reject commands targeting unavailable capabilities
- emit events

Events:

```text
EVT_MODULE_REMOVED
EVT_DEVICE_LOST
EVT_CAPABILITY_UNAVAILABLE
EVT_ERROR_RAISED
```

### Capability Unavailable

Behavior:

- Registry stores `available: false`
- command requests fail with `ERR_CAPABILITY_UNAVAILABLE`
- Logic must not execute unsafe fallback except through allowed safe capability commands

### Driver Timeout

Behavior:

- raise `ERR_DEVICE_TIMEOUT`
- Device fails safe
- Service updates Registry
- Runtime may enter degraded or safe mode depending on capability

### I2C Timeout

Behavior:

- do not block Runtime loop indefinitely
- raise timeout error
- mark affected module/device/capability unavailable if needed
- avoid repeated unsafe rescans during active operation

### Service Unavailable

Behavior:

- command requests handled by that Service are rejected
- affected capabilities are marked unavailable or degraded
- Runtime may enter `DEGRADED` or `SAFE_MODE`

## 8. Runtime Overload

Runtime overload means Runtime cannot process all scheduled work within timing budget.

Error:

```text
ERR_RUNTIME_OVERLOAD
```

Priority rules:

Preserved commands:

- `CAP_MOTOR_CONTROL` stop/brake/disable
- `CAP_POWER_OUTPUT_ENABLE` disable output
- safety-related `CAP_DISPLAY_TEXT` if cheap and available
- safety warning `CAP_AUDIO_OUTPUT` only if non-blocking

Dropped or rejected commands:

- nonessential display updates
- nonessential audio playback
- repeated servo position updates
- non-safety motor speed changes
- optional power enable commands

Runtime overload behavior:

- preserve safety events
- reduce telemetry
- reduce Dashboard updates
- reject low-priority actuator commands
- keep emergency stop path available
- enter `DEGRADED` or `SAFE_MODE` if overload persists

## 9. Error Mapping

| Error Code | Safety Action | Notes |
|---|---|---|
| `ERR_DEVICE_TIMEOUT` | mark unavailable, fail safe | Motion outputs stop/disable. |
| `ERR_CAPABILITY_UNAVAILABLE` | reject command, disable capability | Logic/API must not bypass. |
| `ERR_DRIVER_INIT_FAILED` | mark unavailable, fail safe | Actuator remains inactive. |
| `ERR_RUNTIME_OVERLOAD` | preserve emergency actions, reject low-priority commands | Enter degraded or safe mode if persistent. |
| `ERR_WATCHDOG_RESET` | safe defaults, recovery check | Repeated reset escalates. |
| `ERR_WATCHDOG_BOOT_LOOP` | enter safe mode | Unsafe actuator commands blocked. |
| `ERR_CONFIG_INVALID` | safe defaults or safe mode | Do not enable outputs from invalid config. |
| `ERR_METADATA_INVALID` | reject module capabilities | Do not expose actuator capability. |

## 10. Recovery Rules

Outputs may be re-enabled only after validation.

Recovery requirements:

- Runtime leaves `SAFE_MODE` or `RECOVERY` only through valid transition
- Service validates actuator policy
- Device reports available
- Driver reports initialized
- Registry marks capability available
- errors are cleared only by owning layer or recovery process
- command source is authorized
- safe configuration is loaded
- no repeated watchdog boot loop is active

Automatic recovery may be allowed for:

- non-safety display output
- non-safety audio output
- optional service restart
- stale value recovery

Manual recovery should be required for:

- motion outputs after fatal error
- repeated watchdog reset
- invalid configuration affecting actuators
- metadata invalid for actuator module
- power output re-enable after fault

Recovery must not:

- clear fatal errors automatically during same boot
- re-enable unsafe motion without validation
- allow API/Dashboard bypass
- allow Logic to force unsafe commands

## Required Examples

### CAP_MOTOR_CONTROL Boot

Situation:

```text
Power on -> Runtime not READY
```

Behavior:

```text
CAP_MOTOR_CONTROL unavailable or blocked
outputs stopped/off
forward/reverse/set_speed commands rejected
stop/disable internal safety action allowed
```

### CAP_MOTOR_CONTROL Safe Mode

Situation:

```text
Runtime enters SAFE_MODE
```

Behavior:

```text
active motion stops
new motion commands rejected
stop/brake/disable allowed
Registry stores safe mode command state
Events announce capability/error state
```

### CAP_MOTOR_CONTROL Watchdog Reset

Situation:

```text
ERR_WATCHDOG_RESET
```

Behavior:

```text
outputs return to stopped/off
Runtime blocks motion until READY
repeated reset enters SAFE_MODE
Logic/API/Dashboard cannot re-enable motion early
```

### CAP_MOTOR_CONTROL Module Removal

Situation:

```text
motor module disappears
```

Behavior:

```text
Registry marks module/device/CAP_MOTOR_CONTROL unavailable
Service rejects motor commands
Device is not called
Logic sees CAP_MOTOR_CONTROL.available == false
Dashboard may display unavailable
```

### CAP_MOTOR_CONTROL Capability Unavailable

Situation:

```text
CAP_MOTOR_CONTROL.available == false
```

Behavior:

```text
all motion commands rejected except safe stop if provider can accept it
ERR_CAPABILITY_UNAVAILABLE stored or returned
Logic must choose capability-based fallback
```

### CAP_SERVO_POSITION Boot

Situation:

```text
Power on -> Runtime not READY
```

Behavior:

```text
PWM disabled or held safe
no automatic movement
no sweep
no arbitrary position command
safe neutral only if configured and validated
```

### CAP_SERVO_POSITION Safe Mode

Situation:

```text
Runtime enters SAFE_MODE
```

Behavior:

```text
arbitrary position commands blocked
hold/disable allowed
configured safe position allowed only if validated
Registry stores command failure or safe state
```

### CAP_SERVO_POSITION Watchdog Reset

Situation:

```text
ERR_WATCHDOG_RESET
```

Behavior:

```text
servo output disabled or safe neutral
no movement before Runtime READY
repeated reset blocks servo movement until manual or validated recovery
```

### CAP_SERVO_POSITION Module Removal

Situation:

```text
servo module disappears
```

Behavior:

```text
Registry marks CAP_SERVO_POSITION unavailable
Service rejects position commands
Device is not called
Logic sees unavailable capability
```

### CAP_SERVO_POSITION Capability Unavailable

Situation:

```text
CAP_SERVO_POSITION.available == false
```

Behavior:

```text
position commands rejected
ERR_CAPABILITY_UNAVAILABLE returned
Dashboard may show disabled control
Logic may only use safe fallback through available capabilities
```

## Success Criteria

Cyber32 v1 actuator safety is successful when:

1. Safety-critical actuators fail safe.
2. Motion outputs never activate before Runtime `READY`.
3. Safe mode prevents unsafe motion.
4. Logic cannot bypass safety policy.
5. API cannot bypass safety policy.
6. Dashboard cannot bypass safety policy.
7. Registry stores state only.
8. Runtime coordinates safety transitions.
9. Services own actuator policy.
10. Devices enforce execution safety.
