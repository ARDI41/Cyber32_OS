# Milestone 9.3 Packet Decode Plan

## Goal

Decode the first real Cyber32 Sensor Firmware packet received over ESP-NOW.

This milestone ends before Registry ingestion.

Milestone 9.3 converts receive visibility into bounded packet understanding:

```text
ESP-NOW RX callback
-> raw 60-byte payload
-> bounded decode checks
-> decoded node / sequence / capability / value visibility
-> Serial debug output
```

## Context

Milestone 9.2 proved the first real Sensor Firmware to Cyber32 OS Base Node ESP-NOW receive path.

Confirmed hardware result:

- Sensor Firmware packet validation OK
- Sensor Firmware transport validation OK
- send result `SEND_OK`
- ESP-NOW TX callback status `SUCCESS`
- Base Node ESP-NOW RX callback fired
- received source MAC `AC:27:6E:A5:8C:68`
- received length `60`

Milestone 9.3 does not update Registry, providers, Logic, API, Dashboard, Dev Panel, Mini App, or WirelessService.

## Sensor Visibility Direction

Cyber32 sensors may be wired or wireless.

Both wired and wireless sensors must eventually be visible through the same capability/provider model.

Decoded packet visibility in Milestone 9.3 should prepare for future display fields:

- `node_id`
- `capability_id`
- value
- unit
- update interval / report interval
- `last_seen_ms`
- provider status

Wireless sensor-specific fields:

- MAC address
- battery percent
- battery voltage
- signal strength / RSSI
- `sequence_id`
- diagnostics status

Important boundary:

Milestone 9.3 still only decodes and prints received packet contents.

It does not:

- update Registry
- update Provider
- implement Dashboard
- implement API changes

## Phase 1: Packet Envelope Validation

Validate:

- packet size
- magic
- protocol version

Expected behavior:

- accept only the expected Sensor Firmware packet size
- reject invalid packet sizes
- reject invalid magic
- reject unsupported protocol versions
- print compact Serial diagnostics for accepted and rejected envelope checks

Boundaries:

- no Registry writes
- no provider updates
- no WirelessService call
- no Logic call
- no API change

## Phase 2: Packet Identity Decode

Decode:

- `node_id`
- `sequence_id`
- `capability_id`

Expected behavior:

- decode fields from the bounded packet layout
- print decoded identity fields to Serial
- preserve capability-first naming in output
- do not apply allowlist, trust, or sequence policy in this milestone unless already present in lower receive visibility code

Required Serial visibility:

```text
Cyber32 Packet Node ID: <node_id>
Cyber32 Packet Sequence ID: <sequence_id>
Cyber32 Packet Capability: <capability_id>
```

## Phase 3: Capability Payload Decode

Decode:

- capability payload
- temperature value

Expected behavior:

- decode `CAP_TEMPERATURE`
- decode float payload value
- print decoded temperature to Serial
- do not convert the packet into a Registry `CapabilityPayload` yet
- do not update the wireless provider yet

Required Serial visibility:

```text
Cyber32 Packet Temperature: <value>
```

## Phase 4: Decode Validation

Validate rejection behavior for:

- invalid magic
- invalid protocol version
- invalid packet size
- invalid checksum

Expected behavior:

- invalid packet remains receive-visible but decode-rejected
- rejection is printed compactly to Serial
- decode failures do not affect Registry, providers, Logic, API, Runtime, or WirelessService

## Stop Conditions

Stop implementation if any of these are required:

- Registry update
- provider update
- WirelessService ingestion
- Logic involvement
- API change
- Dashboard / Dev Panel / Mini App behavior
- production provisioning
- NVS persistence
- packet decode inside Registry
- unbounded packet storage
- dynamic allocation
- Arduino `String` in core decode paths
- STL containers in core decode paths

## Rules

- Documentation only for this plan.
- Do not modify source code.
- `src/main.cpp` untouched.
- Registry not updated.
- WirelessService not called.
- Logic not involved.
- API unchanged.

## Completion Criteria

Milestone 9.3 is complete when the Base Node can receive the real Sensor Firmware ESP-NOW packet and print:

- valid packet size result
- valid magic result
- valid protocol version result
- decoded `node_id`
- decoded `sequence_id`
- decoded `capability_id`
- decoded `CAP_TEMPERATURE` value

Registry ingestion remains a future milestone.
