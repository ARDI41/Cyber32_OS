# Milestone 10.18 - Core OS Real Data Path Plan

## 1. Scope

Milestone 10.18 plans the safe public owner-backed real data path required before the Cyber32 Minimal App can display real nodes, real capabilities, real diagnostics, or real sensor values.

This milestone is documentation only.

It defines:

- current Core OS API state
- permanent API/UI access rules
- missing real-data path components
- recommended implementation sequence
- future API contract impact
- safety invariants
- Minimal App implications
- open architecture questions
- stop conditions

No firmware implementation is performed in this milestone.

## 2. Current State Summary

Current known API and app state:

- System API has read-only summary methods.
- Node API returns a safe empty list.
- Node detail methods return `node_not_found` while no public node owner exists.
- Capability API returns a safe empty list.
- Capability detail methods return `capability_not_found` while no public capability owner exists.
- Minimal App is mock-only.
- Minimal App can display system summary mock data.
- Minimal App can display empty node list state.
- Minimal App can display empty capability list state.
- Minimal App can display unavailable diagnostics state.
- Minimal App can display placeholder logo assets.
- No live transport exists.
- No public owner-backed node/capability mapping exists yet.
- No public owner-backed capability value read path exists yet.
- No public owner-backed diagnostics read path exists yet.

The existing empty-state API behavior is correct and must remain valid after real data owners are introduced.

## 3. Core Rule

Permanent rule:

```text
UI / App / Transport -> Cyber32Api public methods only
```

UI, app, and transport layers must never access:

- Registry internals
- private Registry arrays
- packets
- ESP-NOW payloads
- HAL
- Drivers
- Devices
- Modules
- Services
- Logic

Correct future flow:

```text
Minimal App
-> approved transport adapter
-> Cyber32Api
-> public owner-backed Core state
-> Registry / Services / Runtime / Logic through approved internal ownership paths only
```

Forbidden future flows:

```text
Minimal App -> Registry arrays
Minimal App -> packets
Minimal App -> HAL
Minimal App -> Driver
Minimal App -> Device
Minimal App -> Service
Transport -> Registry arrays
Transport -> packet parser
Transport -> provider selection
```

## 4. Missing Real-Data Path Components

### A. Public Node Ownership Model

Cyber32 needs a public node ownership model before the Node API can expose real nodes.

The model must define:

- what owns a public node record
- how public node records are created
- stable `node_id`
- safe indexed access
- display name later
- source identity later
- owner-backed status later
- lifecycle rules
- maximum public node count

Rules:

- no fake nodes
- no nodes inferred directly by UI or app
- no nodes inferred directly from packets by the API
- public node records must be created through an approved Core path
- API consumers must see only public bounded node records

### B. Public Capability Ownership Model

Cyber32 needs a public capability ownership model before the Capability API can expose real capabilities.

The model must define:

- capability instance ownership
- stable capability identity
- safe `capability_index`
- relationship between a capability and a node
- capability display metadata later
- maximum public capability count
- capability lifecycle rules

Rules:

- no fake capabilities
- no hard-coded UI capability cards
- no capabilities inferred directly by app from packets, providers, or hardware
- capabilities must remain `CAP_*` capability-first contracts

### C. Node-to-Capability Mapping

Cyber32 needs a public API-safe mapping between nodes and capabilities.

The mapping must define:

- which capabilities belong to a public node
- how `getNodeCapabilities(...)` reads bounded capability summaries
- how a node detail screen can show capability cards later
- how capability records reference owner node metadata if allowed
- how missing mappings report unavailable state

Rules:

- no direct Registry array access
- no provider selection inside the mapping
- no packet parsing inside the API
- no app-side inference from MAC, packets, or provider records

### D. Capability Value Owner-Backed Read Path

Cyber32 needs an owner-backed capability value read path before the Capability API can expose real sensor values.

The read path must define:

- which Core owner stores the current value
- how payload validity is represented
- how freshness is represented
- how stale/unavailable state is represented
- how value type is represented
- how unit metadata is represented later
- how timestamps are owned

Rules:

- API must read only stable owner-backed state
- no random/default sensor values shown as real
- default zero values are not real readings
- `PayloadValueType::NONE` means no value
- stale/unavailable states must be explicit
- owner must exist before real value is exposed

### E. Diagnostics Owner-Backed Read Path

Cyber32 needs owner-backed diagnostics before UI can show real node or capability diagnostics.

Diagnostics should include:

- node diagnostics
- capability diagnostics
- provider quality/freshness
- battery percent later
- battery voltage later
- signal/RSSI later
- wireless security diagnostics where appropriate
- last error state
- accepted/rejected counters where appropriate

Rules:

- unavailable is valid until a real owner exists
- zero counters on failed reads must not be treated as healthy diagnostics
- diagnostics reads must be read-only
- diagnostics reads must not reset counters
- diagnostics reads must not trigger provider selection
- diagnostics reads must not update provider health as a side effect

### F. Discovery-to-Public-Record Bridge

Cyber32 needs a safe bridge from internal discovery to public node/capability records.

Discovery and PNP may produce internal events, but public records must be created through approved Core ownership paths.

The bridge must define:

- how internal discovery events become pending/public records
- how pairing/trust affects public visibility
- how source identity is preserved
- how capability metadata is attached
- how duplicate discoveries are handled
- how rejected or blocked nodes remain non-public or diagnostic-only

Rules:

- no UI packet parsing
- no app direct discovery access
- no app-created Registry array entries
- no packet parser inside API
- no fake pending nodes

### G. Transport Boundary Later

A transport adapter may expose Cyber32Api output later.

Not in this milestone:

- no HTTP
- no BLE
- no WiFi transport
- no WebSocket
- no JSON transport
- no live Minimal App connection

Future transport must be an adapter over Cyber32Api, not a replacement for Cyber32Api.

## 5. Recommended Implementation Sequence

Recommended future milestones:

### Milestone 10.19 - Public Node Record Contract

Define the safe public node record contract used by Cyber32Api before implementing real node list data.

### Milestone 10.20 - Public Capability Record Contract

Define the safe public capability record contract used by Cyber32Api before implementing real capability list data.

### Milestone 10.21 - Node-to-Capability Mapping Contract

Define the bounded public mapping between public nodes and public capabilities.

### Milestone 10.22 - Owner-Backed Capability Value Read Path Plan

Plan how capability values move from internal provider/canonical state to public API-safe read state without fake readings.

### Milestone 10.23 - Owner-Backed Diagnostics Read Path Plan

Plan node, capability, provider, battery, signal, freshness, and security diagnostic ownership for public API reads.

### Milestone 10.24 - API Real Empty-to-Owned Transition Plan

Plan how existing empty-state API methods transition to owner-backed real data without breaking current empty-state semantics.

### Milestone 10.25 - Minimal App Live Transport Boundary Plan

Plan live transport only after Core API has safe owner-backed real data.

This sequence must remain documentation-first and implementation-gated. Each implementation milestone should be narrow, bounded, validated, and reversible.

## 6. API Contract Impact

Existing Node API methods that may later become backed by real owner-backed data:

- `getNodeList(...)`
- `getNodeSummary(...)`
- `getNodeIdentity(...)`
- `getNodeStatus(...)`
- `getNodePower(...)`
- `getNodeSignal(...)`
- `getNodeDiagnostics(...)`
- `getNodeCapabilities(...)`

Existing Capability API methods that may later become backed by real owner-backed data:

- `getCapabilityList(...)`
- `getCapabilitySummary(...)`
- `getCapabilityIdentity(...)`
- `getCapabilityValue(...)`
- `getCapabilityAvailability(...)`
- `getCapabilityProviderInfo(...)`
- `getCapabilityQuality(...)`

Required compatibility:

- empty list remains valid
- `node_not_found` remains valid
- `capability_not_found` remains valid
- deterministic defaults after failed reads remain unavailable state
- no method should invent data
- no method should parse packets
- no method should expose private Registry arrays
- provider info remains diagnostic metadata and must not trigger provider selection

## 7. Safety Invariants

Safety invariants for future implementation:

- no private Registry access from API consumers
- no private Registry arrays exposed through API
- no Services access from app or transport
- no Drivers access from app or transport
- no Devices access from app or transport
- no HAL access from app or transport
- no Logic access from app or transport
- no fake data
- no fake nodes
- no fake capabilities
- no fake sensor values
- no default zero values treated as real
- stale state must be explicit
- unavailable state must be explicit
- owner must exist before real value is exposed
- capability-first remains the rule
- API remains read-only for read methods
- transport remains an adapter over Cyber32Api
- no dynamic allocation if firmware code is later changed
- no Arduino `String` if firmware core is later changed
- no STL containers if firmware core is later changed
- `src/main.cpp` remains untouched unless explicitly approved

## 8. Minimal App Implications

Current Minimal App state remains valid:

- mock app remains valid
- empty node list remains valid
- empty capability list remains valid
- unavailable diagnostics remains valid
- no real UI change is required yet
- no live transport is required yet

Future app behavior:

- app should continue to show unavailable/empty until API says otherwise
- app must not infer real nodes from packet logs, MAC addresses, or debug output
- app must not show default zero values as real readings
- app should treat owner-backed API records as the first source of truth for real UI data
- app needs transport only after Core API has safe owner-backed data

The Minimal App should remain mock-first and API-shaped until Milestone 10.25 or another approved transport milestone.

## 9. Open Questions

Open architecture questions:

- What is the stable `node_id` format?
- What is the stable capability instance ID or index format?
- Which owner controls public node record lifetime?
- Which owner controls public capability record lifetime?
- What is the maximum public node count?
- What is the maximum public capability count?
- What is the payload freshness timeout?
- What is the diagnostics freshness timeout?
- Does source MAC belong in the public API?
- If source MAC is public, which fields are safe to expose?
- Must pairing/trust precede public node exposure?
- Should pending discovery records be visible before trust?
- How are blocked or rejected nodes represented in public diagnostics?
- How are wired sensors represented beside wireless sensors?
- How are friendly names stored and bounded?
- How are units/catalog metadata owned?

## 10. Stop Conditions

Stop future work and return to architecture review if it tries to:

- add live transport before owner-backed Core API exists
- connect Minimal App directly to Registry
- connect Minimal App directly to packets
- expose private arrays
- fake nodes
- fake capabilities
- fake sensor values
- call HAL from app or transport
- call Drivers from app or transport
- call Devices from app or transport
- call Services from app or transport
- call Logic from app or transport
- modify `src/main.cpp`
- add WebServer before transport plan approval
- add HTTP before transport plan approval
- add JSON before transport plan approval
- add BLE before transport plan approval
- add WiFi app transport before transport plan approval

## 11. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.19 - Public Node Record Contract
```

Purpose:

Define the safe public node record contract used by Cyber32Api before implementing real node list data.

Milestone 10.19 should remain documentation-first and should not implement real node list data until the public record contract is approved.
