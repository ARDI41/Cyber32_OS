# Cyber32 Milestone 2 Architecture Audit

## Scope

This audit reviews the current Milestone 2 implementation:

- `CAP_TEMPERATURE`
- `CAP_DISTANCE`
- Runtime task coordination
- Registry ownership
- PNP discovery and registration
- API state model
- Validation harness

This document is review-only. It does not define new implementation work beyond recommended fixes and future risks.

## Passes

### Capability-First Behavior

Both implemented capabilities follow the capability-first rule:

```text
CAP_TEMPERATURE
CAP_DISTANCE
```

Logic queries Registry by capability ID only.

Temperature Logic:

```text
Registry::getCapabilityPayload(CAP_TEMPERATURE, payload)
```

Distance Logic:

```text
Registry::getCapabilityPayload(CAP_DISTANCE, payload)
```

No Logic implementation depends on:

- module ID
- device ID
- display name
- driver class
- HAL
- API

### Layer Boundaries

Current Milestone 2 layer flow remains consistent:

```text
Driver
-> Device
-> Module metadata
-> PNP discovery
-> PNP registration
-> Registry
-> Runtime task scheduling
-> Services
-> Logic
-> API
-> Validation
```

Observed boundaries:

- Drivers do not know Registry, Runtime, EventBus, Logic, API, or modules.
- Devices wrap Drivers and produce capability payloads.
- Modules are metadata-only.
- PNP discovery reads module metadata only.
- PNP registration calls Registry public APIs.
- Services call Devices and update Registry through public APIs.
- Logic reads Registry capability payloads.
- API reads Registry state and Runtime state only.
- Runtime does not know capability IDs or capability-specific policy.

### Runtime Task Coordination

Runtime task coordination now covers both capabilities.

Registered validation task IDs:

```text
task.temperature_service.update
task.distance_service.update
task.temperature_logic.evaluate
task.distance_logic.evaluate
```

Runtime remains a scheduler:

- it stores fixed task slots
- it runs callbacks
- it updates task timestamps
- it processes bounded EventBus items

Runtime does not:

- evaluate temperature values
- evaluate distance thresholds
- know `CAP_TEMPERATURE`
- know `CAP_DISTANCE`
- call Drivers
- call Devices directly
- implement Service policy
- implement Logic policy

### Registry Ownership

Registry remains the source of truth for current capability state.

Registry stores:

- module records
- device records
- capability records
- latest payload per capability

Services update payload state through:

```text
Registry::updateCapabilityPayload(...)
```

Logic and API read state through:

```text
Registry::getCapabilityPayload(...)
```

Registry does not execute behavior, discover modules, run Logic, or expose API.

### PNP Registration

PNP registration now supports:

- `CAP_TEMPERATURE`
- `CAP_DISTANCE`

Registration still follows the required order:

```text
ModuleRecord
DeviceRecord
CapabilityRecord
```

PNP registration does not write Registry arrays directly. It uses:

```text
registerModule(...)
registerDevice(...)
registerCapability(...)
```

### API State Model

The internal API exposes capability state without transport concerns:

```text
getTemperatureState(...)
getDistanceState(...)
```

API reads Registry state only for capability payloads.

API does not:

- call Devices
- call Drivers
- call HAL
- call Services directly
- own payload state
- use WiFi
- use WebServer
- serialize JSON

### Validation Harness

The validation harness now proves:

- two modules discovered
- two devices registered
- two capabilities registered
- `CAP_TEMPERATURE` updated to `22.4`
- `CAP_DISTANCE` updated to `1.25`
- both Logic paths evaluate through capability IDs
- both API state calls return ok
- Runtime-controlled path runs Service and Logic callbacks through Runtime tasks

The direct validation path remains available, which is useful for comparing direct execution versus Runtime-scheduled execution.

## Architecture Violations

No direct architecture violation was found inside the Milestone 2 implementation.

Important qualification:

`src/main.cpp` still appears to contain legacy WiFi/WebServer/Servo behavior from earlier repository state. It was not modified during Milestone 2. If treated as the active Cyber32 v1 entrypoint, it is outside Milestone 2 scope and not aligned with the current architecture slice. If treated as legacy placeholder code, it is an integration blocker rather than a violation in the new slice code.

## Warnings

### PNP Registration Still Has Weak Ownership Resolution

PNP registration derives owner indexes from Registry counts after registration:

```text
registered_module_index = registry_->moduleCount() - 1
registered_device_index = registry_->deviceCount() - 1
```

This works for the current append-only validation path, but it is fragile.

Risk cases:

- duplicate registration leaves partial records
- future rollback changes counts
- removed records create holes
- multi-provider capabilities need explicit owner resolution
- Registry insertion policy changes from append-only to slot reuse

Recommended fix:

Registry registration APIs should eventually return a compact result with the registered slot index.

### PNP Registration Has No Rollback

If module registration succeeds but device or capability registration fails, partial records remain.

This was explicitly deferred earlier, but now that two capabilities exist, partial registration is a larger risk.

Recommended fix:

Before v1 completion, define one of:

- rollback support
- reserved/incomplete record states
- two-phase registration
- safe unavailable partial records

### Duplicate Capability Policy Is Too Strict For Future Providers

Registry rejects duplicate capability IDs.

This works for one temperature provider and one distance provider. It will not support multiple modules that provide the same capability, such as two distance sensors.

Recommended future model:

- Registry stores provider records separately from capability IDs.
- Capability queries resolve an active provider or return an ambiguity/degraded state.
- Logic still queries `CAP_DISTANCE`, not a specific module.

### Runtime Task Contexts Live In Validation Harness

Task context structs are currently embedded in `VerticalSliceValidation`.

This is acceptable for a validation harness, but production Runtime coordination will need a cleaner ownership model.

Risk:

- validation-specific task context patterns may leak into production design
- task callback timing depends on external context `now_ms`
- callback signature does not receive `now_ms` directly

Recommended fix:

Keep validation task contexts as test harness code. Before production boot integration, document a runtime task context ownership pattern or revise task callback timing carefully.

### Runtime Event Handling Still Discards Events

Runtime currently pops a bounded number of events and discards them.

This satisfies the minimal bounded event-processing requirement, but it does not yet implement event routing, priority handling, or error escalation.

Risk:

- capability update events are not consumed
- error events will not affect Runtime state
- future safety flows cannot rely on EventBus yet

Recommended fix:

Add a bounded event dispatch model later, without making EventBus a second Registry.

### API Failure Responses Fill Payloads

`getTemperatureState()` and `getDistanceState()` fill unavailable payloads when Registry lookup fails.

This is useful and bounded, but the API is constructing a payload state that did not come from Registry.

Risk:

- API may appear to own state
- future API consumers might confuse missing capability with registered unavailable capability

Recommended fix:

Keep `ok = false` and canonical `error_code`, but document the fallback payload as an API error envelope convenience. For full v1, distinguish:

- capability not registered
- capability registered but unavailable
- Registry unavailable

### Validation Harness Is Becoming A Coordinator

The validation harness currently wires everything:

- Drivers
- Devices
- PNP
- Registry
- Services
- Logic
- API
- Runtime tasks

This is acceptable for validation. It should not become the production application coordinator.

Recommended fix:

When moving toward real ESP32 boot, introduce a dedicated bootstrap/app coordination layer that follows `FIRST_VERTICAL_SLICE_BOOTSTRAP.md` and `RUNTIME_TASK_COORDINATION_PLAN.md`.

## Future Scalability Risks

### Multi-Provider Capability Selection

The biggest future architecture risk is multi-provider support.

Examples:

- two distance sensors
- multiple temperature sources
- redundant battery monitors
- simulated and real providers active together

Current model:

```text
capability_id is unique in Registry
```

Future need:

```text
capability_id can have multiple providers
Registry tracks providers
Service or policy selects active provider
Logic remains bound to capability ID
```

Do not solve this by making Logic depend on module or device names.

### Service To Concrete Device Coupling

Current services are tied to simulated device classes:

```text
TemperatureService -> SimTemperatureDevice
DistanceService -> SimDistanceDevice
```

This is acceptable for simulated vertical slices. It will not scale cleanly to real hardware.

Future direction:

- define a capability provider interface
- or let Services resolve provider handles through Registry
- keep concrete device classes below the Service boundary

### Registry Result Semantics

Registry APIs mostly return `bool`.

This limits diagnostics:

- duplicate ID
- full table
- invalid record
- missing capability
- unavailable capability

Future direction:

Add compact result/status codes while preserving ESP32 memory limits.

### Fixed String IDs

The implementation uses stable `const char*` IDs.

This is fine for the current slice, but v1 should define string lifetime rules before accepting metadata from EEPROM or I2C sources.

Future direction:

- boundary strings remain stable
- Registry may map IDs to compact indexes
- avoid storing pointers to transient metadata buffers

### Validation Equality On Floats

Validation checks exact values:

```text
22.4F
1.25F
```

This is acceptable for deterministic simulated drivers. Real sensors should use tolerance checks.

Future direction:

- keep exact checks for simulated drivers
- use documented precision tolerance for real providers

## Multi-Provider Risks

### Duplicate Capability Rejection

Current duplicate capability rejection means a second `CAP_DISTANCE` provider cannot register.

This blocks common robotics layouts such as:

- front distance sensor
- rear distance sensor
- left/right proximity sensors

Future architecture question:

Should these be separate capabilities, indexed providers, or roles attached to one capability?

Recommended approach:

- keep `CAP_DISTANCE` as the canonical capability type
- add provider role or position metadata later
- keep Logic rules capability-first

Example future-safe Logic:

```text
IF CAP_DISTANCE(role=front).value < 0.30
THEN request safe stop
```

Do not implement this yet.

### Owner Index Fragility

Owner indexes are currently count-derived in PNP registration.

This becomes risky when:

- registration is not append-only
- failed records remain
- records can be removed
- hot-plug appears
- multiple providers of one capability exist

Future fix:

Registry should return the assigned index on successful registration.

### Provider Selection Policy Is Missing

There is no policy for:

- active provider
- preferred provider
- fallback provider
- degraded provider
- simulated vs real provider

This is acceptable now, but must be solved before real multi-sensor robotics use.

## Actuator Readiness Gaps

No actuator behavior was added in Milestone 2, which is correct.

However, the architecture is not yet ready to safely add actuators until these gaps are closed:

### Command Dispatch Is Not Implemented

Actuators require the command flow from `COMMAND_DISPATCH_CONTRACT.md`:

```text
API / Logic
-> Services
-> Runtime coordination
-> Devices
-> Drivers
-> HAL
```

Current implementation only covers read capabilities.

### Safety Policy Is Not Enforced In Code

`ACTUATOR_SAFETY_POLICY.md` exists, but no runtime enforcement has been implemented.

Before adding actuator capabilities:

- boot-safe defaults must exist
- safe mode must exist
- command rejection must exist
- watchdog handling must exist
- module loss handling must exist

### Runtime Safe Mode Is Not Implemented

Runtime state includes a compact subset but does not yet implement:

- degraded mode
- safe mode
- recovery mode
- fatal error transitions
- watchdog boot-loop handling

Do not add actuator command execution until this is designed and implemented.

### Error Model Is Still Minimal

Actuator safety requires stronger errors:

- severity
- recoverability
- safe action
- target reference
- repeated failure tracking

Current errors are compact string IDs only.

### EventBus Priority Is Not Safety-Ready

EventBus is FIFO and rejects when full.

Actuator safety will require:

- critical event preservation
- priority-aware processing
- overload/error reporting
- bounded dispatch

## Recommended Fixes

### Near-Term Fixes

1. Add a real compile/build validation path once PlatformIO is available.
2. Keep `src/main.cpp` untouched until explicitly replacing or guarding legacy behavior.
3. Add Registry registration result indexes before more providers are added.
4. Add explicit Registry result/status codes.
5. Document the validation harness as non-production coordination.

### Before More Sensor Capabilities

1. Decide provider ownership model.
2. Decide duplicate capability policy.
3. Add metadata reference rules for provider role/position if needed.
4. Keep Logic capability-first.

### Before Actuators

1. Implement command dispatch contract.
2. Implement actuator safety policy enforcement.
3. Implement Runtime safe/degraded/recovery states.
4. Implement priority-aware EventBus behavior.
5. Implement stronger error state storage.

## Final Assessment

Milestone 2 remains architecturally valid.

The system now proves two independent read capabilities:

```text
CAP_TEMPERATURE
CAP_DISTANCE
```

Both are discovered, registered, updated by Services, stored in Registry, evaluated by Logic, exposed by API, and validated through Runtime task coordination.

No source-level architecture violation was found in the Milestone 2 slice.

The main risks are expected next-stage architecture gaps:

- multi-provider capability ownership
- Registry result semantics
- validation harness becoming too coordinator-like
- EventBus not yet safety-ready
- actuator command/safety model not yet implemented

Recommended next step:

```text
Run real PlatformIO compile validation, then address Registry registration result indexes before adding additional providers or actuator capabilities.
```
