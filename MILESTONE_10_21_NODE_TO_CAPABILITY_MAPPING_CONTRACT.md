# Milestone 10.21 - Node-to-Capability Mapping Contract

## 1. Scope

Milestone 10.21 defines the node-to-capability mapping contract used by Cyber32Api before implementing `getNodeCapabilities(...)` with real data.

This milestone is documentation only.

It defines:

- why public node-to-capability mapping is needed
- what a mapping is
- which future Core component should own mapping
- link record fields
- link identity rules
- mapping lifecycle
- mapping visibility states
- future API mapping behavior
- empty-state compatibility
- bounds and limits
- safety rules
- Minimal App implications
- relationship to future capability values
- local/Core capability policy questions
- open questions and stop conditions

No firmware implementation is performed in this milestone.

## 2. Background

The mapping contract is needed because Cyber32 now has documentation contracts for:

- public node records
- public capability records

Current state:

- `getNodeCapabilities(...)` currently returns no real data.
- Node API still uses safe empty-list / `node_not_found` behavior.
- Capability API still uses safe empty-list / `capability_not_found` behavior.
- Minimal App is mock-only.
- Minimal App must not infer capabilities from node IDs.
- Minimal App must not infer capabilities from packets.
- Minimal App must not infer capabilities from MAC addresses.
- Minimal App must not infer capabilities from Registry arrays.
- Minimal App must not infer capabilities from providers.
- Minimal App must not infer capabilities from driver names.

Real mapping must be owner-backed, bounded, API-safe, and capability-first.

## 3. Mapping Definition

Node-to-capability mapping is a bounded, owner-backed, API-safe relationship between:

- one public node record
- zero or more public capability records

A mapping answers:

```text
Which public capabilities belong to this public node?
```

Mapping is distinct from:

- Registry private arrays
- packet contents
- ESP-NOW payloads
- provider internals
- driver internals
- device internals
- module internals
- UI grouping
- transport session state
- debug serial output

Mappings must not be fabricated to satisfy UI needs. If no owner-backed mapping exists, `getNodeCapabilities(...)` must return an empty result or current safe failure behavior depending on whether the node exists.

## 4. Ownership Model

Future Core OS should introduce a dedicated mapping owner concept.

Possible component names:

- `NodeCapabilityMap`
- `PublicNodeCapabilityMap`
- `CapabilityLinkDirectory`

Recommended concept name for planning:

```text
NodeCapabilityMap
```

This name is conceptual only. Do not implement it yet.

### Ownership Responsibilities

The future owner should be responsible for:

- storing bounded node-to-capability links
- validating public node ownership
- validating public capability ownership
- exposing read-only mapping to Cyber32Api
- preventing fake links
- preventing duplicate links
- enforcing deterministic failure when full
- handling stale node references safely
- handling stale capability references safely
- handling removed node references safely
- handling removed capability references safely
- preserving current empty-state semantics

### Creation Responsibility

Links should be created only by approved Core owner paths, such as:

- future discovery-to-public-record bridge
- future public node owner plus public capability owner coordination
- future wired local device registration bridge
- future trusted static registration path

UI, app, and transport layers must not create mapping links directly.

### Read-Only API Access Responsibility

Cyber32Api should read mappings through public owner methods only.

Cyber32Api must not:

- read private Registry arrays
- parse packets
- infer capabilities from providers
- call Drivers
- call Devices
- call HAL
- call Services directly for UI reads
- trigger provider selection

## 5. Link Record Definition

Conceptual link fields:

- `node_id` or public node index reference
- `capability_instance_id` or public capability index reference
- link visibility state
- link freshness/state placeholder
- link diagnostics availability placeholder
- ordering/grouping placeholder if needed

### Node Reference

The link should reference a stable public node record.

Acceptable conceptual references:

- stable `node_id`
- future public node instance ID
- safe public node index only if validated at read time

### Capability Reference

The link should reference a stable public capability record.

Acceptable conceptual references:

- `capability_instance_id`
- public capability index only if validated at read time
- `CAP_*` type plus instance identity where needed

### Link Visibility State

Controls whether the relationship is visible through public API.

### Link Freshness / State Placeholder

Represents whether the relationship is active, stale, unavailable, or pending.

### Link Diagnostics Availability Placeholder

Represents whether mapping-level diagnostics exist.

### Ordering / Grouping Placeholder

Optional future field for deterministic display order or grouping.

All link fields must be fixed-size and firmware-safe in a future implementation.

## 6. Link Identity Rules

Link identity rules:

- links are not UI-generated
- links are not Minimal App-generated
- links are not transport-generated
- links are not packet-generated by API
- links must be created only by approved Core owner paths
- links should reference stable public records
- links must not reference private pointers
- links must not expose provider pointers
- links must not use provider pointer/index as public link identity
- links must not expose Registry array indexes as public identity
- links must not use dynamic strings
- links must not use Arduino `String`
- links must not use STL containers

If either side of a link is not a valid public record, the link must remain hidden, unavailable, or invalid according to future policy.

## 7. Mapping Lifecycle

Conceptual lifecycle:

1. Public node record exists or is pending.
2. Public capability record exists or is pending.
3. Approved Core path attaches capability to node.
4. Mapping becomes visible to API only when visibility policy allows.
5. Mapping updates when node state changes.
6. Mapping updates when capability state changes.
7. Mapping becomes stale/unavailable if owner references expire.
8. Mapping is removed or hidden if node is removed, hidden, blocked, or rejected.
9. Mapping is removed or hidden if capability is removed, hidden, disabled, blocked, or rejected.
10. Mapping may be retained for diagnostics depending on future policy.

No implementation is performed in this milestone.

## 8. Mapping Visibility States

Possible mapping visibility states:

- none
- pending
- active
- unavailable
- stale
- disabled
- blocked / rejected
- error

### None

No mapping exists.

### Pending

Mapping candidate exists internally but is not yet public.

Future policy decision:

- pending mappings may remain internal
- or may be exposed through a future discovery/pairing API

### Active

Node and capability records are valid and relationship is public.

Active mapping does not prove the capability has a fresh value.

### Unavailable

Relationship is known but currently unavailable.

### Stale

Relationship has not refreshed or has stale owner references.

### Disabled

Relationship is intentionally disabled by policy or configuration.

### Blocked / Rejected

Relationship is blocked or rejected.

Future policy decision:

- blocked/rejected mappings may remain internal
- or may appear only in diagnostics/security views

### Error

Mapping has an owner-backed error state.

## 9. API Mapping

Future Node API method:

```text
getNodeCapabilities(node_index, out_capabilities, max_count, out_count)
```

should use the public mapping owner only.

Rules:

- read public node record by safe index
- if node does not exist, return `node_not_found` behavior
- initialize `out_count = 0` before any return
- if node exists but has no capabilities, return success with `out_count = 0`
- copy bounded capability summaries only
- never exceed `max_count`
- never expose private Registry arrays
- never infer capabilities from providers
- never parse packets
- never call HAL
- never call Drivers
- never call Devices
- never call Services
- never perform provider selection
- never fabricate capabilities

### Capability API Relationship Fields

Future Capability API may expose owner node relationship:

- `owner_node_id`
- `has_owner_node`
- owner node placeholder/unavailable state

Rules:

- owner relationship is optional until mapping exists
- if not mapped, relationship must be unavailable/placeholder
- app must not infer owner node from capability ID, provider name, MAC, packet source, or driver name

## 10. Empty-State Compatibility

Existing empty-state semantics must remain valid.

Required compatibility:

- `getNodeCapabilities(...)` must preserve current safe behavior
- empty node list remains valid
- `node_not_found` remains valid
- empty capability list remains valid
- `capability_not_found` remains valid
- node with zero capabilities is valid and not an error
- capability without public node owner may remain hidden or node-less by policy
- deterministic defaults remain unavailable state
- no fake capability summaries are returned

Future behavior examples:

- no node exists: detail read returns `node_not_found`
- node exists but has no mapped capabilities: success with `out_count = 0`
- node exists and has mapped capabilities: success with bounded copied summaries
- capability exists but has no public owner node: capability may remain node-less if policy allows, or hidden if policy requires node ownership

## 11. Bounds and Limits

Required future bounds:

- maximum links
- maximum capabilities per node
- maximum nodes per capability if allowed
- deterministic failure when link map is full
- deterministic behavior when output buffer is too small

Implementation constraints for future firmware:

- fixed arrays if implemented later
- no heap allocation
- no dynamic strings
- no Arduino `String`
- no STL containers
- bounded copy behavior
- compact result codes

Open decisions:

- final maximum link count
- final maximum capabilities per node
- whether a capability can belong to more than one node
- whether link bounds reuse node/capability bounds or define separate public API limits
- whether ordering/grouping metadata has separate bounds

Do not choose final numbers in this contract unless a later milestone confirms them.

## 12. Safety Rules

Safety rules:

- no fake links
- no fake nodes
- no fake capabilities
- no fake sensor values
- no UI-inferred mapping
- no Minimal App-inferred mapping
- no transport-inferred mapping
- no packet parsing in API
- no packet parsing in UI
- no packet parsing in transport
- no direct Registry array exposure
- no provider pointer exposure
- no provider private index exposure
- no HAL calls from API consumers
- no Driver calls from API consumers
- no Device calls from API consumers
- no Service calls from API consumers
- no Logic calls from API consumers
- no provider selection
- no command execution
- no default values presented as real mapping state
- no node-capability relationship used to fake a value

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
- Nodes screen stays empty until real public node records exist
- Capabilities screen stays empty until real public capability records exist
- future node detail screen can show capabilities only through `getNodeCapabilities(...)`
- node with zero capabilities should be displayed honestly later
- app must not group capabilities by MAC address
- app must not group capabilities by packet source
- app must not group capabilities by provider name
- app must not group capabilities by driver name
- app must not group capabilities by Registry assumptions
- app must not show default zero values as real capability readings

The Minimal App should continue to use mock data until owner-backed public records, mapping, value state, and an approved transport milestone exist.

## 14. Relationship to Future Capability Values

Mapping only links node records and capability records.

Mapping does not prove:

- a fresh value exists
- a provider is healthy
- a sensor reading is valid
- a diagnostic state is healthy
- a command is allowed

Value availability remains separate.

Rules:

- stale value states must still be explicit
- unavailable value states must still be explicit
- node-capability relationship must not cause fake readings
- no default zero values are real
- `PayloadValueType::NONE` remains no value
- provider metadata remains separate diagnostic metadata

## 15. Local / Core Capabilities

Open policy:

- local Core capability may be Core-owned without a remote node
- local wired device capability may be represented as a local node
- local wired device capability may be represented as a Core-owned capability
- node-less capabilities may be allowed only through explicit policy
- Minimal App must not decide this policy
- transport must not decide this policy
- API must report only owner-backed policy decisions

Potential future directions:

- represent Core itself as a local node
- represent local wired devices as local public nodes
- allow node-less system capabilities for Core diagnostics only
- hide node-less capabilities until a policy is approved

No decision is made in this milestone.

## 16. Open Questions

Open decisions:

- final mapping component name: `NodeCapabilityMap`, `PublicNodeCapabilityMap`, or `CapabilityLinkDirectory`
- maximum links
- maximum capabilities per node
- whether a capability can belong to more than one node
- whether node-less capabilities are public
- whether local Core itself is represented as a node
- mapping freshness timeout
- mapping removal policy
- blocked/rejected mapping visibility
- ordering of capabilities in `getNodeCapabilities(...)`
- whether mapping stores indexes or stable IDs internally
- how mapping handles reboot persistence later
- whether mapping diagnostics are needed separately from node/capability diagnostics
- how mapping handles capability replacement
- how mapping handles node replacement

## 17. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.22 - Owner-Backed Capability Value Read Path Plan
```

Purpose:

Plan how capability values move from internal provider/canonical state to public API-safe read state without fake readings or direct provider access.

## 18. Stop Conditions

Stop future work and return to architecture review if it tries to:

- implement node-to-capability mapping before contract approval
- implement `getNodeCapabilities(...)` real data in this milestone
- create fake links
- create fake nodes
- create fake capabilities
- create fake sensor values
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
