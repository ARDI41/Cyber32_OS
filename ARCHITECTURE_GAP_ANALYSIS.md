# Cyber32 Architecture Gap Analysis

This document reviews the current Cyber32 architecture documents and identifies remaining gaps only. It does not create new architecture.

Reviewed documents:

- `ARCHITECTURE.md`
- `CAPABILITY_SCHEMA.md`
- `CAPABILITY_CATALOG.md`
- `CAPABILITY_PAYLOAD_SCHEMA.md`
- `MODULE_METADATA_SCHEMA.md`
- `REGISTRY_SCHEMA.md`
- `REGISTRY_IMPLEMENTATION_PLAN.md`
- `EVENT_MODEL.md`
- `ERROR_MODEL.md`
- `MEMORY_MODEL.md`
- `ESP32_V1_SCOPE.md`
- `BOOT_SEQUENCE.md`
- `RUNTIME_IMPLEMENTATION_PLAN.md`
- `PNP_DISCOVERY_FLOW.md`

Findings are ranked:

- Critical: must resolve before implementation to avoid architectural rework or unsafe behavior.
- Important: should resolve during v1 design before broad coding.
- Future: acceptable to defer beyond the first ESP32 v1 slice.

## Overall Completeness Review

Cyber32 now has a coherent architecture foundation:

- fixed layer order
- capability-first rule
- ESP32 v1 scope
- bounded memory model
- bounded event model
- canonical error model
- capability catalog and payload schemas
- module metadata schema
- Registry implementation plan
- Runtime implementation plan
- PNP discovery flow
- boot sequence

The remaining gaps are mostly contract-level details needed before implementation. The main risk is not architectural direction. The main risk is that implementation may interpret several documents differently because some ownership, state transition, provider selection, command execution, and persistence details are still underspecified.

## Critical Findings

### 1. Capability Catalog And Payload Schema Are Not Fully Aligned

The payload schema defines `CAP_STORAGE_USAGE` and `CAP_AUDIO_OUTPUT`, but the current capability catalog defines storage capabilities as `CAP_STORAGE_READ`, `CAP_STORAGE_WRITE`, `CAP_STORAGE_DELETE`, `CAP_STORAGE_STATUS`, `CAP_STORAGE_CAPACITY`, and `CAP_STORAGE_FREE_SPACE`, and audio capabilities as `CAP_AUDIO_PLAY_TONE`, `CAP_AUDIO_PLAY_SOUND`, `CAP_AUDIO_VOLUME`, `CAP_AUDIO_MUTE`, and `CAP_MICROPHONE_INPUT`.

Impact:

- Registry validation may reject payload-schema capabilities.
- Logic may bind to capability IDs not present in the official catalog.
- API/Dashboard may expose inconsistent capability names.

Gap:

- There is no documented process for adding capability IDs introduced by payload schemas into the official catalog.

### 2. Error Code Catalog Is Not Canonical Enough

`ERROR_MODEL.md` defines categories and examples, but several referenced codes are not fully normalized across documents. Examples include `ERR_RUNTIME_INVALID_TRANSITION`, `ERR_RUNTIME_OVERLOAD`, `ERR_EVENT_QUEUE_OVERFLOW`, `ERR_REGISTRY_DUPLICATE_ID`, and `ERR_WATCHDOG_BOOT_LOOP`.

Impact:

- Runtime, Event Bus, Registry, and API may emit different codes for the same failure.
- Tests cannot validate a closed error set.

Gap:

- There is no single canonical table of allowed v1 error codes with severity, recoverability, safe action, and owner layer for every code.

### 3. Runtime State Machine Has States But Not A Complete Transition Table

`RUNTIME_IMPLEMENTATION_PLAN.md` defines states such as `BOOTING`, `INITIALIZING`, `DISCOVERING`, `REGISTERING`, `STARTING`, `READY`, `RUNNING`, `DEGRADED`, `SAFE_MODE`, `RECOVERY`, `ERROR`, and `SHUTDOWN`.

Impact:

- Implementation may allow invalid transitions.
- Recovery and safe mode may behave inconsistently.
- Watchdog recovery may re-enter unsafe states too early.

Gap:

- No complete transition table defines allowed from/to states, trigger events, guards, side effects, and error behavior.

### 4. Safe Actuator Defaults Are Required But Not Specified Per Capability

Multiple docs require actuators to fail safe, but `CAP_MOTOR_CONTROL`, `CAP_SERVO_POSITION`, `CAP_POWER_OUTPUT_ENABLE`, display, and audio safety defaults are not fully defined.

Impact:

- Boot and recovery behavior may be unsafe.
- Runtime cannot enforce safety consistently.
- Logic/API may command outputs before safe initialization.

Gap:

- No per-actuator safety policy defines boot state, safe mode state, watchdog reset state, timeout behavior, and recovery validation.

### 5. Multi-Provider Capability Selection Is Acknowledged But Undefined

Registry docs say multiple providers may exist only if ownership and provider resolution are tracked, and ambiguity should be degraded if no policy exists.

Impact:

- `CAP_POSITION`, `CAP_DISTANCE`, `CAP_BATTERY_LEVEL`, or other duplicated capabilities may be ambiguous.
- Logic cannot know which provider it is using.
- Dashboard/API may show contradictory providers.

Gap:

- No provider selection contract defines active provider, fallback provider, priority, user selection, service arbitration, or tie-breaking.

### 6. Command Capability Execution Path Is Not Fully Defined

Payload schemas define command/write capabilities such as `CAP_DISPLAY_TEXT`, `CAP_AUDIO_OUTPUT`, `CAP_MOTOR_CONTROL`, and write side of `CAP_SERVO_POSITION`.

Impact:

- It is unclear whether API sends commands to Registry, Services, Devices, or Runtime tasks.
- Registry stores facts but must not execute commands.
- Runtime coordinates but must not implement Logic or Service policy.

Gap:

- No documented command dispatch contract defines API -> Service/Runtime -> Device path, validation, acknowledgement, timeout, result storage, and event emission.

### 7. Registry Schema And Registry Implementation Differ On Metadata Storage

`REGISTRY_SCHEMA.md` includes `raw_metadata` in Metadata records. `REGISTRY_IMPLEMENTATION_PLAN.md` says Registry must not store large metadata blobs and should store compact metadata references.

Impact:

- Implementers may copy raw metadata into RAM, violating ESP32 memory limits.

Gap:

- The conceptual schema and ESP32 implementation plan need a documented v1 interpretation: raw metadata is allowed only outside RAM or as a bounded reference.

### 8. PNP Validation Contract Is Not Concrete Enough

PNP docs list validation checks, but do not define exact validation outputs, error mapping, partial registration behavior, conflict handling, or security decisions.

Impact:

- Invalid metadata may be handled inconsistently.
- Capabilities may be exposed before full validation.
- Recovery from invalid modules may be inconsistent.

Gap:

- No detailed PNP validation result schema exists.

### 9. ESP32 Persistence Contract Is Still Missing

Memory and scope docs mention config, metadata cache, boot error counters, safe boot flags, and storage limits.

Impact:

- Repeated boot recovery, safe mode, config rollback, and metadata caching cannot be implemented consistently.
- Flash wear risk remains unmanaged.

Gap:

- No persistence schema defines what is stored in NVS/flash/EEPROM, write frequency, checksum, rollback, and wear rules.

### 10. API Contract Is Not Yet Defined

ESP32 scope lists allowed endpoints and response size limits, but no complete API request/response schema exists.

Impact:

- API may expose inconsistent capability payloads, errors, registry records, and command responses.
- Dashboard cannot be implemented reliably.

Gap:

- No REST v1 contract defines endpoints, methods, payload shapes, status codes, error responses, rate limits, and authentication.

## Important Findings

### 11. Ownership Rules Are Clear Generally But Not Per Object Type

Memory and Registry docs define general ownership, but object-level ownership is not fully specified for:

- capability payload updates
- service-generated values
- device state
- module lifecycle state
- error records
- command status

Impact:

- Services and Registry may both attempt to own payload values.
- Runtime and Registry may both attempt to own lifecycle state.

Gap:

- No object ownership matrix maps each field to its owning layer and allowed writers.

### 12. Event Consumer Registration Is Undocumented

`EVENT_MODEL.md` defines emitters and consumers, but not how consumers subscribe.

Impact:

- Dynamic subscriptions could accidentally allocate memory.
- Static subscriptions could be implemented inconsistently.

Gap:

- No v1 subscription model defines static table size, consumer IDs, callback limits, and dispatch ordering.

### 13. Event Overflow Policies Need Per-Category Rules

The Event Model defines generic overflow behavior, but not per-category loss policy.

Impact:

- Low-priority telemetry can be dropped, but it is not clear whether `EVT_CAPABILITY_VALUE_UPDATED` for battery, distance, or motor state is droppable.

Gap:

- No event retention/drop matrix exists by category and priority.

### 14. Stale Value Freshness Is Defined Per Payload But Not Enforced Centrally

Payload schemas include recommended freshness windows, but Registry/Runtime do not define who marks stale values and when.

Impact:

- Services, Runtime, or Registry may each implement stale checks differently.

Gap:

- No stale-check ownership and schedule contract exists.

### 15. Capability Payload Storage For Complex Objects Needs Fixed Struct Definitions

Registry implementation says complex payloads such as `CAP_POSITION` should use fixed structs. Payload schema defines fields but not byte-level or struct-level layout.

Impact:

- ESP32 memory budget may drift.
- API serialization may diverge from internal representation.

Gap:

- No compact internal payload layout table exists.

### 16. Logic Contract Remains Conceptual

The documents repeatedly state Logic must bind to capabilities, but they do not define v1 Logic rule representation.

Impact:

- Logic may become too dynamic or string-heavy.
- Rule parsing may allocate memory.
- Capability comparisons may use inconsistent stale/unavailable behavior.

Gap:

- No Logic v1 schema defines rule slots, supported operators, actions, binding validation, and disabled rule behavior.

### 17. Service Boundaries Are Not Fully Documented

Services are listed, and Runtime interaction is described, but individual v1 service contracts are not concrete.

Impact:

- Device Manager, Module Manager, Power Manager, Telemetry Manager, Storage Manager, and Network Manager may overlap.

Gap:

- No v1 service contract defines required services, optional services, update periods, owned state, and Registry writes.

### 18. Boot Sequence And Runtime Startup Flow Differ In Detail

`BOOT_SEQUENCE.md` starts with HAL, then Drivers, Devices, Modules, PNP, Registry, Runtime. `RUNTIME_IMPLEMENTATION_PLAN.md` initializes Event Bus and Registry fixed tables before HAL, then proceeds through HAL and PNP.

Impact:

- There is ambiguity between conceptual layer boot order and practical initialization order.

Gap:

- No distinction is documented between architecture order and minimal infrastructure initialization order.

### 19. Module Metadata Binary Format Is Still Missing

Metadata schema lists fields and size limits, but no compact Level 2/3 wire format exists.

Impact:

- I2C/EEPROM readers cannot be implemented consistently.
- Checksum, endian, version, and field encoding may diverge.

Gap:

- No binary metadata layout defines magic, version, length, field table, capability list encoding, and checksum.

### 20. Security Minimum Is Not Yet Concrete

Security is mentioned in metadata validation, API, errors, and ESP32 scope, but minimum v1 security is not defined.

Impact:

- API authorization and module trust may be inconsistent.
- Dashboard may expose unsafe controls.

Gap:

- No v1 security baseline defines AP password policy, local access assumptions, API token behavior, command authorization, and trusted metadata rules.

### 21. Timing Budgets Exist But Not Per Feature

Runtime timing budgets exist, but there are no per-service, per-driver, per-PNP, or per-API operation budgets.

Impact:

- One blocking driver, storage write, or API request can break control timing.

Gap:

- No timing budget matrix exists for v1 features.

### 22. Memory Budgets Exist But Not Verified Against Record Layouts

Memory model and Registry plan estimate RAM budgets, but actual table layouts are not finalized.

Impact:

- Fixed-size records may exceed target budget once implemented.

Gap:

- No memory accounting checklist maps each planned struct/table to bytes.

### 23. Error Clearing And Recovery Ownership Needs More Precision

Error model allows owning layer, Runtime recovery, Service recovery, API action, or successful revalidation to clear errors.

Impact:

- Errors may clear too early or remain stuck.

Gap:

- No per-error-code clearing owner and clearing condition table exists.

### 24. Dashboard Scope Is Minimal But Not Contracted

ESP32 scope says Dashboard is minimal, but Dashboard docs still describe pages/widgets/assets and logic builder.

Impact:

- Implementation may overbuild Dashboard for ESP32.

Gap:

- No Dashboard v1 contract defines exact pages, controls, polling intervals, asset budget, and disabled features.

### 25. Testing Standards Are Missing

The architecture docs mention validation and constraints, but no test standard exists for architecture compliance.

Impact:

- Capability-first, no dynamic allocation, bounded Registry, and safe boot rules may regress.

Gap:

- No v1 test plan defines architecture boundary tests, memory tests, boot tests, PNP validation tests, and Logic binding tests.

## Future Findings

### 26. Multi-Platform Strategy Is Intentionally Deferred

Docs preserve hardware-agnostic architecture while ESP32 v1 is the only target.

Impact:

- Acceptable for v1, but future portability will need platform abstraction decisions.

Gap:

- Future platform profiles are not defined.

### 27. OTA Is Deferred But Recovery Mentions It Indirectly

Safe boot and recovery docs mention rollback concepts, while ESP32 scope defers OTA.

Impact:

- Acceptable if OTA remains disabled.

Gap:

- Future OTA partition, rollback, and security contracts are missing.

### 28. CAN And Distributed Robotics Are Documented But Out Of v1 Scope

Protocols include CAN and network concepts, but ESP32 v1 does not target distributed multi-node operation.

Impact:

- Acceptable as future vision.

Gap:

- Future distributed registry and bus arbitration are missing.

### 29. SDK Documents Are Broad But Not v1-Bounded

SDK docs describe generators, templates, and validators.

Impact:

- Not blocking for v1 runtime.

Gap:

- Future SDK contracts need to reflect v1 schemas and validators.

### 30. AI-Ready Remains Documentation/Tooling Only

AI rules correctly avoid onboard AI inference for v1.

Impact:

- Acceptable.

Gap:

- Future machine-readable schema export may be useful.

## Contradictions Summary

Critical contradictions:

- `CAP_STORAGE_USAGE` and `CAP_AUDIO_OUTPUT` appear in payload schema but not current catalog.
- Conceptual Registry metadata includes `raw_metadata`, while ESP32 Registry plan forbids large metadata blobs in RAM.
- Boot sequence conceptual order differs from Runtime practical initialization order.

Important contradictions:

- Dashboard docs describe a full UI while ESP32 scope limits Dashboard to minimal pages.
- Runtime docs allow `READY` and `RUNNING` but may merge them in v1.
- Event examples use `ERR_EVENT_QUEUE_OVERFLOW`, but Error Model does not canonically list it.

## Overlapping Responsibilities Summary

Most overlaps are now acknowledged but still need field-level ownership:

- Registry vs Runtime for lifecycle state.
- Registry vs Services for capability payload writes.
- Registry vs Event Bus for current state.
- Runtime vs Services for service policy.
- PNP Registration vs Registry registration storage.
- API vs Dashboard Backend for user-facing data shaping.
- Error owner vs Registry latest error state.

## Undocumented Data Flows

Remaining undocumented or incomplete flows:

- API command to capability provider.
- Logic action to actuator capability.
- Service update to Registry payload update.
- Driver error to Device/Capability unavailable state.
- Watchdog reset to persisted boot counter to safe mode.
- Config load failure to safe defaults or safe mode.
- PNP metadata validation failure to Registry/error/event/API/Dashboard.
- Stale-value detection and update flow.
- Multi-provider capability selection flow.

## Undocumented Ownership Rules

Ownership still needs field-level clarity for:

- capability payload values
- capability availability
- stale flags
- command status
- lifecycle state
- latest error state
- metadata references
- service health
- safe mode flags
- boot recovery counters

## Undocumented State Transitions

State transitions still need complete tables for:

- Runtime top-level states
- component lifecycle states
- service lifecycle states
- module availability states
- device availability states
- capability availability/stale/error states
- error lifecycle states
- safe mode and recovery states

## Missing Schemas

High-priority missing schemas:

- API v1 request/response schema
- command dispatch schema
- PNP validation result schema
- metadata binary wire format
- Logic v1 rule schema
- service record/update schema
- stale policy schema
- provider selection schema
- persistent config schema
- safe boot/recovery storage schema

## Missing Standards

High-priority missing standards:

- ESP32 build flags and profile standard
- API authentication minimum
- Dashboard v1 limits and endpoint usage
- timing budget matrix
- memory accounting checklist
- architecture test standard
- flash persistence and wear policy
- actuator safety policy
- canonical v1 error code table
- canonical v1 event retention policy

## Implementation Risks

Critical implementation risks:

- Dynamic allocation sneaks into Registry, Runtime, Events, or payload parsing.
- Logic binds to display names or module names.
- Registry becomes command executor or Event Bus becomes state store.
- Dashboard/API bypass Services and control hardware directly.
- Safe mode is entered too late or exits too early.
- Metadata parsing stores large blobs in RAM.
- Error codes diverge across layers.

Important implementation risks:

- Struct sizes exceed memory budgets.
- Event queue overflow loses safety-relevant events.
- Blocking PNP/I2C/API operations break timing.
- Multiple providers for one capability produce ambiguous Logic behavior.
- Stale values are treated as valid fresh data.
- ESP32 WiFi and Dashboard load starve Runtime loop.

## ESP32-Specific Risks

Critical ESP32 risks:

- RAM exhaustion from Registry records, payloads, JSON, Dashboard buffers, and WiFi.
- Heap fragmentation if Arduino `String`, dynamic JSON, or containers are used in core runtime.
- Watchdog resets from blocking I2C, storage, WiFi, or long event handlers.
- Unsafe actuator outputs during boot, brownout, watchdog reset, or fatal errors.

Important ESP32 risks:

- Flash wear from logs, metadata cache, config writes, or telemetry history.
- I2C address conflicts and bus lockups during discovery.
- Metadata payloads exceeding I2C/EEPROM practical limits.
- API/WebSocket memory pressure.
- Dashboard asset size exceeding practical flash budget.

Future ESP32 risks:

- OTA partitioning and rollback complexity.
- Secure module metadata validation.
- CAN/distributed expansion.
- Rich Dashboard features.

## Recommended Gap Closure Order

Critical before implementation:

1. Reconcile Capability Catalog with Payload Schema.
2. Create canonical v1 error code table.
3. Define Runtime transition table.
4. Define actuator safety policy.
5. Define command dispatch contract.
6. Define PNP validation result schema.
7. Define v1 persistence/config/safe boot schema.
8. Define API v1 contract.

Important during first implementation slice:

1. Define field-level ownership matrix.
2. Define event subscription and overflow policy tables.
3. Define stale value ownership and scheduling.
4. Define compact internal payload layouts.
5. Define Logic v1 rule schema.
6. Define service contracts.
7. Define metadata binary format.
8. Define timing and memory accounting checklists.

Future:

1. OTA contract.
2. richer Dashboard contract.
3. SDK generator contracts.
4. multi-platform profiles.
5. distributed/CAN profile.

## Final Assessment

Cyber32's architecture documents are now substantially complete for direction and constraints. The remaining work is mostly precision: exact tables, schemas, ownership matrices, state transitions, and ESP32-safe implementation contracts.

The highest-risk unresolved area is command and state ownership: who writes payloads, who dispatches commands, who clears errors, and who transitions recovery states. Resolving that before code will protect the core design rule:

```text
Registry stores facts.
Runtime coordinates execution.
Services own policy.
Logic uses capabilities.
API exposes contracts.
Dashboard displays and requests through API.
```
