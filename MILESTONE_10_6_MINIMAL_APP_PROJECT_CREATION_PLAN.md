# Milestone 10.6 - Minimal App Project Creation Plan

## 1. Purpose

Define the future project creation steps for the Cyber32 Minimal App as a separate project outside Cyber32 Core OS firmware folders.

This plan documents the future commands, folder layout, Flutter setup, first files, mock client file structure, routing, manual run checklist, testing checklist, and commit strategy.

Cyber32Api remains the source contract. The first app implementation must use mock data only, following `MILESTONE_10_5_MINIMAL_APP_MOCK_DATA_CONTRACT.md`.

## 2. Scope

This milestone plans project creation only.

It covers:

- recommended future app location
- future Flutter project creation command
- initial Flutter folder structure
- first app files and responsibilities
- first screen routing
- future implementation order
- mock response source
- no-live-transport boundary
- manual run checklist
- app testing checklist
- commit strategy
- Core OS protection rules

No implementation is performed in this milestone.

## 3. Non-Goals

This milestone does not:

- create the app project
- implement app code
- modify Core OS source files
- modify `src/main.cpp`
- run `flutter create`
- implement WebServer
- implement HTTP
- implement JSON
- implement BLE
- implement WiFi transport
- implement live transport
- parse packets
- access Registry state
- access Registry arrays
- call Services
- call Drivers
- call Devices
- call HAL
- select providers
- execute commands
- modify Runtime architecture
- modify Registry ownership

## 4. Recommended Project Location

The Minimal App must remain outside Cyber32 Core OS firmware folders.

Acceptable future layouts:

Option A - sibling workspace:

```text
Cyber32/
  Cyber32_OS/
  Cyber32_Minimal_App/
```

Option B - separate repository:

```text
Cyber32_Minimal_App/
```

Recommendation:

Use the sibling project folder during the early phase:

```text
Cyber32/
  Cyber32_OS/
  Cyber32_Minimal_App/
```

Reasons:

- keeps the app outside Core OS
- keeps firmware and app development close during early iteration
- makes it easier to compare app models against Cyber32 API contracts
- can be split into a separate repository later
- avoids placing generated Flutter files inside `Cyber32_OS/src/`

## 5. Future Flutter Setup Command

Future command only. Do not run during this milestone.

Recommended package name:

```text
cyber32_minimal_app
```

Future command:

```text
flutter create cyber32_minimal_app
```

If using the exact folder name:

```text
flutter create Cyber32_Minimal_App
```

Recommendation:

Use the lowercase Flutter package name where Flutter package rules require it:

```text
cyber32_minimal_app
```

The generated project folder may still be placed as a sibling project outside `Cyber32_OS`.

## 6. Initial Flutter Folder Structure

Planned first app structure:

```text
lib/
  main.dart
  app/
    cyber32_app.dart
  models/
    system/
    node/
    capability/
  api/
    api_client.dart
    mock_api_client.dart
  viewmodels/
    system_view_model.dart
    node_list_view_model.dart
    capability_list_view_model.dart
    diagnostics_view_model.dart
  screens/
    system_screen.dart
    node_list_screen.dart
    capability_list_screen.dart
    diagnostics_screen.dart
  widgets/
    empty_state_card.dart
    status_chip.dart
    value_placeholder_card.dart

test/
  mock_api_client_test.dart
  empty_state_ui_test.dart
```

Structure rules:

- `models/` contains app-side API-shaped models only.
- `api/` contains the app-side client interface and mock client.
- `viewmodels/` converts API-shaped responses into UI state.
- `screens/` renders first app screens.
- `widgets/` contains reusable UI components.
- no Core OS source files are imported or copied into the app.

## 7. First App Files

### `lib/main.dart`

Purpose:

Starts the Minimal App.

Responsibilities:

- create the app root
- keep bootstrapping small
- do not connect to live Core transport in the first version

### `lib/app/cyber32_app.dart`

Purpose:

Defines app shell, theme, and routes.

Responsibilities:

- configure `MaterialApp`
- define initial route
- define basic navigation routes
- keep UI API-first and mock-client driven

### `lib/api/api_client.dart`

Purpose:

Defines the app-side `ApiClient` interface matching Cyber32 API views.

Responsibilities:

- expose conceptual system reads
- expose conceptual node reads
- expose conceptual capability reads
- remain transport-agnostic
- avoid Core OS implementation details

### `lib/api/mock_api_client.dart`

Purpose:

Returns deterministic mock responses from Milestone 10.5.

Responsibilities:

- return system summary success
- return empty node list
- return empty capability list
- return `node_not_found`
- return `capability_not_found`
- return provider unavailable
- return diagnostics unavailable
- never invent real nodes, capabilities, providers, or sensor readings

### `lib/models/`

Purpose:

Contains API-shaped app models only.

Responsibilities:

- mirror Cyber32 API semantics
- preserve `ok` and `error_code`
- preserve empty-list behavior
- preserve deterministic default semantics
- remain app-side models, not Core OS structs

### `lib/viewmodels/`

Purpose:

Convert API-shaped models into UI state.

Responsibilities:

- distinguish empty state from error state
- distinguish unavailable defaults from real data
- prevent zero defaults from being displayed as real sensor readings
- keep screens independent from mock/live data source

### `lib/screens/`

Purpose:

Render the first empty-state app screens.

Initial screens:

- system screen
- node list screen
- capability list screen
- diagnostics screen

### `lib/widgets/`

Purpose:

Provide shared UI components.

Initial widgets:

- `empty_state_card.dart`
- `status_chip.dart`
- `value_placeholder_card.dart`

Rules:

- widgets are UI composition only
- widgets do not call transport directly
- widgets do not know Drivers, Devices, HAL, Registry arrays, packets, or providers

## 8. First Screen Routing

Planned initial routes:

```text
/system
/nodes
/capabilities
/diagnostics
```

Default route:

```text
/system
```

Optional bottom navigation:

- System
- Nodes
- Capabilities
- Diagnostics

Routing rules:

- routes are app-side only
- no Core OS route or WebServer behavior is implied
- route names must not imply live transport exists

## 9. First Implementation Order

Recommended future implementation order:

1. Create Flutter project outside `Cyber32_OS`.
2. Add folder structure.
3. Add model classes for System summary.
4. Add `ApiClient` interface.
5. Add `MockApiClient` with system summary, empty node list, and empty capability list.
6. Build System screen.
7. Build Node list empty-state screen.
8. Build Capability list empty-state screen.
9. Build Diagnostics empty-state screen.
10. Add `node_not_found` detail-state placeholders.
11. Add `capability_not_found` detail-state placeholders.
12. Add manual test checklist.
13. Commit app project separately.

The first implementation milestone must not add live transport.

## 10. Mock Response Source

Mock responses must follow:

```text
MILESTONE_10_5_MINIMAL_APP_MOCK_DATA_CONTRACT.md
```

Required initial mocks:

- system summary success
- empty node list
- empty capability list
- `node_not_found`
- `capability_not_found`
- provider unavailable
- diagnostics unavailable

Rules:

- no fake sensor values
- no fake nodes
- no fake capabilities
- no fake providers
- deterministic defaults mean unavailable state unless explicitly marked as real

## 11. No Live Transport Yet

The first app version must not connect to ESP32.

The first app version must not require:

- WiFi
- BLE
- HTTP
- WebServer
- JSON
- WebSocket
- serial protocol
- live Cyber32 Core transport

The first app version must not:

- parse ESP-NOW packets
- access Registry state
- access Registry arrays
- call Services
- call Drivers
- call Devices
- call HAL
- select providers
- execute commands

Live transport comes later after an approved transport milestone.

## 12. Manual Run Checklist

Future commands only. Do not run during this milestone.

Future manual commands:

```text
flutter pub get
flutter run
```

Expected first UI behavior:

- System screen opens by default
- System screen shows Cyber32 Core placeholder identity
- System screen shows firmware placeholder version
- System screen shows runtime placeholder state
- Nodes screen shows empty state
- Capabilities screen shows empty state
- Diagnostics screen shows unavailable/placeholder state
- no fake sensor values appear
- no fake nodes appear
- no fake capabilities appear
- no live Core connection is required

## 13. App Testing Checklist

Future tests should verify:

- `MockApiClient` returns system summary `ok`
- `MockApiClient` returns empty node list
- `MockApiClient` returns empty capability list
- `node_not_found` maps to missing node UI
- `capability_not_found` maps to missing capability UI
- `PayloadValueType::NONE` does not render as a value
- zero numeric defaults do not render as sensor readings
- false bool defaults do not render as real false sensor states
- provider `UNKNOWN` renders as unavailable
- diagnostics zero counters on failed reads render as unavailable
- no UI screen requires live Core transport
- mock client can later be replaced by transport client

## 14. Commit Strategy

Core OS commits must remain separate from Minimal App commits.

Recommended future strategy:

- Minimal App should have its own git repository or separate commit history.
- Do not commit generated Flutter files into `Cyber32_OS` unless a monorepo decision is explicitly approved.
- If using a sibling folder, commit from the Minimal App project root, not from `Cyber32_OS`.
- Core OS milestone documentation may reference the app project, but firmware source should remain separate.

Rules:

- no generated Flutter app files inside `Cyber32_OS/src/`
- no app implementation committed to Core OS source paths
- no Core OS source changes for first app bootstrap

## 15. Core OS Protection Rules

Protection rules:

- no app code inside `Cyber32_OS/src/`
- no app screens in firmware
- no app-specific Cyber32Api behavior
- no WebServer in Core OS for this milestone
- no HTTP in Core OS for this milestone
- no JSON in Core OS for this milestone
- no live transport
- no packet parsing
- no Registry access from app
- no Registry array access from app
- no Services calls from app
- no Drivers calls from app
- no Devices calls from app
- no HAL calls from app
- no provider selection from app
- no command execution before command API approval
- no `src/main.cpp` changes

## 16. Stop Conditions

Stop and return to architecture review if future work tries to:

- create the app inside `Cyber32_OS/src/`
- add WebServer, HTTP, or JSON to Core OS
- implement live ESP32 transport now
- parse packets in the app
- read Registry arrays from the app
- call Drivers from the app
- call Devices from the app
- call HAL from the app
- call Services directly from the app
- select providers in the app
- show deterministic defaults as real sensor values
- modify `src/main.cpp`

## 17. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.7 - Minimal App Project Bootstrap
```

Purpose:

Create the actual separate Flutter project with mock-only API client and empty-state screens.

Important:

- this will be the first app implementation milestone
- it must happen outside `Cyber32_OS` firmware source folders
- it must not modify Core OS code
- it must not add live transport
- it must use mock data only
