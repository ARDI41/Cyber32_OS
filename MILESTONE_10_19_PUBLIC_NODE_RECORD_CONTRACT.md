# Milestone 10.19 - Public Node Record Contract

## 1. Scope

Milestone 10.19 defines the public node record contract used by Cyber32Api before implementing real node list data.

This milestone is documentation only.

It defines:

- what a public node record is
- why it is needed
- who should own it in a future implementation
- how it should be indexed safely
- which fields are required
- which fields are optional or future
- how node visibility and lifecycle should work
- how existing Node API methods should map to public node records later
- bounds, safety rules, Minimal App implications, open questions, and stop conditions

No firmware implementation is performed in this milestone.

## 2. Background

The public node record contract is needed because Cyber32 is moving from safe empty-state API contracts toward real owner-backed data.

Current state:

- Node API currently returns a safe empty list.
- Node detail methods currently return `node_not_found`.
- Minimal App is mock-only.
- Minimal App correctly treats empty node lists as valid empty states.
- Real nodes must not be invented.
- App, UI, and transport layers must not infer nodes from packets.
- App, UI, and transport layers must not infer nodes from Registry internals.
- App, UI, and transport layers must not read private Registry arrays.

Before `getNodeList(...)` can return real data, Core OS needs a bounded public owner-backed node record model that Cyber32Api can read safely.

## 3. Public Node Record Definition

A public node record is a bounded, owner-backed, API-safe representation of a real discovered, approved, pending, paired, trusted, unavailable, or diagnostic-visible node.

A public node record is distinct from:

- raw packets
- ESP-NOW payloads
- Registry private arrays
- driver internals
- device internals
- module internals
- provider internals
- transport session state
- debug serial output
- temporary hardware-test state

Public node records exist so API clients can ask:

```text
What nodes does Cyber32 Core currently expose through its public API?
```

Public node records must not be fabricated to satisfy UI needs. If no owner-backed public node record exists, the correct API answer remains an empty list or `node_not_found`.

## 4. Ownership Model

Future Core OS should introduce a dedicated public node owner concept.

Possible component names:

- `PublicNodeStore`
- `NodeDirectory`
- `PublicNodeRegistry`

Recommended concept name for planning:

```text
NodeDirectory
```

This name is conceptual only. Do not implement it yet.

### Ownership Responsibilities

The future owner should be responsible for:

- storing public node records
- assigning or accepting stable `node_id` values through approved Core policy
- maintaining bounded indexed access
- tracking node lifecycle state
- tracking public visibility state
- updating freshness/last-seen state
- exposing read-only public records to Cyber32Api
- preventing fake nodes from being exposed
- enforcing deterministic failure when full

### Creation Responsibility

Public node records should be created only through approved Core paths, such as:

- future discovery-to-public-record bridge
- future pairing/provisioning approval
- future wired device registration bridge
- future trusted static registration path

UI, app, and transport layers must not create public node records directly.

### Update Responsibility

The future owner should update public node records from approved Core inputs only.

Possible update sources:

- discovery state
- pairing/trust state
- heartbeat/freshness state
- diagnostics owner state
- future wired device state
- future wireless node state

Updates must not come from UI guesses, app-side logs, debug output, packets parsed by API, or transport internals.

### Deletion / Expiry Responsibility

The future owner should define:

- stale timeout behavior
- expired record behavior
- blocked/rejected behavior
- whether records are removed, hidden, or retained for diagnostics
- whether user-approved records persist later

### Read-Only API Access Responsibility

Cyber32Api should read public node records through public owner methods only.

Cyber32Api must not:

- read private Registry arrays
- parse packets
- call Drivers
- call Devices
- call HAL
- call Services directly for UI reads
- select providers

## 5. Required Fields

Required public node record fields conceptually:

- `node_id`
- `node_type` or `source_type`
- lifecycle/status
- visibility state
- trust/pairing state placeholder
- `capability_count`
- last-seen timestamp or freshness placeholder
- diagnostics availability placeholder

Field expectations:

### `node_id`

Stable numeric or fixed-size identity used by Core OS and API to refer to the node.

### `node_type` / `source_type`

Describes broad node origin without exposing transport internals as the primary contract.

Possible conceptual values:

- wired
- wireless
- virtual future
- cloud future
- unknown

### Lifecycle / Status

Represents whether the node is available, stale, blocked, pending, or otherwise visible.

### Visibility State

Controls whether the node appears in public API lists or remains internal.

### Trust / Pairing Placeholder

Represents whether the node is unpaired, paired, trusted, blocked, or pending approval.

### `capability_count`

Bounded count of capabilities attached to the public node.

### Last-Seen / Freshness Placeholder

Represents when the node was last seen or whether freshness is unknown.

### Diagnostics Availability Placeholder

Represents whether owner-backed diagnostics exist for the node.

All fields must be fixed-size or primitive firmware-safe values in a future implementation.

## 6. Optional / Future Fields

Optional or future public node fields:

- `display_name`
- `source_mac`
- `manufacturer_id`
- `product_id`
- `hardware_revision`
- `firmware_version`
- battery state
- signal/RSSI
- location/slot/port name
- user label
- icon/type hint

### Public API Safety

Generally safe for public API once bounded and owner-backed:

- `display_name`
- node type/source type
- lifecycle/status
- capability count
- last seen/freshness
- diagnostics availability
- battery availability flags
- signal availability flags
- icon/type hint

May require pairing/trust or explicit policy before public API exposure:

- `source_mac`
- manufacturer ID
- product ID
- hardware revision
- firmware version
- precise battery values
- precise signal/RSSI
- location/slot/port name
- user label if user-owned state is added later

Rules:

- optional fields must remain bounded
- no dynamic strings
- no Arduino `String`
- no STL containers
- no unbounded metadata blobs
- no transport internals exposed as primary identity

## 7. Stable `node_id` Rules

Planned `node_id` rules:

- stable within runtime
- later stable across reboot if persistent identity exists
- bounded numeric or fixed-size type
- generated or accepted only through approved Core policy
- not generated by UI
- not generated by Minimal App
- not generated by transport adapter
- not parsed by API from raw packets
- no dynamic strings
- no Arduino `String`
- no STL containers
- no MAC-as-primary-ID unless explicitly approved in a later milestone

MAC address may be a source identity for wireless nodes, but it should not automatically become the public primary node ID without an explicit architecture decision.

If no stable `node_id` exists, the node should remain pending/internal or use a clearly bounded temporary pending identity through a documented owner.

## 8. Node Visibility States

Possible node visibility states:

- empty / none
- pending_discovery
- unpaired
- paired
- trusted
- unavailable
- stale
- blocked / rejected
- error

### Empty / None

No public node record exists.

Current API behavior:

- `getNodeList(...)` returns count `0`
- detail reads return `node_not_found`

### Pending Discovery

A candidate node may exist internally but has not yet become a fully public trusted node.

Future policy decision:

- pending discovery may be exposed through a dedicated pairing/discovery API
- pending discovery should not automatically appear as a normal trusted node card

### Unpaired

Node has a known identity but is not paired.

May be visible later only if pairing UX requires it.

### Paired

Node has been accepted by user or policy.

### Trusted

Node is allowed to provide capability data.

### Unavailable

Node is known but currently not available.

### Stale

Node has not refreshed within a freshness timeout.

### Blocked / Rejected

Node is known but explicitly blocked or rejected.

Future policy decision:

- blocked/rejected nodes may remain internal
- or they may appear only in diagnostics/security views

### Error

Node record has an owner-backed error state.

## 9. Node Lifecycle

Conceptual lifecycle:

1. Node is discovered internally.
2. Node identity is validated by approved Core policy.
3. Node is optionally paired or trusted.
4. Public node record is created by the future node owner.
5. Capabilities are attached later through the public capability model and node-to-capability mapping.
6. Node is updated on heartbeat, packet acceptance, wired device update, or diagnostics update through approved owners.
7. Node becomes stale if freshness timeout expires.
8. Node becomes unavailable if no valid owner-backed state remains.
9. Node may be removed, hidden, blocked, or retained for diagnostics depending on future policy.

No implementation is performed in this milestone.

## 10. API Mapping

Future Node API methods should map to public node records only through approved owner-backed reads.

### `getNodeList(...)`

Reads bounded public node records.

Current behavior must remain valid:

- empty list success when no public node records exist

### `getNodeSummary(...)`

Reads aggregate summary for one public node by safe index.

Current behavior must remain valid:

- returns `node_not_found` when index is out of range

### `getNodeIdentity(...)`

Reads public identity fields:

- `node_id`
- friendly/display name when available
- source/provider type
- source MAC only if policy allows
- `has_source_mac` flag if exposed

### `getNodeStatus(...)`

Reads owner-backed status fields:

- online/available
- paired
- trusted
- blocked
- last seen
- provider status if appropriate and public

### `getNodePower(...)`

Reads owner-backed power fields only:

- battery percent if available
- battery mV if available
- availability flags

No fake battery data.

### `getNodeSignal(...)`

Reads owner-backed signal fields only:

- RSSI if available
- signal quality if available
- availability flags

No fake signal data.

### `getNodeDiagnostics(...)`

Reads owner-backed node diagnostics only:

- accepted/rejected counts if available
- last error if available
- security diagnostics availability

Unavailable diagnostics remains valid.

### `getNodeCapabilities(...)`

Reads bounded node-to-capability mapping only.

Must not:

- infer capabilities from packets
- infer capabilities from providers directly
- access private Registry arrays
- fabricate capabilities

## 11. Bounds and Limits

Required future bounds:

- maximum public node count
- maximum display name length
- maximum source identity length
- maximum capabilities per node
- maximum manufacturer/product metadata length if added
- maximum location/slot label length if added
- deterministic failure when full

Implementation constraints for future firmware:

- fixed arrays if implemented later
- no heap allocation
- no dynamic strings
- no Arduino `String`
- no STL containers
- bounded copy behavior
- compact result codes

Open decisions:

- final maximum public node count
- final display name length
- final source identity length
- final maximum capabilities per node
- whether these bounds reuse existing Registry limits or define public API-specific limits

Do not choose final numbers in this contract unless a later milestone confirms them.

## 12. Safety Rules

Safety rules:

- no fake nodes
- no node inferred by UI
- no node inferred by Minimal App
- no node inferred by transport
- no packet parsing in API
- no packet parsing in UI
- no packet parsing in transport
- no direct Registry array exposure
- no HAL calls from API consumers
- no Driver calls from API consumers
- no Device calls from API consumers
- no Service calls from API consumers
- no Logic calls from API consumers
- no provider selection
- no command execution
- no default values presented as real state
- no source MAC exposed as public identity unless explicitly approved
- no blocked/rejected nodes shown as normal trusted nodes

API consumers include:

- Minimal App
- Dev Panel
- Mission Control
- Dashboard
- Cloud Bridge
- Marketplace clients
- AI Assistant
- future transport adapters

## 13. Minimal App Implications

Minimal App implications:

- app remains mock-only for now
- empty node list remains valid
- `node_not_found` remains valid
- no real UI change is required yet
- future real node cards require public node records
- future pending/unavailable node states must be shown honestly if API exposes them
- app must not construct nodes from MAC addresses
- app must not construct nodes from packet logs
- app must not construct nodes from serial debug output
- app must not construct nodes from Registry assumptions
- app must not display default zero values as real node state

The Minimal App should continue to use mock data until owner-backed public node records exist and a transport milestone is approved.

## 14. Open Questions

Open decisions:

- final owner component name: `NodeDirectory`, `PublicNodeStore`, or `PublicNodeRegistry`
- maximum public nodes
- `node_id` width/type
- whether source MAC is public
- whether pairing is required before visibility
- whether pending discovery is visible through Node API or a separate discovery API
- stale timeout
- diagnostics timeout
- removal policy
- blocked/rejected visibility policy
- display name storage
- user label storage
- wired node representation
- wireless node representation
- whether wired local devices become nodes or device-backed capability providers
- whether node status should include provider status or leave provider state to capability/provider diagnostics

## 15. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.20 - Public Capability Record Contract
```

Purpose:

Define the safe public capability record contract before implementing real capability list data.

## 16. Stop Conditions

Stop future work and return to architecture review if it tries to:

- implement node list data before contract approval
- create fake nodes
- expose Registry internals
- expose private Registry arrays
- parse packets in API
- add live transport
- call HAL from app or transport
- call Drivers from app or transport
- call Devices from app or transport
- call Services from app or transport
- call Logic from app or transport
- modify `src/main.cpp`
- add WebServer before transport approval
- add HTTP before transport approval
- add JSON before transport approval
- add BLE before transport approval
- add WiFi app transport before transport approval
