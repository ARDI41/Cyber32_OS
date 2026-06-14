# Cyber32 Registry Result Codes Plan

## Goal

Replace bool-based Registry registration, lookup, and update responses with compact result codes.

This document is planning only.

No source code is modified by this plan.

## Scope

Registry APIs reviewed:

```text
registerModule
registerDevice
registerCapability
updateCapabilityPayload
getCapabilityPayload
```

Current behavior:

```text
true  = operation succeeded
false = operation failed
```

Problem:

```text
false does not explain why the operation failed.
```

Milestone 2 now has two capabilities:

```text
CAP_TEMPERATURE
CAP_DISTANCE
```

As the Registry grows, callers need compact failure reasons without dynamic allocation, exceptions, strings, or large diagnostic objects.

## Design Principles

Registry result codes must be:

- compact
- stable
- machine-readable
- ESP32-friendly
- allocation-free
- usable from PNP, Services, Logic, API, and validation

Registry result codes must not:

- contain large messages
- allocate memory
- use Arduino `String`
- use STL containers
- expose private Registry table internals
- replace canonical `ERR_*` error codes at API boundaries

Registry result codes describe operation results.

Canonical `ERR_*` codes describe system errors exposed to API, Events, payloads, and Dashboard translations.

## Proposed Result Code Enum

File location:

```text
src/registry/registry_result.h
```

Proposed enum:

```cpp
namespace Cyber32 {

enum class RegistryResult : unsigned char {
    OK,
    NOT_ATTACHED,
    INVALID_RECORD,
    INVALID_ID,
    UNSUPPORTED_CAPABILITY,
    DUPLICATE_ID,
    TABLE_FULL,
    NOT_FOUND,
    UNAVAILABLE,
    STALE,
    TYPE_MISMATCH,
    INTERNAL_ERROR
};

}  // namespace Cyber32
```

## Result Code Meanings

### OK

Operation succeeded.

Applies to:

- registration
- lookup
- update

### NOT_ATTACHED

The caller depends on a Registry pointer but no Registry is attached.

This is mainly useful for wrappers such as PNP Registration, Services, API, and validation.

Registry member methods usually should not return this for `this`.

### INVALID_RECORD

The submitted record is structurally invalid.

Examples:

- missing required ID
- null required field
- invalid count
- invalid owner index

### INVALID_ID

The submitted ID is null, empty, malformed, or violates naming rules.

Examples:

- capability ID does not start with `CAP_`
- error ID does not start with `ERR_`
- event ID does not start with `EVT_`

### UNSUPPORTED_CAPABILITY

The capability ID is syntactically valid but unsupported by the current implementation phase.

Example:

```text
CAP_PRESSURE submitted before CAP_PRESSURE exists
```

### DUPLICATE_ID

A record with the same unique ID already exists.

Applies to:

- module ID
- device ID
- capability ID under current single-provider policy

### TABLE_FULL

The fixed-size Registry table has no available slot.

Maps to:

```text
ERR_REGISTRY_FULL
```

### NOT_FOUND

Requested ID was not found.

Examples:

- `getCapabilityPayload(CAP_DISTANCE, ...)` before registration
- `updateCapabilityPayload(...)` for an unregistered capability

### UNAVAILABLE

The record exists but is currently unavailable.

This should be used only when the Registry API intentionally distinguishes missing records from unavailable records.

### STALE

The record exists, but its latest value is stale.

This is not necessarily a failure for all callers.

### TYPE_MISMATCH

The submitted payload type does not match the capability record type.

Example:

```text
CAP_DISTANCE expects FLOAT but update uses INTEGER
```

### INTERNAL_ERROR

Unexpected Registry invariant failure.

This should be rare and should map to a compact canonical error.

## API-Specific Result Mapping

Registry result codes should map to existing or future canonical error codes.

| RegistryResult | Suggested Error Code |
|---|---|
| `OK` | `none` |
| `NOT_ATTACHED` | `ERR_REGISTRY_UNAVAILABLE` or `ERR_CONFIG_INVALID` |
| `INVALID_RECORD` | `ERR_CONFIG_INVALID` |
| `INVALID_ID` | `ERR_CONFIG_INVALID` |
| `UNSUPPORTED_CAPABILITY` | `ERR_CAPABILITY_UNAVAILABLE` |
| `DUPLICATE_ID` | `ERR_REGISTRY_DUPLICATE_ID` |
| `TABLE_FULL` | `ERR_REGISTRY_FULL` |
| `NOT_FOUND` | `ERR_CAPABILITY_UNAVAILABLE` |
| `UNAVAILABLE` | `ERR_CAPABILITY_UNAVAILABLE` |
| `STALE` | `none` or capability-specific stale status |
| `TYPE_MISMATCH` | `ERR_CONFIG_INVALID` |
| `INTERNAL_ERROR` | `ERR_REGISTRY_INTERNAL` |

Some suggested error codes do not exist yet.

Near-term implementation may keep mapping unknown cases to:

```text
ERR_CAPABILITY_UNAVAILABLE
ERR_REGISTRY_FULL
ERR_CONFIG_INVALID
```

after those error IDs are added.

## Memory Impact

### Enum Size

Using:

```cpp
enum class RegistryResult : unsigned char
```

Expected size:

```text
1 byte
```

### Function Return Impact

Replacing `bool` with `RegistryResult` has minimal memory impact.

Current:

```text
bool return value, usually 1 byte
```

Future:

```text
RegistryResult return value, 1 byte
```

### Optional Index Return Impact

Future registration APIs should return assigned indexes.

Preferred compact result object:

```cpp
struct RegistryWriteResult {
    RegistryResult result;
    uint8_t index;
};
```

Expected size:

```text
2 bytes before alignment
```

If alignment expands this to 4 bytes, it is still acceptable for stack-returned operation results.

### No Heap Impact

Result codes require:

- no heap allocation
- no dynamic strings
- no STL containers
- no logging buffers

## Target API Shape

### Registration With Result Only

Minimal migration:

```cpp
RegistryResult registerModuleResult(const ModuleRecord& record);
RegistryResult registerDeviceResult(const DeviceRecord& record);
RegistryResult registerCapabilityResult(const CapabilityRecord& record);
```

### Registration With Index

Preferred long-term shape:

```cpp
RegistryWriteResult registerModuleResult(const ModuleRecord& record);
RegistryWriteResult registerDeviceResult(const DeviceRecord& record);
RegistryWriteResult registerCapabilityResult(const CapabilityRecord& record);
```

Where:

```text
result = OK
index  = assigned slot index
```

If failure:

```text
result = failure code
index  = Registry::NOT_FOUND or 255
```

This directly fixes current count-derived owner index coupling in PNP Registration.

### Payload Update

Recommended:

```cpp
RegistryResult updateCapabilityPayloadResult(
    const char* capability_id,
    const CapabilityPayload& payload
);
```

Checks should include:

- capability ID is valid
- capability exists
- payload capability ID matches requested ID
- payload type matches capability record type where applicable
- update stored successfully

### Payload Lookup

Recommended:

```cpp
RegistryResult getCapabilityPayloadResult(
    const char* capability_id,
    CapabilityPayload& out_payload
) const;
```

Behavior:

- `OK` if payload returned
- `NOT_FOUND` if capability is not registered
- `INVALID_ID` if ID is null or invalid
- `UNAVAILABLE` only if the Registry intentionally treats unavailable capability as lookup failure

For current API state endpoints, a registered-but-unavailable payload should usually return:

```text
RegistryResult::OK
```

because the unavailable payload is valid state.

## Backward Compatibility Strategy

Do not remove bool APIs immediately.

Keep existing API names during migration:

```cpp
bool registerModule(const ModuleRecord& record);
bool registerDevice(const DeviceRecord& record);
bool registerCapability(const CapabilityRecord& record);
bool updateCapabilityPayload(const char* capability_id, const CapabilityPayload& payload);
bool getCapabilityPayload(const char* capability_id, CapabilityPayload& out_payload) const;
```

Add new result-based APIs alongside them:

```cpp
RegistryWriteResult registerModuleWithResult(const ModuleRecord& record);
RegistryWriteResult registerDeviceWithResult(const DeviceRecord& record);
RegistryWriteResult registerCapabilityWithResult(const CapabilityRecord& record);
RegistryResult updateCapabilityPayloadWithResult(...);
RegistryResult getCapabilityPayloadWithResult(...);
```

Then implement bool wrappers in terms of result APIs:

```cpp
return registerModuleWithResult(record).result == RegistryResult::OK;
```

This preserves existing callers while allowing new code to use better diagnostics.

## Migration Plan

### Phase R1 - Define Result Types

Add:

```text
src/registry/registry_result.h
```

Define:

- `RegistryResult`
- optional `RegistryWriteResult`

Rules:

- no dynamic allocation
- namespace `Cyber32`
- compact enum storage

### Phase R2 - Add Result-Based Registry APIs

Add methods without removing bool methods:

```text
registerModuleWithResult
registerDeviceWithResult
registerCapabilityWithResult
updateCapabilityPayloadWithResult
getCapabilityPayloadWithResult
```

Keep existing bool methods intact.

### Phase R3 - Refactor Registry Internals

Move validation logic into result-returning methods.

Bool methods become wrappers.

Expected behavior:

- duplicate returns `DUPLICATE_ID`
- full table returns `TABLE_FULL`
- missing lookup returns `NOT_FOUND`
- invalid record returns `INVALID_RECORD`

### Phase R4 - Update PNP Registration

PNP Registration should stop using count-derived indexes:

Current fragile pattern:

```text
registered_device_index = registry_->deviceCount() - 1
```

Future pattern:

```text
device_result = registry_->registerDeviceWithResult(device_record)
owner_device_index = device_result.index
```

If any registration step fails:

```text
return failure result
```

Rollback or partial-record policy remains a separate decision.

### Phase R5 - Update Services

Services should use:

```text
updateCapabilityPayloadWithResult
```

This lets Services distinguish:

- Device read failed but Registry update succeeded
- Registry capability missing
- Registry type mismatch
- Registry full/internal failure

Services still return bool where existing callers expect bool, until Service result codes are introduced.

### Phase R6 - Update API

API state methods should use:

```text
getCapabilityPayloadWithResult
```

API should map Registry result codes to canonical error codes.

Example:

```text
RegistryResult::NOT_FOUND -> ERR_CAPABILITY_UNAVAILABLE
RegistryResult::INVALID_ID -> ERR_CONFIG_INVALID
```

### Phase R7 - Update Validation Harness

Validation should assert specific result codes for:

- duplicate registration
- missing capability lookup
- payload update success
- invalid capability update

Do not expand runtime features during this step.

### Phase R8 - Deprecate Bool APIs

After all internal callers migrate:

- keep bool wrappers for compatibility
- mark them as legacy in documentation
- avoid using them in new code

Do not remove them until the v1 API stabilizes.

## Method-Specific Plan

### registerModule

Current likely failures:

- table full
- duplicate module ID
- invalid record

Future result codes:

```text
OK
INVALID_RECORD
INVALID_ID
DUPLICATE_ID
TABLE_FULL
```

Preferred return:

```text
RegistryWriteResult
```

### registerDevice

Current likely failures:

- table full
- duplicate device ID
- invalid module index
- invalid record

Future result codes:

```text
OK
INVALID_RECORD
INVALID_ID
DUPLICATE_ID
TABLE_FULL
NOT_FOUND
```

`NOT_FOUND` may indicate referenced module index does not exist once owner validation is implemented.

Preferred return:

```text
RegistryWriteResult
```

### registerCapability

Current likely failures:

- table full
- duplicate capability ID
- invalid owner device index
- invalid capability ID
- unsupported capability

Future result codes:

```text
OK
INVALID_RECORD
INVALID_ID
UNSUPPORTED_CAPABILITY
DUPLICATE_ID
TABLE_FULL
NOT_FOUND
TYPE_MISMATCH
```

Preferred return:

```text
RegistryWriteResult
```

### updateCapabilityPayload

Current likely failures:

- capability ID not found
- invalid payload
- mismatched payload capability ID
- mismatched payload type

Future result codes:

```text
OK
INVALID_ID
NOT_FOUND
INVALID_RECORD
TYPE_MISMATCH
```

Should remain state-only.

Must not call Devices, Services, Logic, API, or Drivers.

### getCapabilityPayload

Current likely failures:

- capability ID not found
- invalid ID

Future result codes:

```text
OK
INVALID_ID
NOT_FOUND
```

If capability exists but is unavailable, returning `OK` with an unavailable payload is preferred.

Unavailable is valid state.

## Architecture Rules

Registry result code migration must preserve:

- Registry stores facts/state only.
- Registry does not discover hardware.
- Registry does not call Drivers.
- Registry does not call Devices.
- Registry does not run Logic.
- Registry does not expose API.
- Registry does not allocate memory dynamically.
- Registry does not store large diagnostic strings.

Result codes improve contracts. They must not change layer ownership.

## Validation Checklist

Result code migration is successful when:

1. Existing bool callers still work.
2. New result APIs return compact codes.
3. Registration returns assigned indexes.
4. PNP Registration no longer uses count-derived indexes.
5. Duplicate records return `DUPLICATE_ID`.
6. Full tables return `TABLE_FULL`.
7. Missing capability lookups return `NOT_FOUND`.
8. Valid unavailable payloads return `OK`.
9. API maps results to canonical error codes.
10. No dynamic allocation is introduced.

## Stop Conditions

Stop and review architecture if:

- result codes become strings
- result objects allocate memory
- Registry starts logging large messages
- Registry starts owning API error formatting
- Registry calls higher layers
- PNP writes Registry tables directly
- bool APIs are removed before callers migrate
- result migration adds new capabilities or features

## Recommended Next Step

Implement result code support in a narrow phase:

```text
Milestone 2.1 Phase 2:
Add registry_result.h with RegistryResult and RegistryWriteResult only.
```

Then migrate Registry methods in later phases without changing external behavior.
