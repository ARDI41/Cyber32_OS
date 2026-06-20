# Milestone 9.0 Wireless Security Diagnostics API Plan

## Goal

Expose wireless node security diagnostics through read-only Cyber32 API methods.

## Problem

Registry now stores `WirelessNodeSecurityDiagnosticRecord` entries, and WirelessService updates accepted and rejected packet counters.

The internal API does not yet expose these diagnostics to operators or future dashboards.

Milestone 9.0 defines read-only API visibility without changing normal capability reads.

## API Structs

### `ApiWirelessSecurityDiagnostic`

Required fields:

```cpp
struct ApiWirelessSecurityDiagnostic
{
    bool ok;
    RegistryResult registry_result;
    uint32_t node_id;
    uint8_t mac_address[WIRELESS_MAC_ADDRESS_SIZE];
    bool has_mac_address;
    WirelessNodeAllowState allow_state;
    WirelessTrustState trust_state;
    uint32_t last_seen_ms;
    uint32_t last_accepted_sequence_id;
    uint32_t last_rejected_sequence_id;
    const char* last_error_code;
    uint16_t checksum_reject_count;
    uint16_t mac_not_allowed_reject_count;
    uint16_t mac_node_mismatch_reject_count;
    uint16_t blocked_reject_count;
    uint16_t not_allowed_reject_count;
    uint16_t untrusted_reject_count;
    uint16_t duplicate_sequence_reject_count;
    uint16_t invalid_packet_reject_count;
    uint16_t accepted_packet_count;
    const char* error_code;
};
```

### `ApiWirelessSecuritySummary`

Required fields:

```cpp
struct ApiWirelessSecuritySummary
{
    bool ok;
    RegistryResult registry_result;
    uint8_t node_count;
    uint32_t total_accepted_packets;
    uint32_t total_rejected_packets;
    uint32_t total_checksum_rejects;
    uint32_t total_mac_rejects;
    uint32_t total_duplicate_rejects;
    const char* error_code;
};
```

Summary totals use wider counters so bounded per-node `uint16_t` counts can be summed safely across the fixed Registry table.

## API Methods

### `getWirelessSecurityDiagnostic(...)`

Signature:

```cpp
bool getWirelessSecurityDiagnostic(
    uint32_t node_id,
    ApiWirelessSecurityDiagnostic& out_response);
```

Behavior:

- `node_id == 0`
  - `ok = false`
  - `registry_result = RegistryResult::INVALID_ID`
  - `error_code = ERR_COMMAND_INVALID` or existing API invalid-ID convention
  - return false

- Registry missing
  - `ok = false`
  - `registry_result = RegistryResult::NOT_ATTACHED`
  - return false

- diagnostic record missing
  - `ok = false`
  - `registry_result = RegistryResult::NOT_FOUND`
  - return false

- success
  - copy all fields from `WirelessNodeSecurityDiagnosticRecord`
  - `ok = true`
  - `registry_result = RegistryResult::OK`
  - `error_code = "none"`
  - return true

### `getWirelessSecurityDiagnosticByIndex(...)`

Signature:

```cpp
bool getWirelessSecurityDiagnosticByIndex(
    uint8_t index,
    ApiWirelessSecurityDiagnostic& out_response);
```

Behavior:

- Registry missing
  - `ok = false`
  - `registry_result = RegistryResult::NOT_ATTACHED`
  - return false

- out-of-range index
  - `ok = false`
  - `registry_result = RegistryResult::NOT_FOUND`
  - return false

- success
  - copy diagnostic record fields
  - `ok = true`
  - `registry_result = RegistryResult::OK`
  - `error_code = "none"`
  - return true

### `getWirelessSecuritySummary(...)`

Signature:

```cpp
bool getWirelessSecuritySummary(
    ApiWirelessSecuritySummary& out_response);
```

Behavior:

- Registry missing
  - `ok = false`
  - `registry_result = RegistryResult::NOT_ATTACHED`
  - return false

- success
  - `node_count = registry_->wirelessNodeSecurityDiagnosticCount()`
  - scan records by index using Registry public APIs
  - sum accepted and rejected counters
  - `ok = true`
  - `registry_result = RegistryResult::OK`
  - `error_code = "none"`
  - return true

## Summary Totals

### `node_count`

Number of registered wireless security diagnostic records.

### `total_accepted_packets`

Sum of all per-node `accepted_packet_count`.

### `total_rejected_packets`

Sum of all rejection counters:

- `checksum_reject_count`
- `mac_not_allowed_reject_count`
- `mac_node_mismatch_reject_count`
- `blocked_reject_count`
- `not_allowed_reject_count`
- `untrusted_reject_count`
- `duplicate_sequence_reject_count`
- `invalid_packet_reject_count`

### `total_checksum_rejects`

Sum of all `checksum_reject_count`.

### `total_mac_rejects`

Sum of:

- `mac_not_allowed_reject_count`
- `mac_node_mismatch_reject_count`

### `total_duplicate_rejects`

Sum of all `duplicate_sequence_reject_count`.

## Ownership Rules

### API

API may:

- read Registry diagnostics by node ID
- read Registry diagnostics by index
- summarize diagnostics by bounded Registry scan

API must not:

- reset counters
- mutate diagnostics
- call WirelessService
- expose raw packets
- select providers
- update provider payloads
- update canonical payloads

### Registry

Registry remains diagnostic state owner.

Registry provides public read APIs only for this API layer.

### WirelessService

WirelessService remains policy owner and diagnostic update owner.

API must not call WirelessService to derive diagnostics.

### Logic

Logic remains diagnostics-blind.

Normal capability logic must continue to use `CAP_*` IDs only.

## Validation Requirements

### Diagnostic By Node ID

Setup:

- register diagnostic record for node `1001`
- set known counters and MAC state

Expected:

- `getWirelessSecurityDiagnostic(1001, out)` returns true
- `out.ok == true`
- `out.registry_result == RegistryResult::OK`
- copied fields match Registry record

### Diagnostic By Index

Setup:

- at least one diagnostic record exists

Expected:

- `getWirelessSecurityDiagnosticByIndex(0, out)` returns true
- `out.node_id` matches first record

### Missing Node

Expected:

- `getWirelessSecurityDiagnostic(9999, out)` returns false
- `out.registry_result == RegistryResult::NOT_FOUND`

### Invalid Node

Expected:

- `getWirelessSecurityDiagnostic(0, out)` returns false
- `out.registry_result == RegistryResult::INVALID_ID`

### Out-Of-Range Index

Expected:

- `getWirelessSecurityDiagnosticByIndex(99, out)` returns false
- `out.registry_result == RegistryResult::NOT_FOUND`

### Summary Totals

Setup:

- register one or more diagnostic records with known counter values

Expected:

- `node_count` matches `registry_->wirelessNodeSecurityDiagnosticCount()`
- accepted total is correct
- checksum total is correct
- MAC total includes MAC-not-allowed plus MAC/node mismatch
- duplicate total is correct
- total rejected packets equals the sum of all reject counters

### Read Does Not Mutate

Expected:

- repeated diagnostic reads return the same counter values
- provider payload remains unchanged
- active provider remains unchanged
- canonical payload remains unchanged

## Stop Conditions

Stop implementation if any of the following are required:

- API mutates diagnostics
- API resets counters
- API calls WirelessService
- API exposes raw packets
- API changes normal capability read contracts
- Logic sees diagnostics
- Logic sees MAC address
- dynamic allocation is introduced
- Arduino `String` is introduced
- STL containers are introduced
- `src/main.cpp` must change

## Final Assessment

Milestone 9.0 should make wireless security observable without making diagnostics part of control logic.

PASS: Registry already owns bounded diagnostic records and WirelessService already updates them through public Registry APIs.

WARNING: These diagnostics are runtime state only unless a future persistence milestone explicitly stores them.
