# Milestone 10.57 - NodeCapabilityMap Owner-Backed API Attachment Plan

## 1. Scope

Milestone 10.57 plans the future API attachment path for `getNodeCapabilities(...)` only.

This milestone is documentation only.

It does not:

* modify firmware source code
* modify `src/main.cpp`
* implement API attachment
* change `getNodeCapabilities(...)` behavior
* connect `NodeCapabilityMap` to `Cyber32Api` in code
* add mapping records
* add fake mappings
* add fake capability summaries
* infer capabilities from nodes
* infer nodes from capabilities
* add `lookupByNode`
* add `lookupByCapability`
* add `updateLink`
* add `removeLink`
* add discovery, provider, Registry, value, diagnostics, or transport bridges
* parse packets
* call HAL, Drivers, Devices, Modules, Services, Logic, or Runtime
* add provider selection
* add command execution
* add WebServer, HTTP, JSON, BLE, WiFi, or Minimal App transport
* expose private Registry arrays
* change existing API behavior

The goal is to plan how `Cyber32Api::getNodeCapabilities(...)` can eventually return capability summaries only when a public node exists, a public capability exists, and an explicit `NodeCapabilityMap` link exists between them.

## 2. Background

Current public owner state:

* `NodeDirectory` has owner-backed public nodes.
* `CapabilityDirectory` has owner-backed public capabilities.
* `NodeCapabilityMap` has owner-backed links.
* `PublicOwnerStore` owns all three.
* `Cyber32Api` currently reads `PublicOwnerStore` nodes and capabilities.
* `getNodeCapabilities(...)` currently remains unchanged.
* no API mapping attachment exists yet.

Current behavior remains correct: a node capability list must not appear until there is an approved owner-backed API path from explicit links to bounded capability summaries.

## 3. API Attachment Principle

Future `getNodeCapabilities(...)` must read explicit mapping links only.

It must not infer capabilities from:

* `PublicNodeRecord.capability_count`
* `PublicCapabilityRecord.owner_node_id`
* `CAP_TEMPERATURE`
* Registry state
* packets
* providers
* debug output
* sensor values
* diagnostics
* transport state

The API must move from empty to owner-backed, never from empty to fake.

## 4. Required Data Chain

Required future data chain:

```text
PublicOwnerStore
-> NodeDirectory
-> NodeCapabilityMap
-> CapabilityDirectory
-> ApiCapabilitySummary output
```

Required conditions:

* requested node ID or node index exists according to the approved Node API contract
* `NodeCapabilityMap` has one or more explicit links for that node
* each linked capability instance ID exists in `CapabilityDirectory`
* output contains only mapped capability summaries
* missing linked capabilities are handled deterministically

The chain must not read Registry, parse packets, inspect providers, or call hardware/service/runtime layers.

## 5. Missing Node Behavior

Planned behavior:

* if the requested node does not exist, `getNodeCapabilities(...)` returns `node_not_found`
* `out_count` must be initialized to `0` before any work
* no mapping scan should expose data for a missing node
* no fake empty node should be created

This preserves current Node API detail behavior.

## 6. Existing Node With No Links Behavior

Planned behavior:

* if the node exists but has no explicit links, `getNodeCapabilities(...)` should return success with `out_count = 0`
* no capability should be inferred from node fields
* no capability should be inferred from capability records
* no fake empty capability should be returned

A node with zero mapped capabilities is a valid owner-backed public state.

## 7. Existing Node With Valid Links Behavior

Planned behavior:

* if the node exists and links exist, return summaries for linked capabilities only
* output count equals the number of successfully mapped capability summaries up to output capacity
* ordering should follow `NodeCapabilityMap` storage order unless later defined otherwise
* no values
* no diagnostics
* no provider info
* no quality data
* summary only

The first mapping API attachment should expose relationship and capability summary data only. Values, diagnostics, provider health, and quality remain future store-backed contracts.

## 8. Broken Link Behavior

Broken link means a `NodeCapabilityMap` link references a capability instance that is not present in `CapabilityDirectory`.

Options:

A. Skip broken link and continue.

Benefits:

* no fake placeholder capability
* bounded read remains successful for valid links
* avoids turning one stale link into total API failure

Risks:

* broken link is invisible unless diagnostics are added later

B. Return error.

Benefits:

* exposes owner inconsistency immediately

Risks:

* prevents valid links from being returned
* adds new error behavior to a read path

C. Include unavailable placeholder.

Benefits:

* preserves link count shape

Risks:

* risks fake capability summaries
* can make missing data look real

Recommended first behavior:

* skip broken links during read-only summary mapping
* do not create placeholder capabilities
* do not return fake data
* optionally document a future diagnostics/audit counter separately

Production should not create broken links once validated owner-level add-link behavior exists, but the API must remain safe if one appears.

## 9. Output Capacity Behavior

Planned behavior:

* initialize `out_count = 0` before work
* fill output array only up to caller capacity
* never write out of bounds
* if more links exist than output capacity, truncate deterministically
* preserve existing API error behavior according to current conventions
* if current API has no partial/truncated status, document that limitation

The first implementation should prefer bounded partial output over unsafe writes or dynamic allocation.

## 10. Needed Map Read Operation

Current `NodeCapabilityMap` operations:

* `readByIndex(...)`
* `count()`
* `capacity()`
* `isEmpty()`
* `addLink(...)`

These are enough for a first read-only API attachment.

Possible first implementation:

* scan all links using `count()` and `readByIndex(...)`
* filter links by node ID
* copy summaries only for linked capabilities found in `CapabilityDirectory`

Deferred optimized operations:

* `lookupByNode`
* `countByNode`
* iterator by node
* `lookupByCapability`

Plan:

* first API attachment may use a bounded scan over `NodeCapabilityMap`
* no `lookupByNode` required yet
* performance is acceptable for small fixed capacity
* `lookupByNode` can be planned later if needed

## 11. Needed Capability Lookup Operation

Current likely `CapabilityDirectory` operations:

* `readByIndex(...)`
* `count()`
* `capacity()`

Plan:

* first implementation may scan `CapabilityDirectory` to find a matching capability instance ID
* no new lookup method is required unless implementation discovers it is necessary
* future `lookupByCapabilityInstanceId` may be planned later

This preserves a small API attachment surface and avoids adding lookup behavior before it is needed.

## 12. API Response Mapping

Future mapping from `PublicCapabilityRecord` to `ApiCapabilitySummary` should follow approved summary fields only.

Rules:

* map only fields already approved for summary
* do not read value store
* do not read diagnostics
* do not read provider info
* do not read Registry
* do not call HAL, Drivers, Devices, Services, Logic, or Runtime
* do not infer availability from missing value
* do not infer quality
* do not fabricate names, units, values, timestamps, provider details, or status

The first owner-backed summary may leave value, availability, provider, and quality substructures in deterministic default/unavailable form unless separate owner-backed stores exist.

## 13. Validation Plan For Implementation Milestone

Future implementation validation should prove:

* default production API behavior still empty or `node_not_found` as today
* missing node returns `node_not_found` and `out_count = 0`
* seeded node with no links returns success and `out_count = 0`
* seeded node plus seeded capability but no link returns success and `out_count = 0`
* seeded node plus seeded capability plus explicit link returns success and `out_count = 1`
* returned summary matches linked capability record
* link to missing capability does not create fake summary
* unrelated node link is not returned
* duplicate links should not be possible if `addLink` rejects them
* output capacity limit is respected
* no fake mappings
* no inferred mappings
* no fake capabilities
* no fake `CAP_TEMPERATURE`
* no fake sensor values
* no fake diagnostics
* `src/main.cpp` untouched
* no transport added
* existing Node API default tests still pass
* existing Capability API default tests still pass
* existing public owner validation tests still pass

Validation should use fixed owner-backed public records only and must not parse packets, read Registry, or call providers.

## 14. Production Default Behavior

Production default behavior remains:

* no mapping appears automatically
* no node gains capabilities automatically
* no capability attaches to node automatically
* no setup/loop integration
* no discovery/provider/Registry seeding
* no transport seeding
* Minimal App still sees no live mapped data until a real source and transport are approved

An API attachment milestone may expose validation-seeded owner-backed mappings during tests, but it must not create production mappings.

## 15. Minimal App Implications

Minimal App implications:

* no app change required in this plan
* app remains mock-only
* future API mapping will make a local/mock snapshot more realistic
* app must still use `Cyber32Api` only
* app must not infer node capabilities from separate node/capability lists
* live app transport remains a separate future plan

Until a transport and real source are approved, mapped API behavior remains firmware validation behavior, not live Minimal App data.

## 16. Safety Invariants

Safety invariants:

* no fake mappings
* no inferred mappings
* no fake nodes
* no fake capabilities
* no fake `CAP_TEMPERATURE`
* no fake sensor values
* no fake diagnostics
* no Registry access
* no packet parsing
* no HAL calls
* no Driver calls
* no Device calls
* no Module calls
* no Service calls
* no Logic calls
* no Runtime calls
* no provider selection
* no command execution
* no `src/main.cpp` changes
* no app transport
* no WebServer, HTTP, JSON, BLE, or WiFi

These invariants apply to the future API attachment and every later mapping/value/diagnostics bridge.

## 17. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.58 - getNodeCapabilities Owner-Backed Mapping API Implementation
```

Purpose:

Implement read-only `getNodeCapabilities(...)` mapping through `PublicOwnerStore::nodeCapabilities()` and `CapabilityDirectory` summaries, without fake data, values, diagnostics, Registry, transport, or provider behavior.

Alternative:

```text
Milestone 10.58 - CapabilityDirectory Lookup By Instance Plan
```

## 18. Stop Conditions

Stop future work if it tries to:

* expose capabilities without explicit mapping links
* infer capabilities from node records
* infer mappings from capability `owner_node_id`
* infer mapping from `CAP_TEMPERATURE`
* create fake capability summaries
* create fake sensor values
* parse packets
* read Registry arrays
* call HAL, Drivers, Devices, Services, Logic, or Runtime
* add provider bridge before approval
* add discovery bridge before approval
* add transport before approval
* modify `src/main.cpp`
* expose mutable mapping access to UI/App/Transport
* add values/diagnostics without approved stores
