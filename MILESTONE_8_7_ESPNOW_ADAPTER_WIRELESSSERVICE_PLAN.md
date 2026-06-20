# Milestone 8.7 ESP-NOW Adapter WirelessService Plan

## Goal

Validate that decoded packets from `EspNowTransportDriver` can flow through `WirelessPacketTransportAdapter` into `WirelessService` without requiring real ESP-NOW radio hardware.

This milestone proves that the real ESP-NOW driver boundary can feed the same WirelessService policy pipeline already used by simulated transport.

## Inputs

- `CYBER32_BIBLE.md`
- `MILESTONE_8_5_WIRELESSSERVICE_TRANSPORT_ABSTRACTION_PLAN.md`
- Current source state for Milestone 8.6 adapter path

`MILESTONE_8_6_WIRELESSSERVICE_TRANSPORT_ADAPTER_PATH.md` was not present at the time of this plan, so the current implemented source state is treated as the Milestone 8.6 reference.

## Problem

Cyber32 now has two compatible wireless packet sources:

- `SimEspNowTransportDriver`
- `EspNowTransportDriver`

`EspNowTransportDriver` can capture raw bytes, decode those bytes into:

- `WirelessPacketHeader`
- `WirelessCapabilityValue`
- `WirelessNodeDiagnostics`

It can then expose the decoded structured packet through the same `WirelessPacketTransportAdapter` contract used by simulated transport.

The next validation step must prove that:

- WirelessService does not care which transport produced the packet.
- WirelessService remains the policy owner.
- Registry remains state-only.
- Logic remains provider-blind.
- Real radio hardware is not required for validation.

## Test Path

The validation path should use only public driver, adapter, service, and Registry APIs.

1. Create fixed local validation objects:
   - `Registry`
   - `WirelessTemperatureDevice`
   - `WirelessService`
   - `EspNowTransportDriver`

2. Initialize required local state:
   - begin the Registry
   - begin the wireless temperature device with a fixed node ID such as `1001`
   - begin the WirelessService
   - attach Registry to WirelessService
   - attach WirelessTemperatureDevice to WirelessService

3. Register allowlist state:
   - node ID `1001`
   - `WirelessNodeAllowState::ALLOWED`
   - `WirelessTrustState::TRUSTED`

4. Register wireless provider state:
   - provider ID: `provider-wireless-temperature-001`
   - capability ID: `CAP_TEMPERATURE`
   - provider type: `WIRELESS`
   - status: `STALE` or `AVAILABLE`
   - priority higher than simulated provider if a simulated provider is also present

5. Build a deterministic structured packet:
   - `WirelessPacketHeader`
   - `WirelessCapabilityValue`
   - `WirelessNodeDiagnostics`

6. Set required packet fields:
   - magic = `WIRELESS_PACKET_MAGIC`
   - protocol version = `WIRELESS_PROTOCOL_VERSION`
   - packet type = `WirelessPacketType::CAPABILITY_VALUE`
   - node ID = `1001`
   - sequence ID = a new value
   - capability ID = `CAP_TEMPERATURE`
   - payload type = `WirelessPayloadType::FLOAT`
   - temperature value = deterministic test value

7. Calculate checksum:
   - use `calculateWirelessPacketChecksum(header, value, diagnostics)`
   - write the result to `header.checksum`

8. Pack raw bytes in exact decode order:
   - `WirelessPacketHeader`
   - `WirelessCapabilityValue`
   - `WirelessNodeDiagnostics`

9. Inject raw bytes into `EspNowTransportDriver`:
   - use `injectRawPayloadForTest(...)`
   - provide a fixed source MAC

10. Decode raw payload:
    - call `decodePendingRawPayload()`
    - expect decoded structured packet to become pending

11. Build adapter:
    - call `makeEspNowTransportAdapter(&espnow_driver)`
    - verify `wirelessPacketTransportAdapterValid(adapter)`

12. Attach adapter:
    - call `WirelessService::attachTransportAdapter(adapter)`

13. Process packet:
    - call `WirelessService::processPackets(now_ms)`
    - do not call driver decode or Registry update behavior from Runtime
    - do not require real ESP-NOW callback execution

## Expected Behavior

### WirelessService Policy

WirelessService must continue to own the packet policy pipeline:

1. read structured packet through adapter
2. validate checksum
3. validate allowlist
4. validate trust state
5. validate duplicate sequence
6. call `WirelessTemperatureDevice::updateFromPacket(...)`
7. update provider payload through Registry public API
8. mark sequence accepted after successful provider update

The pipeline must be identical to the simulated transport path after the packet is read.

### Checksum

Checksum validation still happens in WirelessService.

Invalid checksum packets must:

- return false from `processPackets(...)`
- set compact last error code to `wireless_checksum_invalid`
- leave provider payload unchanged
- leave canonical capability payload unchanged
- consume the packet

### Allowlist

Allowlist validation still happens in WirelessService.

Unknown nodes must:

- return false
- set compact last error code to `wireless_node_not_allowed`
- leave provider payload unchanged
- leave canonical capability payload unchanged
- consume the packet

Blocked nodes must:

- return false
- set compact last error code to `wireless_node_blocked`
- leave provider payload unchanged
- leave canonical capability payload unchanged
- consume the packet

### Trust

Trust validation still happens in WirelessService after allowlist validation.

Untrusted, blocked, or unknown trust states must:

- return false
- set compact last error code to `wireless_untrusted`
- leave provider payload unchanged
- leave canonical capability payload unchanged
- consume the packet

### Sequence

Duplicate sequence validation still happens in WirelessService.

Duplicate sequence packets must:

- return false
- set compact last error code to `wireless_duplicate_sequence`
- leave provider payload unchanged
- leave canonical capability payload unchanged
- consume the packet

New sequence packets must be accepted after checksum, allowlist, and trust checks pass.

### Provider Payload

Provider payload updates must happen only through Registry public API.

Expected provider update after a valid packet:

- provider ID remains `provider-wireless-temperature-001`
- provider status becomes `CapabilityProviderStatus::AVAILABLE`
- latest payload capability ID is `CAP_TEMPERATURE`
- latest payload value matches the packet value
- latest payload unit remains `degree_celsius`
- latest payload quality remains `valid`

### Canonical Payload

Canonical `CAP_TEMPERATURE` payload behavior must remain unchanged by this validation unless an explicit Registry selected/best provider update method is called.

For this milestone:

- `WirelessService::processPackets(...)` updates provider payload only
- canonical payload should remain unchanged
- Logic still queries `CAP_TEMPERATURE` only
- Logic does not see provider ID, node ID, MAC, or transport source

## Validation Cases

### Case 1: Valid ESP-NOW Decoded Packet Updates Provider

Setup:

- node `1001` is allowlisted as `ALLOWED`
- wireless temperature device trust state is `TRUSTED`
- provider `provider-wireless-temperature-001` exists
- decoded ESP-NOW packet has valid checksum
- sequence ID is new

Expected:

- `decodePendingRawPayload()` returns true
- adapter reports a pending packet
- `WirelessService::processPackets(now_ms)` returns true
- provider latest payload updates to the packet value
- adapter/driver packet is cleared after processing
- canonical `CAP_TEMPERATURE` remains unchanged

### Case 2: Invalid Checksum Rejected

Setup:

- same valid allowlist and trust state
- packet checksum is deliberately wrong

Expected:

- `processPackets(now_ms)` returns false
- `lastErrorCode()` is `wireless_checksum_invalid`
- provider latest payload remains the previous valid value
- canonical payload remains unchanged
- packet is consumed

### Case 3: Unknown Node Rejected

Setup:

- packet node ID is not registered in the allowlist
- checksum is valid

Expected:

- `processPackets(now_ms)` returns false
- `lastErrorCode()` is `wireless_node_not_allowed`
- provider latest payload remains unchanged
- canonical payload remains unchanged
- packet is consumed

### Case 4: Duplicate Sequence Rejected

Setup:

- first valid packet with sequence ID `N` succeeds
- second valid packet from same node uses sequence ID `N`

Expected:

- first packet updates provider
- duplicate packet returns false
- `lastErrorCode()` is `wireless_duplicate_sequence`
- provider latest payload remains at first packet value
- canonical payload remains unchanged
- duplicate packet is consumed

### Case 5: Packet Clears After Processing

Setup:

- process a valid packet through the ESP-NOW adapter path

Expected:

- adapter `has_received_packet(context)` becomes false after read
- driver `hasReceivedPacket()` becomes false after processing
- no raw payload remains pending after decode/read flow

## Boundary Rules

This milestone must not require:

- real ESP-NOW callback execution
- real radio hardware
- WiFi peer registration
- `src/main.cpp` changes
- Runtime changes
- Registry behavior changes
- API changes
- Logic changes

The validation must not introduce:

- transport-specific branches in WirelessService policy
- Registry awareness of packet origin
- Logic awareness of MAC, node ID, provider ID, or transport source
- driver-level checksum, allowlist, trust, sequence, or provider policy
- dynamic allocation
- Arduino `String`
- STL containers

## Source MAC Policy

The adapter path should preserve source MAC availability from `EspNowTransportDriver`, but Milestone 8.7 does not add MAC enforcement.

For this milestone:

- source MAC may be injected and read
- WirelessService may ignore source MAC for now
- node allowlist behavior remains based on packet `node_id`
- MAC-to-node enforcement remains a future milestone

## Recommended Implementation Order

1. Add a validation helper for ESP-NOW adapter-to-WirelessService flow using local fixed objects.
2. Build a deterministic valid structured wireless packet.
3. Pack the packet into raw bytes in decode order.
4. Inject raw bytes into `EspNowTransportDriver`.
5. Decode pending raw payload.
6. Attach `makeEspNowTransportAdapter(...)` to WirelessService.
7. Validate successful provider update.
8. Validate invalid checksum rejection.
9. Validate unknown node rejection.
10. Validate duplicate sequence rejection.
11. Confirm provider unchanged after rejected packets.
12. Confirm canonical payload unchanged.
13. Confirm existing simulated adapter validation still passes.
14. Run PlatformIO build when available.

## Stop Conditions

Stop implementation if any of the following are required:

- WirelessService branches on `EspNowTransportDriver` type
- Registry knows packet origin
- Runtime parses packets
- Runtime chooses transport behavior
- Logic sees MAC address
- Logic sees node ID
- Logic sees provider ID
- driver performs checksum policy
- driver performs allowlist policy
- driver performs trust policy
- driver performs sequence policy
- driver writes Registry provider payloads
- dynamic allocation is introduced
- Arduino `String` is introduced
- STL containers are introduced
- `src/main.cpp` must change

## Assessment

PASS: The architecture is ready for hardware-free ESP-NOW adapter validation if validation uses `injectRawPayloadForTest(...)`, `decodePendingRawPayload()`, `makeEspNowTransportAdapter(...)`, and the existing WirelessService adapter path.

WARNING: Real MAC-to-node enforcement is not part of this milestone. Source MAC preservation exists at the transport boundary, but node allowlist policy still uses packet `node_id` until a dedicated MAC enforcement milestone.

## Final Assessment

Milestone 8.7 should prove that decoded real-driver packets can enter the same WirelessService policy pipeline as simulated packets without changing Runtime, Registry, API, Logic, or `src/main.cpp`.

The next implementation phase should be validation-only unless a compile fix is required.
