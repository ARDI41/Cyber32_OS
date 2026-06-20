# Milestone 9.1 Phase 1 Base Node Hardware Test Plan

## Goal

Define the minimal Cyber32 Base Node test harness for receiving the first real wireless `CAP_TEMPERATURE` packet.

This phase is documentation only. It does not modify source code and does not modify `src/main.cpp`.

## Base Node Responsibilities

The base-node test harness should perform only the minimum setup needed to receive and process one real wireless temperature packet.

Required responsibilities:

1. Initialize Registry.
2. Initialize `EspNowTransportDriver`.
3. Initialize `WirelessTemperatureDevice`.
4. Initialize `WirelessService`.
5. Register allowlist node with fixed `node_id` and source MAC.
6. Register wireless provider:
   - `provider-wireless-temperature-001`
   - `CAP_TEMPERATURE`
   - provider type `WIRELESS`
7. Attach `EspNowTransportDriver` through `WirelessPacketTransportAdapter`.
8. Attach Registry and `WirelessTemperatureDevice` to `WirelessService`.
9. Process received packets manually or through a small validation loop.
10. Read provider and security diagnostic state after packet processing.

## Hard Boundary

This phase must not wire the full production bootstrap.

Do not add:

- Dashboard
- WebServer
- Mobile Studio
- cloud
- actuator behavior
- OTA
- persistence
- new production boot sequence

This phase is only a hardware test harness for the first real wireless `CAP_TEMPERATURE` receive path.

## Test-Only Entry Strategy

Recommended approach:

- create a separate app/test harness file in a later implementation phase
- or use a narrow compile flag for the base-node hardware test
- keep the normal production entry path unchanged
- avoid modifying `src/main.cpp` unless a later explicit milestone approves it

The harness should be easy to remove or disable after the hardware test is complete.

The harness should not become production bootstrap by accident.

## Required Known Values

The base-node test must record these fixed values before execution:

### Base ESP32 MAC

Required for observing and configuring ESP-NOW pairing behavior.

### Node ESP32 MAC

Required for allowlist setup and MAC-to-node validation.

### Node ID

```text
node_id = 1001
```

### Capability

```text
capability = CAP_TEMPERATURE
```

### Provider ID

```text
provider_id = provider-wireless-temperature-001
```

## Minimal Base Setup Flow

Expected test harness flow:

```text
Registry begin
-> EspNowTransportDriver begin
-> WirelessTemperatureDevice begin node 1001
-> WirelessService begin
-> register allowlist record node 1001 + node MAC
-> register wireless provider
-> makeEspNowTransportAdapter
-> attach adapter to WirelessService
-> process received packet
-> read provider payload
-> read security diagnostic
```

## Test Observation

The base-node test should observe:

- provider payload
- provider status
- security diagnostic accepted count
- last accepted sequence ID
- last error code

Expected successful one-shot result:

- provider payload capability is `CAP_TEMPERATURE`
- provider payload value matches packet temperature
- provider status is `AVAILABLE`
- accepted packet count increments
- last accepted sequence ID equals packet `sequence_id`
- last error is `none`

## Validation Strategy

### Step 1: Deterministic One-Shot Packet

Wireless node sends one fixed temperature packet.

Expected:

- packet received
- checksum passes
- MAC-to-node validation passes
- provider updates
- accepted diagnostic updates

### Step 2: Periodic Packet

Wireless node sends periodic packets with incrementing sequence IDs.

Expected:

- each new sequence is accepted
- provider payload follows latest packet
- accepted count increments

### Step 3: Duplicate Sequence

Wireless node sends the same sequence ID twice.

Expected:

- first packet accepted
- second packet rejected
- duplicate sequence reject count increments
- provider payload remains unchanged after duplicate

### Step 4: Wrong MAC

Use a packet/source identity that does not match the allowlist.

Expected:

- packet rejected
- MAC rejection or MAC/node mismatch diagnostic increments
- provider payload remains unchanged

## Stop Conditions

Stop implementation if this phase requires:

- unrelated architecture changes
- persistence
- API transport or WebServer
- Dashboard
- Mobile Studio
- cloud
- actuator behavior
- production bootstrap wiring
- `src/main.cpp` changes
- dynamic allocation
- Arduino `String`
- STL containers

## Final Assessment

This phase should prepare the smallest possible base-node hardware receive test for real ESP-NOW `CAP_TEMPERATURE`.

It should prove the base node can receive a real packet and route it through the already validated Cyber32 path without turning the test harness into production boot code.
