# Milestone 10.5 - Minimal App Mock Data Contract

## 1. Purpose

Define exact mock response payloads and model expectations for the first Cyber32 Minimal App screens before creating the actual app project.

The Minimal App will initially consume deterministic app-side mock data that matches Cyber32 API semantics. This lets UI work begin without adding Core OS transport, WebServer, HTTP, JSON, BLE, packet parsing, discovery, pairing, or provider ingestion.

The mock data must preserve:

- `ok` fields
- `error_code` fields
- empty-list semantics
- `node_not_found` semantics
- `capability_not_found` semantics
- deterministic defaults as unavailable state
- no fake nodes
- no fake capabilities
- no fake sensor readings

## 2. Scope

This milestone defines app-side mock data contracts only.

It covers:

- conceptual app-side `ApiClient` method shape
- required mock responses
- first empty-state scenarios
- UI interpretation rules
- recommended app-side model names
- first app test checklist
- future positive mock data rules
- Core OS protection rules

No implementation is performed in this milestone.

## 3. Non-Goals

This milestone does not:

- implement the Minimal App
- create app folders
- implement live transport
- implement WebServer
- implement HTTP
- implement JSON
- implement BLE
- implement WiFi transport
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
- modify `src/main.cpp`

## 4. Mock ApiClient Contract

The first Minimal App should define an app-side conceptual `ApiClient` interface that matches approved Cyber32 API views semantically.

This is an app-side contract only. Do not create Core OS code in this milestone.

### System Methods

Conceptual methods:

- `getSystemSummary()`
- `getSystemIdentity()`
- `getSystemFirmware()`
- `getSystemRuntime()`
- `getSystemModes()`
- `getSystemMemory()`

### Node Methods

Conceptual methods:

- `getNodeList()`
- `getNodeSummary(index)`
- `getNodeIdentity(index)`
- `getNodeStatus(index)`
- `getNodePower(index)`
- `getNodeSignal(index)`
- `getNodeDiagnostics(index)`
- `getNodeCapabilities(index)`

### Capability Methods

Conceptual methods:

- `getCapabilityList()`
- `getCapabilitySummary(index)`
- `getCapabilityIdentity(index)`
- `getCapabilityValue(index)`
- `getCapabilityAvailability(index)`
- `getCapabilityProviderInfo(index)`
- `getCapabilityQuality(index)`

### Contract Rules

Rules:

- MockApiClient and future TransportApiClient must expose the same interface.
- UI screens must not know whether data came from mock data or live Core transport.
- Mock responses must preserve Cyber32 API error and empty-state semantics.
- Mock responses must not invent real-looking hardware, nodes, capabilities, providers, packets, or readings.

## 5. Required Mock Responses

### A. System Summary Success Mock

Conceptual response:

```text
ok = true
error_code = "none"
identity.ok = true
firmware.ok = true
runtime.ok = true
modes.ok = true
memory.ok = true
```

Suggested stable placeholder fields:

```text
identity.core_uuid = "cyber32-core-dev"
identity.friendly_name = "Cyber32 Core"
identity.owner_state = "unprovisioned"
identity.provisioning_state = "development"
identity.pairing_state = "manual"

firmware.firmware_version = "dev"
firmware.build_version = "dev"
firmware.hardware_revision = "unknown"
firmware.protocol_version = "1"

runtime.runtime_state = deterministic placeholder
runtime.uptime_ms = 0
runtime.safe_mode = false
runtime.ready = true
runtime.running = true

modes.setup_mode = false
modes.developer_mode = true
modes.local_mode = true
modes.remote_mode = false

memory.free_heap = 0
memory.minimum_free_heap = 0
memory.registry_capacity summary placeholders = 0
```

Interpretation:

- system placeholder data may be displayed as development system state
- memory values are placeholders/unavailable, not measured memory
- runtime ready/running fields are deterministic mock placeholders until live transport exists

### B. Empty Node List Mock

Conceptual response:

```text
ok = true
error_code = "none"
count = 0
entries = empty
```

Meaning:

No known nodes are exposed through the API.

Rules:

- empty node list is not an error
- do not create fake node cards
- do not infer a node from MAC addresses, packets, diagnostics, providers, or hardware

### C. Node Detail Not-Found Mock

For all node detail calls with `index = 0` while the node list is empty:

```text
return false or response ok = false, depending on app model style
error_code = "node_not_found"
deterministic defaults
```

Required absence:

- no fake `node_id`
- no fake MAC
- no fake battery
- no fake RSSI
- no fake diagnostics
- no fake capabilities
- no fake trusted/paired/online state

Suggested deterministic defaults:

```text
node_id = 0
friendly_name = null or empty placeholder
provider_type = UNKNOWN
has_source_mac = false
source_mac = all zeroes if storage exists
online = false
paired = false
trusted = false
blocked = false
last_seen_ms = 0
has_battery_percent = false
has_battery_mv = false
battery_percent = 0
battery_mv = 0
has_rssi = false
rssi = 0
signal_quality = 0
accepted_packet_count = 0
rejected_packet_count = 0
last_error_code = "none"
has_security_diagnostics = false
capability_count = 0
```

Interpretation:

Defaults after `node_not_found` are unavailable state, not real node state.

### D. Empty Capability List Mock

Conceptual response:

```text
ok = true
error_code = "none"
count = 0
entries = empty
```

Meaning:

No known capabilities are exposed through the API.

Rules:

- empty capability list is not an error
- do not create fake capability cards
- do not hard-code a `CAP_TEMPERATURE` card unless returned by API
- do not infer capabilities from packets, providers, diagnostics, transport internals, or hardware

### E. Capability Detail Not-Found Mock

For all capability detail calls with `index = 0` while the capability list is empty:

```text
return false or response ok = false, depending on app model style
error_code = "capability_not_found"
deterministic defaults
```

Required absence:

- no fake capability ID
- no fake friendly name
- no fake unit
- no fake provider
- no fake quality
- no fake timestamp
- no fake sensor reading

Suggested deterministic defaults:

```text
capability_id = 0 or invalid default
friendly_name = null or empty placeholder
category = null or empty placeholder
value_type = PayloadValueType::NONE
value_float = 0
value_int = 0
value_bool = false
unit = null
timestamp_ms = 0
available = false
stale = deterministic unavailable default
last_update_ms = 0
has_provider = false
active_provider_id = 0 or invalid default
provider_type = UNKNOWN
provider_status = UNKNOWN
owner_node_id = 0
has_owner_node = false
selected = false
quality = 0
error_code_payload = "none"
has_error = false
```

Interpretation:

- `PayloadValueType::NONE` means no value
- numeric zero defaults are unavailable, not zero sensor readings
- `false` bool default is unavailable, not a real false sensor state
- timestamp `0` means no timestamp

### F. Provider Metadata Unavailable Mock

Conceptual response:

```text
ok = false
error_code = "capability_not_found" or "provider_not_available"
provider_type = UNKNOWN
provider_status = UNKNOWN
has_owner_node = false
selected = false
```

Scenario selection:

- use `"capability_not_found"` when the selected capability does not exist
- use `"provider_not_available"` later only when a real capability exists but provider metadata is unavailable

Rules:

- no fake provider ID
- no fake provider status
- no fake selected provider
- no provider selection in the app

### G. Diagnostics Unavailable Mock

Conceptual response:

```text
ok = false where applicable
error_code = "node_not_found" or "capability_not_found"
counters = 0 but unavailable
has_security_diagnostics = false
has_error = false
error payload = "none"
```

Rules:

- zero counters on failed reads mean unavailable diagnostics, not healthy zero-error state
- diagnostics unavailable must not be displayed as a healthy node or healthy capability
- diagnostics reads must not reset counters

## 6. Mock Scenarios

### Scenario 1 - Fresh App Start, Core Reachable

State:

- Core reachable through mock client
- no nodes
- no capabilities

Expected responses:

- `getSystemSummary()` succeeds
- `getNodeList()` succeeds with `count = 0`
- `getCapabilityList()` succeeds with `count = 0`

Expected UI:

- show system placeholder data
- show empty node-list state
- show empty capability-list state
- do not show fake sensor cards

### Scenario 2 - Core / Runtime Unavailable

State:

- app cannot reach Core or mock scenario simulates unavailable runtime

Expected responses:

- system calls may return `ok = false`
- stable error code such as `"runtime_not_attached"` or future app-side `"core_unavailable"`

Expected UI:

- show Core unavailable / runtime unavailable state
- do not infer hardware state from WiFi or transport
- do not show fake nodes or capabilities

### Scenario 3 - Missing Node Detail While Node List Is Empty

State:

- `getNodeList()` returns `count = 0`
- user attempts to open node index `0`

Expected responses:

- node detail calls return `false` or `ok = false`
- `error_code = "node_not_found"`

Expected UI:

- show missing node state
- do not show `node_id = 0` as a real node
- do not show zero battery/RSSI/diagnostic defaults as real data

### Scenario 4 - Missing Capability Detail While Capability List Is Empty

State:

- `getCapabilityList()` returns `count = 0`
- user attempts to open capability index `0`

Expected responses:

- capability detail calls return `false` or `ok = false`
- `error_code = "capability_not_found"`

Expected UI:

- show missing capability state
- do not show zero values as readings
- do not show `PayloadValueType::NONE` as a real measurement type

### Scenario 5 - Diagnostics Screen With No Owner-Backed Diagnostics

State:

- no public node owner
- no public capability owner
- no owner-backed diagnostics for selected node/capability

Expected responses:

- node diagnostics return `node_not_found`
- capability quality/provider reads return `capability_not_found`
- counters remain deterministic zero defaults

Expected UI:

- show diagnostics unavailable
- do not show zero counters as proof of healthy state
- system/API diagnostics placeholders may still render if available

### Scenario 6 - Provider Metadata Unavailable

State:

- selected capability does not exist, or future scenario has real capability with no provider metadata

Expected responses:

- current empty-owner response: `capability_not_found`
- future owner-backed no-provider response may use `provider_not_available`

Expected UI:

- show provider metadata unavailable
- do not show provider type/status as real when both are `UNKNOWN`
- do not imply selected provider exists

## 7. UI Interpretation Rules

Minimal App UI must interpret mock responses carefully.

Rules:

- empty node list is not an error
- empty capability list is not an error
- `node_not_found` is a missing node state
- `capability_not_found` is a missing capability state
- `PayloadValueType::NONE` means no value
- zero numeric defaults after failed reads must not be displayed as real readings
- `false` bool value after failed reads must not be displayed as a real false sensor state
- timestamp `0` means no timestamp
- provider `UNKNOWN` means no provider metadata
- diagnostics zero counters on failed reads mean unavailable diagnostics, not healthy zero-error state
- unavailable state must be visually distinct from real zero values
- mock data must not teach users that fake hardware exists

## 8. Mock Model Naming

Recommended app-side model names:

- `SystemSummaryModel`
- `NodeListModel`
- `NodeDetailNotFoundModel`
- `CapabilityListModel`
- `CapabilityDetailNotFoundModel`
- `CapabilityValueModel`
- `CapabilityAvailabilityModel`
- `CapabilityProviderInfoModel`
- `CapabilityQualityModel`
- `DiagnosticsModel`

Additional optional names:

- `SystemIdentityModel`
- `SystemFirmwareModel`
- `SystemRuntimeModel`
- `SystemModesModel`
- `SystemMemoryModel`
- `NodeIdentityModel`
- `NodeStatusModel`
- `NodePowerModel`
- `NodeSignalModel`
- `NodeDiagnosticsModel`
- `NodeCapabilityModel`

Naming rules:

- app-side models should mirror Cyber32 API semantics
- app-side models are not Core OS structs
- app-side models must not expose Drivers, Devices, HAL, Registry arrays, packet internals, or provider-selection logic

## 9. First App Test Checklist

Future Minimal App tests or manual checklist should confirm:

- system screen renders placeholder system data
- node list shows empty state
- capability list shows empty state
- node detail not found shows missing node state
- capability detail not found shows missing capability state
- zero values are not rendered as sensor readings
- `false` bool defaults are not rendered as real false sensor states
- timestamp `0` is shown as no timestamp
- provider unavailable state is shown honestly
- diagnostics unavailable state is shown honestly
- no UI screen requires live Core transport
- no UI screen reads packets
- no UI screen assumes Registry internals
- mock client can later be replaced by transport client
- mock and future live clients share the same conceptual interface

## 10. Future Positive Mock Data

Positive mock data may be added later, but not in the first app implementation unless explicitly approved.

Future examples:

- one fake node
- one fake `CAP_TEMPERATURE`
- one fake value
- one fake battery/signal state
- one fake diagnostics record

Rules for future positive mock data:

- must be visually marked as demo/mock data
- must not influence Core OS architecture
- must not imply Core OS has a real node owner before it exists
- must not imply real packet ingestion before it exists
- must not bypass Cyber32 API-shaped models
- must not introduce app-side provider selection

## 11. Core OS Protection

Protection rules:

- no app code in Core OS `src/`
- no app-specific Cyber32Api behavior
- no live transport in this milestone
- no packet parsing
- no Registry access
- no Registry array access
- no Driver calls
- no Device calls
- no HAL calls
- no Services calls from the app
- no provider selection
- no command execution
- no `src/main.cpp` changes

Correct future direction:

```text
Minimal App
-> ApiClient interface
-> MockApiClient or approved TransportApiClient
-> Cyber32Api-shaped models
```

Future live direction:

```text
Minimal App
-> ApiClient interface
-> TransportApiClient
-> approved transport adapter
-> Cyber32Api
-> Core OS
```

## 12. Stop Conditions

Stop and return to architecture review if future work tries to:

- create the app inside `Cyber32_OS/src/`
- add WebServer, HTTP, or JSON to Core OS
- implement live transport before approval
- parse packets in the app
- read Registry arrays from the app
- call Drivers from the app
- call Devices from the app
- call HAL from the app
- call Services directly from the app
- select providers in the app
- show deterministic defaults as real sensor values
- modify `src/main.cpp`

## 13. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.6 - Minimal App Project Creation Plan
```

Purpose:

Define exact commands, folder layout, Flutter project setup, first files, mock client file structure, and first manual run checklist.

Milestone 10.6 should remain documentation-only unless implementation is explicitly approved.
