# Milestone 10.23 - Owner-Backed Diagnostics Read Path Plan

## 1. Scope

Milestone 10.23 plans the future read path for owner-backed public diagnostics snapshots.

This milestone is documentation only.

It defines how Cyber32Api should eventually expose real diagnostics for nodes, capabilities, providers, value freshness, battery, signal, and wireless security only after stable owner-backed diagnostic snapshots exist.

This milestone does not:

- implement diagnostics storage
- implement real diagnostics reads
- implement real `getNodeDiagnostics(...)`
- implement real `getCapabilityQuality(...)`
- implement real `getCapabilityAvailability(...)`
- implement provider health reads
- create public arrays in code
- add provider selection
- parse packets
- change API behavior
- change source code
- modify `src/main.cpp`

## 2. Background

Recent architecture contracts define the public owner-backed model Cyber32 needs before UI clients can display real data:

- Public Node Record Contract exists.
- Public Capability Record Contract exists.
- Node-to-Capability Mapping Contract exists.
- Owner-Backed Capability Value Read Path Plan exists.

Those contracts make diagnostics a separate owner-backed concern.

Current constraints:

- Minimal App currently shows unavailable diagnostics.
- zero counters must not mean healthy.
- diagnostics availability is separate from node existence.
- diagnostics availability is separate from capability existence.
- value validity is separate from diagnostics health.
- a fresh value does not prove a healthy node.
- a healthy diagnostic snapshot does not prove a fresh value.
- API placeholder behavior must remain honest until real owner-backed diagnostics exist.

## 3. Diagnostics Read Path Definition

Approved future diagnostics read path:

```text
Approved internal diagnostic source
-> diagnostics owner snapshot
-> public API-safe diagnostics view
-> Cyber32Api read methods
-> UI / Transport
```

Forbidden paths:

```text
UI / App / Transport -> packet buffers
UI / App / Transport -> provider internals
UI / App / Transport -> drivers / devices / HAL
Cyber32Api -> packet parser
Cyber32Api -> private Registry arrays
Cyber32Api -> provider selection
Cyber32Api -> diagnostics mutation as side effect
```

Diagnostics reads must be passive and read-only.

## 4. Future Diagnostics Owner Concept

Future Core OS should introduce bounded diagnostics owner concepts.

Possible names:

- `DiagnosticsStore`
- `PublicDiagnosticsStore`
- `NodeDiagnosticsStore`
- `CapabilityDiagnosticsStore`
- `HealthSnapshotStore`

Recommended conceptual model:

```text
DiagnosticsStore
```

with separate bounded node, capability, and provider snapshot regions.

This name and model are conceptual only. Do not implement them yet.

### Ownership Responsibilities

The future diagnostics owner should:

- store node diagnostic snapshots
- store capability diagnostic snapshots
- store provider diagnostic snapshots
- store freshness diagnostics
- store battery diagnostics if available
- store signal / RSSI diagnostics if available
- store security / trust diagnostics if available
- expose read-only snapshots to Cyber32Api
- prevent zero defaults from being treated as healthy
- provide deterministic unavailable state
- avoid resetting counters on reads
- avoid updating provider health on reads
- avoid provider selection on reads
- fail deterministically when full or unavailable

The diagnostics owner must not expose packet buffers, provider pointers, private Registry arrays, driver internals, device internals, module internals, or HAL state.

## 5. Diagnostic Snapshot Categories

### A. Node Diagnostics

Conceptual node diagnostics may include:

- node health availability
- last seen / stale state
- lifecycle status
- pairing / trust status
- last node error placeholder
- accepted update counters if owner-backed
- rejected update counters if owner-backed

Node diagnostics must not be fabricated from missing errors, zero counters, packet logs, or UI guesses.

### B. Capability Diagnostics

Conceptual capability diagnostics may include:

- capability health availability
- value freshness state
- stale / unavailable state
- value rejection reason if owner-backed
- last capability error placeholder
- quality placeholder

Capability diagnostics must not imply that a valid fresh value exists unless value state also says so.

### C. Provider Diagnostics

Conceptual provider diagnostics may include:

- provider availability
- provider status
- provider quality
- last provider error
- provider freshness

Provider diagnostics must not trigger provider selection.

Provider metadata remains diagnostic information and must not become the normal Logic contract.

### D. Power Diagnostics

Conceptual power diagnostics may include:

- battery percent availability
- battery mV availability
- charging placeholder
- power source placeholder

No fake battery percent or battery voltage may be exposed.

### E. Signal Diagnostics

Conceptual signal diagnostics may include:

- RSSI availability
- signal quality availability
- packet / link freshness placeholder

No fake RSSI, signal strength, or signal score may be exposed.

### F. Security / Trust Diagnostics

Conceptual security diagnostics may include:

- paired state
- trusted state
- blocked state
- rejected source reason placeholder
- security diagnostics availability placeholder

Security diagnostics must not expose raw packets or bypass WirelessService policy.

## 6. Snapshot Field Rules

Conceptual diagnostics snapshot fields:

- target type: node, capability, provider, or system
- target public ID or reference
- `diagnostics_available`
- `health_known`
- `health_ok`
- `stale`
- `last_update_ms`
- last error code placeholder
- accepted count placeholder
- rejected count placeholder
- unavailable reason placeholder

Rules:

- `diagnostics_available` must be explicit.
- `health_known` must be explicit.
- `health_ok == false` is not meaningful unless `health_known == true`.
- zero counters are not meaningful unless `diagnostics_available == true`.
- default struct values mean unavailable / unknown.
- timestamps must be owner-backed.
- error codes must be bounded stable literals or bounded IDs.
- no dynamic strings.
- no Arduino `String`.
- no STL containers.

## 7. API Mapping

Future API methods should map to diagnostics snapshots only through public owner-backed reads.

Affected methods include:

- `getNodeDiagnostics(...)`
- `getNodeStatus(...)`
- `getNodePower(...)`
- `getNodeSignal(...)`
- `getCapabilityAvailability(...)`
- `getCapabilityProviderInfo(...)`
- `getCapabilityQuality(...)`

Rules:

- if a public node does not exist, return `node_not_found`.
- if a public capability does not exist, return `capability_not_found`.
- if the record exists but diagnostics owner does not exist, return unavailable diagnostics.
- if diagnostics exist, copy bounded snapshot fields.
- never invent healthy state.
- never treat zero counters as healthy.
- never parse packets.
- never call providers directly.
- never trigger provider selection.
- never reset counters as a side effect.
- never update provider health as a side effect.
- never expose private Registry arrays.

Existing empty-state behavior remains valid until owner-backed public data exists.

## 8. Relationship to Value Read Path

Diagnostics and values are separate contracts.

Rules:

- `value_valid` is not the same as diagnostics healthy.
- diagnostics healthy is not the same as value fresh.
- stale value may still have useful diagnostics.
- unavailable value may still have a diagnostics reason.
- `getCapabilityQuality(...)` may later explain value/source quality.
- `getCapabilityQuality(...)` must not be required for basic value reads.
- diagnostics reads must not mutate value state.
- value reads must not reset diagnostics.
- unavailable value state must not be hidden behind healthy diagnostics.

## 9. Freshness and Timestamps

Diagnostics freshness policy is not implemented in this milestone.

Future policy should define:

- timestamp owner
- diagnostics update interval
- stale timeout
- unavailable timeout
- whether stale diagnostics remain visible
- whether diagnostics disappear when owners expire

Rules:

- last update timestamp must be Core-owned.
- stale diagnostics must be explicit.
- stale diagnostics must not look healthy or current.
- timeout checks should be Core-owned, not UI-owned.
- reads must not refresh diagnostics timestamps.

## 10. Error and Unavailable Reasons

Conceptual unavailable / reason names:

- `no_diagnostics_owner`
- `diagnostics_not_supported`
- `not_updated_yet`
- `stale`
- `owner_unavailable`
- `provider_unavailable`
- `node_unavailable`
- `capability_unavailable`
- `security_blocked`
- `unsupported`
- `internal_error`

Do not implement a new enum in this milestone unless a later implementation milestone explicitly approves it.

For now, these names document future stable reason concepts only.

## 11. Minimal App Implications

Minimal App remains mock-only until owner-backed diagnostics are available through approved API methods.

Minimal App should:

- continue to show unavailable diagnostics for now
- distinguish unknown, unavailable, healthy, and unhealthy states later
- avoid showing zero counters as healthy
- avoid inferring health from missing errors
- avoid polling packets directly
- avoid polling providers directly
- avoid displaying fake battery values
- avoid displaying fake RSSI values
- avoid displaying fake quality scores
- use Cyber32 API only

Initial UI should be prepared to show:

- diagnostics unavailable
- diagnostics unsupported
- health unknown
- healthy
- unhealthy
- stale diagnostics
- last error placeholder
- owner-backed counters when available

## 12. Bounds and Firmware Safety

Future implementation constraints:

- fixed-size diagnostics snapshots
- bounded counters
- bounded error codes
- no heap allocation
- no dynamic strings
- no Arduino `String`
- no STL containers
- deterministic unavailable state
- deterministic failure when storage is full
- compact result codes or stable error literals

Do not choose final storage counts, counter widths, or timeout values unless a later milestone confirms them.

Safety rules:

- no fake diagnostics
- no fake health state
- no zero counters treated as healthy
- no packet parsing in API
- no private Registry array exposure
- no provider selection during diagnostics reads
- no diagnostics mutation during reads
- no command execution during reads
- no app or transport direct access to HAL, Drivers, Devices, Modules, Services, or Logic

## 13. Open Questions

Open decisions:

- final diagnostics owner component name
- node diagnostics bounds
- capability diagnostics bounds
- provider diagnostics bounds
- counter widths
- timestamp source
- diagnostics freshness timeout
- which diagnostics are public before pairing
- whether rejected or blocked nodes appear in diagnostics
- battery metadata ownership
- signal / RSSI metadata ownership
- provider quality scoring formula
- relationship between Runtime counters and public diagnostics
- whether diagnostics survive reboot
- whether diagnostics are separated by owner or unified in one store
- whether security diagnostics remain separate from general node diagnostics
- whether diagnostics reasons use enum IDs, stable strings, or both

## 14. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.24 - API Real Empty-to-Owned Transition Plan
```

Purpose:

Plan how existing empty-state API methods transition to owner-backed real data without breaking current empty-state semantics.

## 15. Stop Conditions

Stop future work and return to architecture review if it tries to:

- implement diagnostics reads before this plan is approved
- create fake diagnostics
- treat zero counters as healthy
- parse packets in Cyber32Api
- call providers directly from Cyber32Api reads
- trigger provider selection from diagnostics reads
- reset counters from read methods
- expose Registry internals
- expose private Registry arrays
- add live transport
- let app or transport call HAL, Drivers, Devices, Modules, Services, or Logic
- modify `src/main.cpp`
- add WebServer before transport approval
- add HTTP before transport approval
- add JSON before transport approval
- add BLE before transport approval
- add WiFi app transport before transport approval
