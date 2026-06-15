# Milestone 5 Implementation Plan

## Goal

Convert `MILESTONE_5_RELAY_ARCHITECTURE_PLAN.md` into an implementation roadmap for `CAP_RELAY_CONTROL`.

This plan is documentation only. It does not change source code.

## Required Architecture Choices

Milestone 5 must:

- use the bounded pending execution model
- follow the MotorService architecture
- execute relay commands through a Runtime task
- keep RelayService as command policy owner
- block relay ON in `SAFE_MODE`
- allow relay OFF in `SAFE_MODE`
- keep OFF as the fail-safe default
- avoid real relay hardware
- avoid GPIO/PWM/H-bridge behavior

Fixed layer order remains:

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

## Phase 1: IDs

Purpose:

Add only shared IDs and minimal relay command type support.

Expected files:

- `src/core/ids/capability_ids.h`
- `src/core/ids/service_ids.h`
- optional `src/core/types/relay_types.h`

Expected additions:

```text
CAP_RELAY_CONTROL
SERVICE_RELAY
```

Optional type:

```text
RelayState or RelayCommandValue if needed
```

Success criteria:

- IDs use existing C++11-safe style.
- no behavior is added.
- no implementation files are created.
- no existing capability behavior changes.
- `src/main.cpp` remains untouched.

Stop conditions:

- implementation logic is needed
- dynamic allocation is introduced
- Arduino `String` or STL containers are introduced
- new architecture is required

## Phase 2: SimRelayDriver

Purpose:

Create simulated relay Driver with no real hardware control.

Expected files:

- `src/drivers/actuators/sim_relay_driver.h`
- `src/drivers/actuators/sim_relay_driver.cpp`

Expected class:

```text
SimRelayDriver
```

Expected methods:

```text
begin()
setEnabled(bool enabled)
disable()
enabled() const
setFailureMode(bool enabled)
initialized() const
```

Expected behavior:

- `begin()` initializes and sets relay OFF.
- `setEnabled(true)` stores ON only when initialized and not failing.
- `setEnabled(false)` stores OFF only when initialized and not failing.
- `disable()` sets OFF.
- failure mode makes commands fail.

Success criteria:

- normal ON/OFF simulation works.
- OFF is default after begin.
- failure mode blocks command execution.
- no Registry/EventBus/Runtime/API dependency.
- no real relay hardware or GPIO writes.

Stop conditions:

- Driver needs module names
- Driver needs Registry
- Driver writes hardware pins
- dynamic allocation or STL is introduced

## Phase 3: SimRelayDevice

Purpose:

Create Device wrapper that exposes `CAP_RELAY_CONTROL` payload.

Expected files:

- `src/devices/actuators/sim_relay_device.h`
- `src/devices/actuators/sim_relay_device.cpp`

Expected class:

```text
SimRelayDevice
```

Expected constants:

```text
device_id = "device-sim-relay-001"
device_type = "actuator"
```

Expected methods:

```text
begin(SimRelayDriver* driver)
readPayload(uint32_t now_ms, CapabilityPayload& out_payload)
setEnabled(uint32_t now_ms, bool enabled, CapabilityPayload& out_payload)
disable(uint32_t now_ms, CapabilityPayload& out_payload)
id() const
type() const
```

Success criteria:

- payload uses `CAP_RELAY_CONTROL`.
- OFF payload uses `value_type = BOOLEAN`, `value_int = 0`.
- ON payload uses `value_type = BOOLEAN`, `value_int = 1`.
- unavailable payload is safe/off-oriented.
- Device does not write Registry.
- Device does not publish events.
- Device does not expose API.

Stop conditions:

- Device registers capabilities
- Device writes Registry
- Device knows module names
- Device includes real hardware control

## Phase 4: SimRelayModule

Purpose:

Create metadata-only relay Module.

Expected files:

- `src/modules/actuators/sim_relay_module.h`
- `src/modules/actuators/sim_relay_module.cpp`

Expected class:

```text
SimRelayModule
```

Expected constants:

```text
module_id = "module-sim-relay-001"
module_type = "actuator"
metadata_level = 1
display_name = "Simulated Relay Module"
```

Expected methods:

```text
id() const
type() const
metadataLevel() const
displayName() const
deviceId() const
deviceType() const
capabilityId() const
```

Success criteria:

- `deviceId()` returns `device-sim-relay-001`.
- `deviceType()` returns `actuator`.
- `capabilityId()` returns `CAP_RELAY_CONTROL`.
- Module does not call Device.
- Module does not write Registry.

Stop conditions:

- Module controls hardware
- Module owns command policy
- Logic depends on module ID or display name

## Phase 5: PNP Discovery

Purpose:

Allow PNP to discover the simulated relay module.

Expected files:

- `src/pnp/pnp_discovery.h`
- `src/pnp/pnp_discovery.cpp`
- `src/modules/actuators/sim_relay_module.h`

Expected method:

```text
discoverSimulatedRelayModule(PnpModuleInfo& out_info)
```

Expected behavior:

- read relay Module metadata
- fill `PnpModuleInfo`
- validate required fields
- validate metadata level 1
- validate `CAP_` prefix
- publish `EVT_MODULE_DISCOVERED` if EventBus is attached
- do not register with Registry

Success criteria:

- relay discovery succeeds with valid metadata.
- invalid metadata would return false.
- EventBus integration remains optional.
- PNP does not call Driver or Device behavior.

Stop conditions:

- PNP writes Registry directly
- PNP calls Device or Driver
- PNP runs Logic
- PNP exposes API

## Phase 6: PNP Registration

Purpose:

Allow PNP Registration to register relay Module, Device, and Capability records.

Expected files:

- `src/pnp/pnp_registration.h`
- `src/pnp/pnp_registration.cpp`
- `src/registry/registry_records.h`
- `src/registry/registry_result.h`

Expected additions:

- support `CAP_RELAY_CONTROL`
- create relay initial payload
- create relay `CapabilityRecord`
- use `RegistryWriteResult` indexes

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
unit = "boolean"
quality = "stale"
error_code = "none"
```

CapabilityRecord:

```text
capability_id = CAP_RELAY_CONTROL
category = "power"
kind = "actuator"
data_type = PayloadValueType::BOOLEAN
access = "read_write"
status = RecordStatus::AVAILABLE
owner_device_index = device_result.index
```

Success criteria:

- Registry module count increases by one when relay is registered.
- Registry device count increases by one.
- Registry capability count increases by one.
- duplicate/full-table behavior remains unchanged.
- existing temperature, distance, servo, and motor registration still works.

Stop conditions:

- PNP Registration writes Registry arrays directly
- rollback complexity is required
- Registry behavior must change
- capability is registered with module-name policy

## Phase 7: RelayService

Purpose:

Create RelayService using bounded pending command execution.

Expected files:

- `src/services/relay/relay_service.h`
- `src/services/relay/relay_service.cpp`

Expected structs:

```text
RelayCommandRequest
RelayCommandResult
RelayPendingCommand
```

Expected request fields:

```text
bool enabled
uint32_t timeout_ms
```

Expected pending slot fields:

```text
bool occupied
bool enabled
uint32_t requested_at_ms
uint32_t timeout_ms
CommandState state
const char* error_code
```

Expected methods:

```text
begin(Registry* registry, SimRelayDevice* device)
attachRuntime(Runtime* runtime)
id() const
updateState(uint32_t now_ms)
setEnabled(uint32_t now_ms, const RelayCommandRequest& request, RelayCommandResult& out_result)
disable(uint32_t now_ms, RelayCommandResult& out_result)
executePendingCommand(uint32_t now_ms)
hasPendingCommand() const
clearPendingCommand()
lastRegistryResult() const
lastCommandState() const
```

Policy:

- ON requires Runtime `READY` or `RUNNING`.
- OFF is allowed in `READY`, `RUNNING`, and `SAFE_MODE`.
- `SAFE_MODE` blocks ON with `ERR_SAFE_MODE_BLOCKED`.
- missing Runtime blocks commands by default.
- invalid timeout fails before pending slot is occupied.
- pending ON may be replaced by OFF if documented and implemented.
- Device is called only from `executePendingCommand()`.
- failed ON command must not store ON payload.

Success criteria:

- RelayService stores accepted commands in one pending slot.
- RelayService writes command state to Registry.
- Runtime task execution performs Device calls.
- OFF is default and fail-safe.
- no real hardware is touched.
- no servo or motor behavior changes.

Stop conditions:

- Service calls Driver directly
- Service exposes API
- command queue grows beyond one slot
- Device is called during API request acceptance
- ON can execute outside Runtime task
- `SAFE_MODE` ON can reach Device

## Phase 8: API

Purpose:

Expose internal API methods for relay state and commands.

Expected files:

- `src/api/api_response.h`
- `src/api/cyber32_api.h`
- `src/api/cyber32_api.cpp`
- `src/services/relay/relay_service.h`

Expected API additions:

```text
attachRelayService(RelayService* service)
getRelayControlState(ApiCapabilityState& out_state)
commandRelayControl(uint32_t now_ms, const ApiRelayCommandRequest& request, ApiRelayCommandResponse& out_response)
commandRelayOff(uint32_t now_ms, ApiRelayCommandResponse& out_response)
getRelayCommandState(ApiCommandStateResponse& out_response)
```

Expected structs:

```text
ApiRelayCommandRequest
ApiRelayCommandResponse
```

Success criteria:

- API reads relay state from Registry.
- API reads relay command state from Registry.
- API commands call RelayService only.
- API does not call Device, Driver, HAL, or Runtime update.
- missing RelayService returns `ERR_ACTUATOR_UNAVAILABLE`.
- existing API behavior is preserved.

Stop conditions:

- API calls RelayDevice or RelayDriver
- API owns relay state
- HTTP/WiFi/WebServer is added
- Dashboard is added
- command transport expands scope

## Phase 9: Validation

Purpose:

Integrate relay into the validation harness and prove architecture behavior.

Expected files:

- `src/app/validation/vertical_slice_validation.h`
- `src/app/validation/vertical_slice_validation.cpp`

Expected validation objects:

```text
SimRelayDriver
SimRelayDevice
RelayService
```

Expected task contexts:

```text
RelayServiceStateTaskContext
RelayServiceCommandTaskContext
```

Expected Runtime tasks:

```text
task.relay_service.update_state
task.relay_service.execute_command
```

Validation cases:

- PNP discovers relay module.
- PNP Registration registers relay module/device/capability.
- Registry counts include relay.
- `CAP_RELAY_CONTROL` exists.
- initial relay state is OFF.
- API returns relay state.
- `READY` ON command is accepted.
- payload remains OFF before Runtime command task executes.
- Runtime command task executes ON.
- command state becomes `COMPLETED`.
- payload becomes ON.
- OFF command is accepted and executed through Runtime task.
- payload becomes OFF.
- invalid timeout fails with `ERR_COMMAND_INVALID_TIMEOUT`.
- driver failure fails with `ERR_ACTUATOR_EXECUTION_FAILED`.
- failed ON command does not store ON payload.
- `SAFE_MODE` blocks ON with `ERR_SAFE_MODE_BLOCKED`.
- `SAFE_MODE` allows OFF.
- `ERROR_STATE` blocks normal commands with `ERR_RUNTIME_NOT_READY`.
- pending command clears after completion/failure/timeout.

Success criteria:

- existing temperature, distance, servo, and motor validation is preserved.
- RelayService policy is proven through public API where possible.
- Runtime remains scheduler-only.
- Registry remains state-only.
- no real hardware is used.

Stop conditions:

- validation requires direct private Registry access
- validation requires API-to-Device bypass
- validation requires Runtime to inspect relay values
- existing actuator validation regresses
- `src/main.cpp` modification is required

## Phase 10: Architecture Audit

Purpose:

Review implemented relay slice against Cyber32 architecture and safety rules.

Expected file:

- `MILESTONE_5_ARCHITECTURE_AUDIT.md`

Audit sections:

- Passes
- Warnings
- Architecture violations
- Safety gaps before real hardware
- Recommended fixes
- Next steps
- Final assessment

Audit checks:

- `CAP_RELAY_CONTROL` is capability-first.
- API commands go through RelayService.
- RelayService owns command validation and safety.
- Runtime executes command task but remains policy-free.
- Registry stores payload and command state only.
- PNP does not call Device or Driver behavior.
- Device does not write Registry.
- Driver is simulated and hardware-free.
- `SAFE_MODE` blocks ON.
- `SAFE_MODE` allows OFF.
- OFF is default-safe and fail-safe.
- no dynamic allocation.
- no Arduino `String`.
- no STL containers.
- no real relay hardware.
- `src/main.cpp` remains untouched.

Success criteria:

- no critical architecture violations remain.
- any safety gaps are documented before real hardware.
- next implementation step is clear.

Stop conditions:

- audit finds layer bypasses
- audit finds unsafe ON path
- audit finds unbounded command storage
- audit finds Registry or Runtime owning relay policy
- audit finds real hardware behavior in simulated milestone

## Milestone 5 Definition Of Done

Milestone 5 is complete when:

- relay IDs exist
- simulated relay Driver exists
- simulated relay Device exists
- simulated relay Module exists
- PNP discovers relay
- PNP registers relay into Registry
- RelayService updates state and owns command policy
- Runtime executes relay commands through a bounded task
- API exposes relay state and commands through Service
- validation proves ON/OFF, Safe Mode, timeout, driver failure, command state, and payload safety
- architecture audit passes with no critical violations

## Global Stop Conditions

Stop Milestone 5 immediately if:

- relay ON can occur before Runtime `READY`
- relay ON can occur in `SAFE_MODE`
- failed ON command can store ON payload
- API calls Device or Driver directly
- Runtime knows `CAP_RELAY_CONTROL`
- Registry executes relay commands
- command storage becomes unbounded
- dynamic allocation is required
- Arduino `String` or STL containers are introduced
- real relay hardware is needed before simulated validation passes
