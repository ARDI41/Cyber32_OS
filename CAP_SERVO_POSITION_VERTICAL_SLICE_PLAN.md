# CAP_SERVO_POSITION Vertical Slice Plan

## Goal

Define the first Cyber32 actuator vertical slice using:

```text
CAP_SERVO_POSITION
```

This plan is documentation only.

No source code is modified by this plan.

## Scope

Included:

- simulated servo Driver
- simulated servo Device
- servo Module metadata
- PNP discovery
- PNP registration
- Registry state model
- ServoService
- command validation
- Runtime task and command flow
- Logic command example
- API command example
- safety behavior
- validation steps

Excluded:

- real servo hardware
- real PWM output
- motor logic
- WiFi
- WebServer
- Dashboard implementation
- Mobile Studio implementation
- cloud
- OTA

## Architecture Rule

The fixed layer order remains:

```text
HAL
-> Drivers
-> Devices
-> Modules
-> PNP
-> Registry
-> Runtime
-> Services
-> Logic
-> API
-> Dashboard
```

Command execution must flow:

```text
Logic or API
-> ServoService
-> Runtime
-> SimServoDevice
-> SimServoDriver
-> HAL abstraction if needed later
```

State must flow back:

```text
Device
-> Service
-> Registry
-> EventBus
-> API / Logic / Dashboard later
```

## Capability ID

Capability:

```text
CAP_SERVO_POSITION
```

Category:

```text
actuators
```

Kind:

```text
servo
```

Direction:

```text
read-write
```

Datatype:

```text
float for latest position state
object for command requests
```

Canonical unit:

```text
degree
```

Valid range:

```text
0.0 to 180.0
```

## Payload Schema

Registry stores latest compact servo state.

Latest state payload:

```text
capability_id = CAP_SERVO_POSITION
schema_version = 1
timestamp_ms
available
stale
value_type = PayloadValueType::FLOAT
value_float = latest_position_degree
value_int = 0
unit = "degree"
quality
error_code
```

Initial registered payload:

```text
capability_id = CAP_SERVO_POSITION
schema_version = 1
timestamp_ms = 0
available = Availability::AVAILABLE
stale = StaleState::STALE
value_type = PayloadValueType::NONE
value_float = 0.0
value_int = 0
unit = "degree"
quality = "stale"
error_code = "none"
```

Unavailable payload:

```text
capability_id = CAP_SERVO_POSITION
schema_version = 1
timestamp_ms = now_ms
available = Availability::UNAVAILABLE
stale = StaleState::FRESH
value_type = PayloadValueType::NONE
value_float = 0.0
value_int = 0
unit = "degree"
quality = "unavailable"
error_code = ERR_DEVICE_TIMEOUT or ERR_CAPABILITY_UNAVAILABLE
```

Logic must not treat unavailable servo position as zero.

## Command Schema

Supported first command:

```text
set_position
```

Command request fields:

```text
capability_id
schema_version
request_id
command
position_degree
duration_ms
timeout_ms
source_type
timestamp_ms
```

First-slice compact command shape:

```text
capability_id = CAP_SERVO_POSITION
schema_version = 1
request_id = fixed compact ID
command = set_position
position_degree = 0.0 to 180.0
duration_ms = 0 to 10000
timeout_ms = 0 to 10000
source_type = logic | api | system
timestamp_ms = uptime
```

Command states:

```text
request
accepted
executing
completed
failed
timed_out
cancelled
```

First slice may implement only:

```text
accepted
executing
completed
failed
```

Timeout support may be represented compactly and completed in a later phase if Runtime timeout enforcement is not ready.

## Simulated Servo Driver

Location:

```text
src/drivers/actuators/
```

Proposed files:

```text
sim_servo_driver.h
sim_servo_driver.cpp
```

Class:

```text
SimServoDriver
```

Methods:

```text
begin()
setPosition(float position_degree)
readPosition(float& out_position_degree)
setFailureMode(bool enabled)
disable()
```

Behavior:

- `begin()` initializes the simulated driver in disabled or safe state.
- `setPosition(position_degree)` succeeds only when initialized, not in failure mode, and position is valid.
- `readPosition(out_position_degree)` returns latest simulated position.
- `setFailureMode(true)` forces execution to fail.
- `disable()` returns simulated servo to safe disabled state.

Driver rules:

- Driver does not register capabilities.
- Driver does not write Registry.
- Driver does not publish events.
- Driver does not run Logic.
- Driver does not expose API.
- Driver does not know module names.
- No dynamic allocation.
- No Arduino `String`.
- No STL containers.
- No real PWM yet.

## Simulated Servo Device

Location:

```text
src/devices/actuators/
```

Proposed files:

```text
sim_servo_device.h
sim_servo_device.cpp
```

Class:

```text
SimServoDevice
```

Constants:

```text
device_id = "device-sim-servo-001"
device_type = "actuator"
```

Methods:

```text
begin(SimServoDriver* driver)
readPayload(uint32_t now_ms, CapabilityPayload& out_payload)
executeSetPosition(float position_degree)
disable()
id() const
type() const
```

Behavior:

- Device wraps `SimServoDriver`.
- Device produces latest `CAP_SERVO_POSITION` payload.
- Device executes only validated commands from Service.
- Device does not validate API policy.
- Device enforces device-level position bounds as a final safety check.
- Device fails safe on driver error.

Device rules:

- Device does not register capabilities.
- Device does not write Registry.
- Device does not publish events.
- Device does not run Logic.
- Device does not expose API.
- Device does not know module names.

## Servo Module Metadata

Location:

```text
src/modules/motion/
```

or if keeping simulated actuators separate:

```text
src/modules/automation/
```

Proposed files:

```text
sim_servo_module.h
sim_servo_module.cpp
```

Class:

```text
SimServoModule
```

Constants:

```text
module_id = "module-sim-servo-001"
module_type = "motion"
metadata_level = 1
display_name = "Simulated Servo Module"
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

Behavior:

- `deviceId()` returns `"device-sim-servo-001"`.
- `deviceType()` returns `"actuator"`.
- `capabilityId()` returns `CAP_SERVO_POSITION`.

Module rules:

- metadata only
- no command execution
- no Registry writes
- no Device calls
- display name is friendly metadata only
- Logic must never depend on module ID or display name

## PNP Discovery

Add a simulated Level 1 discovery method:

```text
discoverSimulatedServoModule(PnpModuleInfo& out_info)
```

Behavior:

1. Read identity and metadata from `SimServoModule`.
2. Fill `PnpModuleInfo`.
3. Validate required fields.
4. Validate `metadata_level == 1`.
5. Validate `capability_id` starts with `CAP_`.
6. Validate capability is `CAP_SERVO_POSITION`.
7. Set `valid = true` on success.
8. Publish `EVT_MODULE_DISCOVERED` if EventBus is attached.
9. Return true on success.

PNP must not:

- write Registry directly
- execute servo commands
- call Driver
- call Device behavior
- run Logic
- expose API

## PNP Registration

PNP Registration should support:

```text
CAP_SERVO_POSITION
```

Registration order:

```text
ModuleRecord
DeviceRecord
CapabilityRecord
```

Use Registry result APIs:

```text
registerModuleWithResult
registerDeviceWithResult
registerCapabilityWithResult
```

Use returned indexes:

```text
module_result.index -> DeviceRecord.module_index
device_result.index -> CapabilityRecord.owner_device_index
```

Servo `CapabilityRecord`:

```text
capability_id = CAP_SERVO_POSITION
category = "actuators"
kind = "servo"
data_type = PayloadValueType::FLOAT
access = "read-write"
status = RecordStatus::AVAILABLE
owner_device_index = device_result.index
latest_payload = initial servo payload
```

No rollback in first actuator slice unless separately planned.

If registration fails after a partial insert, the failure must be documented and future rollback/two-phase registration must be planned before real actuator hardware.

## Registry State

Registry stores:

- servo module record
- servo device record
- `CAP_SERVO_POSITION` capability record
- latest servo position payload
- latest compact command state when command state schema is added
- latest compact error code

Registry must not:

- validate servo command policy
- execute servo movement
- call Device
- call Driver
- schedule Runtime tasks
- expose API directly

Expected counts after adding servo to the current validation environment:

```text
modules = 3
devices = 3
capabilities = 3
```

Existing capabilities must remain:

```text
CAP_TEMPERATURE
CAP_DISTANCE
CAP_SERVO_POSITION
```

## ServoService

Location:

```text
src/services/servo/
```

Proposed files:

```text
servo_service.h
servo_service.cpp
```

Class:

```text
ServoService
```

Responsibilities:

1. Own servo command policy.
2. Validate command payload.
3. Enforce Runtime state restrictions.
4. Enforce safe mode restrictions.
5. Call `SimServoDevice` only after validation.
6. Update Registry latest payload/state through public APIs.
7. Store latest command result through Registry when command-state storage exists.
8. Avoid direct event publishing if Registry already emits state events.

Methods for first slice:

```text
begin(Registry* registry, Runtime* runtime, SimServoDevice* device)
update(uint32_t now_ms)
requestSetPosition(const ServoCommandRequest& request)
lastRegistryResult() const
lastCommandState() const
```

If command request structs are not yet implemented, first phase must define shared command types before `ServoService`.

Service must not:

- expose API directly
- run Logic
- know module names
- bypass Runtime for command coordination once Runtime command tasks exist
- allocate memory dynamically

## Command Validation

ServoService validates:

- `capability_id == CAP_SERVO_POSITION`
- capability exists in Registry
- capability is available
- access is `read-write`
- `schema_version == 1`
- command is `set_position`
- `position_degree` is `0.0 <= value <= 180.0`
- `duration_ms <= 10000`
- `timeout_ms <= 10000`
- source type is allowed
- Runtime state is `READY` or `RUNNING`
- safe mode is not active for arbitrary movement
- Device pointer is attached

Validation failures:

```text
ERR_CAPABILITY_UNAVAILABLE
ERR_CONFIG_INVALID
ERR_RUNTIME_OVERLOAD
ERR_API_UNAUTHORIZED
```

Do not add new error IDs until the error catalog phase explicitly approves them.

## Runtime Task And Command Flow

Runtime remains scheduler/coordinator only.

Runtime task:

```text
task.servo_command.execute
```

Runtime context:

```text
ServoCommandTaskContext
```

Context contains fixed references only:

```text
ServoService* service
uint32_t now_ms
bool ran
bool last_result
RegistryResult last_registry_result
```

Flow:

```text
API or Logic creates command request
-> ServoService validates request
-> ServoService stores accepted command in fixed slot
-> Runtime runs servo command task
-> task calls ServoService command execution method
-> ServoService calls SimServoDevice
-> Device calls SimServoDriver
-> Service updates Registry payload and command state
-> Registry emits capability update event
```

Runtime must not:

- inspect `position_degree`
- validate servo range
- decide whether command is safe
- call Driver
- call Device directly

## Logic Command Example

Logic can request servo commands through capability IDs only.

Example:

```text
IF CAP_DISTANCE.value > 1.0
THEN CAP_SERVO_POSITION set_position 90.0 degree
```

Internal request:

```text
capability_id = CAP_SERVO_POSITION
schema_version = 1
request_id = logic-fixed-001
command = set_position
position_degree = 90.0
duration_ms = 500
timeout_ms = 1000
source_type = logic
```

Logic must not:

- reference `"module-sim-servo-001"`
- reference `"device-sim-servo-001"`
- call `SimServoDevice`
- call `SimServoDriver`
- bypass ServoService

For first actuator validation, Logic command request may be a controlled test rule, not a general visual builder.

## API Command Example

No WiFi or WebServer is added.

Internal API method may be planned as:

```text
requestServoPositionCommand(const ApiServoCommandRequest& request, ApiCommandResponse& out_response)
```

Conceptual HTTP future shape from API contract:

```text
POST /capabilities/CAP_SERVO_POSITION/command
```

Body:

```text
schema_version = 1
request_id = "req-001"
command = "set_position"
params.position_degree = 90.0
params.duration_ms = 500
timeout_ms = 1000
```

Internal response:

```text
ok = true
capability_id = CAP_SERVO_POSITION
request_id = req-001
command_state = accepted
error_code = none
```

API must not:

- call Device
- call Driver
- call HAL
- bypass ServoService
- own command state
- use JSON internally
- require WiFi/WebServer

## Safety Behavior

### Boot

Before Runtime `READY`:

```text
CAP_SERVO_POSITION commands blocked
simulated servo disabled or no movement
no automatic sweep
no arbitrary position command
```

Safe neutral is allowed only if explicitly configured and validated.

For the first simulated slice:

```text
default safe position state = unavailable or stale no movement
```

### Normal Operation

When Runtime is `READY/RUNNING`:

```text
set_position allowed after Service validation
position range enforced
duration bounded
timeout bounded
Device available
```

### Safe Mode

In safe mode:

```text
arbitrary set_position blocked
hold/disable allowed
configured safe position allowed only if validated
```

First slice may represent safe mode rejection as a validation failure until full safe mode Runtime support exists.

### Failure

Driver failure:

```text
command_state = failed
error_code = ERR_DEVICE_TIMEOUT
safe_action = disable or hold
```

Capability unavailable:

```text
command rejected
error_code = ERR_CAPABILITY_UNAVAILABLE
Device not called
```

Runtime overload:

```text
command failed or delayed
error_code = ERR_RUNTIME_OVERLOAD when available
```

Watchdog reset:

```text
servo output disabled/no movement
commands blocked until Runtime READY
repeated reset requires recovery/safe mode
```

## Event Model

Initial events may reuse:

```text
EVT_MODULE_DISCOVERED
EVT_CAPABILITY_REGISTERED
EVT_CAPABILITY_VALUE_UPDATED
EVT_ERROR_RAISED
```

Future command events may be added only after documentation:

```text
EVT_COMMAND_ACCEPTED
EVT_COMMAND_EXECUTING
EVT_COMMAND_COMPLETED
EVT_COMMAND_FAILED
EVT_COMMAND_TIMED_OUT
```

Event payloads must be compact:

- `capability_id`
- compact request reference
- command state enum
- error code
- no full command payload
- no Dashboard data

Registry remains source of truth.

EventBus does not store command state.

## Validation Steps

First simulated actuator validation should prove:

1. Existing `CAP_TEMPERATURE` still passes.
2. Existing `CAP_DISTANCE` still passes.
3. Simulated servo driver initializes in safe state.
4. Simulated servo device initializes with driver.
5. PNP discovers simulated servo module.
6. PNP registers servo module/device/capability.
7. Registry counts become:

```text
modules = 3
devices = 3
capabilities = 3
```

8. `CAP_SERVO_POSITION` exists.
9. Initial servo payload is stale and safe.
10. ServoService rejects invalid position `< 0.0`.
11. ServoService rejects invalid position `> 180.0`.
12. ServoService rejects command before Runtime `READY` if Runtime state gating is active.
13. ServoService accepts valid `set_position 90.0` when allowed.
14. Runtime task executes servo command.
15. Device updates simulated position to `90.0`.
16. Registry stores latest servo payload.
17. API command response reports accepted/completed when command API exists.
18. Logic command path uses `CAP_SERVO_POSITION`, not module/device names.
19. Failure mode maps to compact error state.
20. No real PWM or servo hardware is touched.

## Implementation Order

Recommended phases:

1. Add `CAP_SERVO_POSITION` and `SERVICE_SERVO` IDs.
2. Define command shared types.
3. Define command result/status types.
4. Extend Registry schema for latest command state.
5. Create simulated servo Driver.
6. Create simulated servo Device.
7. Create simulated servo Module metadata.
8. Add PNP discovery support.
9. Add PNP registration support.
10. Create ServoService validation-only path.
11. Add Runtime command task context.
12. Add internal API command method.
13. Add Logic command request example.
14. Extend validation harness.
15. Audit actuator architecture before real hardware.

## Stop Conditions

Stop implementation if:

- real PWM is added before simulated validation
- API calls Device directly
- Logic calls Device directly
- Runtime validates servo angle policy
- Registry executes commands
- EventBus stores command state
- Dashboard or WiFi is introduced
- servo moves before Runtime `READY`
- invalid command can reach Device
- unavailable capability is treated as a valid zero-degree command
- dynamic allocation is introduced

## Success Criteria

The `CAP_SERVO_POSITION` vertical slice is successful when:

```text
simulated servo module is discovered
CAP_SERVO_POSITION is registered
ServoService validates command policy
Runtime coordinates command execution
SimServoDevice executes validated command
Registry stores latest servo state
API/Logic use capability IDs only
no real servo hardware is touched
no layer violations occur
```

The slice must preserve:

```text
CAP_TEMPERATURE
CAP_DISTANCE
```

and add only:

```text
CAP_SERVO_POSITION
```
