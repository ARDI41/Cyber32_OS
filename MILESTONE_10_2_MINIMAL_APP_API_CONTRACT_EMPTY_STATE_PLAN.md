# Milestone 10.2 - Minimal App API Contract and Empty-State UI Plan

## 1. Scope

Milestone 10.2 defines how the future Minimal App should consume Cyber32 API read-only contracts to render the first UI screens safely.

This plan covers API consumption, empty-state UI behavior, conceptual view models, read cadence, error mapping, strict boundaries, and future real-data dependencies.

The Minimal App must consume Cyber32 API only.

The Minimal App must not:

- access Registry arrays
- parse packets
- parse ESP-NOW payloads
- call Services directly
- call Drivers
- call Devices
- call HAL
- choose providers
- mutate Runtime
- mutate Registry
- mutate capability payloads
- bind primary UI identity to hardware names

This milestone is documentation only. It does not implement an app, transport, WebServer, HTTP, JSON, Dashboard, Cloud, Marketplace, AI, packet parsing, discovery, pairing, or source code changes.

## 2. Non-Goals

Non-goals:

- no app implementation
- no WebServer
- no HTTP
- no JSON
- no UI code inside Core OS
- no Dashboard
- no Cloud
- no Marketplace
- no AI implementation
- no transport code
- no pairing implementation
- no discovery implementation
- no provider selection
- no command execution
- no Registry ownership changes
- no Runtime architecture changes
- no `src/main.cpp` changes

## 3. API Dependency Map

The Minimal App may consume only Cyber32 API methods exposed through `Cyber32Api` or future approved API transport boundaries.

### System API

Allowed System API reads:

- `getSystemSummary(...)`
- `getSystemIdentity(...)`
- `getSystemFirmware(...)`
- `getSystemRuntime(...)`
- `getSystemModes(...)`
- `getSystemMemory(...)`

System API data is used for connection, Core identity, firmware/build visibility, runtime status, operating mode hints, and memory/status placeholders.

### Node API

Allowed Node API reads:

- `getNodeList(...)`
- `getNodeSummary(...)`
- `getNodeIdentity(...)`
- `getNodeStatus(...)`
- `getNodePower(...)`
- `getNodeSignal(...)`
- `getNodeDiagnostics(...)`
- `getNodeCapabilities(...)`

Node API data is used for node-list screens, node detail screens, battery/signal cards, diagnostics cards, and future node-to-capability views.

Current Node API behavior is safe empty-state behavior. `getNodeList(...)` returns an empty list until a public node owner exists. Detail methods return `node_not_found` when the requested index is unavailable.

### Capability API

Allowed Capability API reads:

- `getCapabilityList(...)`
- `getCapabilitySummary(...)`
- `getCapabilityIdentity(...)`
- `getCapabilityValue(...)`
- `getCapabilityAvailability(...)`
- `getCapabilityProviderInfo(...)`
- `getCapabilityQuality(...)`

Capability API data is used for capability-list screens, value cards, stale/offline indicators, provider metadata diagnostics, and quality/freshness displays.

Current Capability API behavior is safe empty-state behavior. `getCapabilityList(...)` returns an empty list until a public capability owner exists. Detail methods return `capability_not_found` when the requested index is unavailable.

## 4. First Minimal App Screens

### A. Connection/System Screen

Purpose:

Show that the app can reach Cyber32 Core and display safe system information.

API reads:

- `getSystemSummary(...)`
- fallback child reads if summary is unavailable:
  - `getSystemIdentity(...)`
  - `getSystemFirmware(...)`
  - `getSystemRuntime(...)`
  - `getSystemModes(...)`
  - `getSystemMemory(...)`

Display fields:

- Core identity
- friendly name
- firmware version
- build version placeholder
- runtime state
- ready/running state
- local mode
- developer mode
- memory placeholder/status

Empty/error behavior:

- if System API returns `"none"`, display normal system state or placeholders explicitly as placeholders
- if Runtime is not attached, show Core not ready or runtime unavailable
- do not infer Core readiness from WiFi, ESP-NOW, or hardware state

### B. Node List Screen

Purpose:

Show known nodes once a public node owner exists.

API reads:

- `getNodeList(...)`

Current behavior:

- if `getNodeList(...)` returns `true` with `count == 0`, show an empty node-list state
- this means no known nodes are exposed through the public API yet
- this is not an error

Rules:

- no fake nodes
- no hard-coded discovered node cards
- no node cards inferred from MAC addresses, packets, providers, diagnostics, or hardware

Future behavior:

- when an owner-backed node list exists, render bounded node cards from API records only

### C. Capability List Screen

Purpose:

Show known capabilities once a public capability owner exists.

API reads:

- `getCapabilityList(...)`

Current behavior:

- if `getCapabilityList(...)` returns `true` with `count == 0`, show an empty capability-list state
- this means no known capabilities are exposed through the public API yet
- this is not an error

Rules:

- no fake sensor cards
- no hard-coded `CAP_TEMPERATURE` card unless returned by API
- no capability cards inferred from packets, providers, Registry arrays, or hardware

Future behavior:

- when an owner-backed capability list exists, render bounded capability cards from API records only

### D. Sensor/Capability Detail Screen

Purpose:

Show detail for a selected capability index.

API reads:

- `getCapabilitySummary(...)`
- `getCapabilityIdentity(...)`
- `getCapabilityValue(...)`
- `getCapabilityAvailability(...)`
- `getCapabilityProviderInfo(...)`
- `getCapabilityQuality(...)`

Current behavior:

- if `getCapabilitySummary(...)` returns `capability_not_found`, show a capability empty-state such as `No capability data yet`
- if `getCapabilityValue(...)` returns `capability_not_found`, do not display zero numeric defaults as real sensor readings
- if availability reports unavailable/stale/no-provider, display that state honestly

Rules:

- zero default value is not a real reading on failed reads
- `PayloadValueType::NONE` means no usable value
- no provider ID means no provider metadata
- no quality value means no quality metadata

### E. Node Detail Screen

Purpose:

Show detail for a selected node index.

API reads:

- `getNodeSummary(...)`
- `getNodeIdentity(...)`
- `getNodeStatus(...)`
- `getNodePower(...)`
- `getNodeSignal(...)`
- `getNodeDiagnostics(...)`
- `getNodeCapabilities(...)`

Current behavior:

- if `getNodeSummary(...)` or detail methods return `node_not_found`, show a node empty-state such as `No node data yet`
- deterministic defaults must not be displayed as real node state

Rules:

- `node_id = 0` on failure is not a real node ID
- no source MAC means no MAC metadata
- no battery flags means no battery data
- no RSSI flag means no signal data
- zero counters on failure are not real diagnostics

### F. Diagnostics Screen

Purpose:

Show safe API/system diagnostics first and expand to node/capability diagnostics only when owner-backed records exist.

API reads:

- System summary methods
- Node diagnostics methods when node owner exists
- Capability provider/quality methods when capability owner exists
- existing diagnostics APIs only where read-only contracts already exist

Current behavior:

- show API/system diagnostics only
- node diagnostics remain empty until owner-backed node records exist
- capability diagnostics remain empty until owner-backed capability records exist

Rules:

- diagnostics reads must not reset counters
- diagnostics reads must not mutate provider status
- diagnostics reads must not update stale/lost state
- diagnostics reads must not trigger provider selection

## 5. Empty-State Rules

Minimal App empty-state rules:

- `getNodeList(...)` with `count == 0` means no known nodes are exposed through the API; it is not an error
- `getCapabilityList(...)` with `count == 0` means no known capabilities are exposed through the API; it is not an error
- `node_not_found` means the requested node is not available
- `capability_not_found` means the requested capability is not available
- zero numeric defaults on failed reads must not be shown as real readings
- unavailable/stale/no-provider states must not be hidden
- deterministic default text such as `"none"` must not be presented as discovered user data
- UI must distinguish empty state, unavailable state, and real data state
- UI must never backfill missing API data from packets, Registry arrays, hardware names, or provider guesses

Recommended user-facing empty-state concepts:

- No Core data yet
- No nodes found yet
- No capabilities found yet
- No capability data yet
- No node data yet
- Provider metadata unavailable
- Diagnostics unavailable

Exact UI wording may change later, but the semantic distinction must remain.

## 6. UI Data Model

The following conceptual view models are documentation-only. They are not code and should not be implemented in Core OS during this milestone.

### SystemViewModel

Purpose:

Represent Core system status for connection and system screens.

Conceptual fields:

- connected
- core identity
- friendly name
- firmware version
- build version
- runtime state
- ready
- running
- setup mode
- developer mode
- local mode
- remote mode
- memory status placeholder
- error state

### NodeListViewModel

Purpose:

Represent the node-list screen.

Conceptual fields:

- loading state
- empty state
- node count
- node cards
- error state

### NodeCardViewModel

Purpose:

Represent one node card once owner-backed node data exists.

Conceptual fields:

- node ID
- friendly name
- provider type
- online state
- paired/trusted/blocked state
- last seen
- primary capability hint
- battery/signal indicators
- diagnostics indicator

### CapabilityListViewModel

Purpose:

Represent the capability-list screen.

Conceptual fields:

- loading state
- empty state
- capability count
- capability cards
- error state

### CapabilityCardViewModel

Purpose:

Represent one capability card once owner-backed capability data exists.

Conceptual fields:

- capability ID
- friendly name
- category
- value preview
- unit
- availability
- stale/offline state
- provider metadata indicator
- quality indicator

### CapabilityValueViewModel

Purpose:

Represent value display for sensor/capability detail screens.

Conceptual fields:

- has value
- value type
- float value
- integer value
- bool value
- unit
- timestamp
- unavailable state
- stale state
- error state

Important:

If the API read fails with `capability_not_found`, the view model must set `has value = false` and must not display numeric defaults as readings.

### DiagnosticsViewModel

Purpose:

Represent system, node, capability, and provider diagnostics when available.

Conceptual fields:

- system diagnostics summary
- node diagnostics summary
- capability provider metadata
- capability quality state
- wireless/security diagnostics later
- last error code
- empty/unavailable state

## 7. API Polling and Read Cadence

Read cadence is conceptual only. This milestone does not implement timers or app code.

Recommended cadence:

- system summary may be read periodically
- node list may be refreshed periodically
- capability list may be refreshed periodically
- detail reads should happen only for the selected node or capability index
- diagnostics reads should be periodic only where screens are visible or diagnostic panels are active

Read safety rules:

- reads must be idempotent
- reads must not mutate Core OS state
- reads must not trigger provider selection
- reads must not trigger provider health updates
- reads must not reset diagnostics
- reads must not parse packets
- reads must not call Services directly

Future app implementations may use caching, refresh throttling, or manual refresh controls, but these must preserve API-first read-only semantics.

## 8. Error Mapping

Minimal App should map compact API errors to explicit UI states.

| API error | UI meaning |
| --- | --- |
| `"none"` | normal data or successful empty list |
| `"node_not_found"` | selected node missing or unavailable |
| `"capability_not_found"` | selected capability missing or unavailable |
| `"registry_not_attached"` | Core not ready or API backend unavailable |
| `"runtime_not_attached"` | Runtime not attached or Core runtime unavailable |
| `"node_api_not_available"` | Node API data owner not ready |
| `"capability_api_not_available"` | Capability API data owner not ready |
| `"provider_not_available"` | provider metadata unavailable |

Rules:

- do not convert `"none"` plus empty list into an error
- do not show deterministic defaults as real data after a failure
- prefer explicit empty/unavailable messaging over fake cards
- preserve compact error codes for diagnostics/developer views

## 9. Minimal App Boundaries

Strict forbidden paths:

- no Registry direct reads
- no private Registry array access
- no packet parsing
- no ESP-NOW parsing
- no provider selection
- no command execution
- no Services calls
- no Driver calls
- no Device calls
- no HAL calls
- no assumptions based on MAC addresses
- no UI binding to hardware names as primary identity
- no direct transport internals
- no Core OS mutation from read screens

Correct flow:

```text
Minimal App View -> Cyber32 API -> Runtime / Services / Logic -> Capability Router -> Provider
```

For read-only screens in this phase, the app should stop at API reads and must not call Services directly.

UI must bind to node and capability API contracts and `CAP_*` IDs.

Examples:

- Temperature card binds to `CAP_TEMPERATURE` returned by API, not `DHT22`, `DS18B20`, or a packet field
- Servo UI binds to `CAP_SERVO` or an approved actuator command contract, not a driver
- Battery/signal UI binds to node diagnostics API fields, not ESP-NOW internals

## 10. Future Real-Data Dependencies

Before the Minimal App can show real devices and sensors, Cyber32 needs owner-backed public data sources.

Required future dependencies:

- public node owner
- discovery owner
- pending pairing API
- node-to-capability mapping owner
- public capability enumeration owner
- Registry-backed safe capability list
- canonical capability value read path exposed through the Capability API layer
- provider health/freshness owner
- battery/signal diagnostics owner
- friendly-name storage
- owner-backed source MAC metadata
- owner-backed unit/catalog metadata
- owner-backed quality/freshness metadata
- real empty-state to real-data transition validation

These dependencies must be implemented through documented milestones and public bounded APIs. They must not be introduced by allowing the app to parse packets, read Registry arrays, call hardware, or choose providers.

## 11. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.3 - Minimal App Transport Boundary Plan
```

Purpose:

Decide how the Minimal App will communicate with Cyber32 Core in the future without embedding HTTP, WebServer, or JSON into Core prematurely.

Recommended scope:

- documentation only unless explicitly approved otherwise
- compare possible future transport boundaries
- preserve internal `Cyber32Api` as the source contract
- define that transport layers are adapters over API, not replacements for API
- keep Core OS free of app implementation details

## 12. Stop Conditions

Stop future implementation and return to architecture review if next work tries to:

- add WebServer before contract approval
- add HTTP before contract approval
- add JSON before contract approval
- put app code into Core OS
- parse packets in the app
- read Registry arrays from the app
- call Drivers from the app
- call Devices from the app
- call HAL from the app
- call Services directly from the app
- use provider selection as UI logic
- display deterministic defaults as real sensor data
- modify `src/main.cpp`

Minimal App work must remain API-first, capability-first, and empty-state safe until documented owner-backed data paths exist.
