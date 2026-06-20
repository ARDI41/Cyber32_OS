# Milestone 8.8 MAC-to-Node Enforcement Plan

## Goal

Define how `WirelessService` verifies ESP-NOW source MAC identity against packet `node_id` before allowing provider payload updates.

## Problem

ESP-NOW source MAC is now preserved through the transport adapter path.

Current allowlist enforcement still uses only:

- `WirelessPacketHeader::node_id`

Real ESP-NOW integration needs one more safety gate:

- source MAC is the transport identity
- packet `node_id` is the claimed packet identity
- both must agree when source MAC is available

## Concept

### Source MAC

The ESP-NOW receive callback provides the sender MAC address. This is the transport-level identity of the sender.

### Packet Node ID

`WirelessPacketHeader::node_id` identifies the logical Cyber32 wireless node claimed by the packet.

### Required Agreement

When source MAC is available:

- Registry must find an allowlist record for that MAC.
- The allowlist record must not be blocked.
- The allowlist record `node_id` must match `header.node_id`.
- The matching node must still pass normal node ID allowlist checks.

If source MAC is unavailable, v1 keeps the existing node ID allowlist behavior so simulated transport remains supported.

## Registry Lookup

WirelessService should use:

```cpp
registry_->getWirelessNodeAllowlistRecordByMac(source_mac, out_record)
```

Expected behavior:

- `OK`: compare `out_record.node_id` to `header.node_id`
- `NOT_FOUND`: reject as unknown MAC
- `INVALID_ID`: reject as unknown MAC

Registry stores allowlist records only. Registry must not parse packets or enforce transport policy.

## WirelessService Behavior

### If Source MAC Is Available

1. Look up allowlist record by source MAC.
2. If lookup fails:
   - reject packet
   - set `last_error_code_ = "wireless_mac_not_allowed"`
   - do not call Device
   - do not update provider payload
   - do not update canonical payload

3. If MAC record is blocked:
   - reject packet
   - set `last_error_code_ = "wireless_node_blocked"`
   - do not call Device
   - do not update provider payload
   - do not update canonical payload

4. If MAC record `allow_state` is not `ALLOWED`:
   - reject packet
   - set `last_error_code_ = "wireless_mac_not_allowed"`
   - do not update Device/provider/canonical state

5. If MAC record `node_id != header.node_id`:
   - reject packet
   - set `last_error_code_ = "wireless_mac_node_mismatch"`
   - do not update Device/provider/canonical state

6. If MAC record is allowed and node ID matches:
   - continue to normal node ID allowlist check

### If Source MAC Is Not Available

Use existing v1 behavior:

- validate `header.node_id` through node ID allowlist lookup
- continue through trust, sequence, Device update, and provider update

This preserves simulated transport and older validation paths.

## Error Codes

### `wireless_mac_not_allowed`

Meaning:

- source MAC is available but no allowlist record exists for it
- or the MAC record is not `ALLOWED`

Layer:

- emitted by WirelessService

Validation:

- unknown MAC is rejected
- provider payload unchanged
- canonical payload unchanged

### `wireless_mac_node_mismatch`

Meaning:

- source MAC maps to a valid allowlist record, but the record `node_id` does not match packet `header.node_id`

Layer:

- emitted by WirelessService

Validation:

- mismatched MAC/node packet is rejected
- provider payload unchanged
- canonical payload unchanged

### `wireless_node_blocked`

Meaning:

- source MAC maps to a blocked node record
- or normal node ID allowlist lookup finds a blocked node

Layer:

- emitted by WirelessService

Validation:

- blocked MAC record is rejected
- provider payload unchanged
- canonical payload unchanged

## Policy Order

WirelessService packet processing order should be:

1. read packet through transport adapter or simulated fallback
2. checksum validation
3. MAC-to-node check if source MAC exists
4. node ID allowlist check
5. trust validation
6. sequence duplicate validation
7. `WirelessTemperatureDevice::updateFromPacket(...)`
8. provider payload update through Registry public API
9. mark sequence accepted

The MAC check must happen after checksum validation so corrupted packets fail closed before identity policy.

The MAC check must happen before node ID allowlist validation so a spoofed `node_id` cannot bypass source identity.

## Validation Requirements

### Matching MAC And Node ID Accepted

Setup:

- allowlist node `1001`
- `has_mac_address = true`
- MAC matches packet source MAC
- allow state `ALLOWED`
- trust state `TRUSTED`
- packet `header.node_id = 1001`

Expected:

- `processPackets(...)` returns true
- provider payload updates
- canonical payload remains unchanged unless explicitly updated elsewhere

### Unknown MAC Rejected

Setup:

- source MAC exists on packet
- Registry has no matching MAC record

Expected:

- `processPackets(...)` returns false
- `lastErrorCode() == "wireless_mac_not_allowed"`
- provider unchanged
- canonical unchanged
- packet consumed

### Blocked MAC Rejected

Setup:

- source MAC maps to allowlist record
- allow state is `BLOCKED`

Expected:

- `processPackets(...)` returns false
- `lastErrorCode() == "wireless_node_blocked"`
- provider unchanged
- canonical unchanged
- packet consumed

### MAC Maps To Different Node ID Rejected

Setup:

- source MAC maps to node `1001`
- packet claims `header.node_id = 2002`

Expected:

- `processPackets(...)` returns false
- `lastErrorCode() == "wireless_mac_node_mismatch"`
- provider unchanged
- canonical unchanged
- packet consumed

### No-MAC Sim Packet Still Accepted

Setup:

- simulated packet is injected without source MAC
- node ID is allowlisted
- checksum/trust/sequence checks pass

Expected:

- existing node ID allowlist behavior remains valid
- provider updates normally
- simulated validation path remains preserved

## Boundary Rules

WirelessService may:

- read source MAC from `WirelessPacketTransportAdapter`
- call Registry public allowlist lookup APIs
- enforce MAC-to-node policy

WirelessService must not:

- branch on concrete transport type
- parse raw ESP-NOW bytes
- update Registry arrays directly
- expose MAC to Logic
- expose MAC to Runtime

Registry may:

- store MAC allowlist records
- return allowlist records by MAC

Registry must not:

- parse packets
- enforce WirelessService policy
- call Devices, Drivers, Runtime, API, or Logic

Driver may:

- preserve source MAC
- expose source MAC through adapter-compatible packet reads

Driver must not:

- enforce allowlist
- enforce trust
- enforce sequence policy
- update provider payloads
- update canonical payloads

## Stop Conditions

Stop implementation if any of the following are required:

- Logic sees MAC address
- Logic sees node ID
- Logic sees provider ID
- Registry parses packets
- Registry enforces packet identity policy
- Driver enforces allowlist
- Driver enforces trust
- Driver enforces sequence policy
- WirelessService branches on `EspNowTransportDriver`
- dynamic allocation is introduced
- Arduino `String` is introduced
- STL containers are introduced
- `src/main.cpp` must change

## Final Assessment

Milestone 8.8 should add one bounded identity gate to WirelessService while preserving the existing simulated transport path.

PASS: The required Registry MAC lookup helper and adapter source-MAC plumbing already exist.

WARNING: MAC address alone is not strong authentication. This milestone improves identity consistency, but future keyed message integrity is still required before wireless actuator support.
