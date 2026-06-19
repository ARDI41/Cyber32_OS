# Milestone 8.5 WirelessService Transport Abstraction Plan

## Goal

Define how `WirelessService` can support both `SimEspNowTransportDriver` and `EspNowTransportDriver` without changing capability-first behavior.

This document is documentation only. It does not change source code, does not modify `src/main.cpp`, and does not implement the abstraction yet.

## Reviewed Inputs

- `CYBER32_BIBLE.md`
- `MILESTONE_8_4_ESPNOW_PACKET_DECODE_PLAN.md`
- `MILESTONE_7_5_WIRELESS_SECURITY_AUDIT.md`

## Problem

`WirelessService` currently works with the simulated wireless path.

The real ESP-NOW transport now exists and can:

- initialize ESP-NOW
- register receive callback metadata
- capture raw payload bytes
- decode structured packets into `WirelessPacketHeader`, `WirelessCapabilityValue`, and `WirelessNodeDiagnostics`

A clean transport abstraction is required before real driver integration so `WirelessService` does not become transport-specific and the capability-first model remains intact.

## 1. Ownership Boundaries

### SimEspNowTransportDriver

`SimEspNowTransportDriver` is the simulation/testing transport.

It may:

- expose one pending simulated packet
- support validation injection
- provide packet structs and optional source MAC
- remain the default validation path until real radio integration is explicitly planned

It must not:

- update Registry
- enforce wireless security policy
- know provider selection
- run Logic

### EspNowTransportDriver

`EspNowTransportDriver` is the real radio transport.

It may:

- initialize WiFi STA mode and ESP-NOW
- register ESP-NOW callback
- capture raw bytes
- decode raw bytes into packet structs
- preserve source MAC
- expose one pending structured packet

It must not:

- validate checksum
- validate capability semantics
- enforce allowlist, trust, or sequence policy
- update Registry
- call WirelessService
- run Logic

### WirelessService

`WirelessService` remains the packet policy owner.

It owns:

- checksum validation
- allowlist validation
- trust validation
- sequence validation
- provider update policy
- canonical payload update coordination through Registry public APIs

`WirelessService` must not contain radio-specific logic.

### Registry

Registry is state owner only.

Registry stores:

- provider records
- active provider mappings
- canonical capability payloads
- allowlist records
- diagnostics records

Registry must not parse packets, choose transport behavior, or know whether a packet came from simulation or real ESP-NOW.

### Runtime

Runtime remains scheduler only.

Runtime may schedule:

```text
task.wireless_service.process_packets
task.wireless_service.check_timeouts
```

Runtime must not parse packets, select transports, inspect source MAC, validate checksum, or own provider policy.

## 2. Transport Contract

WirelessService needs a narrow packet transport contract.

Required operations:

```cpp
bool hasReceivedPacket() const;

bool readReceivedPacket(
    WirelessPacketHeader& out_header,
    WirelessCapabilityValue& out_value,
    WirelessNodeDiagnostics& out_diagnostics);

bool readReceivedPacket(
    WirelessPacketHeader& out_header,
    WirelessCapabilityValue& out_value,
    WirelessNodeDiagnostics& out_diagnostics,
    uint8_t out_source_mac[WIRELESS_MAC_ADDRESS_SIZE],
    bool& out_has_source_mac);

void clearReceivedPacket();
```

The contract must preserve:

- one bounded pending packet slot
- optional source MAC
- read-and-clear behavior
- no dynamic allocation
- no Arduino `String`
- no STL containers

WirelessService must not care whether the packet came from:

- simulated transport
- real ESP-NOW transport
- future Bluetooth/BLE/LoRa transport
- future replay/test transport

## 3. Integration Model

Recommended model:

```text
IWirelessPacketTransport
```

Concept only for this phase.

Possible implementation choices:

1. A small abstract interface class with virtual methods.
2. A lightweight adapter struct with function pointers and a context pointer.
3. Separate adapter classes for simulated and real transport with the same public contract.

Preferred ESP32-safe direction:

```text
fixed adapter object
function pointers
void* context
no heap allocation
```

Rationale:

- avoids dynamic allocation
- avoids STL
- avoids requiring transport inheritance in existing drivers
- allows simulated and real drivers to remain concrete
- keeps WirelessService transport-agnostic

## 4. Packet Flow

### Real ESP-NOW Path

```text
ESP-NOW callback
-> raw capture
-> decode
-> transport contract
-> WirelessService
-> checksum validation
-> allowlist validation
-> trust validation
-> sequence validation
-> device update
-> provider update
-> canonical payload update
```

The driver owns only the first three transport steps.

WirelessService owns all policy steps.

### Simulation Path

```text
Sim transport
-> transport contract
-> WirelessService
-> checksum validation
-> allowlist validation
-> trust validation
-> sequence validation
-> device update
-> provider update
-> canonical payload update
```

The same `WirelessService` code path must process both sources.

## 5. Source MAC Handling

Source MAC is transport metadata.

Rules:

- transport returns source MAC when available
- WirelessService or a future adapter maps MAC to allowlist/node identity
- Registry stores allowlist and diagnostics records
- Logic never sees MAC
- API may expose MAC only through future diagnostics, not normal capability reads

Simulation may provide source MAC through validation injection.

Real ESP-NOW provides source MAC from the receive callback.

## 6. Validation Requirements

Future validation must prove:

- simulated transport and real transport adapter expose the same contract
- WirelessService uses the same code path for both transports
- checksum validation remains in WirelessService
- allowlist validation remains in WirelessService
- trust validation remains in WirelessService
- sequence validation remains in WirelessService
- provider update behavior is identical
- failover behavior is identical
- canonical payload behavior is identical
- Runtime only invokes WirelessService task callbacks
- Registry remains transport-agnostic
- Logic remains provider-blind

Validation should include:

1. Simulated packet still updates wireless provider through existing path.
2. Real decoded packet can be read through adapter.
3. Real decoded packet fails checksum when corrupted.
4. Unknown real source MAC is rejected before provider update.
5. Allowed/trusted real source MAC can update provider.
6. Duplicate sequence from either transport is rejected.
7. Canonical `CAP_TEMPERATURE` behavior is unchanged.

## 7. Stop Conditions

Stop implementation if:

- `WirelessService` contains transport-specific branches such as `if real ESP-NOW`
- Registry knows transport type
- Logic knows transport type
- API knows transport type for normal capability reads
- Runtime chooses transport behavior
- duplicate sequence policy appears in transport layer
- allowlist policy appears in transport layer
- trust policy appears in transport layer
- checksum validation moves into transport layer
- provider updates happen inside transport drivers
- dynamic allocation is introduced
- Arduino `String` is introduced
- STL containers are introduced

## Recommended Implementation Order

1. Define a bounded transport adapter contract.
2. Add adapter support to `WirelessService` without removing existing simulated attachment yet.
3. Add `SimEspNowTransportDriver` adapter.
4. Validate simulated path still passes through the adapter.
5. Add `EspNowTransportDriver` adapter.
6. Validate real decoded packet read through the same service path using test hooks.
7. Keep simulated transport as default validation baseline.
8. Add real-radio hardware validation only after adapter behavior passes.
9. Audit layer boundaries before real wireless node testing.

## Assessment

**PASS with warnings**

The architecture is ready for a bounded transport abstraction because:

- simulated transport already exposes structured packet reads
- real ESP-NOW transport now exposes structured packet reads after decode
- WirelessService already owns wireless security policy
- Registry remains state-only
- Runtime remains scheduler-only
- Logic remains provider-blind

Warnings:

- real ESP-NOW integration must not move checksum, allowlist, trust, or sequence policy into the driver
- source MAC mapping must be carefully handled without exposing MAC to Logic
- simulated validation must remain the default baseline until real hardware tests are explicitly planned
- future transport abstraction must avoid heap allocation and STL containers

## Final Assessment

`WirelessService` should receive packets through a narrow bounded transport contract and apply the same policy pipeline regardless of packet origin.

The target architecture is:

```text
transport-specific receive
-> common packet transport contract
-> WirelessService policy
-> Registry state
-> capability-first Logic/API reads
```

This preserves Cyber32's core rule: Logic sees `CAP_*`, not transport.
