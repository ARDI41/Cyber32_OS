# Cyber32 Production Bootstrap Plan

## Goal

Define the official Cyber32 production bootstrap process before additional capabilities are added.

Cyber32 remains capability-first, documentation-first, ESP32 v1 bounded, and fixed-layer. This document does not introduce new architecture.

## Reviewed Documents

- `ARCHITECTURE.md`
- `BOOT_SEQUENCE.md`
- `PNP_DISCOVERY_FLOW.md`
- `CAPABILITY_CATALOG.md`
- `MILESTONE_4_1_ARCHITECTURE_AUDIT.md`
- `MILESTONE_4_2_RUNTIME_TRANSITION_SAFETY_PLAN.md`
- `MILESTONE_4_3_ERROR_CATALOG_PLAN.md`

## Fixed Production Startup Order

Production boot must follow this order exactly:

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

Dashboard remains a future user-facing layer and must use API only.

## 1. System Startup Order

### 1. HAL

Initialize hardware/platform abstractions first.

Expected production objects:

- time
- GPIO
- I2C
- SPI
- UART
- ADC
- PWM abstraction, when actuators need it
- storage abstraction, when metadata persistence needs it
- watchdog/brownout abstraction, when safe boot is implemented

Rules:

- HAL must not know Drivers, Devices, Modules, PNP, Registry, Runtime, Services, Logic, API, or Dashboard.
- HAL exposes platform services only.
- HAL must not use module names or capability policy.

### 2. Drivers

Create and initialize Driver instances after HAL exists.

Rules:

- Drivers may depend on HAL.
- Drivers must not register capabilities.
- Drivers must not write Registry.
- Drivers must not publish user-facing API state.
- Drivers must not run Logic.
- Drivers must not know module names.

### 3. Devices

Create Device instances and attach required Drivers.

Rules:

- Devices wrap Drivers.
- Devices expose compact capability payloads.
- Devices perform direct execution only when called by owning Services.
- Devices do not register themselves.
- Devices do not write Registry.
- Devices do not publish events directly for capability changes.

### 4. Modules

Create Module metadata providers.

Rules:

- Modules provide identity and metadata.
- Modules may reference Device identity and Capability IDs.
- Modules do not call Device behavior.
- Modules do not write Registry.
- Modules do not run Logic.
- Friendly names are metadata only.

### 5. PNP

Run PNP discovery and validation.

Rules:

- PNP reads Module metadata.
- PNP validates required fields.
- PNP assigns discovered module information.
- PNP does not call Driver or Device behavior.
- PNP registration uses Registry public APIs only.

### 6. Registry

Initialize Registry before PNP registration writes to it.

Rules:

- Registry stores facts and latest state only.
- Registry stores Modules, Devices, Capabilities, Services, payloads, errors, and command state.
- Registry does not discover hardware.
- Registry does not execute commands.
- Registry does not run Logic.
- Registry does not own Dashboard decisions.

### 7. Runtime

Initialize Runtime and attach Registry/EventBus if used.

Rules:

- Runtime coordinates execution.
- Runtime schedules bounded tasks.
- Runtime exposes system state.
- Runtime may enter or exit Safe Mode.
- Runtime does not know capability IDs.
- Runtime does not own actuator policy.
- Runtime does not call Devices or Drivers directly.

### 8. Services

Initialize Services after Registry, Devices, and Runtime exist.

Rules:

- Services own capability update policy.
- Services own command validation and actuator safety policy.
- Services may call Devices.
- Services update Registry through public APIs.
- Services may read Runtime state for gating.
- Services do not expose API directly.
- Services do not know module names.

### 9. Logic

Initialize Logic after Registry and Services are ready.

Rules:

- Logic queries capabilities by `CAP_*` ID only.
- Logic never depends on module names, device IDs, driver names, or display names.
- Logic requests commands through capability/service/API-safe paths only.
- Logic must not call Devices, Drivers, or HAL.

### 10. API

Initialize API last among production core layers.

Rules:

- API reads Registry state.
- API reads Runtime state for system status.
- API sends commands through Services only.
- API does not call Devices, Drivers, or HAL.
- API does not own state.
- API keeps capability IDs canonical.

## 2. Driver Registration Pattern

Driver creation is static or fixed-slot.

Pattern:

```text
create Driver object
call driver.begin()
verify initialized state
keep Driver owned by bootstrap/application context
pass Driver pointer to Device begin()
```

Driver rules:

- no dynamic allocation
- no Registry dependency
- no EventBus dependency
- no Runtime dependency
- no API dependency
- no module-name dependency

Driver failure:

- Driver method returns `false`
- Device converts failure into payload error
- Service converts execution failure into command-state error when applicable

## 3. Device Registration Pattern

Device creation happens after Driver initialization.

Pattern:

```text
create Device object
call device.begin(&driver)
Device stores Driver pointer
Device exposes readPayload() or command execution methods
Module metadata references device.id() and device.type()
```

Device rules:

- Device may call Driver.
- Device must not write Registry.
- Device must not publish capability update events.
- Device must not run Logic.
- Device must not expose API.
- Device must not know module names.

## 4. Module Registration Pattern

Module objects provide metadata only.

Pattern:

```text
create Module metadata object
PNP reads module.id()
PNP reads module.type()
PNP reads module.metadataLevel()
PNP reads module.deviceId()
PNP reads module.deviceType()
PNP reads module.capabilityId()
```

Module rules:

- no Device method calls
- no Registry writes
- no Service policy
- no Logic
- no API
- no hardware access

## 5. Capability Registration Pattern

Capability registration flows through PNP Registration into Registry.

Pattern:

```text
PNP discovers module metadata
PNP validates metadata
PNP Registration creates ModuleRecord
PNP Registration creates DeviceRecord
PNP Registration creates CapabilityRecord
Registry stores records in fixed slots
Registry emits capability registered event if EventBus is attached
```

Capability rules:

- `capability_id` must be canonical `CAP_*`
- payload schema must be defined before implementation
- initial payload should be stale or unavailable until first Service update
- Registry must reject duplicates
- Registry must reject full tables
- Registry must return compact result codes

## 6. Runtime Task Registration Pattern

Runtime tasks are registered after Services and Logic are initialized.

Pattern:

```text
create fixed task context
fill service or logic pointer
fill now_ms field when needed
create RuntimeTask
assign task_id
assign period_ms
assign callback
assign context
runtime.registerTask(task)
```

Task rules:

- bounded task count
- no dynamic task creation after boot
- callbacks call Services or Logic
- Runtime does not inspect capability payloads
- Runtime does not implement Service policy
- Runtime does not call Device or Driver directly

Recommended task ID style:

```text
task.<service_or_logic_name>.<action>
```

Examples:

```text
task.temperature_service.update
task.distance_service.update
task.motor_service.update_state
task.motor_service.execute_command
task.temperature_logic.evaluate
```

## 7. Service Registration Pattern

Services are initialized after Registry and their required Devices exist.

Pattern:

```text
create Service object
service.begin(&registry, &device)
service.attachRuntime(&runtime) for actuator services
api.attach<Service>(&service) when API commands are supported
register Runtime task for service update or command execution
```

Service rules:

- Services own command validation.
- Services own actuator safety policy.
- Services write payload state through Registry public APIs.
- Services write command state through Registry public APIs.
- Services do not publish events directly when Registry already emits state changes.
- Services do not know module names.

## 8. Validation Registration Pattern

Validation harnesses prove architecture compliance before production wiring moves into `main.cpp`.

Pattern:

```text
create all fixed objects
initialize in production startup order
register PNP-discovered modules
attach Services to API
register Runtime tasks
run service updates through Runtime
run command execution through Runtime
query API and Registry public APIs
assert layer boundaries
```

Validation rules:

- no direct private Registry access
- no API-to-Device shortcuts
- no Runtime-to-Device shortcuts
- no Logic-to-module or Logic-to-device dependency
- payload safety must be verified after every failed actuator command
- command-state records must be verified through public API where possible

## 9. New Capability Implementation Checklist

Before implementation:

- document capability ID in catalog
- document payload schema
- document Registry initial payload
- document Service ownership
- document API exposure
- document Runtime task impact
- document validation checks

Implementation order:

1. Add `CAP_*` ID.
2. Add Service ID if needed.
3. Add shared type only if required.
4. Add Driver.
5. Add Device.
6. Add Module metadata.
7. Add PNP discovery support.
8. Add PNP registration support.
9. Add Service.
10. Add Logic only if needed.
11. Add API state or command support.
12. Add Runtime task integration.
13. Add validation.
14. Audit architecture.

Completion checks:

- Registry stores the capability.
- Service updates payload state.
- API exposes state.
- Logic uses capability ID only.
- EventBus remains transport only.
- no dynamic allocation is introduced.

## 10. New Actuator Implementation Checklist

Before implementation:

- document actuator capability
- document command schema
- document command-state behavior
- document safety policy
- document Runtime gating
- document Safe Mode behavior
- document timeout behavior
- document fallback behavior
- document precise error IDs

Implementation order:

1. Add `CAP_*` ID.
2. Add `SERVICE_*` ID.
3. Add command/shared actuator types.
4. Add simulated Driver first.
5. Add Device wrapper.
6. Add Module metadata.
7. Add PNP discovery and registration.
8. Add Service state update.
9. Add Service command validation.
10. Add pending command slot if execution must be Runtime-controlled.
11. Add Runtime execution task.
12. Add API state and command methods.
13. Add command-state API reads.
14. Add validation for success, invalid command, timeout, Runtime blocked states, Safe Mode, driver failure, and payload safety.

Actuator rules:

- API commands go through Service only.
- Service owns validation and safety.
- Device executes only when Service calls it.
- Driver remains hardware-specific and policy-free.
- Runtime schedules execution only.
- Registry stores state only.
- Safe Mode must block unsafe motion.
- STOP/emergency commands must be explicitly documented before implementation.

## 11. New Sensor Implementation Checklist

Before implementation:

- document sensor capability
- document units and valid range
- document stale/unavailable behavior
- document update period
- document API exposure

Implementation order:

1. Add `CAP_*` ID.
2. Add Driver.
3. Add Device payload wrapper.
4. Add Module metadata.
5. Add PNP discovery and registration.
6. Add Service update.
7. Add Logic query only if needed.
8. Add API state method.
9. Add Runtime update task.
10. Add validation for normal value, unavailable value, Registry state, API state, and Logic capability-only query.

Sensor rules:

- Service updates Registry.
- Logic reads capability payloads.
- unavailable is never treated as zero.
- stale state is explicit.
- units are canonical.

## 12. Production Stop Conditions

Stop implementation and review architecture if:

- layer order would be bypassed
- Registry would execute behavior
- Runtime would own Service policy
- Logic would depend on module names
- API would call Devices or Drivers directly
- Dashboard would bypass API
- EventBus would become state storage
- dynamic allocation is required in v1 core paths
- Arduino `String` is introduced in core paths
- STL containers are introduced in core paths
- unbounded queues are required
- actuator command failure can store unsafe payload state
- Safe Mode motion can reach Device
- Runtime reset can replay a pending command
- real hardware work is needed before simulated validation passes
- `src/main.cpp` must be modified before bootstrap validation exists

## 13. Architecture Rules That Must Never Be Violated

- Fixed layer order must remain unchanged.
- Capability IDs are canonical.
- Logic must never depend on module names.
- API must never bypass Services for commands.
- Registry must never execute commands.
- Runtime must never own actuator policy.
- EventBus must never become a second Registry.
- Services must not know module display names.
- Devices must not register capabilities.
- Drivers must not know Registry or API.
- Dashboard and future Mobile Studio must use API only.
- Safety-critical actuator errors must fail safe.
- Motion outputs must not activate before Runtime allows them.

## 14. Recommended Folder Structure

Recommended production structure:

```text
src/
  api/
  app/
    validation/
    production/
  core/
    event_bus/
    ids/
    types/
  devices/
    actuators/
    communication/
    display/
    sensors/
  drivers/
    actuators/
    communication/
    display/
    sensors/
    storage/
  hal/
    gpio/
    i2c/
    pwm/
    spi/
    storage/
    time/
    uart/
  logic/
  modules/
    actuators/
    communication/
    display/
    sensing/
    storage/
  pnp/
  registry/
  runtime/
  services/
    distance/
    motor/
    servo/
    temperature/
```

Production bootstrap files, when introduced, should live under:

```text
src/app/production/
```

Validation-only wiring should remain under:

```text
src/app/validation/
```

## 15. Future Roadmap Integration

Future capabilities must follow the same bootstrap and checklist sequence.

### Relay

Likely category:

```text
actuator / power
```

Requirements before implementation:

- `CAP_RELAY_CONTROL` or equivalent documented capability
- fail-safe default state
- Runtime gating
- Safe Mode behavior
- command-state record
- validation for blocked activation

### Stepper

Likely category:

```text
motion / actuator
```

Requirements before implementation:

- position/speed command schema
- acceleration limits
- emergency stop policy
- Safe Mode stop/hold behavior
- timeout and pending command design
- no real hardware before simulated validation

### GPS

Likely category:

```text
position / sensor
```

Requirements before implementation:

- canonical position payload
- fix quality fields
- stale timeout
- unavailable state
- API state exposure
- Logic uses `CAP_POSITION`, not GPS module names

### Display

Likely category:

```text
display / actuator-output
```

Requirements before implementation:

- `CAP_DISPLAY_TEXT` command schema
- bounded string length
- no dynamic allocation
- API command through Service
- Dashboard-friendly metadata without core dependency

### Audio

Likely category:

```text
audio / actuator-output
```

Requirements before implementation:

- `CAP_AUDIO_OUTPUT` command schema
- bounded tone/sample identifiers
- Safe Mode mute behavior
- timeout behavior
- API command through Service

### Camera

Likely category:

```text
sensor / media
```

Requirements before implementation:

- ESP32 memory budget review
- frame size limits
- no large image blobs in EventBus
- Registry stores only compact state
- API response limits
- likely v2+ unless ESP32 target is explicitly scoped

### Wireless

Likely category:

```text
communication
```

Requirements before implementation:

- WiFi stability plan
- API transport constraints
- reconnect behavior
- safe actuator behavior during disconnect
- no Dashboard coupling
- bounded request and response sizes

## Onboarding Checklist

New contributor checklist:

- Read `ARCHITECTURE.md`.
- Read `BOOT_SEQUENCE.md`.
- Read `CAPABILITY_CATALOG.md`.
- Read `CAPABILITY_PAYLOAD_SCHEMA.md`.
- Read `COMMAND_DISPATCH_CONTRACT.md` before actuator work.
- Read `ACTUATOR_SAFETY_POLICY.md` before actuator work.
- Read current milestone audit before modifying code.
- Confirm the fixed layer order.
- Confirm capability-first behavior.
- Confirm ESP32 bounded memory constraints.
- Confirm no `src/main.cpp` changes are needed for validation-only work.
- Add or update documentation before code.

## Architecture Compliance Checklist

Before merging a production bootstrap or new capability:

- Startup follows HAL -> Drivers -> Devices -> Modules -> PNP -> Registry -> Runtime -> Services -> Logic -> API.
- Driver has no Registry, Runtime, Logic, API, or module-name dependency.
- Device has no Registry, Runtime, Logic, API, or module-name dependency.
- Module is metadata/identity only.
- PNP does not call Driver or Device behavior.
- PNP Registration uses Registry public APIs only.
- Registry stores state only.
- Runtime schedules tasks only.
- Service owns update or command policy.
- Logic uses capability IDs only.
- API reads Registry and sends commands through Services.
- EventBus transports compact events only.
- command-state records are bounded.
- payloads are compact and schema-compatible.
- errors use stable `ERR_*` IDs.
- no dynamic allocation was introduced.
- no Arduino `String` was introduced.
- no STL containers were introduced.
- no real hardware was added before simulated validation.

## Production Bootstrap Success Criteria

Production bootstrap is ready to move toward `src/app/production/` when:

- the validation harness proves the same startup order
- all current simulated capabilities register through PNP and Registry
- all Runtime tasks are bounded and registered explicitly
- all Services update or execute through public layer contracts
- all actuator commands preserve safe payloads on failure
- API exposes state and command paths without bypasses
- architecture audit has no critical violations
- `src/main.cpp` integration can be limited to calling production bootstrap begin/update methods
