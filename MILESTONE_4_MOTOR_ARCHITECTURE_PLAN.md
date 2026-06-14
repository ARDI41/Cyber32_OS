# Milestone 4 Motor Architecture Plan

## Goal

Design `CAP_MOTOR_CONTROL` before implementation.

This document is planning only.

No source code is modified by this document.

## Inputs Reviewed

- `ACTUATOR_ARCHITECTURE_PLAN.md`
- `SAFE_MODE_ARCHITECTURE_PLAN.md`
- `MILESTONE_3_4_SAFE_MODE_MANAGEMENT_PLAN.md`
- `CAP_SERVO_POSITION_VERTICAL_SLICE_PLAN.md`

## Scope

Milestone 4 introduces the first motor-control architecture plan, but not motor implementation.

Included:

- `CAP_MOTOR_CONTROL` capability definition
- motor command model
- payload schema
- command schema
- safety rules
- simulated implementation plan
- Registry expectations
- command-state integration
- Runtime integration
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

## 1. CAP_MOTOR_CONTROL Capability

Capability ID:

```text
CAP_MOTOR_CONTROL
```

Category:

```text
actuators
```

Kind:

```text
motor
```

Direction:

```text
read-write
```

Safety profile:

```text
safety-critical motion actuator
```

Default safe state:

```text
stopped
```

Canonical units:

```text
speed_percent = percent
timeout_ms = millisecond
```

Valid range:

```text
speed_percent = 0.0 to 100.0
```

Direction values:

```text
stop
forward
reverse
```

No left/right steering is included in the first motor slice.

## 2. Motor Command Model

First supported commands:

```text
stop
forward
reverse
```

`speed_percent` is included for `forward` and `reverse`.

Command meaning:

```text
stop:
  direction = stop
  speed_percent = 0.0

forward:
  direction = forward
  speed_percent = 0.0 to 100.0

reverse:
  direction = reverse
  speed_percent = 0.0 to 100.0
```

Command state machine uses existing `CommandState`:

```text
REQUEST
ACCEPTED
EXECUTING
COMPLETED
FAILED
TIMED_OUT
CANCELLED
```

First simulated implementation may complete synchronously through `MotorService`, but must still write latest command state to Registry.

Before real hardware, motor commands must be Runtime-coordinated through a bounded Runtime task.

## 3. Payload Schema

Registry stores latest compact motor state.

Latest motor payload:

```text
capability_id = CAP_MOTOR_CONTROL
schema_version = 1
timestamp_ms
available
stale
value_type = PayloadValueType::OBJECT or compact enum-compatible representation
value_float = speed_percent
value_int = direction code
unit = "percent"
quality
error_code
```

Direction encoding:

```text
0 = stop
1 = forward
2 = reverse
```

Initial payload:

```text
capability_id = CAP_MOTOR_CONTROL
schema_version = 1
timestamp_ms = 0
available = Availability::AVAILABLE
stale = StaleState::STALE
value_type = PayloadValueType::NONE
value_float = 0.0F
value_int = 0
unit = "percent"
quality = "stale"
error_code = "none"
```

Stopped payload:

```text
available = Availability::AVAILABLE
stale = StaleState::FRESH
value_type = PayloadValueType::OBJECT
value_float = 0.0F
value_int = 0
unit = "percent"
quality = "valid"
error_code = "none"
```

Unavailable payload:

```text
available = Availability::UNAVAILABLE
stale = StaleState::FRESH
value_type = PayloadValueType::NONE
value_float = 0.0F
value_int = 0
unit = "percent"
quality = "unavailable"
error_code = ERR_DEVICE_TIMEOUT or ERR_CAPABILITY_UNAVAILABLE
```

Rule:

```text
Logic must not treat unavailable motor state as stopped unless Service confirms stop/disable.
```

## 4. Command Schema

Command request:

```text
capability_id = CAP_MOTOR_CONTROL
schema_version = 1
direction
speed_percent
timeout_ms
```

Fields:

```text
direction = stop | forward | reverse
speed_percent = 0.0 to 100.0
timeout_ms = 0 to 10000 for simulated v1
```

Rules:

- `stop` must force `speed_percent = 0.0`
- `forward` requires valid `speed_percent`
- `reverse` requires valid `speed_percent`
- unknown direction fails validation
- negative speed fails validation
- speed above `100.0` fails validation
- timeout above configured maximum fails validation

Compact first implementation request:

```text
MotorCommandRequest
- uint8_t direction
- float speed_percent
- uint32_t timeout_ms
```

Compact first implementation response:

```text
MotorCommandResult
- CommandState state
- RegistryResult registry_result
- bool accepted
- bool executed
- const char* error_code
```

## 5. Safety Rules

### Speed Range Validation

Allowed:

```text
0.0 <= speed_percent <= 100.0
```

Invalid:

```text
speed_percent < 0.0
speed_percent > 100.0
```

Invalid speed must fail before Device is called.

### Direction Validation

Allowed:

```text
stop
forward
reverse
```

Invalid direction must fail before Device is called.

### Runtime READY/RUNNING Requirement

Normal motion commands are allowed only when Runtime is:

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
SAFE_MODE for forward/reverse
```

### SAFE_MODE Blocking

In `SAFE_MODE`:

Allowed:

```text
stop
```

Future allowed commands after documentation:

```text
brake
disable_output
```

Blocked:

```text
forward
reverse
```

If `stop` cannot be executed because the provider is unavailable, command state must fail with compact error and Registry must show motor capability unavailable or error.

### Emergency Stop Behavior

`stop` is the highest-priority motor command.

Rules:

- `stop` must be accepted in `READY`, `RUNNING`, and `SAFE_MODE` if provider is available.
- `stop` must set speed to `0.0`.
- `stop` must update Registry payload.
- `stop` must update command state.
- `stop` must not be delayed behind normal motion commands in future queued designs.
- `stop` may be allowed when other motor commands are blocked.

Before real hardware, validation must prove `stop` remains available in `SAFE_MODE`.

## 6. Simulated Implementation Plan

### SimMotorDriver

Location:

```text
src/drivers/actuators/
```

Files:

```text
sim_motor_driver.h
sim_motor_driver.cpp
```

Methods:

```text
begin()
stop()
setForward(float speed_percent)
setReverse(float speed_percent)
currentDirection() const
currentSpeedPercent() const
setFailureMode(bool enabled)
initialized() const
```

Driver rules:

- no Registry access
- no EventBus access
- no Runtime access
- no API access
- no module names
- no real hardware
- no PWM
- no H-bridge
- no dynamic allocation

### SimMotorDevice

Location:

```text
src/devices/actuators/
```

Files:

```text
sim_motor_device.h
sim_motor_device.cpp
```

Constants:

```text
device_id = "device-sim-motor-001"
device_type = "actuator"
```

Methods:

```text
begin(SimMotorDriver* driver)
readPayload(uint32_t now_ms, CapabilityPayload& out_payload)
stop(uint32_t now_ms, CapabilityPayload& out_payload)
setForward(uint32_t now_ms, float speed_percent, CapabilityPayload& out_payload)
setReverse(uint32_t now_ms, float speed_percent, CapabilityPayload& out_payload)
id() const
type() const
```

Device rules:

- no Registry writes
- no EventBus publishing
- no Logic
- no API
- no module names
- final device-level speed/direction validation
- fail safe on Driver failure

### SimMotorModule

Location:

```text
src/modules/actuators/
```

Files:

```text
sim_motor_module.h
sim_motor_module.cpp
```

Constants:

```text
module_id = "module-sim-motor-001"
module_type = "actuator"
metadata_level = 1
display_name = "Simulated Motor Module"
```

Methods:

```text
id() const
type() const
metadataLevel() const
displayName() const
deviceId() const
deviceType() const
capabilityId() const
```

Module returns:

```text
capabilityId() = CAP_MOTOR_CONTROL
```

Module rules:

- metadata only
- no Device calls
- no Registry writes
- no Logic
- no API

### MotorService

Location:

```text
src/services/motor/
```

Files:

```text
motor_service.h
motor_service.cpp
```

Responsibilities:

- own motor command validation
- enforce Runtime state policy
- enforce Safe Mode policy
- call `SimMotorDevice` after validation
- update Registry payload state
- update Registry command state
- preserve emergency stop path

Methods:

```text
begin(Registry* registry, SimMotorDevice* device)
attachRuntime(Runtime* runtime)
id() const
updateState(uint32_t now_ms)
commandMotor(uint32_t now_ms, const MotorCommandRequest& request, MotorCommandResult& out_result)
lastRegistryResult() const
lastCommandState() const
```

### API Support

Internal API only.

Add:

```text
getMotorState(ApiCapabilityState& out_state)
commandMotor(uint32_t now_ms, const ApiMotorCommandRequest& request, ApiMotorCommandResponse& out_response)
getMotorCommandState(ApiCommandStateResponse& out_response)
attachMotorService(MotorService* service)
```

API rules:

- state reads from Registry
- commands call MotorService
- command-state reads from Registry
- no Device calls
- no Driver calls
- no HAL calls
- no JSON
- no HTTP
- no WiFi

### Validation

Validation must prove:

- simulated motor Driver initializes stopped
- Device initializes with Driver
- PNP discovers motor module
- PNP registers `CAP_MOTOR_CONTROL`
- Registry count increments
- Runtime task updates motor state
- API reads motor state
- forward command succeeds in `READY`
- reverse command succeeds in `RUNNING`
- invalid direction fails
- invalid speed fails
- `SAFE_MODE` blocks forward/reverse
- `SAFE_MODE` allows stop
- stop sets payload speed to `0.0`
- command-state API reflects latest command
- payload remains unchanged after blocked motion command

## 7. Registry Expectations

PNP registration for `CAP_MOTOR_CONTROL`:

```text
category = "actuators"
kind = "motor"
data_type = PayloadValueType::OBJECT
access = "read_write"
status = RecordStatus::AVAILABLE
owner_device_index = device_result.index
latest_payload = initial motor payload
```

Registry stores:

- motor module record
- motor device record
- `CAP_MOTOR_CONTROL` capability record
- latest motor payload
- latest motor command state

Registry must not:

- validate motor policy
- execute motor commands
- call MotorService
- call Device
- call Driver
- schedule Runtime tasks

## 8. Command-State Integration

Use existing `CommandStateRecord`.

For successful forward:

```text
capability_id = CAP_MOTOR_CONTROL
command_state = COMPLETED
timestamp_ms = now_ms
registry_result = OK
error_code = "none"
value_float = speed_percent
value_int = 1
```

For successful reverse:

```text
value_float = speed_percent
value_int = 2
```

For successful stop:

```text
value_float = 0.0
value_int = 0
```

For failed command:

```text
command_state = FAILED
error_code = compact canonical error
value_float = requested speed
value_int = requested direction code
```

Rules:

- latest command only
- no history
- no command payload blobs
- no Dashboard text
- no heap-owned strings

## 9. Runtime Integration

Runtime state gating:

Normal motion commands allowed:

```text
READY
RUNNING
```

Normal motion commands blocked:

```text
BOOTING
INITIALIZING
DISCOVERING
REGISTERING
STARTING
ERROR_STATE
SAFE_MODE
```

Exception:

```text
stop may be allowed in SAFE_MODE
```

Runtime task:

```text
task.motor_service.update_state
```

Future command task:

```text
task.motor_command.execute
```

Runtime must not:

- know `CAP_MOTOR_CONTROL`
- inspect direction
- inspect speed
- decide whether command is safe
- call Device
- call Driver

MotorService owns all motor command policy.

## 10. Stop Conditions Before Real Hardware

Stop before real motor hardware if:

- `CAP_MOTOR_CONTROL` is implemented before this plan is validated.
- motor API calls Device or Driver directly.
- MotorService does not enforce Runtime state.
- MotorService does not enforce Safe Mode.
- `SAFE_MODE` blocks stop with no alternative emergency path.
- forward/reverse can execute in `SAFE_MODE`.
- invalid speed reaches Device.
- invalid direction reaches Device.
- Registry executes motor commands.
- Runtime owns motor policy.
- EventBus stores command state.
- command-state storage cannot represent stop/forward/reverse.
- dynamic allocation is introduced.
- Arduino `String` is introduced.
- STL containers are introduced.
- real PWM is introduced.
- H-bridge code is introduced.
- `src/main.cpp` is modified for motor behavior before simulated validation passes.

## Recommended Phase Order

1. Add `CAP_MOTOR_CONTROL` and `SERVICE_MOTOR` IDs.
2. Add compact motor direction type.
3. Create `SimMotorDriver`.
4. Create `SimMotorDevice`.
5. Create `SimMotorModule`.
6. Add PNP discovery.
7. Add PNP registration.
8. Create `MotorService`.
9. Add internal API support.
10. Add validation state update path.
11. Add command validation tests.
12. Add Safe Mode stop/blocked-motion tests.
13. Audit motor architecture before any real hardware.

## Success Criteria

Milestone 4 architecture is ready for implementation when:

- motor commands are capability-first
- MotorService owns command validation
- Runtime provides state only
- Registry stores state only
- API cannot bypass Service
- simulated Driver/Device/Module are defined
- Safe Mode blocks motion
- Safe Mode preserves emergency stop
- command-state integration is defined
- validation requirements are explicit
- no real hardware, PWM, or H-bridge work is included
