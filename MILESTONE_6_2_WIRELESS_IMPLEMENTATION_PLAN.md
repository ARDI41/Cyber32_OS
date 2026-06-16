# Milestone 6.2 Wireless Implementation Plan

## Goal

Convert the ESP-NOW wireless architecture and bounded packet schema into a phased implementation roadmap.

This document is documentation only. It does not change source code, does not modify `src/main.cpp`, does not add real ESP-NOW radio behavior, and does not add WiFi, WebServer, Dashboard, cloud, or Mobile Studio behavior.

Use:

- `MILESTONE_6_ESPNOW_ARCHITECTURE_PLAN.md`
- `MILESTONE_6_1_WIRELESS_PACKET_SCHEMA.md`
- `PRODUCTION_BOOTSTRAP_PLAN.md`

## Scope

First implementation slice:

```text
simulated wireless CAP_TEMPERATURE
```

Diagnostics included:

```text
CAP_BATTERY_LEVEL
CAP_SIGNAL_STRENGTH
last_seen_ms diagnostic state
```

Explicitly out of scope:

- real ESP-NOW radio
- WiFi/WebServer/Dashboard
- wireless actuators
- command packets
- cloud
- OTA
- dynamic allocation
- Arduino `String`
- STL containers
- `src/main.cpp` changes

## Architecture Rules

- Wireless changes provider path, not capability contract.
- Logic must not know whether a capability is wired or wireless.
- API must not send raw ESP-NOW packets.
- Runtime schedules wireless tasks only.
- Registry stores latest compact state only.
- Services own packet-processing, timeout, stale/lost, and diagnostics policy.
- PNP owns wireless node discovery and registration coordination.
- Virtual Devices convert validated packets to capability payloads.
- Virtual Modules are metadata only.

## Phase 1: Wireless Packet Types And Structs

Purpose:

Create compact C++11-safe packet constants and structs matching `MILESTONE_6_1_WIRELESS_PACKET_SCHEMA.md`.

Expected files:

```text
src/core/ids/wireless_packet_ids.h
src/core/types/wireless_packet_types.h
```

Expected content:

- packet magic constant
- protocol version constant
- max packet size constant
- packet type enum
- payload type enum or reuse existing `PayloadValueType`
- packet header struct
- capability value packet payload struct
- node heartbeat payload struct
- node announce payload struct
- checksum field convention

Success criteria:

- all packet structs are fixed-size or bounded-size
- max payload is `<= 250` bytes
- no JSON
- no dynamic allocation
- no Arduino `String`
- no STL containers
- no behavior or parsing yet

Stop conditions:

- packet schema requires heap allocation
- packet schema exceeds ESP-NOW limit
- object/string/binary payload support is needed in the first slice

## Phase 2: Simulated ESP-NOW Transport Driver

Purpose:

Create a simulated transport Driver that can inject and retrieve bounded wireless packets without real radio hardware.

Expected files:

```text
src/drivers/communication/sim_espnow_transport_driver.h
src/drivers/communication/sim_espnow_transport_driver.cpp
```

Expected methods:

```text
begin()
injectReceivedPacket(...)
hasPacket()
readPacket(...)
setFailureMode(bool enabled)
initialized() const
```

Behavior:

- stores at most one or a small fixed number of received packets
- rejects packets larger than v1 maximum
- failure mode blocks packet read/write behavior
- no real ESP-NOW, WiFi, or radio APIs

Success criteria:

- simulated packet injection works
- bounded packet storage only
- no Registry/EventBus/Runtime/API dependency
- no real ESP-NOW includes

Stop conditions:

- transport needs dynamic packet queues
- transport writes Registry
- transport knows capability policy

## Phase 3: Wireless Node Record Types

Purpose:

Define compact records for wireless node state and diagnostics.

Expected files:

```text
src/core/types/wireless_node_types.h
```

Expected records:

```text
WirelessNodeStatus
WirelessNodeRecord
WirelessNodeDiagnostics
```

Suggested fields:

```text
node_id
last_seen_ms
last_sequence_id
status
battery_present
battery_level_percent
battery_voltage
signal_known
rssi_dbm
link_quality_percent
```

Success criteria:

- fixed-size records
- no packet history
- no dynamic allocation
- supports stale/lost state
- supports battery and signal diagnostics

Stop conditions:

- diagnostics need unbounded metadata
- node identity requires heap strings

## Phase 4: Virtual Wireless Temperature Device

Purpose:

Create a virtual Device that converts validated wireless `CAP_TEMPERATURE` packets into `CapabilityPayload`.

Expected files:

```text
src/devices/communication/wireless_temperature_device.h
src/devices/communication/wireless_temperature_device.cpp
```

Expected methods:

```text
begin(...)
applyTemperaturePacket(...)
readPayload(uint32_t now_ms, CapabilityPayload& out_payload)
readBatteryPayload(uint32_t now_ms, CapabilityPayload& out_payload)
readSignalPayload(uint32_t now_ms, CapabilityPayload& out_payload)
id() const
type() const
```

Behavior:

- exposes `CAP_TEMPERATURE`
- optionally exposes `CAP_BATTERY_LEVEL`
- optionally exposes `CAP_SIGNAL_STRENGTH`
- unavailable values are explicit and never treated as zero
- stores latest compact decoded state only

Success criteria:

- wireless temperature packet becomes standard `CAP_TEMPERATURE` payload
- battery level uses unit `"percent"`
- signal quality can be `"unknown"`
- Device does not write Registry
- Device does not expose API

Stop conditions:

- Device parses raw packet headers directly when that belongs to Service/transport
- Device writes Registry
- Logic-visible wireless naming appears

## Phase 5: Virtual Wireless Module Metadata

Purpose:

Create metadata-only virtual Module for the simulated wireless temperature node.

Expected files:

```text
src/modules/communication/wireless_temperature_module.h
src/modules/communication/wireless_temperature_module.cpp
```

Expected metadata:

```text
module_id = "module-wireless-temperature-001"
module_type = "wireless_node"
metadata_level = 1
display_name = "Wireless Temperature Node"
device_id = "device-wireless-temperature-001"
device_type = "wireless_sensor"
capabilities = CAP_TEMPERATURE, CAP_BATTERY_LEVEL, CAP_SIGNAL_STRENGTH
```

Success criteria:

- metadata only
- no packet parsing
- no Device method calls
- no Registry writes
- no Logic/API behavior

Stop conditions:

- Module owns wireless policy
- Module exposes transport-specific state to Logic

## Phase 6: Wireless PNP Discovery

Purpose:

Allow PNP to discover the simulated wireless temperature node from a validated announce packet or simulated metadata source.

Expected files:

```text
src/pnp/wireless_pnp_discovery.h
src/pnp/wireless_pnp_discovery.cpp
```

Possible integration:

```text
PnpDiscovery::discoverSimulatedWirelessTemperatureModule(...)
```

Behavior:

- detects `NODE_ANNOUNCE`
- validates packet magic/version/type/length/checksum before using metadata
- validates capability IDs start with `CAP_`
- fills bounded `PnpModuleInfo` or wireless-specific discovery record
- does not register with Registry
- does not call Device behavior

Success criteria:

- simulated wireless announce produces valid discovery info
- unsupported packet is rejected safely
- existing wired PNP behavior remains unchanged

Stop conditions:

- PNP writes Registry directly
- PNP calls transport internals beyond public Driver/Service contract
- unknown actuator nodes can register automatically

## Phase 7: Wireless PNP Registration

Purpose:

Register virtual wireless Module, Device, and Capabilities into Registry.

Expected files:

```text
src/pnp/wireless_pnp_registration.h
src/pnp/wireless_pnp_registration.cpp
```

Possible integration:

```text
PnpRegistration support for wireless temperature diagnostics capabilities
```

Capabilities for first slice:

```text
CAP_TEMPERATURE
CAP_BATTERY_LEVEL
CAP_SIGNAL_STRENGTH
```

Initial payloads:

- `CAP_TEMPERATURE`: stale/unavailable until first value packet
- `CAP_BATTERY_LEVEL`: stale/unavailable until heartbeat or value packet
- `CAP_SIGNAL_STRENGTH`: stale/unknown until heartbeat or value packet

Success criteria:

- Registry stores virtual module
- Registry stores virtual device
- Registry stores all three capability records
- duplicate/full-table behavior remains bounded
- Registry public result APIs are used

Stop conditions:

- Registry arrays are written directly
- wireless metadata requires unbounded storage
- Logic needs provider-specific IDs

## Phase 8: Wireless Service Packet Processing

Purpose:

Create WirelessService to process simulated packets, update virtual Device state, and write Registry payloads.

Expected files:

```text
src/services/wireless/wireless_service.h
src/services/wireless/wireless_service.cpp
```

Expected methods:

```text
begin(Registry* registry, SimEspnowTransportDriver* transport, WirelessTemperatureDevice* device)
processPackets(uint32_t now_ms)
checkTimeouts(uint32_t now_ms)
lastRegistryResult() const
lastNodeStatus() const
```

Behavior:

- reads bounded packets from simulated transport
- validates packet before use
- updates local `last_seen_ms`
- handles duplicate sequence IDs
- converts value packets into Device state
- updates Registry through public APIs
- marks stale/lost on timeout
- does not expose API
- does not run Logic

Success criteria:

- `CAP_TEMPERATURE` updates from wireless packet
- `CAP_BATTERY_LEVEL` updates from heartbeat/value packet
- `CAP_SIGNAL_STRENGTH` updates or becomes unknown
- invalid packets fail closed
- no packet history is stored

Stop conditions:

- Service requires dynamic queues
- Service makes Registry parse packets
- Service exposes raw packets to API

## Phase 9: Runtime Tasks

Purpose:

Coordinate wireless service execution with bounded Runtime tasks.

Required Runtime tasks:

```text
task.wireless_service.process_packets
task.wireless_service.check_timeouts
```

Expected task contexts:

```text
WirelessServiceProcessTaskContext
WirelessServiceTimeoutTaskContext
```

Task behavior:

- process task calls `WirelessService::processPackets(now_ms)`
- timeout task calls `WirelessService::checkTimeouts(now_ms)`
- Runtime does not parse packets
- Runtime does not know `CAP_TEMPERATURE`
- Runtime does not decide stale/lost policy

Success criteria:

- Runtime task count remains within fixed limit
- packet processing budget is bounded
- timeout checks are bounded
- existing actuator and sensor tasks remain unchanged

Stop conditions:

- Runtime parses packet payloads
- Runtime writes Registry for wireless state
- task count exceeds v1 budget

## Phase 10: API Wireless Diagnostics

Purpose:

Expose wireless diagnostics without changing capability-first API behavior.

Expected files:

```text
src/api/api_response.h
src/api/cyber32_api.h
src/api/cyber32_api.cpp
```

Expected API additions:

```text
getWirelessNodeDiagnostics(...)
getWirelessTemperatureState(...) optional only if it still maps to CAP_TEMPERATURE
```

Preferred behavior:

- existing capability state methods read Registry as usual
- diagnostics API reads bounded node diagnostic records
- API does not expose raw packet bytes
- API does not call transport Driver
- API does not parse ESP-NOW packets

Success criteria:

- API can show node status, last_seen_ms, battery, and signal quality
- `CAP_TEMPERATURE` remains exposed as normal capability state
- no WiFi/WebServer/Dashboard transport added

Stop conditions:

- API bypasses Service/Registry
- API sends raw ESP-NOW packets
- diagnostics response becomes unbounded

## Phase 11: Validation

Purpose:

Prove the simulated wireless temperature slice end-to-end without real radio hardware.

Expected files:

```text
src/app/validation/vertical_slice_validation.h
src/app/validation/vertical_slice_validation.cpp
```

Validation cases:

- simulated wireless temperature packet becomes `CAP_TEMPERATURE`
- `CAP_TEMPERATURE` value is available through Registry
- API exposes `CAP_TEMPERATURE`
- Logic still uses `CAP_TEMPERATURE` only
- Logic does not know wireless origin
- battery level is exposed as `CAP_BATTERY_LEVEL`
- signal quality is exposed as `CAP_SIGNAL_STRENGTH`
- unknown signal quality is handled safely
- `last_seen_ms` updates locally on packet receive
- stale node behavior marks payload stale/unavailable
- lost node behavior marks capability unavailable
- invalid packet does not update Registry
- duplicate sequence packet is ignored safely
- existing wired temperature/distance/actuator validation remains preserved

Success criteria:

- one simulated wireless node is discovered
- wireless node registers virtual module/device/capabilities
- wireless temperature value updates Registry
- Logic queries `CAP_TEMPERATURE`
- API returns capability state
- diagnostics are visible without replacing the main capability
- no architecture violations

Stop conditions:

- validation requires direct private Registry access
- validation requires real radio
- validation makes Logic depend on wireless module/device/node name
- validation changes `src/main.cpp`

## Phase 12: Architecture Audit

Purpose:

Review the implemented wireless slice before any real ESP-NOW or wireless actuator work.

Expected file:

```text
MILESTONE_6_ARCHITECTURE_AUDIT.md
```

Audit checks:

- wireless capabilities remain capability-first
- Runtime remains scheduler-only
- Registry stores state only
- WirelessService owns packet policy
- PNP owns discovery/registration
- API exposes state/diagnostics safely
- Logic does not know wired vs wireless
- packet parsing is bounded
- no dynamic allocation
- no Arduino `String`
- no STL containers
- no real ESP-NOW radio
- no wireless actuators
- no `src/main.cpp` changes

Success criteria:

- no critical architecture violations
- remaining risks documented before real hardware
- next implementation step is clear

## First Slice Definition Of Done

Milestone 6 first wireless slice is complete when:

- packet structs exist
- simulated transport accepts bounded packets
- virtual wireless temperature Device exists
- virtual wireless Module metadata exists
- wireless PNP discovery works
- wireless PNP registration stores records
- WirelessService processes packet updates
- Runtime runs wireless process and timeout tasks
- Registry stores `CAP_TEMPERATURE`, `CAP_BATTERY_LEVEL`, and `CAP_SIGNAL_STRENGTH`
- Logic queries `CAP_TEMPERATURE`
- API exposes temperature and diagnostics
- stale/lost behavior is validated
- no source layer violations exist

## Global Stop Conditions

Stop implementation and review architecture if:

- wireless packet parsing needs heap allocation
- packet schema needs JSON
- packet size can exceed 250 bytes
- STL containers are introduced
- Arduino `String` is introduced
- Runtime parses packet payloads
- Registry parses raw packets
- API sends raw ESP-NOW packets
- Logic depends on node ID, MAC address, or wireless module name
- unknown wireless actuator nodes can register automatically
- wireless actuator commands are requested before sensor/lost-node validation passes
- real ESP-NOW radio is required before simulated validation passes
- `src/main.cpp` must be modified for the validation slice

## Final Assessment

Milestone 6.2 defines the shortest safe path from ESP-NOW architecture to implementation:

```text
bounded packet structs
-> simulated transport
-> virtual wireless temperature provider
-> wireless PNP and Registry records
-> WirelessService packet processing
-> Runtime tasks
-> API diagnostics
-> validation
-> audit
```

The first slice must prove that a wireless provider can become ordinary Cyber32 capability state without exposing wireless details to Logic.
