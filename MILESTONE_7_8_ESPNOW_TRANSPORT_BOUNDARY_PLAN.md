# Milestone 7.8 ESP-NOW Transport Boundary Plan

## Goal

Design the boundary for real ESP-NOW transport before adding `WiFi.h` or `esp_now.h`.

This document is documentation only. It does not change source code, does not modify `src/main.cpp`, and does not add real ESP-NOW, WiFi, or radio behavior.

## Reviewed Inputs

- `CYBER32_BIBLE.md`
- `MILESTONE_7_7_ESPNOW_IDENTITY_PAIRING_PLAN.md`
- `MILESTONE_7_5_WIRELESS_SECURITY_AUDIT.md`
- `MILESTONE_6_3_WIRELESS_SERVICE_PLAN.md`

## Problem

Cyber32 has a simulated ESP-NOW-style transport and a wireless security flow:

- bounded packet structs
- checksum validation
- allowlist enforcement
- trust enforcement
- duplicate sequence protection
- provider/canonical payload protection

Before real ESP-NOW code is added, the transport boundary must be explicit so radio-specific includes and callbacks do not leak into Registry, Runtime, Logic, API, Services, or validation.

## 1. Real ESP-NOW Driver Location

Real ESP-NOW transport belongs only under:

```text
src/drivers/communication/espnow_transport_driver.h
src/drivers/communication/espnow_transport_driver.cpp
```

This driver is the only planned location for direct ESP-NOW and WiFi platform includes:

```cpp
#include <WiFi.h>
#include <esp_now.h>
```

No other Cyber32 layer should include those headers.

## 2. Driver Responsibilities

The real ESP-NOW transport driver may:

- initialize WiFi radio mode required for ESP-NOW
- initialize ESP-NOW
- register ESP-NOW receive callback
- receive raw ESP-NOW payload bytes
- capture sender MAC from ESP-NOW callback metadata
- decode raw payload into bounded packet structs only if the packet format is transport-level
- store one bounded pending packet
- expose whether a packet is pending
- expose read methods for the pending packet
- expose the source MAC associated with the pending packet
- support failure mode for validation, if useful

Required packet slot behavior:

- one pending packet slot
- no queue in v1
- if slot occupied, reject or drop new packet according to a documented v1 policy
- no dynamic allocation

## 3. Driver Must Not

The real ESP-NOW driver must not:

- parse capability semantics
- decide whether a capability is supported
- update Registry
- register providers
- choose active providers
- enforce trust policy
- enforce allowlist policy
- enforce sequence policy beyond storing raw packet facts
- run Logic
- expose API
- call Runtime update
- know Dashboard or Mobile Studio concepts
- allocate dynamic packet buffers

The driver is transport only.

## 4. Shared Transport Interface Expectation

The simulated and real transport drivers should eventually expose compatible behavior.

Current simulated behavior:

```cpp
bool begin();
bool initialized() const;
bool injectReceivedCapabilityValue(...);
bool hasReceivedPacket() const;
bool readReceivedPacket(...);
void clearReceivedPacket();
```

Future shared expectations:

```cpp
bool begin();
bool initialized() const;
bool hasReceivedPacket() const;
bool readReceivedPacket(
    WirelessPacketHeader& out_header,
    WirelessCapabilityValue& out_value,
    WirelessNodeDiagnostics& out_diagnostics);
bool readReceivedPacketSourceMac(
    uint8_t out_mac[WIRELESS_MAC_ADDRESS_SIZE]) const;
void clearReceivedPacket();
```

Alternative:

- return source MAC as part of a bounded packet envelope
- keep simulated and real driver APIs aligned through a small adapter

The interface should remain fixed-size and allocation-free.

## 5. MAC Handling

Source MAC comes from the ESP-NOW receive callback.

Expected flow:

```text
ESP-NOW callback
-> real transport driver captures source MAC
-> real transport driver stores bounded packet structs + source MAC
-> WirelessService reads packet + source MAC
-> WirelessService or adapter maps MAC to allowlist/node identity
-> Registry stores allowlist records
```

Rules:

- Registry stores MAC identity in bounded records.
- WirelessService enforces allowlist and trust policy.
- Logic never sees MAC.
- Runtime never sees MAC.
- API may expose MAC only through future diagnostics, not normal capability reads.

## 6. Packet Format

The real transport must carry the bounded Cyber32 wireless packet format:

- `WirelessPacketHeader`
- `WirelessCapabilityValue`
- `WirelessNodeDiagnostics`

Packet constraints:

- maximum payload size must remain ESP-NOW-safe
- current architecture target: `<= 250 bytes`
- no JSON
- no variable-length heap payloads
- checksum is included in `WirelessPacketHeader`

The packet should include enough information for WirelessService and virtual devices to validate:

- magic
- protocol version
- packet type
- sequence ID
- node ID
- payload length
- checksum
- capability ID
- payload type
- value fields
- diagnostics fields

## 7. Runtime Behavior

Runtime remains scheduler only.

Runtime may call task callbacks such as:

```text
task.wireless_service.process_packets
task.wireless_service.check_timeouts
```

Runtime must not:

- call ESP-NOW directly
- include `WiFi.h`
- include `esp_now.h`
- parse raw packets
- inspect MAC addresses
- choose providers
- own trust policy
- own pairing policy

## 8. Validation

Core validation should continue to use the simulated transport driver by default.

Real driver validation should be separate and hardware-gated.

Validation expectations:

- simulated driver remains default path for CI/core validation
- real driver can be compiled separately later
- no real radio required for core validation
- fake source MAC injection may be added to simulated driver later
- hardware tests must not be required for normal architecture validation

## 9. Stop Conditions

Stop implementation if any of these occur:

- `WiFi.h` is included outside `src/drivers/communication/espnow_transport_driver.*`
- `esp_now.h` is included outside `src/drivers/communication/espnow_transport_driver.*`
- Registry parses ESP-NOW packets
- Runtime parses ESP-NOW packets
- Logic sees MAC address
- Logic sees packet type, checksum, or sequence ID
- API exposes raw ESP-NOW packet bytes
- Driver updates Registry
- Driver chooses providers
- Driver enforces trust policy
- dynamic allocation is introduced
- Arduino `String` is introduced
- STL containers are introduced
- validation requires real radio for normal build validation

## Final Assessment

The real ESP-NOW transport boundary should be narrow:

```text
real ESP-NOW driver
-> bounded packet + source MAC
-> WirelessService policy
-> Registry state
-> capability-first reads
```

This keeps Cyber32 local-first, capability-first, bounded, and ready for real ESP-NOW work without contaminating Runtime, Registry, Logic, API, or validation with radio-specific behavior.
