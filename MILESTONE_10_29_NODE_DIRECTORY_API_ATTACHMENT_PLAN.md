# Milestone 10.29 - NodeDirectory API Attachment Plan

## 1. Scope

Milestone 10.29 plans how the empty-by-default `NodeDirectory` skeleton can later be attached to `Cyber32Api`.

This milestone is documentation only.

It does not:

- modify firmware source code
- modify `src/main.cpp`
- connect `NodeDirectory` to `Cyber32Api`
- implement real node data
- add node creation
- add discovery, pairing, trust, or Registry bridges
- parse packets
- add transport, WebServer, HTTP, JSON, BLE, or WiFi app behavior
- change current API behavior

The only goal is to define a safe future attachment path that preserves existing Node API empty-state behavior while `NodeDirectory` is empty.

## 2. Background

Recent milestones established the owner-backed public data path:

- public owner types now exist
- `NodeDirectory` empty skeleton now exists
- `NodeDirectory` is bounded and deterministic
- `NodeDirectory::count()` is `0` by default
- `NodeDirectory::readByIndex(...)` fails while empty
- failed reads return a deterministic default unavailable `PublicNodeRecord`
- current Node API methods already return safe empty-state behavior

Current Node API behavior:

- `getNodeList(...)` returns success with `ok = true`, `error_code = "none"`, and `count = 0`
- node detail methods return failure with `error_code = "node_not_found"`
- `getNodeCapabilities(...)` initializes `out_count = 0` and fails while no node exists

The first API attachment must preserve this behavior exactly.

## 3. Attachment Principle

API attachment must be behavior-preserving while `NodeDirectory` is empty.

Meaning:

- `getNodeList(...)` remains `ok = true` with `count = 0`
- `getNodeSummary(...)` remains `node_not_found`
- `getNodeIdentity(...)` remains `node_not_found`
- `getNodeStatus(...)` remains `node_not_found`
- `getNodePower(...)` remains `node_not_found`
- `getNodeSignal(...)` remains `node_not_found`
- `getNodeDiagnostics(...)` remains `node_not_found`
- `getNodeCapabilities(...)` remains failed with `out_count = 0` while no node exists
- no fake node data appears
- no fake capability data appears
- no fake diagnostics appears
- no app-visible behavior changes yet

The API must not switch from empty to fake. It may only switch from empty to owner-backed.

## 4. Proposed Future Ownership Location

Future `NodeDirectory` ownership options:

### Static / Global Owner Instance In API Layer

Pros:

- simplest first attachment
- minimal dependency movement
- does not require Runtime ownership changes
- easy to keep empty and deterministic

Cons:

- may become awkward once multiple public owner stores exist
- lifetime policy could become implicit if not documented carefully

### Dedicated Public Owner Module

Pros:

- clean separation from API
- can eventually group `NodeDirectory`, `CapabilityDirectory`, `NodeCapabilityMap`, `CapabilityValueStore`, and `DiagnosticsStore`
- keeps public owner lifetime independent from UI-facing API methods

Cons:

- requires a small ownership/lifetime pattern before implementation
- may be slightly more scaffolding than the first attachment needs

### Runtime-Owned Public Owner

Pros:

- central Core lifetime owner
- may eventually align with production bootstrap

Cons:

- risks implying Runtime owns public data policy
- could blur the rule that Runtime schedules and Services own policy
- not needed for empty attachment

### Core Context Object Later

Pros:

- best long-term shape if Cyber32 gains an explicit Core context/bootstrap object
- can hold all public owners and pass references cleanly

Cons:

- requires architecture work not needed for the first attachment
- may imply bootstrap changes and `src/main.cpp` changes later

### Recommendation

For the current project layout, the safest first option is a dedicated public owner module or API-attached public owner reference that is explicitly reset and remains empty.

The immediate implementation milestone should prefer the smallest behavior-preserving attachment:

```text
Cyber32Api -> bounded read-only NodeDirectory reference
```

No real records should be created. No ownership should move into Runtime yet.

## 5. API Methods Affected Later

### `getNodeList(...)`

Current behavior:

- succeeds
- returns `ok = true`
- returns `error_code = "none"`
- returns `count = 0`

Future `NodeDirectory`-backed behavior while empty:

- read `NodeDirectory::count()`
- if count is `0`, return the same current empty success

Future behavior when records exist later:

- copy bounded summaries from owner-backed `PublicNodeRecord` values
- never exceed `ApiNodeList` capacity
- unavailable fields remain unavailable or deterministic placeholders

Stop conditions:

- fake nodes
- Registry private array access
- packet parsing
- provider selection

### `getNodeSummary(...)`

Current behavior:

- returns false with `error_code = "node_not_found"` while the list is empty

Future `NodeDirectory`-backed behavior while empty:

- `readByIndex(...)` fails
- return the same `node_not_found`

Future behavior when records exist later:

- map one `PublicNodeRecord` into `ApiNodeSummary`
- nested identity/status/power/signal/diagnostics/capability fields must remain honest about unknown or unavailable state

Stop conditions:

- invented node metadata
- inferred provider status
- hardware reads

### `getNodeIdentity(...)`

Current behavior:

- returns `node_not_found`

Future `NodeDirectory`-backed behavior while empty:

- failed `readByIndex(...)` maps to `node_not_found`

Future behavior when records exist later:

- map public identity fields only
- source MAC and names remain unavailable unless explicitly owner-backed and policy-approved

Stop conditions:

- MAC-derived fake node identity
- source identity exposed before policy approval

### `getNodeStatus(...)`

Current behavior:

- returns `node_not_found`

Future `NodeDirectory`-backed behavior while empty:

- failed `readByIndex(...)` maps to `node_not_found`

Future behavior when records exist later:

- map lifecycle, visibility, trust, freshness, and last-seen placeholders from owner-backed fields
- unavailable status remains unavailable

Stop conditions:

- status inferred from transport callbacks
- provider health refresh triggered by read

### `getNodePower(...)`

Current behavior:

- returns `node_not_found`
- deterministic defaults do not represent real battery state

Future `NodeDirectory`-backed behavior while empty:

- failed `readByIndex(...)` maps to `node_not_found`

Future behavior when records exist later:

- remain unavailable until a diagnostics/power owner exists
- do not invent battery percent or voltage from defaults

Stop conditions:

- fake battery values
- hardware or driver reads

### `getNodeSignal(...)`

Current behavior:

- returns `node_not_found`
- deterministic defaults do not represent real signal state

Future `NodeDirectory`-backed behavior while empty:

- failed `readByIndex(...)` maps to `node_not_found`

Future behavior when records exist later:

- remain unavailable until a diagnostics/signal owner exists
- do not infer RSSI from ESP-NOW internals unless a later approved owner stores it

Stop conditions:

- fake RSSI
- transport internals read by API

### `getNodeDiagnostics(...)`

Current behavior:

- returns `node_not_found`
- deterministic counters do not mean healthy

Future `NodeDirectory`-backed behavior while empty:

- failed `readByIndex(...)` maps to `node_not_found`

Future behavior when records exist later:

- expose only owner-backed diagnostic availability
- detailed counters should wait for a `DiagnosticsStore` attachment

Stop conditions:

- zero counters treated as healthy
- diagnostics reset during reads

### `getNodeCapabilities(...)`

Current behavior:

- initializes `out_count = 0`
- returns false while no node exists

Future `NodeDirectory`-backed behavior while empty:

- preserve failure with `out_count = 0`

Future behavior when records exist later:

- validate the node exists through `NodeDirectory`
- do not infer capabilities from `PublicNodeRecord::capability_count`
- wait for `NodeCapabilityMap` before returning mapped capabilities

Stop conditions:

- fake capability summaries
- provider inference
- packet parsing

## 6. `getNodeList(...)` Transition Plan

Future implementation plan:

1. API obtains a read-only `NodeDirectory` reference.
2. API reads `NodeDirectory::count()`.
3. If count is `0`, API returns current empty success:

```text
ok = true
error_code = "none"
count = 0
```

4. If records exist later, API copies only bounded public summaries.
5. API never exceeds output capacity.
6. API never exposes private storage or private indexes.
7. API never reads Registry arrays.
8. API never parses packets.
9. API never selects providers.

No behavior change should be visible during the first empty attachment.

## 7. Node Detail Transition Plan

Future implementation plan:

1. Validate `node_index` by calling `NodeDirectory::readByIndex(...)`.
2. If the read fails, return exactly the current `node_not_found` behavior.
3. If the read succeeds later, map `PublicNodeRecord` fields into the requested `ApiNode*` response.
4. Unknown/unavailable fields remain unavailable or deterministic defaults.
5. Default zero values are not presented as real data.
6. No hardware reads are allowed.
7. No provider selection is allowed.
8. No Registry private array access is allowed.

The first attachment should primarily prove that failure mapping remains correct.

## 8. `getNodeCapabilities(...)` Transition Plan

The first `NodeDirectory` API attachment should not use `NodeCapabilityMap` unless a later milestone explicitly approves it.

While no node exists:

- initialize `out_count = 0`
- return false
- preserve current no-node behavior

If a node exists later but no mapping owner is attached:

- recommended future behavior is success with `out_count = 0` only after an approved contract says an existing node with no mapped capabilities is valid
- alternatively, return a compact unavailable/not-ready state if a response wrapper is added later

Important:

- do not infer capabilities from `PublicNodeRecord::capability_count`
- do not infer capabilities from providers
- do not infer capabilities from packets
- do not fabricate `CAP_TEMPERATURE` or any other capability

## 9. Initialization / Reset Plan

Future initialization rules:

- `NodeDirectory` should be explicitly reset during public owner setup
- reset must produce deterministic empty state
- no `src/main.cpp` change should occur unless a later milestone explicitly approves bootstrap wiring
- no app, WebServer, transport, or packet path should secretly initialize public owners
- API reads must not reset or mutate `NodeDirectory`

Open lifetime choices remain documented in Section 13.

## 10. Test Strategy For Future Implementation

Future attachment validation should prove:

- existing Node API empty tests still pass
- `getNodeList(...)` returns `count = 0` via `NodeDirectory`
- detail methods still return `node_not_found`
- `getNodeCapabilities(...)` still initializes `out_count = 0`
- `NodeDirectory` read failure maps to `node_not_found`
- no fake node summaries appear
- API reads do not mutate `NodeDirectory`
- repeated API reads are deterministic
- existing System API validation still passes
- existing Capability API validation still passes
- provider, diagnostics, command-state, wireless, and legacy capability validation remain unchanged
- full validation suite passes

No validation is added in this documentation milestone.

## 11. Minimal App Implications

No Minimal App change is required for the first attachment.

The app remains mock-only until an approved transport milestone.

After the first empty attachment, a future live app would still see:

- empty node list
- `node_not_found` for detail reads
- no real node cards
- no pending pairing list
- no transport
- no live sensor values

This is correct. Empty is a valid Cyber32 API state.

## 12. Safety Invariants

Future API attachment must preserve these invariants:

- no fake nodes
- no fake capabilities
- no fake sensor values
- no fake diagnostics
- no private Registry access
- no private Registry arrays exposed
- no packet parsing
- no HAL calls
- no Driver calls
- no Device calls
- no Module calls
- no Service calls
- no Logic calls
- no Runtime calls for node data
- no provider selection
- no command execution
- no `src/main.cpp` changes unless explicitly approved
- no app transport
- no WebServer
- no HTTP
- no JSON
- no BLE
- no WiFi app transport

## 13. Open Questions

Open decisions:

- final owner location
- initialization owner
- whether `NodeDirectory` should be singleton or passed dependency
- whether `Cyber32Api` owns public owner references
- whether a later Core context should own all public stores
- whether Runtime should own public stores later or remain scheduling-only
- how reset is triggered in production bootstrap
- how future discovery bridge attaches records
- how future pairing/trust approval creates public records
- when `getNodeCapabilities(...)` changes from `node_not_found` to success with `out_count = 0` for an existing node with no mappings
- whether pending discovery belongs in normal Node API or a separate Discovery API
- whether blocked/rejected nodes are hidden, diagnostic-only, or visible as unavailable

## 14. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.30 - NodeDirectory API Empty Attachment Implementation
```

Purpose:

Attach Cyber32Api Node API reads to the empty `NodeDirectory` skeleton while preserving all current empty-state behavior and validation.

Alternative if more planning is needed:

```text
Milestone 10.30 - Public Owner Lifetime Plan
```

Purpose:

Define exact ownership and lifetime for public owner stores before API attachment.

## 15. Stop Conditions

Stop future work and return to architecture review if it tries to:

- attach real node data before an owner-backed creation path exists
- create fake nodes
- create fake capabilities
- create fake sensor values
- create fake diagnostics
- parse packets
- read Registry private arrays
- call HAL
- call Drivers
- call Devices
- call Modules
- call Services
- call Logic
- modify `src/main.cpp` without explicit approval
- add live transport
- add WebServer before transport approval
- add HTTP before transport approval
- add JSON before transport approval
- add BLE before transport approval
- add WiFi app transport before transport approval
