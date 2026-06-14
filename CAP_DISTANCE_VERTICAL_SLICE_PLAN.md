# CAP_DISTANCE Vertical Slice Plan

## Goal

Define the second Cyber32 capability implementation using the existing architecture.

This document does not introduce implementation code.

Capability:

```text
CAP_DISTANCE
```

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
-> Dashboard
```

Dashboard is not implemented in this slice.

## Scope

Included:

- simulated distance Driver
- simulated distance Device
- simulated distance Module metadata
- PNP discovery support
- PNP registration support
- Registry records and payload state
- Distance Service
- capability-first Logic example
- internal API exposure
- validation updates
- Runtime task updates

Explicitly excluded:

- servo logic
- motor logic
- actuator control
- WiFi
- WebServer
- Dashboard
- Mobile Studio
- OTA
- cloud
- real distance sensor

## Capability Definition

Capability ID:

```text
CAP_DISTANCE
```

Category:

```text
sensors
```

Kind:

```text
sensor
```

Direction:

```text
read
```

Datatype:

```text
float
```

Canonical unit:

```text
meter
```

Valid range:

```text
0.0 to 100.0
```

Recommended precision:

```text
0.001 meter
```

Recommended freshness window:

```text
1000 ms
```

Logic rule:

```text
Logic must query CAP_DISTANCE.
Logic must not query distance module names, device IDs, driver names, or friendly labels.
```

## Payload Schema

Payload fields:

```text
capability_id
schema_version
timestamp_ms
available
stale
value_type
value_float
value_int
unit
quality
error_code
```

Normal payload:

```text
capability_id = CAP_DISTANCE
schema_version = 1
timestamp_ms = now_ms
available = Availability::AVAILABLE
stale = StaleState::FRESH
value_type = PayloadValueType::FLOAT
value_float = simulated_distance_m
value_int = 0
unit = "meter"
quality = "valid"
error_code = "none"
```

Failure payload:

```text
capability_id = CAP_DISTANCE
schema_version = 1
timestamp_ms = now_ms
available = Availability::UNAVAILABLE
stale = StaleState::FRESH
value_type = PayloadValueType::NONE
value_float = 0.0
value_int = 0
unit = "meter"
quality = "unavailable"
error_code = ERR_DEVICE_TIMEOUT
```

Important rule:

```text
Logic must not treat unavailable distance as zero.
```

## Phase 1 Impact: Core IDs

Add one capability ID:

```text
CAP_DISTANCE
```

Do not add actuator IDs.

Do not add servo or motor IDs.

Error IDs can reuse existing:

```text
ERR_DEVICE_TIMEOUT
ERR_CAPABILITY_UNAVAILABLE
ERR_REGISTRY_FULL
```

Add service ID:

```text
SERVICE_DISTANCE
```

No new event ID is required for the happy path if existing events are reused:

```text
EVT_MODULE_DISCOVERED
EVT_CAPABILITY_REGISTERED
EVT_CAPABILITY_VALUE_UPDATED
EVT_ERROR_RAISED
```

## Simulated Distance Driver

Location:

```text
src/drivers/sensors/
```

Proposed files:

```text
sim_distance_driver.h
sim_distance_driver.cpp
```

Class:

```text
SimDistanceDriver
```

Methods:

```text
begin()
readDistance(float& out_meter)
setFailureMode(bool enabled)
```

Behavior:

- `begin()` marks the driver initialized.
- `readDistance(out_meter)` returns `true` and sets a deterministic value when initialized and not in failure mode.
- Recommended simulated value:

```text
1.25 meter
```

- `readDistance(out_meter)` returns `false` if not initialized or failure mode is enabled.
- `setFailureMode(true)` forces reads to fail.
- `setFailureMode(false)` restores normal reads.

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

## Simulated Distance Device

Location:

```text
src/devices/sensors/
```

Proposed files:

```text
sim_distance_device.h
sim_distance_device.cpp
```

Class:

```text
SimDistanceDevice
```

Constants:

```text
device_id = "device-sim-distance-001"
device_type = "sensor"
```

Methods:

```text
begin(SimDistanceDriver* driver)
readPayload(uint32_t now_ms, CapabilityPayload& out_payload)
id() const
type() const
```

Behavior:

- `begin(driver)` stores the driver pointer and returns false if null.
- `readPayload(now_ms, out_payload)` calls `driver->readDistance(distance_m)`.
- On success, fills a valid `CAP_DISTANCE` payload.
- On failure, fills an unavailable `CAP_DISTANCE` payload with `ERR_DEVICE_TIMEOUT`.

Device rules:

- Device does not register capabilities.
- Device does not write Registry.
- Device does not publish events.
- Device does not run Logic.
- Device does not expose API.
- Device does not know module names.
- No dynamic allocation.
- No Arduino `String`.
- No STL containers.

## Simulated Distance Module

Location:

```text
src/modules/sensing/
```

Proposed files:

```text
sim_distance_module.h
sim_distance_module.cpp
```

Class:

```text
SimDistanceModule
```

Constants:

```text
module_id = "module-sim-distance-001"
module_type = "sensing"
metadata_level = 1
display_name = "Simulated Distance Module"
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

- `deviceId()` returns `"device-sim-distance-001"`.
- `deviceType()` returns `"sensor"`.
- `capabilityId()` returns `CAP_DISTANCE`.

Module rules:

- Module is metadata/identity only.
- Module does not register itself.
- Module does not write Registry.
- Module does not publish events.
- Module does not run Logic.
- Module does not expose API.
- Module does not call Device methods.
- Display name is friendly metadata only.
- Logic must never depend on display name or module ID.

## PNP Discovery Impact

Current PNP discovery supports one simulated temperature module.

Add distance discovery without changing architecture.

Recommended method:

```text
discoverSimulatedDistanceModule(PnpModuleInfo& out_info)
```

Behavior:

1. Read identity and metadata from `SimDistanceModule`.
2. Fill `PnpModuleInfo`.
3. Validate required fields.
4. Validate `metadata_level == 1`.
5. Validate `capability_id` starts with `CAP_`.
6. Set `valid = true` on success.
7. Publish `EVT_MODULE_DISCOVERED` if EventBus is attached.
8. Return true on success.

PNP rules:

- PNP does not write Registry.
- PNP does not register modules directly.
- PNP does not run Logic.
- PNP does not expose API.
- PNP does not call Driver.
- PNP does not call Device behavior.
- PNP only reads Module metadata.

## PNP Registration Impact

Current PNP registration registers the temperature slice.

Add support for registering `CAP_DISTANCE`.

Recommended approach:

- keep `registerModuleInfo(const PnpModuleInfo& info)`
- create records based on `info.capability_id`
- support both:

```text
CAP_TEMPERATURE
CAP_DISTANCE
```

Registration order remains:

```text
ModuleRecord
DeviceRecord
CapabilityRecord
```

Distance initial payload:

```text
capability_id = CAP_DISTANCE
schema_version = 1
timestamp_ms = 0
available = Availability::AVAILABLE
stale = StaleState::STALE
value_type = PayloadValueType::NONE
value_float = 0.0
value_int = 0
unit = "meter"
quality = "stale"
error_code = "none"
```

Distance CapabilityRecord:

```text
capability_id = CAP_DISTANCE
category = "sensors"
kind = "sensor"
data_type = PayloadValueType::FLOAT
access = "read"
status = RecordStatus::AVAILABLE
owner_device_index = resolved distance device index
latest_payload = initial distance payload
```

First implementation warning:

```text
Do not keep hardcoded owner_device_index = 0 once both temperature and distance exist.
```

Registry ownership must resolve the correct device slot.

No rollback is required unless explicitly added in a later phase.

## Registry Impact

Registry already supports:

```text
MAX_MODULES = 8
MAX_DEVICES = 16
MAX_CAPABILITIES = 48
```

Adding distance should result in:

```text
module_count = 2
device_count = 2
capability_count = 2
```

Registered capabilities:

```text
CAP_TEMPERATURE
CAP_DISTANCE
```

Registry required behavior:

- reject duplicate `module_id`
- reject duplicate `device_id`
- reject duplicate `capability_id`
- store latest `CAP_DISTANCE` payload
- allow `getCapabilityPayload(CAP_DISTANCE, out_payload)`
- allow `updateCapabilityPayload(CAP_DISTANCE, payload)`
- emit `EVT_CAPABILITY_REGISTERED` on capability registration
- emit `EVT_CAPABILITY_VALUE_UPDATED` on payload update

Registry must not:

- discover distance hardware
- call distance Driver
- call distance Device
- run Logic
- expose API directly

## Distance Service Implementation

Location:

```text
src/services/distance/
```

Proposed files:

```text
distance_service.h
distance_service.cpp
```

Class:

```text
DistanceService
```

Methods:

```text
begin(Registry* registry, SimDistanceDevice* device)
update(uint32_t now_ms)
id() const
```

Service ID:

```text
SERVICE_DISTANCE
```

Behavior:

1. `begin()` stores Registry and Device pointers.
2. `begin()` returns false if either pointer is null.
3. `update(now_ms)` calls `device->readPayload(now_ms, payload)`.
4. `update(now_ms)` calls `registry->updateCapabilityPayload(CAP_DISTANCE, payload)`.
5. Return true only if Device read and Registry update both succeed.
6. If Device read fails, still update Registry with the unavailable/error payload if possible.

Service rules:

- Service may call Device.
- Service may update Registry through public API.
- Service owns distance update policy.
- Service does not run Logic.
- Service does not expose API.
- Service does not publish events directly if Registry emits payload update events.
- Service does not know module name.
- No dynamic allocation.
- No Arduino `String`.
- No STL containers.

## Logic Example

Location:

```text
src/logic/
```

Possible files:

```text
distance_logic.h
distance_logic.cpp
```

Example statuses:

```text
IDLE
DISTANCE_SEEN
DISTANCE_NEAR
DISTANCE_UNAVAILABLE
```

Behavior:

1. Query Registry using `CAP_DISTANCE`.
2. If query fails, status becomes `DISTANCE_UNAVAILABLE`.
3. If payload unavailable, status becomes `DISTANCE_UNAVAILABLE`.
4. If payload value type is not float, status becomes `DISTANCE_UNAVAILABLE`.
5. Store valid distance.
6. If distance is less than `0.30`, status becomes `DISTANCE_NEAR`.
7. Otherwise status becomes `DISTANCE_SEEN`.

Example rule:

```text
IF CAP_DISTANCE.value < 0.30
THEN status = DISTANCE_NEAR
```

Important:

```text
No servo or motor action is added.
```

This is observation-only Logic for the second sensor capability.

Logic rules:

- Logic queries `CAP_DISTANCE` only.
- Logic must not know module ID.
- Logic must not know device ID.
- Logic must not call Driver.
- Logic must not call Device.
- Logic must not call HAL.
- Logic must not expose API.
- Logic must not treat unavailable as zero.

## API Exposure

Current internal API exposes temperature state.

Add distance read methods without adding WiFi or WebServer.

Recommended method:

```text
getDistanceState(ApiCapabilityState& out_state)
```

Behavior:

- Query Registry for `CAP_DISTANCE`.
- On success:

```text
ok = true
payload = Registry payload
error_code = "none"
```

- On failure:

```text
ok = false
payload.capability_id = CAP_DISTANCE
payload.available = Availability::UNAVAILABLE
payload.stale = StaleState::STALE
payload.value_type = PayloadValueType::NONE
payload.unit = "meter"
payload.quality = "unavailable"
payload.error_code = ERR_CAPABILITY_UNAVAILABLE
error_code = ERR_CAPABILITY_UNAVAILABLE
```

API rules:

- API reads Registry state.
- API may read Runtime state.
- API does not call Device.
- API does not call Driver.
- API does not call HAL.
- API does not call Service directly.
- API does not own state.
- API does not use WiFi or WebServer.

## Validation Updates

Extend validation without removing the temperature checks.

Expected object additions:

```text
SimDistanceDriver
SimDistanceDevice
SimDistanceModule through PNP discovery
DistanceService
DistanceLogic
```

Updated validation sequence:

1. Initialize EventBus.
2. Initialize Registry.
3. Attach EventBus to Registry.
4. Initialize Runtime.
5. Attach EventBus and Registry to Runtime.
6. Initialize HAL time.
7. Initialize temperature Driver and Device.
8. Initialize distance Driver and Device.
9. Discover temperature Module through PNP.
10. Discover distance Module through PNP.
11. Register temperature Module through PNP Registration.
12. Register distance Module through PNP Registration.
13. Initialize Temperature Service.
14. Initialize Distance Service.
15. Initialize Temperature Logic.
16. Initialize Distance Logic.
17. Initialize internal API.
18. Register Runtime tasks.
19. Run Runtime update.
20. Query API temperature state.
21. Query API distance state.
22. Validate Registry, Logic, and API state.

Expected Registry counts:

```text
module_count = 2
device_count = 2
capability_count = 2
```

Expected payloads:

```text
CAP_TEMPERATURE.value = 22.4 degree_celsius
CAP_DISTANCE.value = 1.25 meter
```

Expected Logic:

```text
TemperatureLogic = TEMPERATURE_SEEN
DistanceLogic = DISTANCE_SEEN
```

If simulated distance is changed below `0.30`:

```text
DistanceLogic = DISTANCE_NEAR
```

Validation must confirm:

- `CAP_DISTANCE` exists.
- `CAP_DISTANCE` is available.
- `CAP_DISTANCE` value is `1.25`.
- API distance state is ok.
- Distance Logic does not know module name.
- Existing temperature validation still passes.

## Runtime Task Updates

Current Runtime task path includes:

```text
task.temperature_service.update
task.temperature_logic.evaluate
```

Add:

```text
task.distance_service.update
task.distance_logic.evaluate
```

Recommended task registration order:

1. `task.temperature_service.update`
2. `task.distance_service.update`
3. `task.temperature_logic.evaluate`
4. `task.distance_logic.evaluate`

Reason:

```text
All sensor Services update Registry before Logic evaluates.
```

Recommended periods:

| Task | Period |
|---|---:|
| Temperature Service Update | 1000 ms |
| Distance Service Update | 250 ms |
| Temperature Logic Evaluation | 1000 ms |
| Distance Logic Evaluation | 250 ms |

Distance uses a shorter period because robotics distance readings often need fresher updates.

Runtime rules:

- Runtime schedules tasks only.
- Runtime does not know distance policy.
- Runtime does not compare distance values.
- Runtime does not evaluate `CAP_DISTANCE`.
- Runtime does not call Distance Driver or Device directly.
- Runtime does not expose API.

## EventBus Updates

No new EventBus behavior is required for the second slice.

Expected additional events:

```text
EVT_MODULE_DISCOVERED for module-sim-distance-001
EVT_CAPABILITY_REGISTERED for CAP_DISTANCE
EVT_CAPABILITY_VALUE_UPDATED for CAP_DISTANCE
```

Event rules:

- events remain compact
- events do not include full metadata
- events do not include full payload blobs
- EventBus does not become Registry
- current state remains in Registry

## Error Path

Simulated error:

```text
SimDistanceDriver failure mode enabled
```

Flow:

```text
Driver read fails
-> Device fills unavailable CAP_DISTANCE payload
-> Service updates Registry with unavailable payload
-> Registry stores latest error_code
-> Registry emits EVT_CAPABILITY_VALUE_UPDATED
-> Logic sees unavailable CAP_DISTANCE
-> API returns CAP_DISTANCE unavailable state
```

Expected error code:

```text
ERR_DEVICE_TIMEOUT
```

Logic behavior:

```text
DistanceLogic = DISTANCE_UNAVAILABLE
```

API behavior:

```text
ok = true if Registry returns the unavailable payload
payload.available = UNAVAILABLE
payload.error_code = ERR_DEVICE_TIMEOUT
```

Missing capability behavior:

```text
ok = false
error_code = ERR_CAPABILITY_UNAVAILABLE
```

## Implementation Roadmap

### Phase D1 - Core IDs

Add:

```text
CAP_DISTANCE
SERVICE_DISTANCE
```

Validation:

- IDs use documented naming rules.
- No unrelated IDs added.

### Phase D2 - Simulated Distance Driver

Create:

```text
sim_distance_driver.h
sim_distance_driver.cpp
```

Validation:

- returns `1.25`
- supports failure mode
- no Registry/EventBus/Runtime/API dependency

### Phase D3 - Simulated Distance Device

Create:

```text
sim_distance_device.h
sim_distance_device.cpp
```

Validation:

- exposes `CAP_DISTANCE`
- emits valid and unavailable payloads
- no Registry writes
- no events

### Phase D4 - Simulated Distance Module

Create:

```text
sim_distance_module.h
sim_distance_module.cpp
```

Validation:

- metadata level `1`
- display name friendly only
- capability is `CAP_DISTANCE`
- no Device behavior calls

### Phase D5 - PNP Discovery

Update:

```text
PnpDiscovery
```

Add:

```text
discoverSimulatedDistanceModule(...)
```

Validation:

- validates required fields
- validates `CAP_` prefix
- publishes `EVT_MODULE_DISCOVERED`
- does not write Registry

### Phase D6 - PNP Registration

Update:

```text
PnpRegistration
```

Required change:

- create capability records based on `info.capability_id`
- create correct initial payload for distance
- resolve correct owner device index

Validation:

- temperature still registers
- distance registers
- counts become 2/2/2
- no direct Registry table writes

### Phase D7 - Distance Service

Create:

```text
distance_service.h
distance_service.cpp
```

Validation:

- updates `CAP_DISTANCE`
- writes Registry through public API
- handles failure payload
- no module name dependency

### Phase D8 - Distance Logic

Create:

```text
distance_logic.h
distance_logic.cpp
```

Validation:

- queries `CAP_DISTANCE`
- no module/device dependency
- unavailable is not zero
- status becomes `DISTANCE_SEEN` for `1.25`

### Phase D9 - API Distance State

Update:

```text
Cyber32Api
```

Add:

```text
getDistanceState(...)
```

Validation:

- API reads Registry only
- no Device/Driver/HAL/Service call
- missing capability returns `ERR_CAPABILITY_UNAVAILABLE`

### Phase D10 - Runtime Tasks

Update validation task coordination:

Add contexts and callbacks for:

```text
DistanceService
DistanceLogic
```

Register tasks:

```text
task.distance_service.update
task.distance_logic.evaluate
```

Validation:

- Runtime task count includes four tasks
- Services run before Logic
- Runtime contains no distance policy

### Phase D11 - Validation

Update vertical slice validation:

- keep temperature validation
- add distance validation
- validate counts `2/2/2`
- validate `CAP_DISTANCE` payload
- validate Distance Logic status
- validate API distance state
- validate EventBus expected events if exposed

## Stop Conditions

Stop implementation and review architecture if:

- Logic references `module-sim-distance-001`
- Logic references `device-sim-distance-001`
- API calls Distance Device directly
- Runtime evaluates distance thresholds
- Registry calls Distance Driver or Device
- PNP calls Driver or Device behavior
- EventBus stores distance state
- servo, motor, or actuator behavior is introduced
- WiFi or WebServer is introduced
- Dashboard or Mobile Studio implementation is introduced
- dynamic allocation is introduced
- `CAP_DISTANCE` implementation breaks `CAP_TEMPERATURE`

## Success Criteria

The `CAP_DISTANCE` vertical slice is complete when:

```text
one simulated distance module is discovered
one distance device is registered
CAP_DISTANCE is registered
Distance Service updates CAP_DISTANCE
Registry stores latest CAP_DISTANCE payload
Distance Logic queries CAP_DISTANCE
API returns CAP_DISTANCE state
Runtime schedules distance Service and Logic tasks
CAP_TEMPERATURE still passes
no layer violations occur
```

Expected final Registry state:

```text
modules = 2
devices = 2
capabilities = 2
```

Expected implemented capabilities:

```text
CAP_TEMPERATURE
CAP_DISTANCE
```
