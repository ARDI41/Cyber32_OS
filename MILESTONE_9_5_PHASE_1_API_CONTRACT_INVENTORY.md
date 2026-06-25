# Milestone 9.5 Phase 1 - API Contract Inventory and Naming Rules

## Purpose

Milestone 9.5 Phase 1 inventories the current Cyber32 internal C++ API contracts and defines naming rules for future API expansion.

This phase is documentation only. It does not create headers, source files, structs, methods, WebServer handlers, HTTP routes, JSON schemas, app screens, Dashboard behavior, Cloud behavior, Marketplace behavior, or AI behavior.

The goal is to make future API work consistent, bounded, capability-first, and safe for all future UI clients.

## Scope

Scope for this phase:

- document existing API response/request structs
- document existing API methods
- identify current naming patterns
- define future naming rules
- separate read-only API contracts from command API contracts
- define diagnostics naming rules
- define compatibility rules for future UI clients

Out of scope:

- No WebServer
- No HTTP
- No JSON
- No app implementation
- No source changes
- No API struct implementation
- No API method implementation
- No Registry behavior changes
- No Runtime behavior changes
- No Service behavior changes
- No wireless packet parsing

## 1. Current Existing API Inventory

Current API files:

- `src/api/api_response.h`
- `src/api/cyber32_api.h`
- `src/api/cyber32_api.cpp`
- `src/api/README.md`
- `src/api/internal/README.md`
- `src/api/external/README.md`
- `src/api/rest/README.md`
- `src/api/websocket/README.md`

Current implemented API layer:

- `Cyber32Api`
- internal C++ contracts only
- no HTTP implementation
- no JSON implementation
- no WebServer implementation

### Current Response and Request Structs

System:

- `ApiSystemStatus`

Capability reads:

- `ApiCapabilityState`

Command requests:

- `ApiServoCommandRequest`
- `ApiMotorCommandRequest`
- `ApiRelayCommandRequest`

Command responses:

- `ApiServoCommandResponse`
- `ApiMotorCommandResponse`
- `ApiRelayCommandResponse`
- `ApiCommandStateResponse`

Provider diagnostics:

- `ApiProviderDiagnostic`
- `ApiProviderSummary`

Wireless node diagnostics:

- `ApiWirelessNodeDiagnostic`
- `ApiWirelessNodeSummary`

Wireless security diagnostics:

- `ApiWirelessSecurityDiagnostic`
- `ApiWirelessSecuritySummary`

### Current Cyber32Api Lifecycle and Attachment Methods

Lifecycle:

- `Cyber32Api()`
- `begin(Registry* registry, Runtime* runtime)`

Service attachment:

- `attachServoService(ServoService* service)`
- `attachMotorService(MotorService* service)`
- `attachRelayService(RelayService* service)`

### Current Read-Only API Methods

System:

- `getSystemStatus(ApiSystemStatus& out_status)`

Capability state:

- `getTemperatureState(ApiCapabilityState& out_state)`
- `getDistanceState(ApiCapabilityState& out_state)`
- `getServoPositionState(ApiCapabilityState& out_state)`
- `getMotorControlState(ApiCapabilityState& out_state)`
- `getRelayControlState(ApiCapabilityState& out_state)`

Command state reads:

- `getServoCommandState(ApiCommandStateResponse& out_response)`
- `getMotorCommandState(ApiCommandStateResponse& out_response)`
- `getRelayCommandState(ApiCommandStateResponse& out_response)`

Provider diagnostics:

- `getProviderDiagnostic(const char* provider_id, ApiProviderDiagnostic& out_response)`
- `getActiveProviderDiagnostic(const char* capability_id, ApiProviderDiagnostic& out_response)`
- `getProviderSummary(ApiProviderSummary& out_response)`

Wireless node diagnostics:

- `getWirelessNodeDiagnostic(uint32_t node_id, ApiWirelessNodeDiagnostic& out_response)`
- `getWirelessNodeDiagnosticByIndex(uint8_t index, ApiWirelessNodeDiagnostic& out_response)`
- `getWirelessNodeSummary(ApiWirelessNodeSummary& out_response)`

Wireless security diagnostics:

- `getWirelessSecurityDiagnostic(uint32_t node_id, ApiWirelessSecurityDiagnostic& out_response)`
- `getWirelessSecurityDiagnosticByIndex(uint8_t index, ApiWirelessSecurityDiagnostic& out_response)`
- `getWirelessSecuritySummary(ApiWirelessSecuritySummary& out_response)`

### Current Command API Methods

Servo:

- `commandServoPosition(uint32_t now_ms, const ApiServoCommandRequest& request, ApiServoCommandResponse& out_response)`

Motor:

- `commandMotorControl(uint32_t now_ms, const ApiMotorCommandRequest& request, ApiMotorCommandResponse& out_response)`
- `commandMotorStop(uint32_t now_ms, ApiMotorCommandResponse& out_response)`

Relay:

- `commandRelayControl(uint32_t now_ms, const ApiRelayCommandRequest& request, ApiRelayCommandResponse& out_response)`
- `commandRelayOff(uint32_t now_ms, ApiRelayCommandResponse& out_response)`

### Current API Dependencies

Current `Cyber32Api` depends on:

- `Registry`
- `Runtime`
- `ServoService`
- `MotorService`
- `RelayService`

Current API response structs reference bounded core/registry types including:

- `CapabilityPayload`
- `CommandState`
- `RuntimeState`
- `RegistryResult`
- `CapabilityProviderRecord` related enums
- wireless node allowlist and trust enums
- wireless security diagnostics records

## 2. Existing API Naming Patterns

Current observed naming patterns:

- API structs use `Api` prefix.
- Read responses use suffixes such as `Status`, `State`, `Diagnostic`, `Summary`, and `Response`.
- Command input structs use `CommandRequest`.
- Command output structs use `CommandResponse`.
- Read methods use `get...`.
- Command methods use `command...`.
- Output parameters use names such as `out_status`, `out_state`, and `out_response`.
- Methods return `bool` for success/failure and also fill response fields.
- Response structs commonly include `bool ok`.
- Diagnostics and summary structs commonly include `RegistryResult registry_result`.
- Most responses include `const char* error_code`.
- Successful responses usually set `error_code = "none"`.
- Missing/unavailable paths use compact error IDs and Registry result codes.

Current inconsistency to keep in mind:

- `ApiSystemStatus` uses `latest_error_code`, while most other structs use `error_code`.
- `ApiCapabilityState` does not include `RegistryResult`, while diagnostics responses usually do.
- Capability read methods are capability-specific today, such as `getTemperatureState(...)`, rather than generic `getCapabilityState(capability_id, ...)`.

These are not bugs in this phase. They are inventory notes for future alignment.

## 3. Future API Naming Rules

Future API contract names must be:

- stable
- capability-first
- bounded
- explicit about read vs command behavior
- reusable by Dev Panel, Minimal App, Mission Control, Dashboard, Cloud Bridge, Marketplace, and AI Assistant

Rules:

- Use `Api` prefix for all internal API contract structs.
- Use `get...` prefix for read-only methods.
- Use `command...` prefix for commands that request actuator or state-changing behavior through Services.
- Use `request...` only for future workflows that begin a bounded request but do not directly command hardware.
- Use `set...` only if the API method is clearly a configuration operation and goes through the correct owner.
- Avoid transport-specific names in normal capability APIs.
- Avoid driver, device, module, pin, and HAL names in UI-facing contracts.
- Use `Capability`, `Node`, `Provider`, `Diagnostic`, `Summary`, `Command`, `Request`, and `Response` consistently.

## 4. API Struct Naming Rules

Struct naming rules:

- `Api...State` for current state of one concept.
- `Api...Diagnostic` for detailed read-only diagnostic view of one record/entity.
- `Api...Summary` for bounded aggregate counts or totals.
- `Api...Request` for input to a state-changing API method.
- `Api...Response` for output from a state-changing API method.
- `Api...Result` should be avoided unless it wraps a specific result-domain enum or future validation result.
- `Api...Config` should be reserved for future configuration contracts.
- `Api...Placeholder` should be avoided in source; use documented placeholders in roadmap docs until implementation is approved.

Field rules:

- Include `bool ok` for response structs.
- Include `RegistryResult registry_result` when the response directly reflects Registry read status.
- Include `const char* error_code` for compact error visibility.
- Use fixed-size arrays for bounded byte data such as MAC addresses.
- Use `const char*` only for stable string literals or owned static identifiers.
- Do not use Arduino `String`.
- Do not use STL containers.
- Do not allocate heap memory.
- Do not include raw wireless packet buffers in UI-facing API structs.

## 5. API Method Naming Rules

Read-only method rules:

- Use `get...`.
- Fill an output reference.
- Return `true` only when the requested read succeeds.
- Return `false` and fill a bounded failure response on invalid input, missing Registry attachment, missing record, or unavailable data.
- Do not mutate Registry state.
- Do not call Services unless a future documented read-only Service query is explicitly approved.
- Do not call Drivers, Devices, HAL, or transport internals.

Command method rules:

- Use `command...`.
- Accept a bounded `Api...CommandRequest` when parameters are needed.
- Fill an `Api...CommandResponse`.
- Return `true` only when the Service accepts/executes according to its policy.
- Route through Services only.
- Do not write Registry arrays directly.
- Do not bypass Runtime safety state.

Index read method rules:

- Use `get...ByIndex(...)`.
- Index type should remain bounded, normally `uint8_t`, unless a larger bounded table is explicitly approved.
- Out-of-range index must return `false` and fill `RegistryResult::NOT_FOUND` where applicable.

Summary method rules:

- Use `get...Summary(...)`.
- Summary responses should contain bounded counts/totals only.
- Summary reads must not reset counters.
- Summary reads must not mutate records.

## 6. API Result Naming Rules

Current result model:

- API method return value is `bool`.
- Response structs contain `ok`.
- Registry-backed diagnostics usually include `RegistryResult registry_result`.
- Command responses include `CommandState`.
- Error visibility uses compact `const char* error_code`.

Future result rules:

- Continue using `bool` method return for simple API success/failure.
- Continue using `ok` inside response structs.
- Include domain-specific result fields when useful:
  - `RegistryResult` for Registry reads
  - `CommandState` for command execution state
  - future validation result enums only when approved
- Keep error codes compact and string-literal based.
- Use `"none"` for successful `error_code` fields.
- Do not add dynamically allocated error messages.
- Do not add JSON-style error objects in this phase.

## 7. Capability-First API Rules

Normal UI reads must prefer `CAP_*` capability contracts over hardware identity.

Capability-first rules:

- UI clients bind to `CAP_*` IDs.
- Capability cards display capability payloads.
- Logic Builder blocks bind to capabilities.
- Custom project pages bind widgets to capabilities.
- Templates declare required and optional capabilities.

Allowed diagnostic metadata:

- provider ID
- provider type
- active provider flag
- node ID
- source MAC
- allow/trust state
- security diagnostic counters

These fields are diagnostics and management metadata. They must not replace `CAP_*` IDs as the normal UI/Logic contract.

Forbidden:

- API normal reads exposing driver names as the primary contract.
- Logic depending on provider IDs.
- UI widgets commanding hardware by driver/module/pin names.
- API parsing ESP-NOW or other wireless packet bytes.

## 8. Read-Only vs Command API Separation

Read-only APIs:

- inspect Registry-backed state
- inspect Runtime state where already attached
- inspect command-state records
- inspect diagnostics
- inspect summaries
- do not mutate state
- do not call Drivers or Devices
- do not execute commands

Command APIs:

- request state-changing behavior
- go through Services only
- receive bounded request structs
- return bounded response structs
- preserve Runtime safety gates
- preserve actuator safety policy

Separation rule:

```text
Read method names start with get...
Command method names start with command...
```

No future method should mix read-only diagnostics and command execution.

## 9. Diagnostics Naming Rules

Diagnostics naming rules:

- Use `Diagnostic` for one detailed diagnostic record.
- Use `Summary` for aggregate counts.
- Use `SecurityDiagnostic` for wireless security counters and accepted/rejected packet visibility.
- Use `ProviderDiagnostic` for provider record status, priority, payload, and active mapping visibility.
- Use `NodeDiagnostic` for node identity, allow/trust state, last seen, and node-level fields.
- Use `ActiveProviderDiagnostic` only when the method resolves the active provider for a capability.

Diagnostic method rules:

- `getProviderDiagnostic(provider_id, out)`
- `getActiveProviderDiagnostic(capability_id, out)`
- `getWirelessNodeDiagnostic(node_id, out)`
- `getWirelessNodeDiagnosticByIndex(index, out)`
- `getWirelessSecurityDiagnostic(node_id, out)`
- `getWirelessSecurityDiagnosticByIndex(index, out)`
- future methods should follow the same pattern.

Diagnostic boundaries:

- Diagnostics are read-only unless a future explicit reset/clear milestone exists.
- Diagnostics reads must not update provider payloads.
- Diagnostics reads must not update canonical payloads.
- Diagnostics reads must not change active provider mappings.
- Diagnostics reads must not parse packets.

## 10. Future UI Client Compatibility Rules

All future UI clients must use the same API contracts:

- Dev Panel
- Minimal App
- Mission Control
- Dashboard
- Cloud Bridge
- Marketplace
- AI Assistant

Compatibility rules:

- API remains the only UI-facing contract.
- No UI client gets privileged direct Driver, Device, HAL, Registry array, or wireless packet access.
- The same capability state contract should support Dev Panel cards, Minimal App sensor views, Mission Control widgets, Dashboard panels, Cloud Bridge summaries, Marketplace templates, and AI Assistant explanations.
- Contracts must be bounded and deterministic so they can run on ESP32.
- Contracts should be composable for future Project pages and Mini App Builder widgets.
- Future transport layers such as REST, WebSocket, or Cloud may wrap these contracts later, but this phase does not implement those transports.
- UI screens should be designed so they can later become custom project pages.
- Template Scripts and Logic Builder blocks must compile into capability-first rules, not arbitrary hardware scripts.

## 11. Stop Conditions

Stop and return to architecture review if any of these occur during Phase 1 or follow-on contract phases:

- source code is modified during documentation-only phases
- headers are created before an approved implementation phase
- source files are created before an approved implementation phase
- WebServer implementation starts
- HTTP routes are added
- JSON serialization is added
- app implementation starts
- Dashboard implementation starts
- Cloud implementation starts
- Marketplace implementation starts
- AI implementation starts
- API calls Drivers
- API calls Devices
- API calls HAL
- API parses wireless packets
- API writes Registry arrays directly
- command APIs bypass Services
- Runtime architecture changes
- Registry ownership changes
- Service policy is bypassed
- STL containers are introduced
- Arduino `String` is introduced
- heap allocation is introduced
- contracts become unbounded
- UI clients receive privileged hardware access

## 12. Recommended Next Phase

Recommended next phase:

```text
Milestone 9.5 Phase 2 - System API structs only
```

Phase 2 should define bounded system-level API structs only. It should not implement methods yet unless a later milestone explicitly expands scope.

Recommended Phase 2 focus:

- system identity summary
- firmware/build summary
- runtime state summary
- mode summary
- uptime placeholder
- compact error/status fields

Phase 2 must remain:

- internal C++ contract only
- bounded
- ESP32-safe
- no WebServer
- no HTTP
- no JSON
- no source behavior changes beyond explicitly approved struct definitions

