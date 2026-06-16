# Milestone 6 ESP-NOW Architecture Plan

## Goal

Design Cyber32 wireless capability transport using ESP-NOW.

This document is architecture planning only. It does not change source code, does not modify `src/main.cpp`, and does not add WiFi, Dashboard, cloud, or real hardware behavior.

Cyber32 must support:

- wired sensors
- wireless sensors
- wired actuators
- future wireless actuators

Core rule:

```text
Logic must not know whether a capability is wired or wireless.
```

Example:

```text
wired temperature sensor
-> CAP_TEMPERATURE

wireless temperature node
-> CAP_TEMPERATURE

Logic sees identical capability state.
```

## Reviewed Documents

- `ARCHITECTURE.md`
- `PRODUCTION_BOOTSTRAP_PLAN.md`
- `CAPABILITY_CATALOG.md`
- `PNP_DISCOVERY_FLOW.md`
- `MILESTONE_5_ARCHITECTURE_AUDIT.md`

## Fixed Layer Placement

ESP-NOW support must fit the existing layer order:

```text
HAL
-> Drivers
-> Devices
-> Modules
-> PNP
-> Registry
-> Runtime
-> Services
-> Logic
-> API
-> Dashboard
```

Recommended placement:

| Concept | Layer | Responsibility |
|---|---|---|
| ESP32 radio primitives | HAL | minimal platform abstraction for ESP-NOW send/receive hooks |
| ESP-NOW packet transport | Drivers | packet send/receive, peer table, radio status |
| Wireless node endpoint | Devices | converts received packets into capability payloads and node state |
| Virtual wireless module | Modules | metadata wrapper for remote node capabilities |
| Wireless discovery | PNP | detects and validates wireless node announcements |
| Capability storage | Registry | stores virtual module/device/capability records and latest payloads |
| Scheduling and timeout checks | Runtime | runs bounded wireless service tasks |
| Wireless service | Services | owns packet processing policy, last-seen rules, timeout handling |
| Automation | Logic | queries `CAP_*` only |
| External visibility | API | exposes capability state and optional wireless diagnostics |

## 1. Wireless Node Concept

A wireless node is a remote ESP32-compatible Cyber32 participant that publishes one or more capabilities over ESP-NOW.

Examples:

- wireless temperature node exposing `CAP_TEMPERATURE`
- wireless distance node exposing `CAP_DISTANCE`
- wireless battery-powered relay node exposing `CAP_RELAY_CONTROL` in a future actuator phase

Wireless node rules:

- A node is not a new top-level architecture layer.
- A node is represented locally as a virtual Module and virtual Device.
- Node transport details are not visible to Logic.
- Node display names are metadata only.
- Node capability IDs must use the canonical `CAP_*` catalog.

Required node metadata:

```text
node_id
node_type
protocol_version
metadata_level
capability_count
capability_ids
battery_present
security_mode
```

## 2. ESP-NOW Transport Layer

ESP-NOW is the first planned wireless transport for Cyber32 ESP32 v1.

Transport responsibilities:

- initialize ESP-NOW radio support
- manage bounded peer slots
- receive compact packets
- send compact acknowledgements when required
- expose signal/transport status upward
- reject oversized packets

Transport must not:

- register capabilities
- write Registry
- run Logic
- expose API
- own actuator command policy
- store large payload history

Recommended future files:

```text
src/hal/wireless/
src/drivers/communication/espnow_transport_driver.*
src/devices/communication/wireless_node_device.*
src/modules/communication/wireless_node_module.*
src/services/wireless/wireless_capability_service.*
```

## 3. Packet Structure

ESP-NOW packets must be compact and bounded.

Maximum v1 packet size:

```text
<= 250 bytes payload
```

Canonical packet fields:

```text
magic
protocol_version
packet_type
sequence_id
node_id
capability_id
payload_type
timestamp_ms
value_float
value_int
status_flags
error_code
checksum
```

Allowed packet types:

```text
NODE_ANNOUNCE
NODE_HEARTBEAT
CAPABILITY_VALUE
CAPABILITY_STATUS
COMMAND_REQUEST
COMMAND_ACK
COMMAND_RESULT
ERROR_REPORT
```

v1 packet rules:

- no dynamic allocation
- no JSON
- no long strings
- no Dashboard-only text
- no large metadata blobs
- no unbounded capability arrays
- capability IDs must be compact references or bounded strings
- packet parsing must fail closed

## 4. Node Identity Model

Node identity must be stable and compact.

Required identity fields:

```text
node_id
mac_address
node_role
protocol_version
metadata_level
trust_state
last_seen_ms
```

`node_id` rules:

- stable across reboot when possible
- unique within the local Cyber32 network
- not used by Logic
- may be exposed through API for diagnostics

`mac_address` rules:

- used by ESP-NOW transport and peer table
- not used by Logic
- not used as a capability ID
- may be hidden or abbreviated in API if security requires

## 5. Virtual Device Concept

A virtual Device represents a remote wireless node or one capability-producing endpoint on that node.

Virtual Device responsibilities:

- convert validated wireless packets into `CapabilityPayload`
- expose device identity to PNP/Registry
- track transport availability through Service-owned state
- provide a hardware-agnostic Device interface to Services

Virtual Device must not:

- write Registry directly
- expose API
- run Logic
- decide command policy
- know Dashboard behavior

Example:

```text
WirelessTemperatureNodeDevice
-> receives packet for CAP_TEMPERATURE
-> produces CapabilityPayload for CAP_TEMPERATURE
```

## 6. Virtual Module Concept

A virtual Module is metadata for a wireless node.

Virtual Module fields:

```text
module_id
module_type = "wireless_node"
metadata_level
display_name
node_id
device_ids
capability_ids
transport = "espnow"
```

Virtual Module rules:

- metadata only
- no packet parsing
- no Registry writes
- no Logic
- no API
- no direct hardware behavior

PNP treats virtual modules like other modules after metadata validation.

## 7. Capability Mapping

Wireless capabilities map to the same canonical capability IDs as wired providers.

Examples:

| Provider | Transport | Registered Capability |
|---|---|---|
| wired temperature sensor | direct Driver/Device | `CAP_TEMPERATURE` |
| wireless temperature node | ESP-NOW virtual Device | `CAP_TEMPERATURE` |
| wired distance sensor | direct Driver/Device | `CAP_DISTANCE` |
| wireless distance node | ESP-NOW virtual Device | `CAP_DISTANCE` |
| wireless battery monitor | ESP-NOW virtual Device | `CAP_BATTERY_LEVEL` |

Rules:

- Logic queries `CAP_TEMPERATURE`, not `wireless_temperature_node`.
- API exposes provider metadata separately from capability state.
- Registry may store owner module/device indexes but Logic must not bind to them.
- Multi-provider conflict resolution must be Service-owned or Registry-query-policy-owned in a future plan.

## 8. Wireless Node Diagnostics Capabilities

Every wireless node should expose diagnostics capabilities when the node hardware can provide them.

Diagnostics are ordinary capability-first state. They do not replace the node's main sensor or actuator capability.

Example:

```text
wireless temperature node
-> CAP_TEMPERATURE
-> CAP_BATTERY_LEVEL
-> CAP_SIGNAL_STRENGTH
```

Logic may use diagnostics by capability ID when needed:

```text
IF CAP_BATTERY_LEVEL.value < 20.0
THEN mark node as low power
```

Core Logic must still not know whether the capability provider is wired or wireless.

### CAP_BATTERY_LEVEL

Purpose:

```text
Report remaining node battery percentage.
```

Payload rules:

```text
capability_id = CAP_BATTERY_LEVEL
value_type = FLOAT
value_float = 0.0F..100.0F
unit = "percent"
```

Rules:

- unavailable battery level must not be treated as `0`
- unavailable battery must use explicit unavailable/stale payload state
- battery level is diagnostic unless a Logic rule explicitly uses `CAP_BATTERY_LEVEL`
- Registry stores latest compact state only

### CAP_BATTERY_VOLTAGE

Purpose:

```text
Report node battery voltage when hardware supports measurement.
```

Payload rules:

```text
capability_id = CAP_BATTERY_VOLTAGE
value_type = FLOAT
unit = "volt"
```

Rules:

- optional if the node hardware cannot measure voltage
- voltage must not replace percentage if `CAP_BATTERY_LEVEL` is available
- unavailable voltage must be explicit, not `0`

### CAP_SIGNAL_STRENGTH

Purpose:

```text
Report wireless link quality for diagnostics.
```

Payload options:

```text
RSSI dBm
or
link quality percent
```

Rules:

- RSSI, when available, should use dBm
- link quality percent, when used, should be `0.0F..100.0F`
- if RSSI is unavailable, expose `quality = "unknown"`
- signal quality is diagnostic and not required for Logic
- poor signal may be used by Services to mark payload stale/degraded

### last_seen_ms

Purpose:

```text
Track when a wireless node was last observed.
```

Rules:

- `last_seen_ms` is node diagnostic state
- it may be exposed through API diagnostics
- it is not necessarily a `CAP_*` payload in v1
- Registry stores latest compact state only
- no packet history is stored
- Service owns stale/lost policy

Diagnostics rules:

- diagnostics must not replace the main capability
- diagnostics must not expose packet history
- diagnostics must not require dynamic allocation
- diagnostics must not make Logic depend on wireless transport details
- API may show diagnostics, but capability IDs remain canonical

## 9. Last Seen Tracking

Wireless nodes require bounded last-seen tracking.

Suggested record:

```text
node_id
last_seen_ms
last_packet_sequence
missed_heartbeat_count
status
```

Status values:

```text
AVAILABLE
STALE
UNAVAILABLE
LOST
```

Rules:

- last-seen state is stored compactly
- no packet history in Registry
- timeout calculation uses unsigned wrap-safe time comparisons
- Service owns stale/lost policy
- Registry stores resulting availability state

## 10. Signal Quality Tracking

Signal quality should be exposed as diagnostics and optionally as capabilities.

Preferred capability:

```text
CAP_SIGNAL_STRENGTH
```

Possible fields:

```text
rssi_dbm
link_quality_percent
packet_loss_percent
last_seen_ms
```

ESP-NOW v1 may not always provide reliable RSSI per packet through Arduino APIs. If exact RSSI is unavailable, Cyber32 should store:

```text
quality = "unknown"
value_type = NONE
```

Rules:

- signal quality must not be required for Logic unless explicitly available
- poor signal may mark payload stale or degraded
- signal diagnostics must remain compact

## 11. Battery Status Capability

Wireless nodes should optionally expose battery state.

Preferred capabilities from the catalog:

```text
CAP_BATTERY_LEVEL
CAP_BATTERY_VOLTAGE
```

Battery payload rules:

- `CAP_BATTERY_LEVEL.value_float` uses percent, `0.0..100.0`
- `CAP_BATTERY_VOLTAGE.value_float` uses volts
- unavailable battery data must not be treated as zero
- low battery does not remove the node by itself
- critical battery may mark node degraded or trigger an API warning

Wireless battery state is a capability like any other. Logic may use it by capability ID:

```text
IF CAP_BATTERY_LEVEL.value < 20.0
THEN request safe behavior
```

## 12. Wireless PNP Discovery Flow

Wireless PNP follows the same Cyber32 flow:

```text
Detect
-> Identify
-> Read Metadata
-> Validate
-> Register
-> Expose Capabilities
```

### Detect

Detection source:

```text
ESP-NOW NODE_ANNOUNCE packet
```

Detected data:

```text
mac_address
node_id
protocol_version
packet_type
metadata_level
```

### Identify

PNP identifies the packet as a Cyber32 wireless node.

Validation:

- magic matches Cyber32
- protocol version supported
- node_id present
- capability count within v1 limit

### Read Metadata

Metadata may arrive in:

- announce packet, if compact enough
- one or more bounded metadata packets, future
- local allowlist table for v1

v1 should prefer compact single-packet metadata or local allowlist mapping.

### Validate

Checks:

- required fields present
- capability IDs start with `CAP_`
- packet size within limit
- capability count within fixed table limits
- security/trust mode accepted
- duplicate node behavior is defined
- no unsupported actuator commands are enabled by default

### Register

PNP Registration creates:

- virtual ModuleRecord
- virtual DeviceRecord
- CapabilityRecord for each exposed capability

Registry public APIs must be used.

### Expose Capabilities

Once registered, higher layers see ordinary capabilities:

```text
Registry -> Services -> Logic -> API -> Dashboard
```

## 13. Registry Integration

Registry must store wireless state as facts, not behavior.

Registry may store:

- virtual module records
- virtual device records
- capability records
- latest capability payloads
- latest command state for future wireless actuators
- latest availability/error state
- compact node diagnostics if schema is added

Registry must not:

- parse ESP-NOW packets
- send ESP-NOW packets
- retry wireless commands
- own timeout policy
- run discovery
- select actuator safety policy

Wireless-specific state should be bounded:

```text
MAX_WIRELESS_NODES
MAX_WIRELESS_CAPABILITIES
MAX_PEERS
MAX_PENDING_WIRELESS_COMMANDS
```

Exact values must be defined before implementation.

## 14. Runtime Integration

Runtime schedules wireless work. It does not own wireless policy.

Recommended Runtime tasks:

```text
task.wireless_service.process_packets
task.wireless_service.check_timeouts
task.wireless_service.flush_outbound
```

Runtime rules:

- bounded packets processed per update
- bounded command processing
- no dynamic task creation
- no blocking radio waits
- no capability-specific policy in Runtime
- no direct Device/Driver calls from Runtime

Runtime may enter `SAFE_MODE` if Services report repeated wireless actuator risk in future phases.

## 15. Service Responsibilities

Wireless Service responsibilities:

- own packet processing policy
- validate packet structure
- validate node trust state
- update virtual Device payloads
- update Registry through public APIs
- track last seen
- track stale/unavailable/lost state
- map transport errors to compact `ERR_*`
- handle bounded outbound packet queue, if command support is enabled

Sensor Service responsibilities:

- consume virtual Device payloads like wired Device payloads
- update `CAP_*` payloads in Registry
- keep units and schemas identical to wired providers

Future actuator Service responsibilities:

- own command validation
- enforce Runtime gating
- enforce SAFE_MODE policy
- use bounded pending command slots
- send wireless command requests only through transport Driver/Device contracts
- require acknowledgement/result handling before marking command completed

## 16. API Visibility

API must expose wireless capabilities as normal capabilities.

Required API behavior:

- `GET /capabilities/CAP_TEMPERATURE` returns state regardless of wired or wireless provider
- `GET /capabilities` may include provider metadata
- system status may include wireless service health
- errors remain canonical `ERR_*`

Optional diagnostics:

```text
GET /registry/summary
GET /wireless/nodes
GET /wireless/nodes/{node_id}
```

Diagnostics must be bounded and API-safe:

- no raw packet dumps
- no long metadata blobs
- no Dashboard-only text
- no unbounded node lists

## 17. Security Model v1

ESP-NOW v1 security should be conservative.

Minimum v1 model:

- local allowlist of node MAC or node_id
- protocol magic/version validation
- bounded packet size validation
- sequence number replay protection, if feasible
- optional ESP-NOW peer encryption where supported
- reject unknown nodes by default in production mode

Development mode may allow:

```text
allow_untrusted_wireless_nodes = true
```

Production default should be:

```text
allow_untrusted_wireless_nodes = false
```

Rules:

- unknown nodes must not register actuator capabilities automatically
- wireless actuator command support must require explicit trust
- API should expose trust state as diagnostics
- security failures should map to `ERR_SECURITY_*` or future compact wireless errors

## 18. Lost Node Handling

Lost node behavior:

```text
last_seen timeout expires
-> Wireless Service marks node unavailable/lost
-> Registry updates related capability payloads to unavailable or stale
-> Events may announce capability unavailable
-> Logic sees capability unavailable
-> API reports unavailable state
```

Rules:

- Logic must not know the node was wireless
- unavailable is not zero
- stale values must be marked stale
- wireless actuator loss must fail safe
- Registry stores final state, not timeout policy

Suggested timeout states:

| Condition | State |
|---|---|
| heartbeat within window | available/fresh |
| heartbeat missed once | stale |
| timeout exceeded | unavailable |
| repeated timeout | lost |

## 19. Timeout Handling

Wireless timeout types:

- packet receive timeout
- heartbeat timeout
- metadata discovery timeout
- command acknowledgement timeout
- command result timeout

Rules:

- all timeouts must be bounded
- Service owns timeout policy
- Runtime schedules checks
- Registry stores resulting state
- EventBus announces compact changes only

Suggested errors:

```text
ERR_DEVICE_TIMEOUT
ERR_CAPABILITY_UNAVAILABLE
ERR_ACTUATOR_UNAVAILABLE
ERR_COMMAND_TIMEOUT
ERR_SECURITY_*
```

Future wireless-specific errors may be added only through the canonical error catalog.

## 20. Future Actuator Support

Wireless actuator support is future work and must not be enabled casually.

Required before wireless actuators:

- trusted node registration
- command acknowledgement packet
- command result packet
- timeout enforcement
- duplicate command protection
- SAFE_MODE blocking
- emergency OFF/STOP behavior
- lost-node fail-safe behavior
- brownout/watchdog behavior on remote node
- validation harness with simulated wireless command loss

Command flow:

```text
API or Logic
-> Service
-> bounded pending command
-> Runtime command task
-> Wireless Service outbound packet
-> ESP-NOW transport
-> remote node Device/Driver
-> command result packet
-> Wireless Service
-> Registry command state
```

Rules:

- API must not send ESP-NOW packets directly
- Runtime must not own actuator policy
- Registry must not execute wireless commands
- remote command timeout must fail safe
- wireless relay ON must remain blocked in SAFE_MODE

## 21. Future Gateway Support

Future gateway support may bridge Cyber32 capabilities across networks.

Possible gateway roles:

- ESP-NOW to local API bridge
- ESP-NOW to WiFi/IP gateway
- multi-node capability aggregator
- remote diagnostics bridge

Gateway rules:

- gateway is not a new architecture bypass
- gateway exposes capabilities through API/Registry contracts
- gateway must not let Dashboard call Devices directly
- gateway must preserve `CAP_*` IDs
- gateway must enforce bounded packet and response sizes
- gateway must not expose untrusted actuator control by default

Gateway remains v2+ unless explicitly scoped for ESP32 v1.

## Example Flows

### Wired Temperature Sensor

```text
Temperature Driver
-> Temperature Device
-> Temperature Module
-> PNP Registration
-> Registry CAP_TEMPERATURE
-> Temperature Service update
-> Logic queries CAP_TEMPERATURE
```

### Wireless Temperature Node

```text
ESP-NOW packet
-> ESP-NOW Transport Driver
-> Wireless Node Virtual Device
-> Wireless Node Virtual Module
-> Wireless PNP Registration
-> Registry CAP_TEMPERATURE
-> Wireless/Temperature Service update
-> Logic queries CAP_TEMPERATURE
```

Logic rule for both:

```text
IF CAP_TEMPERATURE.value > 40.0
THEN react to high temperature
```

Logic does not know whether `CAP_TEMPERATURE` came from a wired sensor or a wireless node.

## Implementation Roadmap

Before implementation, create `MILESTONE_6_1_WIRELESS_PACKET_SCHEMA.md`.

That schema document must define:

- packet header
- packet types
- node ID format
- sequence ID
- capability encoding
- battery fields
- signal fields
- last_seen handling
- checksum
- max packet size

Milestone 6 should be split before code:

1. ESP-NOW scope and memory budget.
2. `MILESTONE_6_1_WIRELESS_PACKET_SCHEMA.md`.
3. Wireless node record types.
4. Simulated ESP-NOW transport driver.
5. Virtual wireless temperature Device.
6. Virtual wireless temperature Module.
7. Wireless PNP discovery.
8. Wireless PNP registration.
9. Wireless Service packet processing.
10. Runtime task integration.
11. API diagnostics.
12. Validation harness.
13. Architecture audit.

First implementation slice should use one wireless sensor only:

```text
CAP_TEMPERATURE
```

No wireless actuator should be implemented until wireless sensor discovery, timeout, and lost-node behavior are validated.

## Stop Conditions

Stop and review architecture if:

- Logic needs to know whether a provider is wireless
- API sends ESP-NOW packets directly
- Runtime parses ESP-NOW packet payloads
- Registry parses packets or runs timeout policy
- PNP calls wireless Driver internals directly
- wireless command storage becomes unbounded
- packet payloads require dynamic allocation
- Arduino `String` or STL containers are introduced in core paths
- unknown wireless actuator nodes can register automatically
- lost wireless actuator nodes can remain active without fail-safe behavior
- Dashboard or Mobile Studio needs direct ESP-NOW access

## Final Assessment

ESP-NOW can fit Cyber32 v1 if it is treated as a bounded transport beneath virtual Devices and virtual Modules.

The essential architecture rule is:

```text
wireless changes the provider path, not the capability contract
```

With this model, wired and wireless providers can expose identical `CAP_*` records. Registry, Logic, API, and future Dashboard behavior remain capability-first and hardware-agnostic.
