# Milestone 10.60 - API Test Owner Injection Seam Plan

## 1. Scope

Milestone 10.60 plans a validation-only API owner injection seam.

This milestone is documentation only.

It does not:

* modify firmware source code
* modify `src/main.cpp`
* implement an owner injection seam
* change `Cyber32Api` implementation
* change `PublicOwnerStore` implementation
* change `NodeCapabilityMap` implementation
* add production mapping records
* add fake mappings
* add fake capability summaries
* add fake nodes
* add fake capabilities
* infer capabilities from nodes
* infer mappings from node or capability fields
* add discovery, provider, Registry, value, diagnostics, or transport bridges
* parse packets
* call HAL, Drivers, Devices, Modules, Services, Logic, or Runtime
* add provider selection
* add command execution
* add WebServer, HTTP, JSON, BLE, WiFi, or Minimal App transport
* expose private Registry arrays
* change existing API behavior

The purpose is to plan how `Cyber32Api` can be tested with seeded owner-backed data safely, without exposing mutable production state.

## 2. Background

Current state:

* `Cyber32Api` now reads owner-backed `PublicOwnerStore` data.
* `getNodeCapabilities(...)` can read nodes, mappings, and capabilities.
* default API behavior remains safe.
* positive seeded mapping validation was deferred.
* `Cyber32Api` currently uses a private static `PublicOwnerStore`.
* unsafe mutation of the production static API store must be avoided.

The current private static owner is acceptable for empty/default reads, but it is not a safe place to seed validation records directly.

## 3. Problem Statement

Positive API validation needs controlled seeded data.

Seeded data must include:

* one owner-backed node
* one owner-backed capability
* one explicit node-capability link

Validation must not:

* mutate production runtime state
* expose mutable owner access to UI/App/Transport
* require `src/main.cpp`
* add transport
* create fake production records
* parse packets
* read Registry internals
* call hardware, services, logic, or runtime

The project needs a safe validation seam before positive-path `getNodeCapabilities(...)` mapping can be validated.

## 4. Seam Principle

The seam must be:

* validation-only or internal-only
* explicit
* bounded
* deterministic
* resettable
* not available to UI/App/Transport
* not a transport API
* not a production data source
* not a fake data source
* not a Registry/provider bridge

The seam must allow tests to provide a local owner-backed store without turning mutable owner state into a public API.

## 5. Possible Design Options

### Option A: Constructor Reference Only

Concept:

```cpp
Cyber32Api(PublicOwnerStore& owner_store);
```

Benefits:

* explicit dependency
* caller owns lifetime
* no global mutable test state
* easy to construct validation-local seeded stores

Risks:

* changes the API constructor surface
* must be clearly documented as internal/validation support

### Option B: Default Constructor Plus Optional Explicit Owner

Concept:

```cpp
Cyber32Api();
Cyber32Api(PublicOwnerStore& owner_store);
```

Behavior:

* default constructor uses the existing private production static owner
* explicit constructor uses caller-owned validation/internal owner store

Benefits:

* production default remains unchanged
* validation can seed local owner-backed data
* no global mutable public accessor required
* no heap allocation required

Risks:

* API must store a pointer/reference safely
* injected owner lifetime must be documented and validated

### Option C: Internal-Only Temporary Test Binding

Concept:

```cpp
bool bindPublicOwnerStoreForValidation(PublicOwnerStore* owner_store);
void clearPublicOwnerStoreForValidation();
```

Benefits:

* can be hidden from normal UI/App/Transport paths
* explicit bind/clear lifecycle

Risks:

* risks hidden global state
* tests must clear state carefully
* easier to misuse than a constructor-owned dependency

### Option D: Expose Production Static Mutable Accessor

Concept:

```cpp
PublicOwnerStore& mutableApiPublicOwnerStore();
```

This option is rejected.

Reasons:

* exposes production static mutable state
* encourages unsafe validation mutation
* creates an accidental app/transport backdoor
* weakens the API-first boundary
* makes production/test state isolation harder

## 6. Recommended Seam Design

Recommended design:

```cpp
Cyber32Api();
Cyber32Api(PublicOwnerStore& owner_store);
```

Conceptual behavior:

```text
Cyber32Api()
-> uses private production static owner store

Cyber32Api(PublicOwnerStore& owner_store)
-> uses caller-owned validation/internal owner store
```

Rules:

* production default constructor remains unchanged
* production static owner store remains private
* validation owner store is local and resettable
* test constructor must not become an app/transport contract
* no global mutable public accessor
* no UI/App/Transport access
* no heap allocation required
* no STL required
* no Arduino `String` required

This preserves production behavior while enabling safe positive-path validation.

## 7. Lifetime Rules

Planned lifetime rules:

* production static store lifetime remains internal
* validation store lifetime is caller-owned
* `Cyber32Api` must not outlive an injected validation store
* no dynamic allocation
* no ownership transfer
* no global test state if avoidable
* reset is explicit through the local owner store

Validation code must construct the local owner store, seed it, construct or bind the API to it, run the test, and let the local store fall out of scope.

## 8. Access Boundary

Planned access boundary:

* `Cyber32Api` reads from owner store through read-only accessors
* validation code seeds data through local `PublicOwnerStore` mutable accessors before constructing or invoking API
* `Cyber32Api` itself must not seed data
* `Cyber32Api` must not expose mutable owner store
* UI/App/Transport must not get access to injected owner store

Allowed validation seeding:

```text
local_store.mutableNodes().addNode(...)
local_store.mutableCapabilities().addCapability(...)
local_store.mutableNodeCapabilities().addLink(...)
```

Forbidden API behavior:

```text
Cyber32Api -> mutableNodes()
Cyber32Api -> mutableCapabilities()
Cyber32Api -> mutableNodeCapabilities()
```

## 9. Validation Use Case

Target validation:

1. Create local `PublicOwnerStore`.
2. Seed one valid `PublicNodeRecord` through `mutableNodes().addNode(...)`.
3. Seed one valid `PublicCapabilityRecord` through `mutableCapabilities().addCapability(...)`.
4. Seed one valid `PublicNodeCapabilityLink` through `mutableNodeCapabilities().addLink(...)`.
5. Create `Cyber32Api` using the validation owner store seam.
6. Call `getNodeCapabilities(node_index, out_array, max_count, out_count)`.

Expected result:

* success
* `out_count = 1`
* returned summary matches seeded capability summary fields that can be represented safely

Also validate:

* node with no links returns success with count `0`
* node plus capability without link returns success with count `0`
* broken link skips missing capability
* unrelated node link is not returned
* missing node returns `node_not_found` / failure with count `0`
* no fake data

## 10. Production Default Behavior

Production default behavior remains unchanged:

* existing production API construction remains unchanged
* production static owner store remains empty by default
* no production auto-seed
* no setup/loop integration
* no transport integration
* no discovery/provider/Registry integration
* no Minimal App live change

The seam validates API behavior with a local owner-backed store only.

## 11. Minimal App Implications

Minimal App implications:

* no app change required
* app still uses the normal `Cyber32Api` path later
* app must not access the injection seam
* injection seam is validation/internal only
* live app transport remains separate future work

This milestone does not create a live app connection or live app data source.

## 12. Safety Invariants

Safety invariants:

* no fake mappings
* no fake nodes
* no fake capabilities
* no fake `CAP_TEMPERATURE`
* no fake sensor values
* no fake diagnostics
* no fake capability summaries
* no private Registry arrays exposed
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

## 13. Validation Plan For Implementation Milestone

Future implementation validation should prove:

* production default `Cyber32Api` still passes existing default tests
* injected local owner store starts empty
* seeded node with no links returns success with count `0`
* seeded node plus capability without link returns success with count `0`
* seeded node plus capability plus link returns success with count `1`
* missing node returns `node_not_found` / failure with count `0`
* broken link skips missing capability
* unrelated node link is not returned
* output capacity is respected
* local owner store reset clears seeded records
* production default API state is unaffected after injected tests
* no fake data
* `src/main.cpp` untouched
* no transport added

The validation should use local fixed objects only.

## 14. Compatibility Statement

This seam is for validation only and must not change production default API behavior.

The data transition remains:

```text
empty -> owner-backed empty -> owner-backed validation-seeded -> future owner-backed real
```

Never:

```text
empty -> fake
```

## 15. Known Limitations

Known limitations:

* not live app transport
* not cloud/local connection
* not discovery bridge
* not provider bridge
* not Registry bridge
* not value store
* not diagnostics store
* not production data creation
* not external app API

The seam exists only to make owner-backed API validation safe.

## 16. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.61 - API Test Owner Injection Seam Implementation
```

Purpose:

Implement the safe validation-only owner-store injection seam for `Cyber32Api` and positive `getNodeCapabilities(...)` mapping validation without changing production default behavior.

Alternative:

```text
Milestone 10.61 - getNodeCapabilities Positive Mapping Validation Harness Implementation
```

## 17. Stop Conditions

Stop future work if it tries to:

* expose mutable production owner store publicly
* expose injection seam to UI/App/Transport
* create fake production data
* mutate production static state from validation unsafely
* add transport
* parse packets
* read Registry arrays
* call HAL, Drivers, Devices, Services, Logic, or Runtime
* add provider/discovery bridge before approval
* modify `src/main.cpp`
* add values/diagnostics without approved stores
