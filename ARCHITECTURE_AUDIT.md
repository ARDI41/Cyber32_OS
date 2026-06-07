# Cyber32 First Vertical Slice Architecture Audit

## Scope

This audit reviews the implemented Cyber32 first vertical slice against the current architecture documents.

Reviewed implementation areas:

- `src/core/ids`
- `src/core/types`
- `src/core/event_bus`
- `src/hal/time`
- `src/drivers/sensors`
- `src/devices/sensors`
- `src/modules/sensing`
- `src/pnp`
- `src/registry`
- `src/runtime`
- `src/services/temperature`
- `src/logic`
- `src/api`
- `src/app/validation`

Reviewed architecture documents:

- `ARCHITECTURE.md`
- `FIRST_VERTICAL_SLICE_PLAN.md`
- `IMPLEMENTATION_ORDER.md`
- `FIRST_VERTICAL_SLICE_BOOTSTRAP.md`
- `EVENT_MODEL.md`
- `REGISTRY_IMPLEMENTATION_PLAN.md`
- `RUNTIME_IMPLEMENTATION_PLAN.md`
- `CAPABILITY_PAYLOAD_SCHEMA.md`
- `API_V1_CONTRACT.md`
- `MEMORY_MODEL.md`
- `ESP32_V1_SCOPE.md`

## Passes

### Layer Boundaries

The implemented first vertical slice follows the intended layer order:

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

Observed boundary compliance:

- HAL time does not depend on Registry, Runtime, Logic, API, Devices, Modules, or PNP.
- Driver does not register capabilities, publish events, write Registry state, expose API, or run Logic.
- Device wraps Driver behavior and produces a capability payload without registering itself.
- Module is metadata-only and does not call Device behavior.
- PNP discovery reads Module metadata and does not call Driver or Device behavior.
- PNP registration uses public Registry registration APIs.
- Registry stores records and latest payload state.
- Runtime owns task slots, runtime state, and bounded event pumping.
- Temperature Service calls Device and updates Registry through public API.
- Logic queries Registry by `CAP_TEMPERATURE`.
- API reads Registry and Runtime only.
- Validation harness uses public APIs and does not access private Registry or Runtime internals.

### Dependency Directions

Dependency direction is mostly consistent with the architecture:

- Lower layers do not include higher-layer headers.
- Logic does not include Module, Device, Driver, HAL, Service, or API headers.
- API does not include Device, Driver, HAL, or Service headers.
- EventBus does not know Registry or Runtime.
- Registry optionally knows EventBus only for compact state-change announcements.

### Capability-First Compliance

The slice is capability-first where it matters most:

- The only implemented capability is `CAP_TEMPERATURE`.
- Logic queries `CAP_TEMPERATURE` from Registry.
- Logic does not know `module-sim-temperature-001`.
- Logic does not know `device-sim-temperature-001`.
- API exposes temperature state from Registry using `CAP_TEMPERATURE`.
- Payloads use canonical fields: `capability_id`, `schema_version`, `timestamp_ms`, `available`, `stale`, `value_type`, `unit`, `quality`, and `error_code`.

### Registry Responsibilities

The Registry implementation respects the core rule that Registry stores state only:

- Fixed-size tables are used.
- Duplicate module, device, and capability IDs are rejected.
- Full tables are rejected.
- Registry does not discover hardware.
- Registry does not call Drivers or Devices.
- Registry does not run Logic.
- Registry does not expose API.
- Registry emits compact capability events through optional EventBus attachment.

### Service Responsibilities

The Temperature Service owns the temperature update policy for the slice:

- It calls `SimTemperatureDevice::readPayload()`.
- It updates Registry through `Registry::updateCapabilityPayload()`.
- It updates Registry even when the Device reports an unavailable/error payload.
- It does not publish events directly, relying on Registry payload update events.
- It does not know module names.
- It does not expose API.
- It does not run Logic.

### Logic Responsibilities

Temperature Logic is correctly constrained:

- It uses `Registry::getCapabilityPayload(CAP_TEMPERATURE, ...)`.
- It checks `Availability::AVAILABLE`.
- It checks `PayloadValueType::FLOAT`.
- It does not treat unavailable values as zero.
- It sets `TEMPERATURE_UNAVAILABLE` on missing, unavailable, or wrongly typed payloads.
- It stores the last valid temperature only after payload validation.

### API Responsibilities

The internal API surface is appropriately minimal for the first slice:

- It reads Registry state.
- It reads Runtime state.
- It does not call Device, Driver, HAL, or Service directly.
- It does not own state.
- It does not implement WiFi or WebServer.
- It returns bounded structs instead of dynamic JSON.

### EventBus Responsibilities

The EventBus satisfies the first bounded transport requirement:

- Fixed queue size: 32.
- No dynamic allocation.
- FIFO transport.
- Rejects new events when full.
- Tracks dropped event count.
- Does not store Registry state.
- Does not execute behavior.

### ESP32 Memory Discipline

The implemented slice is mostly ESP32-friendly:

- No `std::vector`, `std::map`, or `std::queue` in the slice implementation.
- No Arduino `String` in the slice implementation.
- No dynamic allocation in the slice implementation.
- Registry, Runtime, and EventBus use fixed arrays.
- Payloads and events are compact structs.

## Warnings

### Runtime Does Not Yet Coordinate The Slice

Documentation says Runtime coordinates startup, scheduling, Service updates, Logic evaluation, and API servicing.

Current implementation:

- Runtime has fixed task slots and bounded event pumping.
- The validation harness directly calls PNP, Service, Logic, and API setup.
- The validation harness directly calls `temperature_service.update()` and `temperature_logic.evaluate()`.

This is acceptable for a controlled Phase 14 validation harness, but it is not yet the production Runtime execution model described by `RUNTIME_IMPLEMENTATION_PLAN.md`.

### Runtime State Is Not Advanced Through Boot States

Documentation describes explicit states:

```text
BOOTING
INITIALIZING
DISCOVERING
REGISTERING
STARTING
READY
RUNNING
```

Current implementation:

- `Runtime::begin()` sets `BOOTING`.
- The validation harness does not drive Runtime through `DISCOVERING`, `REGISTERING`, `STARTING`, `READY`, or `RUNNING`.

This differs from `BOOT_SEQUENCE.md`, `RUNTIME_IMPLEMENTATION_PLAN.md`, and `FIRST_VERTICAL_SLICE_BOOTSTRAP.md`.

### EventBus Priority Rules Are Not Implemented Yet

Documentation requires priority-aware behavior and specific overflow semantics:

- preserve critical events
- preserve high-priority safety events
- coalesce low-priority repeated updates
- emit or retain overload/error events where possible

Current implementation:

- FIFO queue only.
- Full queue rejects new event.
- Dropped counter increments.
- No priority preservation.
- No coalescing.
- No overload event.

This is acceptable for the first minimal transport but differs from `EVENT_MODEL.md`.

### Event Record Is Smaller Than Documented Event Model

Documentation includes fields such as:

- category
- status
- compact event value union

Current `EventRecord` includes:

- `event_id`
- `priority`
- `timestamp_ms`
- `source_layer`
- `source_id`
- `target_id`
- `value_type`
- `value_float`
- `value_int`

This is compact and ESP32-friendly, but it does not fully match the documented event record contract.

### Registry Is First-Slice Minimal, Not Full V1 Registry

Documentation describes tables for:

- modules
- devices
- capabilities
- services
- errors
- metadata references

Current Registry implements only:

- modules
- devices
- capabilities
- latest capability payload inline in capability record

Missing from implementation:

- service table
- error table
- metadata reference table
- latest error indexes
- pnp level field
- created/updated timestamps
- record validation beyond duplicate/full checks

This is acceptable for the first vertical slice but must be closed before claiming full Cyber32 v1 Registry compliance.

### Registry Does Not Return Canonical Error Codes

Documentation says Registry failures should map to errors such as:

- `ERR_REGISTRY_FULL`
- duplicate ID errors
- metadata invalid errors

Current Registry APIs return `bool`.

This keeps the slice compact, but callers cannot distinguish full table, duplicate ID, missing capability, or invalid record without future error reporting.

### PNP Registration Uses Hardcoded Owner Index

Current PNP registration sets:

```text
owner_device_index = 0
module_index = 0
```

This is valid for the single-module, single-device first slice, but it will break once more devices or modules are registered.

### PNP Discovery Event Timestamp Is Zero

PNP discovery emits `EVT_MODULE_DISCOVERED` with:

```text
timestamp_ms = 0
```

The event model expects event timestamps to use system uptime. This is acceptable before HAL time is threaded into PNP, but differs from the documented event timing model.

### Service Does Not Resolve Provider Through Registry

`FIRST_VERTICAL_SLICE_PLAN.md` says the Service should query Registry for the `CAP_TEMPERATURE` provider.

Current implementation:

- `TemperatureService::begin()` receives a direct `SimTemperatureDevice*`.
- `TemperatureService::update()` calls that device directly.

This was explicitly allowed during Phase 11, but it is a coupling risk as soon as multiple providers or real PNP devices exist.

### API Is Internal Struct Surface, Not Full API Contract

Current API implements:

- `getSystemStatus()`
- `getTemperatureState()`

It does not implement:

- `GET /capabilities`
- `GET /capabilities/{id}`
- `GET /events`
- `GET /errors`
- `GET /registry/summary`
- request IDs
- schema wrappers
- bounded serialization
- authentication

This is correct for a minimal internal API surface, but not yet the full `API_V1_CONTRACT.md`.

### Error Model Is Only Partially Integrated

Current implementation uses:

- `ERR_DEVICE_TIMEOUT`
- `ERR_REGISTRY_FULL`
- `ERR_CAPABILITY_UNAVAILABLE`

Missing from implementation:

- error severity
- error records
- error clearing
- fatal vs recoverable handling
- safe mode transitions
- `EVT_ERROR_RAISED` emission on actual failures

This is expected for the first slice but remains a gap against `ERROR_MODEL.md`.

### Main Entry Point Is Not The Vertical Slice

`src/main.cpp` still contains pre-existing WiFi, WebServer, and Servo code.

This was intentionally left untouched by implementation phases. However, if `pio run` compiles all source files, the repository entry point still does not represent the first vertical slice architecture.

This is a source-layout and integration risk, not a violation inside the new first-slice implementation.

## Recommended Fixes

### Add A Compile-Time Source Isolation Plan

Before running the first vertical slice on hardware, decide how `src/main.cpp` will select either:

- legacy sketch behavior, or
- Cyber32 first vertical slice validation runner.

Recommended approach:

- keep `src/main.cpp` untouched until explicitly approved
- add a documented compile-time flag later, such as `CYBER32_ENABLE_VERTICAL_SLICE_VALIDATION`
- ensure only the selected entry path runs on ESP32

### Drive Runtime State During Bootstrap

Update the future ESP32 bootstrap code to call:

```text
setState(INITIALIZING)
setState(DISCOVERING)
setState(REGISTERING)
setState(STARTING)
setState(READY)
setState(RUNNING)
```

This should be done when integrating the runner into the actual entry point.

### Register Runtime Tasks For Service And Logic

Move from direct validation calls toward Runtime-owned task execution:

- temperature service update task
- logic evaluation task
- optional API polling/service task
- event pump task

This will align implementation with `RUNTIME_IMPLEMENTATION_PLAN.md`.

### Expand EventRecord Or Document The Minimal V1 Slice Record

Either:

- update implementation to include compact `category` and `status` fields, or
- document that the current `EventRecord` is a first-slice subset.

Do not add large strings or dynamic payloads.

### Improve EventBus Overflow Behavior

Future EventBus work should add:

- priority-aware insertion or draining
- critical/high event preservation
- repeated low-priority update coalescing
- overflow/error event signaling

This should remain bounded and allocation-free.

### Add Registry Error Return Contract

Replace or extend boolean registration/update results with compact status/error reporting.

Recommended first step:

```text
RegistryResult
OK
FULL
DUPLICATE
NOT_FOUND
INVALID_RECORD
```

This can map to canonical `ERR_*` codes at the API boundary.

### Add Registry Service And Error Tables

To match the Registry implementation plan, add fixed-size tables later for:

- services
- latest errors
- metadata references

Keep these bounded and avoid metadata blobs in RAM.

### Remove Hardcoded Registration Indexes

PNP registration should use Registry-returned or looked-up indexes once multiple records exist.

For the current single-device slice, `0` is acceptable. For v1 growth, it must become resolved ownership.

### Thread Time Into PNP Events

PNP discovery should eventually receive a timestamp or time provider so discovery events do not use `timestamp_ms = 0`.

### Keep API Transport Separate

When adding WiFi or WebServer later, keep transport outside the internal API contract:

```text
WebServer handler
-> Cyber32Api
-> Registry/Runtime
```

Do not allow HTTP handlers to call Device, Driver, HAL, or Service directly.

## Future Risks

### Coupling Risk: Service To Concrete Device

`TemperatureService` currently depends on `SimTemperatureDevice`.

Risk:

- adding a real temperature provider may require changing Service code
- multiple providers cannot be selected through Registry

Future mitigation:

- introduce a Device capability provider interface or Registry-resolved provider handle
- keep Logic and API unchanged

### Scalability Risk: String IDs Everywhere

Current implementation stores `const char*` IDs directly.

Risk:

- string comparison cost grows with module/capability count
- flash/RAM ownership rules for strings must remain clear
- future dynamic metadata cannot safely point to temporary buffers

Future mitigation:

- keep boundary IDs as strings
- add compact indexes or numeric IDs after Registry registration
- define string lifetime rules explicitly

### Scalability Risk: Linear Lookups

Registry uses linear scans.

This is acceptable for v1 limits:

- 8 modules
- 16 devices
- 48 capabilities

Risk increases if v2 expands limits.

Future mitigation:

- cache resolved capability indexes for high-frequency Logic paths
- keep table sizes bounded on ESP32

### ESP32 Memory Risk: Validation Harness Owns All Objects

The validation harness owns all slice objects as members.

This is fine for controlled validation, but production integration should watch:

- global/static RAM use
- EventBus queue footprint
- Registry table footprint
- Runtime task table footprint
- API response object copies

Future mitigation:

- measure actual `.bss`, `.data`, and heap after PlatformIO build
- add a RAM budget report once `pio run` is available

### ESP32 Build Risk: PlatformIO Not Currently Verified

`pio run` could not be executed in the current environment because PlatformIO was not available on PATH.

Risk:

- include path or framework-specific issues may remain undiscovered
- `src/main.cpp` legacy dependencies may dominate the build result

Future mitigation:

- run `pio run` in an environment with PlatformIO installed
- isolate the vertical slice runner behind an explicit compile-time flag

### Event Processing Risk: Events Are Currently Discarded By Runtime

Runtime currently pops up to four events and discards them.

This matches the Phase 4 requirement, but future consumers will need a clear subscription or dispatch model.

Future mitigation:

- define bounded consumer registration
- avoid turning EventBus into Registry
- keep consumers querying Registry for current state

### Documentation Drift Risk: First-Slice Exceptions Need Labels

Some current implementation choices are valid first-slice shortcuts but differ from full v1 docs:

- direct Service-to-simulated-Device pointer
- hardcoded owner indexes
- partial API contract
- partial EventRecord
- partial Registry tables
- Runtime not yet coordinating the full slice

Future mitigation:

- mark these as first-slice limitations in implementation docs
- close each gap before declaring Cyber32 v1 complete

## Architecture Violations

No direct first-slice source-code violation was found in the implemented vertical slice.

Important qualification:

- `src/main.cpp` still contains legacy WiFi/WebServer/Servo code and is not aligned with the current first vertical slice.
- This file was not modified during the slice phases.
- If considered part of the active Cyber32 v1 implementation, it violates the first-slice scope.
- If treated as legacy placeholder code pending integration, it is an integration blocker rather than a slice-layer violation.

## Summary

The implemented Cyber32 first vertical slice is architecturally sound as a minimal validation path.

It proves the key goal:

```text
CAP_TEMPERATURE is discovered, registered, updated, stored, queried by Logic, and exposed by API without Logic depending on module names.
```

The main gaps are not redesign problems. They are expected first-slice incompleteness:

- Runtime is present but not yet the true coordinator.
- Registry is minimal and lacks service/error/metadata tables.
- EventBus is bounded but not priority-aware.
- API is internal and partial.
- PNP/Service contain single-provider shortcuts.
- The repository entry point still points at legacy WiFi/Servo behavior.

Recommended next step:

```text
Do not add features.
Make the existing vertical slice compile and run behind an explicit validation entry path, then move Service and Logic execution under Runtime task coordination.
```
