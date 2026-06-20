# Milestone 9.1 Phase 8 First Real Packet Test Plan

## Goal

Define the first real over-the-air `CAP_TEMPERATURE` packet test between the Cyber32 base ESP32 and sender ESP32.

This plan is documentation only. It does not modify source code and does not modify `src/main.cpp`.

## Pre-Test Requirements

Before running the test:

- base ESP32 MAC is known
- sender ESP32 MAC is known
- sender MAC is added to the base allowlist
- base MAC is configured as sender ESP-NOW peer
- base harness firmware builds
- sender firmware builds
- both ESP32 boards have stable power
- both boards can be monitored over serial

Required identity mapping:

```text
sender MAC -> node_id 1001
base MAC   -> sender peer target
```

## Base Test Run

1. Flash base harness firmware to the base ESP32.
2. Open base serial monitor.
3. Confirm ESP-NOW initialization.
4. Confirm allowlist record registration:
   - `node_id = 1001`
   - sender MAC
   - `allow_state = ALLOWED`
   - `trust_state = TRUSTED`
5. Confirm wireless provider registration:
   - `provider-wireless-temperature-001`
   - `CAP_TEMPERATURE`
   - provider type `WIRELESS`
6. Confirm security diagnostic record registration:
   - `node_id = 1001`
   - sender MAC
   - counters initialized
7. Start base receive loop or manual `run(now_ms)` loop.

## Sender Test Run

1. Flash sender firmware to the sender ESP32.
2. Open sender serial monitor.
3. Confirm WiFi STA mode.
4. Confirm ESP-NOW initialization.
5. Confirm base peer registration.
6. Send one packet:

```text
node_id = 1001
sequence_id = 1
capability_id = CAP_TEMPERATURE
payload_type = FLOAT
value_float = 24.5F
```

7. Confirm checksum is calculated after packet fields are populated.
8. Confirm packet is serialized:

```text
WirelessPacketHeader
-> WirelessCapabilityValue
-> WirelessNodeDiagnostics
```

9. Confirm sender attempts ESP-NOW send to base MAC.

## Expected Base Result

The base should observe:

- packet received
- raw payload captured
- decode succeeds
- structured packet becomes available
- WirelessService `processPackets(...)` succeeds
- checksum validation succeeds
- MAC-to-node validation succeeds
- allowlist validation succeeds
- trust validation succeeds
- sequence validation succeeds
- provider payload becomes `24.5F`
- provider status becomes `AVAILABLE`
- `accepted_packet_count` increments
- `last_accepted_sequence_id == 1`
- `last_error_code = none`

Provider expectation:

```text
provider_id = provider-wireless-temperature-001
capability_id = CAP_TEMPERATURE
latest_payload.value_float = 24.5F
status = AVAILABLE
```

Security diagnostic expectation:

```text
node_id = 1001
accepted_packet_count >= 1
last_accepted_sequence_id = 1
last_error_code = none
```

## Failure Indicators

### Checksum Invalid

Indicator:

```text
last_error_code = wireless_checksum_invalid
```

Expected:

- provider payload unchanged
- checksum reject count increments

### MAC Not Allowed

Indicator:

```text
last_error_code = wireless_mac_not_allowed
```

Expected:

- sender MAC not found or not allowed
- provider payload unchanged
- MAC-not-allowed reject count increments

### MAC / Node Mismatch

Indicator:

```text
last_error_code = wireless_mac_node_mismatch
```

Expected:

- sender MAC maps to a different node than packet `node_id`
- provider payload unchanged
- mismatch reject count increments

### Node Not Allowed

Indicator:

```text
last_error_code = wireless_node_not_allowed
```

Expected:

- `node_id` allowlist lookup failed
- provider payload unchanged

### Duplicate Sequence

Indicator:

```text
last_error_code = wireless_duplicate_sequence
```

Expected:

- sequence ID was already accepted
- provider payload unchanged
- duplicate sequence reject count increments

### Provider Not Found

Indicator:

```text
last_error_code = provider_update_failed
```

Expected:

- provider record missing or invalid
- packet policy may have passed
- provider payload not updated

## Pass Condition

PASS only when both of these update from a real ESP-NOW packet:

1. wireless provider payload
2. wireless security diagnostic record

Minimum PASS evidence:

```text
provider latest_payload.value_float = 24.5F
provider status = AVAILABLE
security accepted_packet_count increments
security last_error_code = none
```

## Stop Conditions

Stop if this test requires:

- Dashboard
- WebServer
- Mobile Studio
- cloud
- actuator behavior
- persistence
- architecture rewrite
- Runtime packet parsing
- Registry packet parsing
- Logic visibility into MAC, node ID, provider ID, or transport
- dynamic allocation
- Arduino `String`
- STL containers
- `src/main.cpp` changes before an explicit bootstrap milestone

## Final Assessment

Milestone 9.1 Phase 8 is the first real over-the-air proof.

The only goal is to prove that a real sender ESP32 can deliver one valid `CAP_TEMPERATURE` packet into the base node's existing Cyber32 wireless path.
