# Milestone 9.8 - Read-Only Node API Method Plan

## Purpose

Milestone 9.8 plans future read-only Node API methods for Cyber32.

These methods will expose node information through `Cyber32Api` for future UI clients:

- Dev Panel
- Minimal App
- Mission Control
- Dashboard
- Cloud Bridge
- Marketplace
- AI Assistant

All future UI clients must use the same Cyber32 API. Node API methods must remain internal C++ contracts first. This milestone is documentation only and does not implement methods.

Planned Node API structs already exist from Milestone 9.5 Phase 3:

- `ApiNodeIdentity`
- `ApiNodeStatus`
- `ApiNodePower`
- `ApiNodeSignal`
- `ApiNodeDiagnosticsSummary`
- `ApiNodeCapabilitySummary`
- `ApiNodeSummary`
- `ApiNodeList`

## 1. Scope

This milestone plans read-only Node API methods only.

Planned future method names:

```cpp
bool getNodeList(ApiNodeList& out_response);
bool getNodeSummary(uint8_t node_index, ApiNodeSummary& out_response);
bool getNodeIdentity(uint8_t node_index, ApiNodeIdentity& out_response);
bool getNodeStatus(uint8_t node_index, ApiNodeStatus& out_response);
bool getNodePower(uint8_t node_index, ApiNodePower& out_response);
bool getNodeSignal(uint8_t node_index, ApiNodeSignal& out_response);
bool getNodeDiagnostics(uint8_t node_index, ApiNodeDiagnosticsSummary& out_response);
bool getNodeCapabilities(
    uint8_t node_index,
    ApiNodeCapabilitySummary* out_capabilities,
    uint8_t max_count,
    uint8_t& out_count);
```

The plan covers method purpose, ownership, allowed data sources, failure behavior, placeholder policy, validation expectations, and implementation order.

## 2. Non-Goals

Milestone 9.8 does not implement behavior.

Non-goals:

- No Node API method implementation
- No behavior changes
- No discovery implementation
- No pairing implementation
- No provider selection
- No command execution
- No packet parsing
- No hardware access
- No WebServer
- No HTTP
- No JSON
- No UI or app code
- No Dashboard
- No Cloud
- No Marketplace
- No AI implementation
- No Registry ownership changes
- No Runtime architecture changes
- No `src/main.cpp` changes

## 3. Read-Only Philosophy

Node API methods are observation methods only.

They may observe already known node, provider, diagnostic, and Registry state through existing public APIs where those APIs already exist.

They must not mutate:

- Registry records
- Runtime state
- provider records
- capability payloads
- wireless packets
- Services
- command state
- pairing state
- discovery state

They must not call Services for command behavior, select providers, parse packets, start transports, or create nodes.

Where no authoritative owner exists yet, a future method may return bounded placeholders, `ok = false`, and a compact error such as `"node_api_not_available"`.

## 4. Ownership

Node API methods should report data owned by the correct subsystem.

| Data | Owner |
| --- | --- |
| node identity | future node registry / discovery owner, with current wireless allowlist records where applicable |
| node status | Registry/provider diagnostics, future node health owner |
| power/battery | wireless node diagnostics or future device diagnostic owner |
| signal/RSSI | wireless diagnostics / transport diagnostic owner |
| diagnostics | Registry wireless node and security diagnostics |
| node capability list | Registry provider records and future node-to-capability mapping |
| last update timestamp | provider diagnostics or future node health records |
| provider state | Registry provider records and provider diagnostics |

The API layer is not the owner of node state. It only formats bounded read-only responses from public owners.

## 5. Allowed Data Sources

Future Node API methods may read:

- existing public Registry summaries, if available
- existing provider diagnostics, if available
- existing wireless node diagnostics, if available
- existing wireless security diagnostics, if available
- allowlist records through public Registry methods, if available
- stable placeholders where no owner exists yet

Allowed reads must remain bounded and use public APIs only.

## 6. Forbidden Data Sources

Future Node API methods must not read or call:

- private Registry arrays
- Drivers
- Devices
- HAL
- raw packet buffers
- ESP-NOW callbacks
- packet parser internals
- Services for command behavior
- provider selection logic
- Runtime scheduling internals
- UI transport layers

The API must not infer node state by parsing packets or inspecting transport internals.

## 7. Method-By-Method Plan

### getNodeList(...)

Purpose:

Return a bounded list of node summaries for UI node-list screens.

Output struct:

- `ApiNodeList`

Required attachment or owner:

- Registry attachment should be required once node data is Registry-backed.

Success behavior:

- return `true`
- `out_response.ok = true`
- `out_response.error_code = "none"`
- fill up to `API_MAX_NODE_SUMMARY_COUNT` entries
- set `count` to the number of returned summaries

Failure behavior:

- missing Registry: return `false`, `error_code = "registry_not_attached"`
- node API not yet backed by an owner: return `false`, `error_code = "node_api_not_available"`

Placeholder policy:

- may return count `0` only if the method is explicitly defined as available with no known nodes
- should not invent fake nodes

Validation strategy:

- empty list
- bounded count
- Registry missing failure
- repeated reads do not mutate Registry, providers, or diagnostics

Future expansion notes:

- may later include pending discovery records once a discovery owner exists
- should remain bounded by `API_MAX_NODE_SUMMARY_COUNT`

### getNodeSummary(...)

Purpose:

Return one aggregate node view for a bounded node index.

Output struct:

- `ApiNodeSummary`

Required attachment or owner:

- Registry attachment and a public node/index lookup owner.

Success behavior:

- return `true`
- `out_response.ok = true`
- `out_response.error_code = "none"`
- compose identity, status, power, signal, diagnostics, and capability summary fields

Failure behavior:

- missing Registry: `registry_not_attached`
- out-of-range index: `invalid_node_index`
- missing node: `node_not_found`
- unavailable owner: `node_api_not_available`

Placeholder policy:

- substructures may use placeholders only for fields that have no current owner
- placeholders must not imply hardware was discovered or paired

Validation strategy:

- valid index
- invalid index
- missing Registry
- repeated reads do not mutate state

Future expansion notes:

- should compose child Node API methods once those methods exist

### getNodeIdentity(...)

Purpose:

Return node identity fields such as node ID, friendly name, provider type, and source MAC.

Output struct:

- `ApiNodeIdentity`

Required attachment or owner:

- Registry public node, allowlist, provider, or discovery record owner.

Success behavior:

- return `true`
- `ok = true`
- `error_code = "none"`
- fill `node_id`
- fill `friendly_name` with a stable literal or future stored name
- fill `provider_type`
- copy `source_mac` only when available
- set `has_source_mac`

Failure behavior:

- missing Registry: `registry_not_attached`
- invalid index: `invalid_node_index`
- missing node: `node_not_found`
- unavailable owner: `node_api_not_available`

Placeholder policy:

- `friendly_name` may be a stable placeholder until naming storage exists
- MAC fields must not be invented

Validation strategy:

- valid identity read
- no-MAC wired node path when available
- wireless node with MAC when available
- invalid index

Future expansion notes:

- future pairing/discovery may add pending nodes, but identity reads must remain read-only

### getNodeStatus(...)

Purpose:

Return node status fields for online, paired, trusted, blocked, last seen, and provider status.

Output struct:

- `ApiNodeStatus`

Required attachment or owner:

- Registry diagnostics, allowlist records, provider records, and future node health owner.

Success behavior:

- return `true`
- `ok = true`
- `error_code = "none"`
- fill status booleans from current public owners where available

Failure behavior:

- missing Registry: `registry_not_attached`
- invalid index: `invalid_node_index`
- missing node: `node_not_found`
- unavailable owner: `node_api_not_available`

Placeholder policy:

- online/paired/trusted/blocked may be conservative placeholders only when no owner exists
- status must not trigger provider health updates

Validation strategy:

- trusted/allowed wireless node status
- blocked node status
- invalid index
- read does not change provider status

Future expansion notes:

- should eventually align with lost/stale provider health and pending discovery state

### getNodePower(...)

Purpose:

Return node power and battery information.

Output struct:

- `ApiNodePower`

Required attachment or owner:

- wireless node diagnostics or future device diagnostic owner.

Success behavior:

- return `true`
- `ok = true`
- `error_code = "none"`
- fill battery fields only if present
- set `has_battery_percent` and `has_battery_mv` accurately

Failure behavior:

- missing Registry: `registry_not_attached`
- invalid index: `invalid_node_index`
- missing node: `node_not_found`
- unavailable owner: `node_api_not_available`

Placeholder policy:

- for wired nodes or unknown battery state, use `has_battery_percent = false` and `has_battery_mv = false`
- do not invent battery values

Validation strategy:

- no battery data path
- battery data path once diagnostics owner exists
- invalid index

Future expansion notes:

- may support external-powered nodes through future power state fields if added by a documented contract phase

### getNodeSignal(...)

Purpose:

Return wireless signal/RSSI information for node detail and diagnostics cards.

Output struct:

- `ApiNodeSignal`

Required attachment or owner:

- wireless diagnostics or future transport diagnostic owner.

Success behavior:

- return `true`
- `ok = true`
- `error_code = "none"`
- fill `rssi` only when available
- set `has_rssi`
- fill signal quality placeholder where no calibrated mapping exists

Failure behavior:

- missing Registry: `registry_not_attached`
- invalid index: `invalid_node_index`
- missing node: `node_not_found`
- unavailable owner: `node_api_not_available`

Placeholder policy:

- wired nodes should report no RSSI
- signal quality must not be invented as a real radio measurement

Validation strategy:

- no signal path
- wireless signal path once owner exists
- invalid index

Future expansion notes:

- future ESP-NOW metadata may provide RSSI or quality if supported by the transport layer

### getNodeDiagnostics(...)

Purpose:

Return compact diagnostics for accepted/rejected packets and latest error visibility.

Output struct:

- `ApiNodeDiagnosticsSummary`

Required attachment or owner:

- Registry wireless security diagnostics or future node diagnostic records.

Success behavior:

- return `true`
- `ok = true`
- `error_code = "none"`
- fill accepted and rejected packet counts
- fill last error code
- set `has_security_diagnostics`

Failure behavior:

- missing Registry: `registry_not_attached`
- invalid index: `invalid_node_index`
- missing node: `node_not_found`
- unavailable owner: `node_api_not_available`

Placeholder policy:

- if diagnostics do not exist for a known node, `has_security_diagnostics` should be false rather than fabricating counters

Validation strategy:

- diagnostics present
- diagnostics missing but node known
- invalid index
- reads do not reset counters

Future expansion notes:

- should align with `ApiWirelessSecurityDiagnostic` without duplicating all security fields in normal node cards

### getNodeCapabilities(...)

Purpose:

Return a bounded list of capability summaries associated with a node.

Output contract:

- caller-provided `ApiNodeCapabilitySummary* out_capabilities`
- `uint8_t max_count`
- `uint8_t& out_count`

Required attachment or owner:

- Registry provider records and future node-to-capability mapping.

Success behavior:

- return `true`
- fill no more than `max_count`
- set `out_count`
- each returned summary uses capability IDs, not hardware names

Failure behavior:

- null output pointer with nonzero max count: future compact invalid argument error if a response wrapper is added
- missing Registry: `registry_not_attached`
- invalid index: `invalid_node_index`
- missing node: `node_not_found`
- unavailable owner: `node_api_not_available`

Placeholder policy:

- return zero capabilities only when the node is known and has no mapped capabilities
- do not infer capabilities by parsing packets

Validation strategy:

- bounded output count
- max count smaller than available count
- zero max count
- invalid index
- no Registry mutation

Future expansion notes:

- this method may need a response wrapper in a later contract phase if richer error reporting is required

## 8. Error Handling Rules

Future Node API methods should use compact stable error literals:

- `"none"`
- `"node_not_found"`
- `"registry_not_attached"`
- `"node_api_not_available"`
- `"invalid_node_index"`

Rules:

- success responses set `ok = true`
- success responses set `error_code = "none"`
- failure responses set `ok = false` where the output struct has an `ok` field
- methods return `false` on failure
- failures must not mutate state
- avoid long or dynamic error messages

## 9. ESP32 Safety Rules

Node API implementation must remain ESP32-safe:

- no dynamic allocation
- no Arduino `String`
- no STL containers
- no heap allocation
- use bounded arrays only
- respect `API_MAX_NODE_SUMMARY_COUNT`
- respect related API-level constants
- use `uint8_t` counts where practical
- use `const char*` only for stable literals/placeholders
- no unbounded node, capability, or diagnostic lists

## 10. Minimal App Readiness

The future Node API should support the first Minimal App screens without exposing internals.

Supported future screens:

- node list screen
- sensor detail screen
- battery/signal card
- diagnostics card
- future "New sensor found" pending pairing screen

Minimal App rules:

- consume Cyber32 API only
- do not parse packets
- do not access Registry arrays
- do not call Drivers
- do not call Devices
- do not call HAL
- do not call Services directly
- remain capability-first

The app may show node and MAC fields as diagnostics/management metadata, but capability values and actions must remain bound to `CAP_*` IDs.

## 11. Recommended Implementation Order

Recommended future implementation order:

1. `getNodeList(...)`
2. `getNodeSummary(...)`
3. `getNodeIdentity(...)`
4. `getNodeStatus(...)`
5. `getNodePower(...)`
6. `getNodeSignal(...)`
7. `getNodeDiagnostics(...)`
8. `getNodeCapabilities(...)`

Rationale:

`getNodeList(...)` is the safest first method because it can establish bounded enumeration and empty-list behavior before detailed node fields are attached to concrete owners.

`getNodeSummary(...)` should follow because it can compose the smaller node views once the list/index contract is stable.

Detailed methods should come afterward so each ownership boundary can be validated independently.

## 12. Validation Expectations

Future Node API validation should prove:

- Registry missing failure where Registry is required
- invalid node index failure
- missing node failure
- valid node read
- bounded node list count
- bounded capability output count
- placeholders are stable where used
- read methods do not mutate Registry
- read methods do not mutate Runtime
- read methods do not update providers
- read methods do not update canonical payloads
- read methods do not reset diagnostics
- normal capability reads remain unchanged
- provider diagnostics remain unchanged
- wireless security diagnostics remain unchanged

Validation must not require hardware.

## 13. Stop Conditions

Stop implementation and return to architecture review if future Node API method work would require:

- packet parsing
- private Registry array access
- provider selection logic
- command behavior
- hardware access
- Driver calls
- Device calls
- HAL calls
- Runtime architecture changes
- Registry ownership changes
- WebServer
- HTTP
- JSON
- UI or app code
- Dashboard
- Cloud
- Marketplace
- AI implementation
- dynamic memory
- Arduino `String`
- STL containers
- `src/main.cpp` changes

## 14. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 9.8 Phase 1 - Node API List Method
```

Recommended scope:

- implement only `getNodeList(ApiNodeList& out_response)`
- prefer safe placeholder or empty-list behavior if no public node owner exists yet
- no detail methods
- no discovery
- no pairing
- no provider selection
- no WebServer, HTTP, JSON, or app code
- no `src/main.cpp` changes

