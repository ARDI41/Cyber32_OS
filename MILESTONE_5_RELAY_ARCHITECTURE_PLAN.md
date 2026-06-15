# Milestone 5 Relay Architecture Plan

## Goal

Define `CAP_RELAY_CONTROL` architecture before implementation.

This plan introduces no source code, no behavior changes, and no real relay hardware. It defines the intended simulated relay vertical slice using the existing Cyber32 architecture.

## Reviewed Documents

- `PRODUCTION_BOOTSTRAP_PLAN.md`
- `ACTUATOR_SAFETY_POLICY.md`
- `COMMAND_DISPATCH_CONTRACT.md`
- `MILESTONE_4_1_ARCHITECTURE_AUDIT.md`

## Architecture Position

Relay support must follow the fixed layer order:

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
```

Dashboard and Mobile Studio remain future API clients only.

## Capability Definition

Capability ID:

```text
CAP_RELAY_CONTROL
```

Category:

```text
Power / Actuator
```

Direction:

```text
read_write
```

Purpose:

Expose a bounded relay output as a capability-first actuator. The relay can be commanded on or off, with off as the default-safe and fail-safe state.

Core rule:

Logic, API, and future Dashboard/App must refer to `CAP_RELAY_CONTROL`, not module names, relay board names, pin numbers, or display names.

## Relay Payload Schema

Registry stores latest relay state as a compact `CapabilityPayload`.

Payload fields:

| Field | Value |
|---|---|
| `capability_id` | `CAP_RELAY_CONTROL` |
| `schema_version` | `1` |
| `timestamp_ms` | update time |
| `available` | `AVAILABLE` or `UNAVAILABLE` |
| `stale` | `FRESH` or `STALE` |
| `value_type` | `BOOLEAN` |
| `value_float` | `0.0F` |
| `value_int` | `0` for off, `1` for on |
| `unit` | `"boolean"` |
| `quality` | `"valid"`, `"stale"`, or `"unavailable"` |
| `error_code` | `"none"` or compact `ERR_*` |

Initial payload:

```text
capability_id = CAP_RELAY_CONTROL
schema_version = 1
timestamp_ms = 0
available = AVAILABLE
stale = STALE
value_type = NONE
value_float = 0.0F
value_int = 0
unit = boolean
quality = stale
error_code = none
```

Safe unavailable payload:

```text
available = UNAVAILABLE
stale = FRESH
value_type = NONE
value_float = 0.0F
value_int = 0
unit = boolean
quality = unavailable
error_code = ERR_ACTUATOR_UNAVAILABLE or ERR_ACTUATOR_EXECUTION_FAILED
```

## Relay Command Schema

Command request:

| Field | Type | Meaning |
|---|---|---|
| `enabled` | `bool` | `false` = off, `true` = on |
| `timeout_ms` | `uint32_t` | bounded command timeout |

Command result:

| Field | Type | Meaning |
|---|---|---|
| `state` | `CommandState` | final or accepted command state |
| `registry_result` | `RegistryResult` | Registry write result |
| `accepted` | `bool` | request accepted by Service |
| `executed` | `bool` | Device execution completed |
| `error_code` | `const char*` | compact canonical error |

Valid command rules:

- `enabled == false` is always the safe/off command.
- `enabled == true` is an activation command and requires Runtime `READY` or `RUNNING`.
- `timeout_ms` must be within the v1 bounded command timeout range.
- no dynamic strings or unbounded payloads are allowed.

## Runtime Gating

RelayService owns command gating policy. Runtime only provides state.

Allowed states for relay on/off commands:

| Runtime state | ON allowed | OFF allowed |
|---|---:|---:|
| `READY` | yes | yes |
| `RUNNING` | yes | yes |
| `SAFE_MODE` | no | yes |
| `BOOTING` | no | no |
| `INITIALIZING` | no | no |
| `DISCOVERING` | no | no |
| `REGISTERING` | no | no |
| `STARTING` | no | no |
| `ERROR_STATE` | no | no for normal command path |

Rationale:

- relay on may energize real hardware and must be blocked in `SAFE_MODE`
- relay off is the safe command and may be allowed in `SAFE_MODE`
- `ERROR_STATE` blocks normal commands until explicit emergency behavior is documented

## SAFE_MODE Behavior

Default relay policy in `SAFE_MODE`:

- block ON commands
- allow OFF commands
- do not call Device for blocked ON commands
- leave payload unchanged for blocked ON commands
- write command state `FAILED` with `ERR_SAFE_MODE_BLOCKED`
- OFF command may execute and update payload to off

Future real hardware policy may require RelayService to force off on safe mode entry, but that must be implemented explicitly and validated before real hardware.

## Command-State Behavior

Registry stores the latest command state only.

Expected command-state records:

| Condition | CommandState | Error Code | Payload |
|---|---|---|---|
| ON accepted | `ACCEPTED` or `COMPLETED` depending execution model | `"none"` | unchanged until execution |
| OFF accepted | `ACCEPTED` or `COMPLETED` depending execution model | `"none"` | unchanged until execution |
| ON completed | `COMPLETED` | `"none"` | on |
| OFF completed | `COMPLETED` | `"none"` | off |
| invalid timeout | `FAILED` | `ERR_COMMAND_INVALID_TIMEOUT` | unchanged |
| Runtime blocked | `FAILED` | `ERR_RUNTIME_NOT_READY` | unchanged |
| SAFE_MODE ON blocked | `FAILED` | `ERR_SAFE_MODE_BLOCKED` | unchanged |
| command timeout | `TIMED_OUT` | `ERR_COMMAND_TIMEOUT` | unchanged |
| device/driver failure | `FAILED` | `ERR_ACTUATOR_EXECUTION_FAILED` | previous safe or unavailable |
| missing attachment | `FAILED` | `ERR_ACTUATOR_UNAVAILABLE` | unchanged or unavailable |

Relay v1 may choose immediate execution like ServoService or bounded pending execution like MotorService. Before real hardware, the preferred path is bounded pending execution through Runtime task so activation never happens directly from API.

## Simulated Driver Plan

Class:

```text
SimRelayDriver
```

Folder:

```text
src/drivers/actuators/
```

Methods:

```text
begin()
setEnabled(bool enabled)
disable()
enabled() const
setFailureMode(bool enabled)
initialized() const
```

Behavior:

- `begin()` initializes driver and sets relay off
- `setEnabled(true)` turns simulated relay on if initialized and not failing
- `setEnabled(false)` turns simulated relay off if initialized and not failing
- `disable()` is equivalent to off
- failure mode forces command execution failure

Driver rules:

- no Registry
- no Runtime
- no API
- no PNP
- no module names
- no real relay hardware
- no GPIO writes in simulated phase

## Simulated Device Plan

Class:

```text
SimRelayDevice
```

Folder:

```text
src/devices/actuators/
```

Constants:

```text
device_id = "device-sim-relay-001"
device_type = "actuator"
```

Methods:

```text
begin(SimRelayDriver* driver)
readPayload(uint32_t now_ms, CapabilityPayload& out_payload)
setEnabled(uint32_t now_ms, bool enabled, CapabilityPayload& out_payload)
disable(uint32_t now_ms, CapabilityPayload& out_payload)
id() const
type() const
```

Device rules:

- Device calls Driver only.
- Device fills `CAP_RELAY_CONTROL` payload.
- Device does not write Registry.
- Device does not publish events.
- Device does not run Logic.
- Device does not expose API.
- Device does not know module names.

## Simulated Module Plan

Class:

```text
SimRelayModule
```

Folder:

```text
src/modules/actuators/
```

Constants:

```text
module_id = "module-sim-relay-001"
module_type = "actuator"
metadata_level = 1
display_name = "Simulated Relay Module"
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

Module rules:

- metadata only
- no Device calls
- no Registry writes
- no Service policy
- no Logic
- no API
- display name is friendly metadata only

## PNP and Registry Plan

PNP Discovery:

- add `discoverSimulatedRelayModule(PnpModuleInfo& out_info)`
- validate required fields
- validate metadata level 1
- validate `CAP_` prefix
- publish module discovered event if EventBus attached
- do not write Registry

PNP Registration:

- support `CAP_RELAY_CONTROL`
- register ModuleRecord
- register DeviceRecord
- register CapabilityRecord
- use `RegistryWriteResult` indexes
- initial payload is stale/off-safe

CapabilityRecord:

```text
capability_id = CAP_RELAY_CONTROL
category = power
kind = actuator
data_type = BOOLEAN
access = read_write
status = AVAILABLE
owner_device_index = device_result.index
```

## RelayService Responsibilities

Class:

```text
RelayService
```

Folder:

```text
src/services/relay/
```

Responsibilities:

- initialize with Registry and SimRelayDevice
- optionally attach Runtime
- update relay state into Registry
- validate relay commands
- enforce Runtime gating
- enforce `SAFE_MODE` ON blocking
- allow `SAFE_MODE` OFF command
- write command-state records
- update Registry payload through public result APIs
- preserve safe payload on failed ON commands
- avoid module names

Suggested methods:

```text
begin(Registry* registry, SimRelayDevice* device)
attachRuntime(Runtime* runtime)
id() const
updateState(uint32_t now_ms)
setEnabled(uint32_t now_ms, const RelayCommandRequest& request, RelayCommandResult& out_result)
disable(uint32_t now_ms, RelayCommandResult& out_result)
executePendingCommand(uint32_t now_ms) if pending execution is used
lastRegistryResult() const
lastCommandState() const
```

RelayService must not:

- expose API
- call HAL directly
- call Driver directly
- change Runtime state
- execute Dashboard decisions
- use dynamic allocation

## API State and Command Plan

API additions:

```text
attachRelayService(RelayService* service)
getRelayControlState(ApiCapabilityState& out_state)
commandRelayControl(uint32_t now_ms, const ApiRelayCommandRequest& request, ApiRelayCommandResponse& out_response)
commandRelayOff(uint32_t now_ms, ApiRelayCommandResponse& out_response)
getRelayCommandState(ApiCommandStateResponse& out_response)
```

API request:

```text
enabled: bool
timeout_ms: uint32_t
```

API rules:

- API reads Registry for state.
- API reads Registry for command state.
- API sends commands through RelayService only.
- API does not call Device.
- API does not call Driver.
- API does not call HAL.
- API does not own state.
- no JSON, HTTP, WiFi, or WebServer in this milestone unless separately planned.

## Runtime Task Plan

If RelayService uses pending command execution, register:

```text
task.relay_service.update_state
task.relay_service.execute_command
```

Runtime task rules:

- Runtime callback calls RelayService methods.
- Runtime does not inspect relay command values.
- Runtime does not know `CAP_RELAY_CONTROL`.
- Runtime remains scheduler-only.

## Validation Plan

Validation objects:

- `SimRelayDriver`
- `SimRelayDevice`
- `RelayService`

Setup validation:

- initialize Driver
- initialize Device with Driver
- discover Relay Module through PNP
- register Relay Module through PNP Registration
- initialize RelayService
- attach Runtime to RelayService
- attach RelayService to API
- register Runtime tasks

State validation:

- Registry module count increases by 1
- Registry device count increases by 1
- Registry capability count increases by 1
- `CAP_RELAY_CONTROL` exists
- initial payload is off or stale/off-safe
- after state update, payload is off and available
- API state returns `ok == true`

Command validation:

- Runtime `READY`, command ON succeeds or is accepted then completed
- relay payload becomes on
- Runtime `READY`, command OFF succeeds or is accepted then completed
- relay payload becomes off
- invalid timeout fails with `ERR_COMMAND_INVALID_TIMEOUT`
- driver failure fails with `ERR_ACTUATOR_EXECUTION_FAILED`
- failed command does not store unsafe on payload
- command state is readable through API

Runtime gating validation:

- ON blocked before `READY`
- ON blocked in `SAFE_MODE`
- OFF allowed in `SAFE_MODE`
- commands blocked in `ERROR_STATE`
- blocked ON command does not call Device
- blocked ON command leaves payload unchanged

Safety validation:

- power-on default is off
- before Runtime `READY`, relay cannot turn on
- `SAFE_MODE` does not allow relay on
- failed ON command does not store on state
- missing Runtime attachment blocks ON by default

## Logic Plan

No relay Logic behavior is required for the first relay slice.

Future Logic may request relay commands through capability IDs only:

```text
IF CAP_TEMPERATURE.value > threshold
THEN CAP_RELAY_CONTROL command off
```

Logic must not depend on:

- relay module name
- relay device ID
- GPIO pin
- display name

## Stop Conditions Before Real Hardware

Stop before implementing real relay hardware if:

- simulated relay validation is incomplete
- ON can occur before Runtime `READY`
- ON can occur in `SAFE_MODE`
- failed ON command can store payload as on
- API can call Device or Driver directly
- Runtime must inspect `CAP_RELAY_CONTROL`
- Registry would need to execute commands
- RelayService needs dynamic allocation
- relay command queue becomes unbounded
- GPIO pin behavior is not abstracted through HAL
- boot default off behavior is not proven
- watchdog/brownout off behavior is not documented
- electrical relay active-high/active-low behavior is not documented
- real hardware requires changes to fixed architecture

## Implementation Roadmap

1. Add `CAP_RELAY_CONTROL` and `SERVICE_RELAY` IDs.
2. Add relay command type if needed.
3. Add simulated Relay Driver.
4. Add simulated Relay Device.
5. Add simulated Relay Module.
6. Add PNP discovery.
7. Add PNP registration.
8. Add RelayService.
9. Add API state and command support.
10. Add Runtime task integration.
11. Add validation.
12. Create Milestone 5 architecture audit.

## Architecture Compliance Checklist

- fixed layer order is preserved
- capability-first behavior is preserved
- RelayService owns command policy
- Runtime remains scheduler/state provider only
- Registry stores state only
- API commands go through Service
- Device executes only when Service calls it
- Driver remains simulated and hardware-free
- PNP reads metadata only
- Logic has no module-name dependency
- Safe Mode blocks relay on
- relay off remains the safe state
- no dynamic allocation
- no Arduino `String`
- no STL containers
- no real relay hardware

## Final Assessment

`CAP_RELAY_CONTROL` is suitable as the next actuator slice after motor because it exercises power-output safety without requiring motion control complexity.

The first implementation must remain simulated. Real relay hardware must wait until off-by-default, Runtime gating, Safe Mode blocking, command-state reporting, and failed-command payload safety are validated end to end.
