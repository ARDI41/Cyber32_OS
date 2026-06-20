# Milestone 9.1 Phase 2 Wireless Temperature Node Sender Plan

## Goal

Define the minimal ESP32 wireless temperature sender for the first real `CAP_TEMPERATURE` packet.

This sender exists only to emit a valid bounded Cyber32 wireless packet for the base-node receive test.

## Sender Node Responsibilities

The sender ESP32 must:

1. initialize WiFi STA mode
2. initialize ESP-NOW
3. know the base station MAC address
4. build a Cyber32 wireless packet
5. calculate packet checksum
6. send one `CAP_TEMPERATURE` packet

The sender should be as small and deterministic as possible for the first hardware proof.

## Packet Fields

The sender must build:

- `WirelessPacketHeader`
- `WirelessCapabilityValue`
- `WirelessNodeDiagnostics`

Required packet values:

```text
node_id = 1001
sequence_id = fixed or incrementing test value
capability_id = CAP_TEMPERATURE
payload_type = FLOAT
value_float = 24.5F
checksum = calculateWirelessPacketChecksum(...)
```

Diagnostics may use placeholders:

```text
battery_present = false
battery_level_percent = 0.0F
battery_voltage = 0.0F
signal_quality_percent = 0.0F
```

The packet must be serialized in exact decode order:

```text
WirelessPacketHeader
WirelessCapabilityValue
WirelessNodeDiagnostics
```

No JSON is allowed.

No dynamic allocation is allowed.

## Sender Boundary

The sender is not a full Cyber32 OS node yet.

The sender does not run:

- Registry
- Runtime
- Logic
- Provider selection
- PNP
- API
- Services

The sender only emits a valid bounded Cyber32 packet.

This keeps Milestone 9.1 focused on proving the first real wireless `CAP_TEMPERATURE` receive path.

## Test Phases

### Phase 1: One-Shot Packet

Sender transmits one deterministic packet:

```text
node_id = 1001
sequence_id = 1
value_float = 24.5F
```

Expected base-node result:

- packet received
- checksum validated
- MAC identity validated
- provider payload updated
- accepted diagnostic count increments

### Phase 2: Repeated Packet With Incrementing Sequence

Sender transmits periodic packets:

```text
sequence_id = 1, 2, 3, ...
```

Expected:

- each packet accepted
- provider payload updates
- accepted packet count increments

### Phase 3: Duplicate Sequence Test

Sender deliberately repeats a sequence ID.

Expected:

- first packet accepted
- duplicate rejected
- duplicate sequence reject counter increments
- provider payload remains unchanged after duplicate

### Phase 4: Wrong Node ID Test

Sender sends a packet with unexpected `node_id` if needed.

Expected:

- packet rejected by MAC/node or allowlist policy
- provider payload remains unchanged
- relevant diagnostic counter increments

## Required Manual Setup

Before sender test:

1. Read sender ESP32 MAC address.
2. Add sender MAC to the base allowlist.
3. Confirm allowlist entry:
   - `node_id = 1001`
   - `allow_state = ALLOWED`
   - `trust_state = TRUSTED`
   - sender MAC copied exactly
4. Read base ESP32 MAC address.
5. Configure sender ESP-NOW peer with base MAC.

## Sender Packet Build Flow

Expected sender flow:

```text
WiFi.mode(WIFI_STA)
-> esp_now_init()
-> add base ESP32 peer
-> fill WirelessPacketHeader
-> fill WirelessCapabilityValue
-> fill WirelessNodeDiagnostics
-> calculate checksum
-> pack bytes in fixed order
-> esp_now_send(base_mac, payload, payload_length)
```

## Stop Conditions

Stop implementation if sender requires:

- Dashboard
- Mobile Studio
- cloud
- actuator behavior
- persistence
- full Cyber32 Registry
- full Cyber32 Runtime
- Cyber32 Logic
- Cyber32 API
- dynamic allocation
- Arduino `String`
- STL containers
- source changes to `src/main.cpp`

## Final Assessment

The sender should be a minimal deterministic ESP32 ESP-NOW packet emitter.

It should prove that a real ESP32 can send the exact bounded Cyber32 wireless packet format expected by the base node.
