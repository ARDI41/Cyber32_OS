# Milestone 10.4 - Minimal App Separate Project Architecture Plan

## 1. Purpose

Define the architecture for building the Cyber32 Minimal App as a separate project outside the Cyber32 Core OS firmware.

The Minimal App will consume Cyber32 API-shaped models first through deterministic mock data, and later through an approved transport-backed API client. The app must remain separate from Core OS implementation details. Cyber32 Core OS must not contain UI or app implementation logic.

Cyber32Api remains the source contract for app-facing system, node, capability, provider, diagnostics, and future command models.

## 2. Scope

This milestone covers architecture planning only.

It defines:

- Recommended app project boundary
- Recommended first app technology direction
- App-side layering
- API-shaped app models
- Mock API client behavior
- First screens
- Empty-state UX rules
- Development flow
- Future live transport boundary
- Testing strategy
- Core OS protection rules

No app code is implemented in this milestone.

## 3. Non-Goals

This milestone does not:

- implement the Minimal App
- create app project folders
- add WebServer support
- add HTTP support
- add JSON support
- add BLE support
- add WiFi live transport
- add WebSocket transport
- parse ESP-NOW packets
- access Registry arrays directly
- call Services directly from the app
- call Drivers
- call Devices
- call HAL
- select providers
- modify Runtime architecture
- modify Registry ownership
- modify `src/main.cpp`
- add UI/App/Dashboard/Cloud/Marketplace/AI code to Cyber32 Core OS

## 4. Recommended App Technology

Candidate options:

- Flutter
- React Native
- simple web/PWA prototype
- native Android later

Recommended first direction: Flutter.

Reasons:

- mobile-first from the beginning
- supports Android now and iOS later
- cleanly separates app work from Core OS firmware
- supports mock-data development well
- can later consume HTTP, BLE, WebSocket, or another approved transport adapter
- allows UI screens to be built before Core transport exists

A web/PWA mock may be useful later for quick dashboard-style testing, but the Minimal App should start as a separate mobile-first app.

## 5. Project Boundary

The Minimal App must live outside Cyber32 Core OS firmware folders.

Acceptable layout options:

Option A - separate repository:

```text
Cyber32_Minimal_App/
```

Option B - sibling project in a broader workspace:

```text
Cyber32/
  Cyber32_OS/
  Cyber32_Minimal_App/
```

Recommendation:

Use either a sibling project folder or a separate repository. Do not place the Minimal App inside `Cyber32_OS/src/` or any Core OS firmware source folder.

## 6. App Architecture Layers

The Minimal App should use these layers:

- UI Screens
- ViewModels
- Mock Api Client
- Future Transport Api Client
- API Models
- Local App State

Layer rules:

- UI Screens depend on ViewModels.
- ViewModels depend on API-shaped models.
- MockApiClient and future TransportApiClient expose the same interface.
- No screen should know whether data came from mock data or live Core transport.
- API-shaped models mirror Cyber32 API contracts semantically.
- App code must never call Core OS Drivers, Devices, HAL, Registry arrays, packet parsers, or transport internals.

Recommended flow:

```text
UI Screen
-> ViewModel
-> ApiClient interface
-> MockApiClient or Future TransportApiClient
-> API-shaped models
```

Future live flow:

```text
Minimal App
-> ApiClient interface
-> TransportApiClient
-> approved transport adapter
-> Cyber32Api
-> Core OS
```

## 7. API-Shaped Models

App-side models should mirror Cyber32 API contracts semantically. These are app models only; this milestone does not create Core OS code.

System models:

- `SystemSummaryModel`
- `SystemIdentityModel`
- `SystemFirmwareModel`
- `SystemRuntimeModel`
- `SystemModesModel`
- `SystemMemoryModel`

Node models:

- `NodeListModel`
- `NodeSummaryModel`
- `NodeIdentityModel`
- `NodeStatusModel`
- `NodePowerModel`
- `NodeSignalModel`
- `NodeDiagnosticsModel`
- `NodeCapabilityModel`

Capability models:

- `CapabilityListModel`
- `CapabilitySummaryModel`
- `CapabilityIdentityModel`
- `CapabilityValueModel`
- `CapabilityAvailabilityModel`
- `CapabilityProviderInfoModel`
- `CapabilityQualityModel`

Model rules:

- Preserve `ok` semantics.
- Preserve `error_code` semantics.
- Preserve empty-list behavior.
- Preserve `node_not_found` behavior.
- Preserve `capability_not_found` behavior.
- Preserve deterministic defaults as unavailable state, not real readings.

## 8. First Mock API Client

The first Minimal App implementation should use a deterministic `MockApiClient`.

Initial mock responses:

- successful system summary
- empty node list
- empty capability list
- `node_not_found` response
- `capability_not_found` response
- unavailable provider metadata
- no fake sensor values
- no fake nodes
- no fake capabilities

Mock API rules:

- `getSystemSummary` may return successful placeholder system data.
- `getNodeList` returns `ok = true`, `error_code = "none"`, `count = 0`.
- Node detail reads return `ok = false`, `error_code = "node_not_found"`.
- `getCapabilityList` returns `ok = true`, `error_code = "none"`, `count = 0`.
- Capability detail reads return `ok = false`, `error_code = "capability_not_found"`.
- Zero numeric defaults must not be displayed as real sensor readings.
- `PayloadValueType::NONE` means no value is available.

## 9. First Screens

Initial screens should prove the API-first empty-state contract before live transport exists.

### A. Connection / System Screen

Shows:

- Core identity
- firmware version
- runtime state
- system modes
- memory placeholders

### B. Node List Screen

Shows:

- empty-state when node count is `0`
- no invented nodes
- no hardware assumptions

### C. Capability List Screen

Shows:

- empty-state when capability count is `0`
- no invented capabilities
- no fake sensor cards

### D. Node Detail Empty-State

Shows:

- `node_not_found`
- no default node identity as if it were real
- no fake MAC, RSSI, battery, status, or capability data

### E. Capability Detail Empty-State

Shows:

- `capability_not_found`
- no zero defaults as readings
- no fake provider metadata
- no fake timestamp

### F. Diagnostics Empty-State

Shows:

- system/API diagnostics placeholders
- node diagnostics unavailable until owner-backed data exists
- capability diagnostics unavailable until owner-backed data exists

## 10. Empty-State UX Rules

The app must treat empty and missing states carefully.

Rules:

- An empty node list is not an error.
- An empty capability list is not an error.
- `node_not_found` means the selected node does not exist.
- `capability_not_found` means the selected capability does not exist.
- Zero defaults after a failed read are not real readings.
- `PayloadValueType::NONE` means no capability value.
- No provider means no provider metadata.
- No RSSI flag means no signal data.
- No battery flag means no battery data.
- Unavailable diagnostics must be shown as unavailable, not as healthy.

## 11. Development Flow

Recommended implementation sequence for the future Minimal App project:

1. Create separate Minimal App project.
2. Define app-side API models.
3. Define `ApiClient` interface.
4. Implement `MockApiClient`.
5. Build Connection/System screen.
6. Build Node list empty-state screen.
7. Build Capability list empty-state screen.
8. Build Node detail empty-state screen.
9. Build Capability detail empty-state screen.
10. Build Diagnostics empty-state screen.
11. Add local UI tests or manual checklist.
12. Later replace `MockApiClient` with approved transport-backed `ApiClient`.

## 12. Future Live Transport Switch

The Minimal App must not choose or implement live transport in this milestone.

Future shape:

```text
Minimal App
-> ApiClient interface
-> TransportApiClient
-> approved transport adapter
-> Cyber32Api
-> Core OS
```

Possible future transports may include HTTP, BLE, WebSocket, or another bounded adapter. The transport decision must be made in a future approved milestone.

Live transport rules:

- Transport client must expose the same interface as MockApiClient.
- UI screens must not know the transport type.
- App must not parse ESP-NOW packets.
- App must not access Registry arrays.
- App must not call Drivers, Devices, HAL, or Services directly.
- Commands must wait for approved command API and safety policy milestones.

## 13. Testing Strategy

Future Minimal App tests should verify:

- system summary renders successfully
- empty node list renders as empty-state
- empty capability list renders as empty-state
- `node_not_found` renders as missing node
- `capability_not_found` renders as missing capability
- zero defaults are not displayed as real readings
- unavailable provider metadata is shown as unavailable
- unavailable diagnostics are shown as unavailable
- mock and live clients share the same interface
- screens remain API-first and capability-first

Testing should begin with mock data and no live Core OS dependency.

## 14. Core OS Protection Rules

Future app work must not weaken Cyber32 Core OS architecture.

Protection rules:

- no app code inside Core OS `src/`
- no app screens in firmware
- no app-specific logic in Cyber32Api
- no direct Registry calls from app
- no direct Driver calls from app
- no direct Device calls from app
- no direct HAL calls from app
- no packet parsing in app
- no provider selection in app
- no command execution before command API approval
- no deterministic defaults displayed as real sensor data
- no modification to `src/main.cpp` for app work

The Core OS owns Registry, Runtime, Services, Logic, Devices, Drivers, HAL, packet ingestion, provider update policy, and safety policy. The Minimal App consumes API-shaped contracts only.

## 15. Stop Conditions

Stop implementation and return to architecture review if any future work requires:

- creating the app inside Core OS `src/`
- adding WebServer, HTTP, or JSON to Core OS without an approved transport milestone
- adding live transport before approval
- parsing packets in the app
- reading Registry arrays from the app
- calling Drivers from the app
- calling Devices from the app
- calling HAL from the app
- calling Services directly from the app
- selecting providers in the app
- showing default zero values as real sensor data
- modifying `src/main.cpp`

## 16. Recommended Next Milestone

Recommended next milestone:

Milestone 10.5 - Minimal App Mock Data Contract

Purpose:

Define exact mock response payloads and app-side model expectations for System, Node, and Capability empty-state screens before creating the actual app project.

Milestone 10.5 should remain documentation-only unless implementation is explicitly approved.
