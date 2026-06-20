# Milestone 9.1 First Real Wireless Node Plan

## Goal

Define the first real Cyber32 wireless temperature node test.

The goal is only:

```text
First real wireless CAP_TEMPERATURE node
```

This milestone does not add Dashboard, Mobile Studio, cloud, actuator, cryptography, or persistence work.

## Test Topology

### ESP32 #1

Cyber32 Base Node.

Responsibilities:

- run Cyber32 base firmware
- initialize ESP-NOW receive path
- maintain wireless node allowlist
- receive wireless temperature packet
- decode packet
- route packet through `WirelessPacketTransportAdapter`
- process packet through WirelessService
- update provider state
- update wireless security diagnostics
- expose state through internal API

### ESP32 #2

Cyber32 Wireless Temperature Node.

Responsibilities:

- boot as a wireless temperature sender
- initialize ESP-NOW transmit path
- produce or simulate a temperature value
- build a bounded Cyber32 wireless packet
- calculate checksum
- send packet to the Cyber32 Base Node

## Success Criteria

Milestone 9.1 succeeds when:

1. Wireless node boots.
2. ESP-NOW initializes on both ESP32 boards.
3. Wireless temperature node sends a temperature packet.
4. Base node receives the packet.
5. Raw packet is captured.
6. Packet is decoded into Cyber32 wireless structs.
7. Checksum is validated.
8. MAC identity is validated.
9. MAC maps to the expected `node_id`.
10. Allowlist state is validated.
11. Trust state is validated.
12. Sequence ID is validated.
13. Provider payload is updated.
14. Wireless security diagnostic record is updated.
15. API can read provider state.
16. API can read wireless security diagnostics.

## Required Packet Contents

The first real wireless temperature packet must include:

- `node_id`
- `sequence_id`
- `CAP_TEMPERATURE`
- temperature value
- checksum

The packet must use the bounded Cyber32 wireless packet schema:

- `WirelessPacketHeader`
- `WirelessCapabilityValue`
- `WirelessNodeDiagnostics`

No JSON is allowed.

No dynamic allocation is allowed.

## Required Allowlist Setup

The base node must have a fixed allowlist entry before packet acceptance:

- `node_id`
- MAC address
- `allow_state`
- `trust_state`

Expected v1 values:

```text
allow_state = ALLOWED
trust_state = TRUSTED
```

The source MAC received by ESP-NOW must match the allowlist record for the packet `node_id`.

## Test Phases

### Phase 1: Static One-Shot Temperature Packet

The wireless node sends one deterministic temperature packet.

Expected:

- packet received
- checksum passes
- MAC identity passes
- allowlist passes
- trust passes
- sequence passes
- provider payload updates
- security diagnostic accepted count increments

### Phase 2: Periodic Packet

The wireless node sends periodic temperature packets with incrementing sequence IDs.

Expected:

- each valid packet is accepted
- provider payload updates to latest value
- accepted packet count increments
- duplicate sequence counter remains unchanged

### Phase 3: Lost/Stale Provider Behavior

Stop sending packets long enough to trigger provider health timeout behavior.

Expected:

- provider transitions according to Registry health timeout rules
- selected/canonical payload behavior follows explicit provider update policy
- Logic remains provider-blind

### Phase 4: Duplicate Sequence Rejection

Send two packets with the same sequence ID.

Expected:

- first packet accepted
- duplicate packet rejected
- provider payload remains unchanged after duplicate
- duplicate sequence reject counter increments
- last rejected sequence ID updates

### Phase 5: Wrong MAC Rejection

Send or simulate a packet where the ESP-NOW source MAC does not match the allowlisted node identity.

Expected:

- packet rejected
- provider payload remains unchanged
- MAC rejection or MAC/node mismatch diagnostic counter increments
- canonical payload remains unchanged

## Validation Requirements

Validation must prove:

- provider payload updates after accepted real wireless temperature packet
- wireless security diagnostics update after accepted packet
- wireless security diagnostics update after rejected packet
- summary API totals update correctly
- API can read provider diagnostics
- API can read wireless security diagnostics
- canonical `CAP_TEMPERATURE` payload remains unchanged unless explicit selected/best provider update is called
- normal capability API reads remain capability-first
- Logic does not know the packet came from ESP-NOW

## API Expectations

Provider state should be readable through existing provider diagnostics API.

Wireless security state should be readable through:

```text
getWirelessSecurityDiagnostic(...)
getWirelessSecurityDiagnosticByIndex(...)
getWirelessSecuritySummary(...)
```

These API methods must remain read-only.

They must not:

- reset counters
- mutate diagnostics
- call WirelessService
- expose raw packets
- update providers
- update canonical payloads

## Stop Conditions

Stop Milestone 9.1 implementation if it requires:

- Dashboard work
- Mobile Studio work
- cloud work
- actuator work
- cryptography work
- persistence work
- dynamic allocation
- Arduino `String`
- STL containers
- Runtime packet parsing
- Registry packet parsing
- Logic seeing MAC, node ID, provider ID, or transport details
- `src/main.cpp` changes unless a later explicit milestone authorizes production bootstrap wiring

## Final Assessment

Milestone 9.1 is the first real wireless temperature proof.

It should validate real ESP-NOW transport against the same Cyber32 architecture already proven in simulation:

```text
real packet
-> bounded decode
-> adapter
-> WirelessService policy
-> Registry provider state
-> Registry diagnostics
-> read-only API visibility
```

No broader product surface should be added during this milestone.
