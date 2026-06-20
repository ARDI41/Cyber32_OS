# Milestone 9.1 Phase 3 Real Node Test Checklist

## Goal

Create a practical checklist for running the first real Cyber32 wireless `CAP_TEMPERATURE` test with two ESP32 boards.

This checklist is documentation only. It does not modify source code and does not modify `src/main.cpp`.

## 1. Hardware Checklist

- ESP32 base board available
- ESP32 sender board available
- USB cable for base board
- USB cable for sender board
- serial monitor for base board
- serial monitor for sender board
- known base board COM port
- known sender board COM port
- stable power for both boards
- boards close enough for ESP-NOW test range
- no conflicting firmware running on either board

## 2. MAC Address Checklist

- read base ESP32 MAC address
- read sender ESP32 MAC address
- record base MAC in test notes
- record sender MAC in test notes
- confirm sender MAC is added to base allowlist
- confirm base MAC is used by sender ESP-NOW peer setup
- confirm allowlist record uses:
  - `node_id = 1001`
  - sender MAC
  - `allow_state = ALLOWED`
  - `trust_state = TRUSTED`

## 3. Base Firmware Checklist

- ESP-NOW initialized
- `EspNowTransportDriver` initialized
- allowlist record registered
- wireless provider registered:
  - `provider-wireless-temperature-001`
  - `CAP_TEMPERATURE`
  - provider type `WIRELESS`
- security diagnostic record registered for node `1001`
- `WirelessTemperatureDevice` initialized
- `WirelessService` initialized
- `WirelessPacketTransportAdapter` created for `EspNowTransportDriver`
- adapter attached to `WirelessService`
- provider diagnostic readable
- wireless security diagnostic readable
- no Dashboard required
- no WebServer required
- no cloud required
- no actuator behavior enabled

## 4. Sender Firmware Checklist

- WiFi STA mode initialized
- ESP-NOW initialized
- base peer registered with base MAC
- packet built with:
  - `node_id = 1001`
  - `sequence_id` set
  - `capability_id = CAP_TEMPERATURE`
  - payload type `FLOAT`
  - temperature value set
  - diagnostics placeholder set
- checksum calculated after packet fields are filled
- packet serialized in exact order:
  - `WirelessPacketHeader`
  - `WirelessCapabilityValue`
  - `WirelessNodeDiagnostics`
- packet sent to base MAC
- sender serial output shows send attempt

## 5. Expected Serial / Debug Observations

Expected base observations:

- packet received
- raw payload captured
- raw payload length matches expected structured packet size
- decoded packet available
- `processPackets(...)` returns true
- provider payload updates
- provider status becomes `AVAILABLE`
- provider payload capability is `CAP_TEMPERATURE`
- provider payload temperature matches sender value
- `accepted_packet_count` increments
- `last_accepted_sequence_id` matches packet sequence
- `last_error_code == none`
- canonical payload remains unchanged unless explicit selected/best provider update is called

Expected sender observations:

- ESP-NOW initialized
- base peer added
- packet checksum calculated
- packet send attempted
- send result is success or shows a concrete ESP-NOW send error

## 6. Failure Test Checklist

### Duplicate Sequence

- send packet with sequence ID `N`
- send another packet with sequence ID `N`

Expected:

- first packet accepted
- second packet rejected
- duplicate sequence reject count increments
- provider payload remains unchanged after duplicate

### Wrong Node ID

- send packet with sender MAC but unexpected `node_id`

Expected:

- packet rejected
- MAC/node mismatch or allowlist rejection recorded
- provider payload remains unchanged

### Unknown MAC

- use sender MAC not present in base allowlist

Expected:

- packet rejected
- MAC-not-allowed diagnostic increments
- provider payload remains unchanged

### Bad Checksum

- send packet with corrupted checksum

Expected:

- packet rejected
- checksum reject count increments
- provider payload remains unchanged

### Sender Stopped

- stop sender long enough for stale/lost behavior

Expected:

- provider health timeout behavior follows Registry timeout policy
- canonical payload changes only through explicit selected/best provider update flow
- Logic remains provider-blind

## 7. Pass / Fail Criteria

### PASS

PASS when a real ESP32 sender packet reaches:

```text
ESP-NOW receive
-> raw payload capture
-> packet decode
-> WirelessPacketTransportAdapter
-> WirelessService
-> provider update
-> wireless security diagnostic update
-> API diagnostic read
```

Required PASS evidence:

- provider payload updated from real packet
- provider status is `AVAILABLE`
- security diagnostic accepted count increments
- API can read provider state
- API can read wireless security diagnostics

### FAIL

FAIL if:

- packet is accepted without checksum validation
- packet is accepted without MAC/allowlist/trust/sequence policy
- provider updates without WirelessService
- Registry parses raw packets
- Runtime parses raw packets
- Logic sees MAC, node ID, provider ID, or transport details
- diagnostics mutate provider or canonical payload state

### BLOCK

BLOCK if:

- `src/main.cpp` changes are required before an explicit bootstrap milestone
- full production bootstrap must be wired to run the test
- Dashboard/WebServer/Mobile Studio/cloud work becomes required
- actuator behavior becomes required
- dynamic allocation, Arduino `String`, or STL containers are required

## Final Note

This checklist is for the first real wireless `CAP_TEMPERATURE` proof only.

Keep the bench test small, observable, and reversible.
