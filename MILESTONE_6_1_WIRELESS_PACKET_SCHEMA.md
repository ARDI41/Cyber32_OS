# Milestone 6.1 Wireless Packet Schema

## Goal

Define the bounded ESP-NOW packet schema for Cyber32 wireless nodes before implementation.

This document is documentation only. It does not change source code, does not modify `src/main.cpp`, and does not add real ESP-NOW behavior.

Reviewed:

- `MILESTONE_6_ESPNOW_ARCHITECTURE_PLAN.md`
- `PRODUCTION_BOOTSTRAP_PLAN.md`
- `CAPABILITY_CATALOG.md`

## 1. Packet Goals

Cyber32 ESP-NOW packets must be:

- compact
- fixed-size or bounded-size
- ESP32-friendly
- capability-first
- safe to parse with static buffers
- independent of Dashboard/UI concerns

Hard packet rules:

- no JSON
- no dynamic allocation
- no STL containers
- no heap-owned strings
- no unbounded arrays
- no packet history in Registry
- max ESP-NOW packet payload must be `<= 250` bytes

The packet schema must support wired/wireless equivalence:

```text
wired temperature sensor -> CAP_TEMPERATURE
wireless temperature node -> CAP_TEMPERATURE
```

Logic must see only the capability state, not the transport.

## 2. Packet Header

Every Cyber32 wireless packet starts with a fixed header.

Recommended v1 header:

| Field | Type | Size | Description |
|---|---:|---:|---|
| `magic` | `uint16_t` | 2 | Cyber32 packet marker. |
| `protocol_version` | `uint8_t` | 1 | Wireless protocol version. |
| `packet_type` | `uint8_t` | 1 | Packet type code. |
| `flags` | `uint8_t` | 1 | Bit flags for diagnostics/security/ack behavior. |
| `sequence_id` | `uint16_t` | 2 | Per-node sequence counter. |
| `node_id` | `uint32_t` | 4 | Compact node identity. |
| `payload_length` | `uint8_t` | 1 | Number of bytes after the header and before checksum. |

Header size:

```text
12 bytes
```

Recommended magic:

```text
0x4332
```

`0x4332` represents `C2` for Cyber32. The exact constant must be defined before implementation and remain stable after release.

Packet layout:

```text
header
payload[payload_length]
checksum
```

Packet size rule:

```text
header + payload_length + checksum <= 250 bytes
```

## 3. Packet Types

Packet type values must be compact and stable.

Recommended v1 values:

| Packet Type | Code | Direction | Purpose |
|---|---:|---|---|
| `NODE_ANNOUNCE` | `1` | node -> host | Announce node identity and compact capability metadata. |
| `NODE_HEARTBEAT` | `2` | node -> host | Report node alive state and diagnostics. |
| `CAPABILITY_VALUE` | `3` | node -> host | Report a capability value. |
| `CAPABILITY_STATUS` | `4` | node -> host | Report availability/stale/error state for a capability. |
| `ERROR_REPORT` | `5` | node -> host | Report compact node or capability error. |
| `COMMAND_REQUEST` | `16` | host -> node | Future actuator command request. |
| `COMMAND_ACK` | `17` | node -> host | Future actuator command acknowledgement. |
| `COMMAND_RESULT` | `18` | node -> host | Future actuator command result. |

v1 implementation should start with:

```text
NODE_ANNOUNCE
NODE_HEARTBEAT
CAPABILITY_VALUE
CAPABILITY_STATUS
ERROR_REPORT
```

Future command packet types must remain disabled until wireless actuator safety is separately designed and validated.

## 4. Node ID Format

Preferred v1 node ID:

```text
uint32_t node_id
```

Rules:

- stable across reboot if possible
- unique within the local Cyber32 wireless network
- not used by Logic
- not used as a capability ID
- may be derived from MAC address, stored configuration, or manufacturing ID

Alternative bounded char ID:

```text
char node_id[16]
```

If bounded char ID is used:

- ASCII only
- null-terminated when possible
- full field must be bounded
- no dynamic allocation

v1 preference:

```text
uint32_t node_id
```

API may expose node ID for diagnostics, but Logic must never bind to it.

## 5. Capability Encoding

Capability IDs are canonical `CAP_*` contracts.

v1 preferred encoding:

```text
char capability_id[24]
```

Rules:

- must start with `CAP_`
- uppercase ASCII letters, numbers, underscores
- null-terminated when shorter than the field
- no heap strings
- unknown capability may be ignored or rejected safely

Future compact encoding may use:

```text
uint16_t capability_code
```

Compact codes may be introduced only after a stable capability-code table is documented. Until then, bounded char IDs are clearer and safer.

## 6. Payload Encoding

Supported payload types:

| Payload Type | Code | Fields Used |
|---|---:|---|
| `NONE` | `0` | no value |
| `FLOAT` | `1` | `value_float` |
| `INT` | `2` | `value_int` |
| `BOOLEAN` | `3` | `value_int`, `0` or `1` |

Unsupported in v1 packets:

- object payloads
- strings as values
- binary payloads
- arrays
- nested structures

If a capability requires object, string, stream, or binary data, Milestone 6.1 must stop and define a new bounded schema before implementation.

## 7. Sensor Value Packet

`CAPABILITY_VALUE` payload for a simple sensor:

| Field | Type | Size | Description |
|---|---:|---:|---|
| `capability_id` | `char[24]` | 24 | Canonical `CAP_*` ID. |
| `payload_type` | `uint8_t` | 1 | `NONE`, `FLOAT`, `INT`, or `BOOLEAN`. |
| `value_float` | `float` | 4 | Float value, if used. |
| `value_int` | `int32_t` | 4 | Integer or boolean value, if used. |
| `unit` | `char[16]` | 16 | Bounded unit string. |
| `quality` | `uint8_t` | 1 | Quality code. |
| `error_code` | `char[24]` | 24 | Compact `ERR_*` or `"none"`. |

Payload size:

```text
74 bytes
```

With 12-byte header and 2-byte checksum:

```text
88 bytes total
```

Quality codes:

| Quality | Code | Meaning |
|---|---:|---|
| `valid` | `0` | Value is usable. |
| `stale` | `1` | Value is old. |
| `unavailable` | `2` | Value is unavailable. |
| `unknown` | `3` | Quality cannot be determined. |
| `degraded` | `4` | Value is usable with reduced confidence. |

Unit rules:

- use canonical unit strings when possible
- examples: `"degree_celsius"`, `"meter"`, `"percent"`, `"volt"`, `"dBm"`
- unit field must be bounded
- if unit is not applicable, use `"none"`

Error rules:

- use `"none"` when no error exists
- use compact canonical `ERR_*` IDs when available
- unknown errors must fail closed into a supported error category

## 8. Battery Fields

Battery diagnostics may be sent in `NODE_HEARTBEAT`, `NODE_ANNOUNCE`, or as separate `CAPABILITY_VALUE` packets.

Recommended compact heartbeat battery fields:

| Field | Type | Size | Description |
|---|---:|---:|---|
| `battery_present` | `uint8_t` | 1 | `1` if battery data is available. |
| `battery_level_percent` | `float` | 4 | `0.0F..100.0F`. |
| `battery_voltage` | `float` | 4 | volts, optional. |

Rules:

- `CAP_BATTERY_LEVEL` uses unit `"percent"`
- `CAP_BATTERY_VOLTAGE` uses unit `"volt"`
- unavailable battery data must not be treated as zero
- if `battery_present == 0`, battery values are ignored
- battery diagnostics do not replace the main sensor capability

Separate capability packets may be emitted:

```text
CAP_BATTERY_LEVEL
CAP_BATTERY_VOLTAGE
```

## 9. Signal Fields

Signal diagnostics may be carried in heartbeat packets or as `CAP_SIGNAL_STRENGTH`.

Recommended fields:

| Field | Type | Size | Description |
|---|---:|---:|---|
| `signal_known` | `uint8_t` | 1 | `1` if signal value is known. |
| `rssi_dbm` | `int16_t` | 2 | RSSI in dBm, if available. |
| `link_quality_percent` | `float` | 4 | `0.0F..100.0F`, if calculated. |

Rules:

- RSSI may be unavailable depending on ESP-NOW/Arduino support
- if RSSI is unavailable, expose `quality = "unknown"` through capability state
- signal quality is diagnostic, not required for Logic
- poor signal may cause Service to mark node stale/degraded
- signal data must remain compact

Preferred capability:

```text
CAP_SIGNAL_STRENGTH
```

## 10. last_seen Handling

`last_seen_ms` is calculated locally by the receiver when a packet is received.

Rules:

- remote node time is not authoritative for `last_seen_ms`
- packets do not need to transmit `last_seen_ms`
- local Wireless Service records receive time using HAL/Runtime time
- API may expose latest `last_seen_ms` as diagnostics
- Registry stores latest compact state only
- no packet history is stored

Example:

```text
packet received at host uptime 123456 ms
-> node.last_seen_ms = 123456
```

## 11. Sequence ID

`sequence_id` is a per-node packet counter.

Rules:

- used to detect duplicate packets
- used for basic replay protection
- wrap is allowed
- receiver must handle wrap safely
- duplicate packet should be ignored safely
- sequence state must be bounded per node

Recommended type:

```text
uint16_t sequence_id
```

When sequence wraps:

```text
65535 -> 0
```

The receiver must not treat wrap as fatal by itself.

## 12. Checksum

Every packet should include a checksum.

Recommended v1 checksum:

```text
uint16_t checksum
```

Checksum scope:

```text
header + payload
```

Acceptable v1 choices:

- simple additive checksum for initial simulated validation
- CRC-16 placeholder before production hardware

Rules:

- invalid checksum must fail closed
- invalid packet must not update Registry
- invalid packet must not register a node
- invalid packet must not execute command behavior
- invalid packet may increment a compact error counter

Production should prefer CRC-16 over a simple additive checksum.

## 13. Packet Validation Rules

Every received packet must be validated before it affects PNP, Registry, Services, Logic, or API.

Validation checklist:

- `magic` matches Cyber32 packet marker
- `protocol_version` is supported
- `packet_type` is known
- `payload_length` fits within packet buffer
- total packet size is `<= 250` bytes
- checksum is valid
- node ID is valid
- sequence ID is not a duplicate, unless duplicate handling explicitly allows it
- capability count is within maximum for announce packets
- `capability_id` starts with `CAP_`
- `capability_id` contains only allowed characters
- payload type is supported
- unit string is bounded
- error code is bounded
- unknown packet types are rejected safely
- unsupported capabilities are ignored or rejected safely

Validation failure behavior:

- do not update Registry
- do not expose capability state
- do not execute commands
- optionally record compact error state
- optionally emit compact error event

## 14. Example Packets

Examples are logical field views, not JSON wire format.

### Wireless Temperature Value

Header:

```text
magic = 0x4332
protocol_version = 1
packet_type = CAPABILITY_VALUE
flags = 0
sequence_id = 42
node_id = 0x00001001
payload_length = 74
```

Payload:

```text
capability_id = CAP_TEMPERATURE
payload_type = FLOAT
value_float = 22.4
value_int = 0
unit = degree_celsius
quality = valid
error_code = none
```

Receiver behavior:

```text
validate packet
calculate local last_seen_ms
convert to CapabilityPayload
Wireless Service updates Registry CAP_TEMPERATURE
Logic sees CAP_TEMPERATURE only
```

### Heartbeat With Battery

Header:

```text
packet_type = NODE_HEARTBEAT
sequence_id = 43
node_id = 0x00001001
```

Payload:

```text
battery_present = 1
battery_level_percent = 78.0
battery_voltage = 3.92
signal_known = 1
rssi_dbm = -62
link_quality_percent = 84.0
status = available
```

Receiver behavior:

```text
update last_seen_ms locally
update compact node diagnostics
optionally update CAP_BATTERY_LEVEL
optionally update CAP_BATTERY_VOLTAGE
optionally update CAP_SIGNAL_STRENGTH
```

### Node Announce

Header:

```text
packet_type = NODE_ANNOUNCE
sequence_id = 1
node_id = 0x00001001
```

Payload:

```text
node_role = sensor
metadata_level = 1
capability_count = 3
capability_0 = CAP_TEMPERATURE
capability_1 = CAP_BATTERY_LEVEL
capability_2 = CAP_SIGNAL_STRENGTH
battery_present = 1
security_mode = allowlisted
```

Receiver behavior:

```text
PNP detects wireless node
PNP validates metadata
PNP Registration creates virtual Module, virtual Device, and Capability records
Registry exposes CAP_TEMPERATURE and diagnostics capabilities
Logic remains capability-only
```

## 15. Stop Conditions

Stop implementation and review architecture if:

- packet requires heap allocation
- packet requires JSON
- packet can exceed ESP-NOW limit
- packet requires STL containers
- packet requires Arduino `String`
- packet needs unbounded strings
- packet stores history in Registry
- Logic needs to know wireless origin
- Registry parses raw packets
- API sends raw ESP-NOW packets directly
- Runtime owns packet parsing or wireless policy
- unknown wireless actuator capability can register automatically
- checksum failure can update state
- duplicate packet can execute command behavior

## Final Assessment

This schema keeps Cyber32 ESP-NOW packets bounded, parseable, and capability-first.

The first implementation should use the simplest safe path:

```text
NODE_ANNOUNCE
NODE_HEARTBEAT
CAPABILITY_VALUE for CAP_TEMPERATURE
optional CAP_BATTERY_LEVEL
optional CAP_SIGNAL_STRENGTH
```

Wireless transport must remain invisible to Logic. Registry stores only latest compact state. Services own packet processing policy, stale/lost handling, and future command safety.
