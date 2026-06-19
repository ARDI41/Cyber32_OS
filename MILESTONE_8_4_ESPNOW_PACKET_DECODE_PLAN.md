# Milestone 8.4 ESP-NOW Packet Decode Plan

## Goal

Design bounded decode from raw ESP-NOW payload bytes into Cyber32 wireless packet structs.

This document is documentation only. It does not change source code, does not modify `src/main.cpp`, and does not add packet decode behavior yet.

## Reviewed Inputs

- `CYBER32_BIBLE.md`
- `MILESTONE_7_8_ESPNOW_TRANSPORT_BOUNDARY_PLAN.md`
- current `EspNowTransportDriver` raw packet capture state

`MILESTONE_8_3_RAW_PACKET_CAPTURE.md` does not currently exist, so this plan uses the implemented driver state as the Milestone 8.3 reference.

## Problem

`EspNowTransportDriver` can capture raw ESP-NOW payload bytes into a bounded buffer, but it does not yet decode those bytes into:

- `WirelessPacketHeader`
- `WirelessCapabilityValue`
- `WirelessNodeDiagnostics`

The next step must decode only the Cyber32 packet binary layout. Capability meaning, trust, allowlist, checksum enforcement, sequence policy, provider updates, and canonical payload updates remain outside the driver.

## 1. Decode Ownership

`EspNowTransportDriver` may decode raw bytes into Cyber32 wireless packet structs because this is transport packet layout work.

The driver may:

- verify that raw length matches the expected binary packet size
- copy bytes into local fixed structs
- store one pending structured packet
- preserve source MAC with the decoded packet
- expose decoded packet through existing `readReceivedPacket(...)` methods

The driver must not:

- validate capability semantics
- decide whether a capability is supported
- validate allowlist or trust state
- enforce sequence duplicate policy
- update Registry
- call WirelessService
- call Runtime
- expose API
- run Logic

## 2. Packet Binary Layout

Milestone 8.4 should define a fixed-order binary layout compatible with existing structs.

V1 packet order:

```text
WirelessPacketHeader
WirelessCapabilityValue
WirelessNodeDiagnostics
```

The binary payload is a compact Cyber32 packet, not JSON.

Rules:

- fixed field order
- fixed maximum size
- no dynamic allocation
- no Arduino `String`
- no STL containers
- no variable-length heap buffers
- raw ESP-NOW payload must remain `<= WIRELESS_MAX_PACKET_SIZE`

Expected decoded size:

```text
sizeof(WirelessPacketHeader)
+ sizeof(WirelessCapabilityValue)
+ sizeof(WirelessNodeDiagnostics)
```

Implementation should use explicit bounded byte copy helpers rather than assuming unaligned pointer casts are safe on all platforms.

## 3. Decode Behavior

Decode entry point should be internal to the driver at first.

Recommended public behavior:

```text
raw payload captured
-> decode pending raw payload
-> structured packet slot becomes pending
-> raw payload is cleared after successful decode
-> readReceivedPacket(...) returns decoded structs
```

Required behavior:

- if no raw payload exists, decode returns false
- if raw length does not equal expected packet size, decode returns false
- if raw length is valid, copy bytes into `WirelessPacketHeader`
- then copy bytes into `WirelessCapabilityValue`
- then copy bytes into `WirelessNodeDiagnostics`
- source MAC from raw capture is preserved with the structured pending packet
- `hasReceivedPacket()` becomes true only after successful decode

The driver must not inspect capability ID meaning, packet type meaning, checksum validity, trust state, or provider eligibility during decode.

## 4. Raw Payload Clear Policy

V1 policy:

- successful decode clears the raw payload slot
- successful decode creates one structured pending packet
- failed decode keeps no structured pending packet
- failed decode should clear invalid raw payload to avoid repeated decode attempts on the same bad bytes

Rationale:

- the driver has one bounded slot
- bad packets should fail closed
- repeated attempts on invalid raw bytes provide no value
- future diagnostics can count decode failures in a bounded counter if needed

## 5. Failure Behavior

Invalid cases must fail closed:

- no raw payload pending
- null output pointer if a public decode helper is added
- raw length is `0`
- raw length is less than expected packet size
- raw length is greater than expected packet size
- raw length is greater than `WIRELESS_MAX_PACKET_SIZE`
- source MAC unavailable is allowed, but `out_has_source_mac` must be false

On failure:

- no structured packet is created
- `hasReceivedPacket()` remains false
- `readReceivedPacket(...)` remains false
- Registry is unchanged
- WirelessService is not called
- Runtime is unchanged
- Logic remains provider-blind

## 6. Source MAC Preservation

The source MAC captured during raw receive remains transport metadata.

Successful decode must copy:

```text
pending raw source MAC
-> pending structured packet source MAC
```

If raw capture had no source MAC:

```text
pending structured packet has_source_mac = false
```

Normal packet payload bytes must not include or depend on the source MAC. WirelessService or a future adapter maps source MAC to node identity later.

## 7. Validation Requirements

Validation should use public driver APIs and test hooks only.

Required validation:

1. Valid raw packet decode:
   - build bounded raw bytes matching the V1 layout
   - inject with `injectRawPayloadForTest(...)`
   - call decode path
   - expect `hasReceivedPacket() == true`
   - call `readReceivedPacket(...)`
   - expect decoded `WirelessPacketHeader` fields match
   - expect decoded `WirelessCapabilityValue` fields match
   - expect decoded `WirelessNodeDiagnostics` fields match
   - expect source MAC is preserved

2. Invalid length rejected:
   - shorter than expected packet size
   - longer than expected packet size
   - `0`
   - greater than `WIRELESS_MAX_PACKET_SIZE`
   - expect no structured pending packet

3. Empty path:
   - decode with no raw payload returns false
   - `readReceivedPacket(...)` remains false

4. Boundary preservation:
   - no WirelessService integration
   - no Registry updates
   - no Runtime parsing
   - no Logic exposure

## 8. Stop Conditions

Stop implementation if any of these occur:

- Registry parses raw ESP-NOW bytes
- Runtime parses raw ESP-NOW bytes
- Logic sees raw packet bytes
- Logic sees MAC, node, or provider details
- WirelessService is bypassed for provider policy
- driver updates Registry
- driver validates capability semantics
- decode requires dynamic allocation
- decode requires Arduino `String`
- decode requires STL containers
- packet size can exceed `WIRELESS_MAX_PACKET_SIZE`

## Final Assessment

Milestone 8.4 should introduce only a bounded transport decode step:

```text
raw ESP-NOW bytes
-> EspNowTransportDriver fixed binary decode
-> pending WirelessPacketHeader / WirelessCapabilityValue / WirelessNodeDiagnostics
-> later WirelessService policy
```

This keeps the driver responsible for packet layout only while preserving Cyber32's capability-first, Registry-owned, Runtime-scheduler-only architecture.
