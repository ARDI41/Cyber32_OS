# Milestone 9.6 - UI API Contract Readiness Audit

## 1. Purpose

Milestone 9.6 audits the internal Cyber32 UI API contract work completed through Milestone 9.5 Phases 2-10 before any new API methods are added.

The purpose is to confirm that the contract layer is ready to support future UI clients while preserving Cyber32's API-first, capability-first, safety-first architecture.

This audit is documentation only.

## 2. Scope

This audit covers:

- System API structs
- Node API structs
- Capability API structs
- Provider and diagnostics alignment
- Project placeholders
- Logic Builder placeholders
- Template metadata placeholders
- Command request structures
- Read-only / compile validation result

The audited contract layer is currently internal C++ only.

## 3. Non-Goals

This milestone does not implement behavior.

Non-goals:

- No WebServer
- No HTTP
- No JSON
- No app implementation
- No Dashboard
- No Cloud
- No Marketplace
- No AI implementation
- No API method implementation
- No Registry behavior change
- No Runtime architecture change
- No Service behavior change
- No packet parsing
- No provider ingestion
- No `src/main.cpp` changes

## 4. Struct Inventory From Phases 2-9

### Phase 2 - System API Structs

- `ApiSystemIdentity`
- `ApiSystemFirmware`
- `ApiSystemRuntime`
- `ApiSystemModes`
- `ApiSystemMemory`
- `ApiSystemSummary`

Readiness: PASS.

These structs prepare bounded system-level identity, firmware, runtime, mode, memory, and summary views.

### Phase 3 - Node API Structs

- `ApiNodeIdentity`
- `ApiNodeStatus`
- `ApiNodePower`
- `ApiNodeSignal`
- `ApiNodeDiagnosticsSummary`
- `ApiNodeCapabilitySummary`
- `ApiNodeSummary`
- `ApiNodeList`

Readiness: PASS.

These structs prepare bounded wired and wireless node views for future UI clients.

### Phase 4 - Capability API Structs

- `ApiCapabilityIdentity`
- `ApiCapabilityValue`
- `ApiCapabilityAvailability`
- `ApiCapabilityProviderInfo`
- `ApiCapabilityQuality`
- `ApiCapabilitySummary`
- `ApiCapabilityList`

Readiness: PASS.

These structs prepare capability-first value cards, availability views, quality views, and diagnostic provider metadata.

### Phase 5 - Provider / Diagnostics API Alignment

Existing related structs:

- `ApiProviderDiagnostic`
- `ApiProviderSummary`
- `ApiWirelessNodeDiagnostic`
- `ApiWirelessNodeSummary`
- `ApiWirelessSecurityDiagnostic`
- `ApiWirelessSecuritySummary`

Added alignment placeholders:

- `ApiRuntimeDiagnosticSummary`
- `ApiErrorSummary`
- `ApiSafeModeSummary`
- `ApiDiagnosticsSummary`

Readiness: PASS.

Provider, wireless node, wireless security, runtime, error, and safe-mode diagnostics are conceptually aligned for future UI clients.

### Phase 6 - Project API Placeholders

- `ApiProjectIdentity`
- `ApiProjectZone`
- `ApiProjectPage`
- `ApiProjectWidget`
- `ApiProjectTemplateRef`
- `ApiProjectNotes`
- `ApiProjectSummary`
- `ApiProjectList`

Readiness: PASS.

These are placeholders only. No project persistence, rendering, export/import, app builder, or marketplace behavior exists.

### Phase 7 - Logic Builder Placeholders

- `ApiLogicTrigger`
- `ApiLogicCondition`
- `ApiLogicAction`
- `ApiLogicSafety`
- `ApiLogicRule`
- `ApiLogicRuleSummary`
- `ApiLogicRuleList`
- `ApiLogicValidationResult`
- `ApiLogicSimulationResult`

Readiness: PASS.

These are declarative placeholders only. They do not execute rules, store rules, parse scripts, call hardware, or bypass Services.

### Phase 8 - Template Metadata Placeholders

- `ApiTemplateIdentity`
- `ApiTemplateCapabilityRequirement`
- `ApiTemplateSafetyRequirement`
- `ApiTemplateWidgetHint`
- `ApiTemplateLogicHint`
- `ApiTemplateMetadata`
- `ApiTemplateList`

Readiness: PASS.

These structs prepare capability-first Template Script and Project Template metadata. Templates remain project starters, not privileged code.

### Phase 9 - Command Request Structures

Existing command structs preserved:

- `ApiServoCommandRequest`
- `ApiServoCommandResponse`
- `ApiMotorCommandRequest`
- `ApiMotorCommandResponse`
- `ApiRelayCommandRequest`
- `ApiRelayCommandResponse`
- `ApiCommandStateResponse`

Added future command placeholders:

- `ApiGenericCommandTarget`
- `ApiGenericCommandSafety`
- `ApiGenericCommandRequest`
- `ApiGenericCommandResponse`
- `ApiCameraCommandRequest`
- `ApiAudioOutputCommandRequest`
- `ApiDisplayCommandRequest`
- `ApiEmergencyStopRequest`
- `ApiEmergencyStopResponse`

Readiness: PASS with caution.

The new command structs are contracts only. Future command methods must route through Services and Runtime safety policy.

### Phase 10 - Read-Only / Compile Validation

Validation result:

- Duplicate struct names were not found in `api_response.h`.
- Forbidden dynamic allocation patterns were not found in `api_response.h`.
- Arduino `String` was not found in `api_response.h`.
- STL container usage was not found in `api_response.h`.
- The new structs remained data-only contracts.
- Existing API methods were not changed.
- Existing command behavior was not changed.

PlatformIO result:

- `pio run` could not execute in the Codex shell because `pio` is not available on PATH.
- Final build verification still requires a local environment with PlatformIO available.

Readiness: WARNING until PlatformIO compile is verified locally.

## 5. API Client Readiness Matrix

| Future client | Contract readiness | Notes |
| --- | --- | --- |
| Dev Panel | READY FOR METHOD PLANNING | System, diagnostics, capability, command-state, Test Lab-adjacent contracts are present. |
| Minimal App | READY FOR METHOD PLANNING | Node, capability, provider, project, and diagnostics contracts support initial app screens. |
| Mission Control | READY FOR METHOD PLANNING | Project pages, Logic Builder, templates, commands, and diagnostics have placeholder contracts. |
| Dashboard | READY FOR METHOD PLANNING | Capability lists, summaries, diagnostics, and project views are prepared. |
| Cloud Bridge | PARTIAL | Read summaries exist, but identity, permissions, transport wrapping, and persistence remain future work. |
| Marketplace | PARTIAL | Template metadata placeholders exist, but storage, sharing, versioning, and trust rules are future work. |
| AI Assistant | PARTIAL | Capability-first explanatory surfaces are prepared, but AI must remain API-only and non-authoritative. |

## 6. Contract Gaps Before Adding Methods

Known gaps:

- No generic `getCapability...` method plan yet.
- No node discovery or pending pairing contract methods yet.
- No project persistence or project storage owner exists.
- No Logic Builder storage or validation owner exists.
- No Template metadata storage owner exists.
- No permission/role API contracts exist yet.
- No notification API contracts exist yet.
- No OTA API contracts exist yet.
- No API method validation plan exists for the newly added placeholder structs.
- PlatformIO compile validation must be completed in an environment where `pio` is available.

These gaps do not block the contract inventory, but they must be resolved before production UI behavior.

## 7. Naming Consistency Review

Current naming is generally consistent with Phase 1 rules:

- Structs use the `Api` prefix.
- Detailed diagnostic records use `Diagnostic`.
- Aggregate count views use `Summary`.
- State-changing input contracts use `Request`.
- State-changing output contracts use `Response`.
- Lists use `List`.
- Validation and simulation placeholder outputs use `Result`.

Known tolerated inconsistencies:

- `ApiSystemStatus` uses `latest_error_code` instead of `error_code`.
- `ApiCapabilityState` is older and does not include `RegistryResult`.
- Existing capability methods remain capability-specific, such as `getTemperatureState(...)`.

These are compatibility notes, not failures.

## 8. Safety Boundary Review

Safety boundary result: PASS.

The current contracts do not grant hardware access.

Future command rules:

- API commands must go through Services only.
- Services must preserve Runtime safety gates.
- API must not call Drivers.
- API must not call Devices.
- API must not call HAL.
- API must not write Registry arrays directly.
- API must not parse wireless packets.
- Emergency Stop and motor/motion safety must remain globally visible where relevant.

Command request structs are not permission to bypass Services or safety gates.

## 9. Capability-First Compliance Review

Capability-first result: PASS.

The contract set is oriented around:

- `capability_id`
- `target_capability_id`
- `bound_capability_id`
- `trigger_capability_id`
- `action_capability_id`
- provider diagnostics as metadata
- node and MAC fields as diagnostics/management metadata

The contracts do not require Logic or normal UI behavior to bind to driver names, device class names, pins, provider IDs, transport IDs, or Registry array indexes.

## 10. Read-Only vs Command Separation Review

Separation result: PASS.

Read-oriented structs cover:

- system
- node
- capability
- provider diagnostics
- wireless diagnostics
- security diagnostics
- project summaries
- template metadata

Command-oriented structs cover:

- servo, motor, and relay existing command requests
- generic future command requests
- camera, audio, display, and emergency-stop placeholders

Future API methods must preserve the method naming split:

```text
get...      -> read-only
command...  -> Service-routed command behavior
request...  -> bounded workflow request, not direct hardware command
```

## 11. ESP32 Boundedness Review

Boundedness result: PASS with build verification pending.

Current contract approach:

- fixed-size list arrays
- `uint8_t` list counts
- compact primitive fields
- `const char*` for stable identifiers and placeholders
- no Arduino `String`
- no STL containers
- no heap allocation

Current API-level bounded constants:

- `API_MAX_NODE_SUMMARY_COUNT`
- `API_MAX_CAPABILITY_SUMMARY_COUNT`
- `API_MAX_PROJECT_SUMMARY_COUNT`
- `API_MAX_LOGIC_RULE_SUMMARY_COUNT`
- `API_MAX_TEMPLATE_METADATA_COUNT`

The contracts remain ESP32-safe by design.

## 12. Stop Conditions

Stop implementation and return to architecture review if any future API method work introduces:

- WebServer during internal API contract phases
- HTTP during internal API contract phases
- JSON during internal API contract phases
- app implementation during API method groundwork
- Dashboard implementation
- Cloud implementation
- Marketplace implementation
- AI implementation
- API calls to Drivers
- API calls to Devices
- API calls to HAL
- API packet parsing
- direct Registry array writes
- Runtime architecture changes
- Registry ownership changes
- Service policy bypass
- command execution outside Services
- STL containers
- Arduino `String`
- heap allocation
- unbounded lists
- Logic dependency on providers, nodes, transport, pins, or raw diagnostics

## 13. Recommended First API Method Implementation Milestone

Recommended next milestone:

```text
Milestone 9.7 - Read-Only System API Method Implementation Plan
```

Recommended scope:

- documentation first
- plan read-only methods for system identity, firmware, runtime, modes, memory, and summary
- no WebServer
- no HTTP
- no JSON
- no app implementation
- no command behavior
- no Registry ownership changes
- no Runtime architecture changes

Recommended first implementation after that plan:

```text
Milestone 9.7 Phase 1 - System API read-only methods
```

Rationale:

System API methods are the lowest-risk next step because they can expose bounded status and placeholders without provider selection, command routing, project persistence, template storage, or Logic Builder behavior.
