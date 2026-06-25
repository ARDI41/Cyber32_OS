# Milestone 9.9 - Node API Readiness Audit

## 1. Scope

Milestone 9.9 audits the completed Milestone 9.8 read-only Node API methods.

This audit covers the current placeholder behavior for:

- `getNodeList(...)`
- `getNodeSummary(...)`
- `getNodeIdentity(...)`
- `getNodeStatus(...)`
- `getNodePower(...)`
- `getNodeSignal(...)`
- `getNodeDiagnostics(...)`
- `getNodeCapabilities(...)`

The audited implementation is internal C++ API only. It does not expose WebServer, HTTP, JSON, UI, Dashboard, Cloud, Marketplace, or AI behavior.

The current Node API deliberately uses an empty node-list contract because Cyber32 does not yet have a public node owner, discovery owner, pending pairing owner, or node-to-capability mapping owner.

## 2. Method-By-Method Status

### getNodeList(...)

Current behavior:

- returns `true`
- sets `out_response.ok = true`
- sets `out_response.error_code = "none"`
- sets `out_response.count = 0`
- does not invent fake node summaries

Expected empty-node-owner behavior:

- empty list is a successful read
- UI clients can safely display an empty state

Success/failure contract:

- current success: empty list
- current failure: none in placeholder mode
- future failure may include missing Registry or missing node owner once a public owner exists

Current error handling:

- success uses `"none"`

Validation coverage:

- empty-list success
- `ok == true`
- `error_code == "none"`
- `count == 0`
- count bounded by `API_MAX_NODE_SUMMARY_COUNT`
- repeated reads remain stable

Future owner needed:

- public node registry, discovery owner, or node summary owner

### getNodeSummary(...)

Current behavior:

- calls or aligns with `getNodeList(...)`
- returns `false` while the node list is empty
- sets `out_response.ok = false`
- sets `out_response.error_code = "node_not_found"`
- does not fill fake aggregate node data

Expected empty-node-owner behavior:

- any detail request is out of range while count is zero
- no fake summary is created

Success/failure contract:

- future success returns a populated `ApiNodeSummary`
- current failure returns `false` with `node_not_found`

Current error handling:

- uses compact stable error literal `"node_not_found"`

Validation coverage:

- index `0` fails when list count is zero
- `ok == false`
- `error_code == "node_not_found"`
- repeated reads remain stable

Future owner needed:

- public indexed node owner and child field owners for identity, status, power, signal, diagnostics, and capabilities

### getNodeIdentity(...)

Current behavior:

- aligns with `getNodeList(...)`
- returns `false` while the node list is empty
- sets `ok = false`
- sets `error_code = "node_not_found"`
- clears deterministic identity defaults
- does not invent `node_id`, friendly name, provider type, source MAC, or MAC availability

Expected empty-node-owner behavior:

- no node identity is exposed until a public node owner exists

Success/failure contract:

- future success returns node identity from a public node/discovery/allowlist owner
- current failure returns `false` with deterministic defaults

Current error handling:

- `"node_not_found"`

Validation coverage:

- failure while node list count is zero
- `node_id == 0`
- `has_source_mac == false`
- repeated reads remain stable

Future owner needed:

- node identity owner
- friendly-name storage
- source MAC owner for wireless nodes
- provider type mapping for wired and wireless nodes

### getNodeStatus(...)

Current behavior:

- aligns with `getNodeList(...)`
- returns `false` while the node list is empty
- sets `ok = false`
- sets `error_code = "node_not_found"`
- sets deterministic status defaults
- does not invent online, paired, trusted, blocked, last-seen, or provider status data

Expected empty-node-owner behavior:

- no status is reported for nonexistent nodes

Success/failure contract:

- future success returns node status from public node health, allowlist, provider, and diagnostic owners
- current failure returns deterministic defaults

Current error handling:

- `"node_not_found"`

Validation coverage:

- empty-list detail failure
- status flags remain false
- `last_seen_ms == 0`
- repeated reads remain stable

Future owner needed:

- node health owner
- pairing/discovery state owner
- trust/blocked state mapping
- provider status owner

### getNodePower(...)

Current behavior:

- aligns with `getNodeList(...)`
- returns `false` while the node list is empty
- sets `ok = false`
- sets `error_code = "node_not_found"`
- sets `has_battery_percent = false`
- sets `has_battery_mv = false`
- sets battery values to deterministic zero defaults
- does not invent real-looking battery or power state

Expected empty-node-owner behavior:

- no power data exists for nonexistent nodes

Success/failure contract:

- future success returns power data only when an owner has real data
- current failure returns deterministic empty power state

Current error handling:

- `"node_not_found"`

Validation coverage:

- failure while list count is zero
- battery-present flags remain false
- battery values remain zero
- repeated reads remain stable

Future owner needed:

- wireless diagnostics owner for battery values
- future wired/device power diagnostic owner

### getNodeSignal(...)

Current behavior:

- aligns with `getNodeList(...)`
- returns `false` while the node list is empty
- sets `ok = false`
- sets `error_code = "node_not_found"`
- sets `has_rssi = false`
- sets `rssi = 0`
- sets signal quality to zero
- does not infer radio signal from transport internals or hardware

Expected empty-node-owner behavior:

- no signal data exists for nonexistent nodes

Success/failure contract:

- future success returns signal fields from public wireless diagnostic owners
- current failure returns deterministic empty signal state

Current error handling:

- `"node_not_found"`

Validation coverage:

- failure while list count is zero
- no RSSI present
- signal values remain deterministic defaults
- repeated reads remain stable

Future owner needed:

- wireless node signal/RSSI diagnostic owner
- calibrated signal-quality mapping if required

### getNodeDiagnostics(...)

Current behavior:

- aligns with `getNodeList(...)`
- returns `false` while the node list is empty
- sets `ok = false`
- sets `error_code = "node_not_found"`
- sets accepted and rejected packet counts to zero
- sets `last_error_code = "none"`
- sets `has_security_diagnostics = false`
- does not invent security diagnostics
- does not reset counters

Expected empty-node-owner behavior:

- no diagnostics are exposed for nonexistent nodes

Success/failure contract:

- future success returns diagnostics from public node or wireless security diagnostic owners
- current failure returns deterministic empty diagnostic state

Current error handling:

- `"node_not_found"`

Validation coverage:

- failure while list count is zero
- deterministic zero counters
- stable `last_error_code`
- `has_security_diagnostics == false`
- repeated reads remain stable and do not reset counters

Future owner needed:

- public node diagnostics owner
- mapping from node list entries to wireless security diagnostics

### getNodeCapabilities(...)

Current behavior:

- initializes `out_count = 0` before any failure return
- returns `false` while the node list is empty
- leaves `out_count == 0`
- supports `max_count == 0` with `out_capabilities == nullptr`
- rejects `out_capabilities == nullptr` when `max_count > 0`
- does not fill fake capabilities
- does not infer capabilities from providers, packets, diagnostics, transport internals, or hardware

Expected empty-node-owner behavior:

- no node capabilities are returned until a public node-to-capability mapping owner exists

Success/failure contract:

- current failure uses `false` and `out_count == 0`
- current method has no response wrapper and no error-code output
- future success should fill no more than `max_count` entries and set `out_count`

Current error handling:

- bool return only
- no `error_code` channel exists for this method

Validation coverage:

- empty-list failure with `out_count == 0`
- repeated read stability
- `max_count == 0` path keeps `out_count == 0` and does not write to buffer
- null pointer with `max_count > 0` returns `false` and keeps `out_count == 0`

Future owner needed:

- public node-to-capability mapping owner
- optional response wrapper if richer error reporting is needed later

## 3. Empty Node-List Contract

The current `getNodeList(...)` contract is:

- returns `true`
- `ok = true`
- `error_code = "none"`
- `count = 0`

This means the Node API is currently available as a safe read contract, but there are no known nodes exposed through a public node owner.

All detail methods must not invent fake nodes.

Current detail methods fail with:

- return `false`
- `ok = false` where the response struct supports `ok`
- `error_code = "node_not_found"`

The detail failure behavior is intentional. It prevents UI clients from seeing placeholder nodes as real discovered, paired, trusted, powered, or capability-owning devices.

## 4. Capabilities Method Contract

`getNodeCapabilities(...)` is intentionally different because it uses a caller-provided output buffer rather than a response wrapper.

Current contract:

- initializes `out_count = 0`
- returns `false` while node list is empty
- supports `max_count == 0` with `out_capabilities == nullptr`
- rejects `out_capabilities == nullptr` when `max_count > 0`
- has no response wrapper
- has no `error_code` output
- does not write fake capability summaries

Future expansion may add a response wrapper if richer error reporting becomes necessary, but the current method remains bounded and simple.

## 5. Safety Audit

Safety result: PASS.

Confirmed for Milestone 9.8 Node API methods:

- no packet parsing
- no private Registry array access
- no provider selection
- no discovery implementation
- no pairing implementation
- no Services calls
- no Drivers calls
- no Devices calls
- no HAL calls
- no WebServer
- no HTTP
- no JSON
- no UI/App code
- no Runtime architecture changes
- no Registry ownership changes
- no `src/main.cpp` changes
- no Arduino `String`
- no STL containers
- no heap allocation

The Node API methods remain read-only placeholder contracts and do not mutate Registry, Runtime, providers, diagnostics, command state, or capability payloads.

## 6. Minimal App Readiness

Ready now:

- Minimal App can safely call `getNodeList(...)`.
- Minimal App can handle an empty node-list state.
- Minimal App can attempt detail reads and receive `node_not_found`.
- Minimal App can treat current Node API as stable empty-state groundwork.

Not ready yet:

- real discovered node list
- pending pairing list
- live battery data
- live signal/RSSI data
- live node diagnostics
- node-to-capability mapping
- friendly names
- source MAC from owner-backed records
- real node status
- real node capability list

The Minimal App still cannot show real nodes until node owner, discovery, pending pairing, and node-to-capability mapping work exists behind public API-safe owners.

## 7. Validation Audit

Current validation coverage includes:

- node list empty success
- detail method `node_not_found` failures
- deterministic defaults for identity, status, power, signal, and diagnostics
- repeated read stability
- capabilities `out_count` stability
- `getNodeCapabilities(...)` null pointer and `max_count` behavior
- existing System API validation still passes in the validation flow
- existing provider, diagnostics, command-state, wireless, and capability validation remains preserved by the same validation path

PlatformIO note:

- The Codex shell has previously reported that `pio` is not on PATH.
- Final build verification still requires a local PlatformIO environment.

## 8. Known Gaps

Known gaps before real Node API data can be exposed:

- no public node owner yet
- no discovery owner exposed through API
- no pending pairing API
- no node-to-capability mapping owner
- no real power owner
- no real signal owner
- no real node diagnostics owner
- no friendly-name storage
- no stable source MAC owner in Node API
- no response wrapper for `getNodeCapabilities(...)`
- no production plug-and-play backing for real node list entries yet

These gaps are expected. They should be resolved through documented milestones before UI clients rely on real node data.

## 9. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.0 - Capability API Method Implementation Plan
```

Rationale:

System API and Node API now have safe read-only contracts. Capability API is the next UI-critical API area because Minimal App sensor cards need capability values through the Cyber32 API only.

Capability API planning should define read-only methods for capability lists, capability summaries, value cards, availability, provider diagnostic metadata, and quality state without allowing UI clients to call Drivers, Devices, HAL, packet parsers, provider selection logic, or private Registry arrays.

## 10. Stop Conditions

Stop future implementation and return to architecture review if next work requires:

- packet parsing inside API
- private Registry array access
- provider selection inside API
- direct Driver calls
- direct Device calls
- direct HAL calls
- command execution
- WebServer
- HTTP
- JSON
- UI/App code
- dynamic memory
- Arduino `String`
- STL containers
- `src/main.cpp` changes

Node API must remain read-only until documented owner-backed data sources are introduced through public, bounded contracts.
