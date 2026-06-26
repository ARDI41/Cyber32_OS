# Milestone 10.34 - CapabilityDirectory API Attachment Plan

## 1. Scope

Milestone 10.34 plans how Cyber32Api can later attach read-only Capability API methods to the empty-by-default `CapabilityDirectory` skeleton.

This milestone is documentation only.

It does not:

- modify firmware source code
- modify `src/main.cpp`
- connect `CapabilityDirectory` to `Cyber32Api`
- implement real capability list data
- add capability creation
- add discovery, provider, Registry, or node-to-capability bridges
- add `CapabilityValueStore`
- add `DiagnosticsStore`
- parse packets
- add WebServer, HTTP, JSON, BLE, WiFi, or Minimal App transport
- change existing API behavior

The purpose is to define a safe attachment path before implementation begins.

## 2. Background

Recent milestones established the owner-backed public capability path:

- public owner types exist
- `PublicCapabilityRecord` exists
- `CapabilityDirectory` exists as an empty skeleton
- `CapabilityDirectory::count()` is `0` by default
- `CapabilityDirectory::readByIndex(...)` fails when empty
- failed reads reset the output record to deterministic unavailable/default state
- Capability API currently returns a safe empty list
- Capability detail methods currently return `capability_not_found`

The first API attachment must preserve current app-visible behavior exactly while `CapabilityDirectory` is empty.

Current empty behavior is correct architecture, not a bug.

## 3. Attachment Principle

API attachment must be behavior-preserving while `CapabilityDirectory` is empty.

Meaning:

- `getCapabilityList(...)` remains `ok = true`, `error_code = "none"`, `count = 0`
- `getCapabilitySummary(...)` remains `capability_not_found`
- `getCapabilityIdentity(...)` remains `capability_not_found`
- `getCapabilityValue(...)` remains `capability_not_found` while no capability exists
- `getCapabilityAvailability(...)` remains `capability_not_found` while no capability exists
- `getCapabilityProviderInfo(...)` remains `capability_not_found` while no capability exists
- `getCapabilityQuality(...)` remains `capability_not_found` while no capability exists
- no fake capability data appears
- no fake `CAP_TEMPERATURE` appears
- no fake sensor values appear
- no fake diagnostics appear
- no app-visible behavior changes yet

The API must switch from empty to owner-backed only. It must never switch from empty to fake.

## 4. Proposed Future Ownership Location

Future implementation must decide where the `CapabilityDirectory` instance lives.

Options:

- static/global owner instance in the API layer
- dedicated public owner module
- Runtime-owned public owner
- Core context object later

### Static/API-Layer Owner

A provisional static owner in the API layer is the smallest behavior-preserving first step for the current project layout.

Benefits:

- mirrors the recent NodeDirectory empty attachment approach
- requires no production bootstrap changes
- requires no `src/main.cpp` change
- keeps the owner empty and deterministic
- avoids lifetime ambiguity during validation

Risks:

- not the final ownership model
- may need migration to a Core context or public owner module later

### Dedicated Public Owner Module

A dedicated public owner module is cleaner long term.

Benefits:

- separates public owners from API method implementation
- prepares for `NodeDirectory`, `CapabilityDirectory`, `NodeCapabilityMap`, `CapabilityValueStore`, and `DiagnosticsStore`

Risks:

- requires a broader lifetime and initialization plan
- may be premature before real creation paths exist

### Runtime-Owned Public Owner

Runtime ownership is not recommended for the first attachment.

Runtime should remain scheduler/state owner, not public data owner, unless a later architecture milestone explicitly approves that boundary.

### Core Context Object

A future Core context object may be the best production owner.

Benefits:

- explicit lifetime
- avoids hidden globals
- can own multiple public stores consistently

Risks:

- requires production bootstrap planning
- likely touches broader architecture and may require `src/main.cpp` approval

### Recommendation

For the first attachment implementation, use the safest current-project option:

```text
provisional internal static CapabilityDirectory in the API layer
```

This recommendation is limited to the empty attachment milestone. A future public owner lifetime plan should replace or formalize it before real records are created.

## 5. API Methods Affected Later

### `getCapabilityList(...)`

Current behavior:

- returns true
- `ok = true`
- `error_code = "none"`
- `count = 0`

Future `CapabilityDirectory`-backed behavior while empty:

- read `CapabilityDirectory::count()`
- return the same empty success when count is `0`

Future behavior when records exist later:

- copy bounded summaries from owner-backed `PublicCapabilityRecord` entries
- never exceed `API_MAX_CAPABILITY_SUMMARY_COUNT`
- unavailable fields remain unavailable/default

Stop conditions:

- fake capability summary
- fake `CAP_TEMPERATURE`
- Registry array reads
- packet parsing
- provider selection

### `getCapabilitySummary(...)`

Current behavior:

- returns false
- `ok = false`
- `error_code = "capability_not_found"`

Future `CapabilityDirectory`-backed behavior while empty:

- `readByIndex(...)` fails
- method returns the same `capability_not_found`

Future behavior when records exist later:

- map owner-backed identity, availability, provider, and quality fields that are present
- keep missing value/provider/diagnostic fields unavailable

Stop conditions:

- inventing values
- implying healthy diagnostics from defaults
- reading providers directly
- triggering provider selection

### `getCapabilityIdentity(...)`

Current behavior:

- returns `capability_not_found`

Future `CapabilityDirectory`-backed behavior while empty:

- failed read maps to `capability_not_found`

Future behavior when records exist later:

- map `capability_id`, capability instance identity, category, lifecycle, and visibility where supported by the API response
- use stable placeholders only for unsupported metadata

Stop conditions:

- fake friendly names
- fake units
- hardware names as identity
- packet-derived identity in API

### `getCapabilityValue(...)`

Current behavior:

- returns `capability_not_found` while no capability exists

Future `CapabilityDirectory`-backed behavior while empty:

- preserve `capability_not_found`

Future behavior when records exist later:

- if a capability exists but no value owner exists, return no-value/unavailable according to the approved value contract
- do not infer values from `PublicCapabilityRecord` alone

Stop conditions:

- default zero presented as a real reading
- fake temperature value
- fake `CAP_TEMPERATURE`
- packet parsing
- direct provider reads

### `getCapabilityAvailability(...)`

Current behavior:

- returns `capability_not_found`

Future `CapabilityDirectory`-backed behavior while empty:

- failed read maps to `capability_not_found`

Future behavior when records exist later:

- map owner-backed lifecycle, visibility, value availability, and freshness fields
- keep unavailable fields explicitly unavailable or unknown

Stop conditions:

- marking a default record available
- triggering stale checks or provider health updates
- treating missing owner state as healthy

### `getCapabilityProviderInfo(...)`

Current behavior:

- returns `capability_not_found`

Future `CapabilityDirectory`-backed behavior while empty:

- failed read maps to `capability_not_found`

Future behavior when records exist later:

- provider metadata remains diagnostic/read-only
- if no provider owner exists, return unavailable/unknown

Stop conditions:

- provider selection
- direct provider reads
- exposing private provider indexes or pointers
- treating provider metadata as the Logic contract

### `getCapabilityQuality(...)`

Current behavior:

- returns `capability_not_found`

Future `CapabilityDirectory`-backed behavior while empty:

- failed read maps to `capability_not_found`

Future behavior when records exist later:

- quality remains unknown/unavailable until an approved diagnostics owner exists
- zero counters must not mean healthy

Stop conditions:

- fake diagnostics
- resetting counters during reads
- treating missing errors as healthy
- provider health refresh from API reads

## 6. `getCapabilityList(...)` Transition Plan

Future implementation should:

1. read `CapabilityDirectory::count()`
2. set `out_response.count = 0` before copying
3. if count is `0`, return current empty success
4. if records exist later, read by index through `CapabilityDirectory::readByIndex(...)`
5. copy only bounded summaries
6. never exceed `API_MAX_CAPABILITY_SUMMARY_COUNT`
7. leave unsupported fields unavailable/default

It must not:

- expose private data
- read Registry arrays
- parse packets
- infer capabilities from providers or diagnostics
- select providers
- fabricate summaries for UI convenience

## 7. Capability Detail Transition Plan

Future detail methods should:

1. validate `capability_index` against owner-backed reads
2. call `CapabilityDirectory::readByIndex(...)`
3. if read fails, return `capability_not_found` exactly as current methods do
4. if read succeeds later, map `PublicCapabilityRecord` fields into the matching `ApiCapability*` response
5. keep unavailable fields unavailable/placeholder

Detail methods must not:

- perform hardware reads
- perform provider selection
- read providers directly
- parse packets
- read Registry arrays
- infer values or diagnostics from default state

## 8. `getCapabilityValue(...)` Transition Plan

The first `CapabilityDirectory` API attachment must not add `CapabilityValueStore`.

While no capability exists:

- preserve `capability_not_found`

When a capability record exists later but no value owner exists:

- return a no-value/unavailable state according to the approved value contract
- use `PayloadValueType::NONE` where applicable
- keep numeric values deterministic defaults, but not real readings

The API must not:

- infer values from `PublicCapabilityRecord`
- return default zero as real
- fake `CAP_TEMPERATURE`
- fake sensor values
- read packets
- call providers directly

## 9. Availability, ProviderInfo, And Quality Transition Plan

The first attachment should not add `DiagnosticsStore`, provider health ownership, or value freshness ownership.

While no capability exists:

- `getCapabilityAvailability(...)` preserves `capability_not_found`
- `getCapabilityProviderInfo(...)` preserves `capability_not_found`
- `getCapabilityQuality(...)` preserves `capability_not_found`

When capability records exist later:

- availability may map owner-backed lifecycle, visibility, value availability, and freshness
- provider info remains unavailable unless a provider metadata owner exists
- quality remains unknown/unavailable unless a diagnostics owner exists

The API must not:

- trigger provider selection
- refresh provider health
- reset diagnostics counters
- treat zero counters as healthy
- treat missing errors as healthy

## 10. Initialization And Reset Plan

Future implementation should initialize/reset `CapabilityDirectory` explicitly during owner setup.

Rules:

- reset must produce deterministic empty state
- no `src/main.cpp` change unless explicitly approved
- no hidden app initialization
- no hidden transport initialization
- no capability creation during attachment
- no fake records during reset

For the first empty attachment, a provisional static owner may be default-constructed and remain empty. A later owner lifetime milestone should decide the production initialization path.

## 11. Test Strategy For Future Implementation

Future validation should prove:

- existing Capability API empty tests still pass
- `getCapabilityList(...)` returns count `0` through `CapabilityDirectory`
- detail methods still return `capability_not_found`
- `getCapabilityValue(...)` still returns `capability_not_found` while no capability exists
- `CapabilityDirectory` read failure maps to `capability_not_found`
- no fake capability summaries appear
- no fake `CAP_TEMPERATURE` appears
- no fake sensor values appear
- no fake diagnostics appear
- API reads do not mutate `CapabilityDirectory`
- repeated API reads are deterministic
- full validation suite passes
- `src/main.cpp` remains untouched

No tests are implemented in this milestone.

## 12. Minimal App Implications

No Minimal App change is required for the first API attachment.

Expected app-visible behavior after first attachment:

- capability list remains empty
- detail reads still fail with `capability_not_found`
- app remains mock-only
- no live transport exists
- no real capability cards appear
- no real sensor values appear

Future live app work must continue to use Cyber32 API only and must tolerate empty capability lists as a normal state.

## 13. Safety Invariants

Future attachment must preserve:

- no fake capabilities
- no fake `CAP_TEMPERATURE`
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
- no Runtime calls
- no provider selection
- no command execution
- no `src/main.cpp` changes
- no app transport
- no WebServer
- no HTTP
- no JSON
- no BLE
- no WiFi app transport

## 14. Open Questions

Open questions before production owner-backed capability data:

- final `CapabilityDirectory` owner location
- initialization owner
- whether `CapabilityDirectory` should be a singleton or passed dependency
- whether Cyber32Api owns public owner references
- whether Runtime should own public stores later
- how reset is triggered
- how future discovery bridge attaches records
- how future provider bridge attaches records
- how future node-to-capability mapping attaches records
- when capability details move from `capability_not_found` to unavailable/no-value for existing capabilities
- whether node-less capabilities are public later
- whether local Core capabilities are represented as Core-owned node capabilities
- how owner-backed capability state persists across reboot later

## 15. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.35 - CapabilityDirectory API Empty Attachment Implementation
```

Purpose:

Attach Cyber32Api Capability API reads to the empty `CapabilityDirectory` skeleton while preserving all current empty-state behavior and tests.

Alternative if more planning is needed:

```text
Milestone 10.35 - Public Owner Lifetime Plan
```

Purpose:

Decide the production lifetime and ownership model for public owner stores before any additional API attachment.

## 16. Stop Conditions

Stop future work and return to architecture review if it tries to:

- attach real capability data before an owner-backed creation path exists
- create fake capabilities
- create fake `CAP_TEMPERATURE`
- create fake sensor values
- create fake diagnostics
- parse packets
- read Registry arrays
- call HAL
- call Drivers
- call Devices
- call Modules
- call Services
- call Logic
- call Runtime
- trigger provider selection
- execute commands
- modify `src/main.cpp`
- add live transport
- add WebServer before transport approval
- add HTTP before transport approval
- add JSON before transport approval
- add BLE before transport approval
- add WiFi app transport before transport approval
