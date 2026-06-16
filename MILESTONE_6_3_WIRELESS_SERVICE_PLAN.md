# Milestone 6.3 Wireless Service Plan

## Goal

Design `WirelessService` for simulated ESP-NOW packet processing.

This document is documentation only. It does not change source code, does not modify `src/main.cpp`, and does not add real ESP-NOW, WiFi, WebServer, Dashboard, or WirelessService implementation.

## Reviewed Inputs

- `MILESTONE_6_ESPNOW_ARCHITECTURE_PLAN.md`
- `MILESTONE_6_1_WIRELESS_PACKET_SCHEMA.md`
- `MILESTONE_6_2_WIRELESS_IMPLEMENTATION_PLAN.md`
- `MILESTONE_6_1_MULTI_PROVIDER_CAPABILITY_PLAN.md`
- `MILESTONE_6_2_AUTOMATIC_PROVIDER_SELECTION_PLAN.md`

## Core Architecture Rules

- Runtime remains scheduler only.
- Registry owns provider storage.
- Registry owns provider selection.
- WirelessService owns packet processing policy.
- Logic never sees wireless origin.
- API never reads raw packets.
- No dynamic allocation.
- No Arduino `String`.
- No STL containers.

## 1. WirelessService Responsibilities

`WirelessService` is the Service-layer coordinator for simulated ESP-NOW packet processing.

Responsibilities:

- attach to `Registry`
- attach to `SimEspNowTransportDriver`
- attach to virtual wireless Devices
- process bounded received packets
- validate packet headers and capability value fields
- update local node last-seen state
- update virtual Device state from validated packets
- update Registry provider records through public APIs
- request Registry provider selection through public APIs
- mark provider stale, unavailable, or lost when timeouts occur
- map packet/timeout failures to compact `ERR_*` states

WirelessService must not:

- expose API
- run Logic
- call Runtime update methods
- allocate memory dynamically
- store raw packet history
- register real ESP-NOW peers
- include `WiFi.h` or `esp_now.h` in the simulated phase
- let Logic know whether a capability is wireless

## 2. SimEspNowTransportDriver Integration

The simulated transport Driver owns one bounded received-packet slot.

WirelessService uses only public Driver methods:

```text
hasReceivedPacket()
readReceivedPacket(header, value, diagnostics)
clearReceivedPacket()
initialized()
```

Processing rules:

- process a bounded number of packets per Runtime tick
- first simulated phase may process at most one packet per call because Driver has one slot
- failure mode or missing Driver results in no Registry payload update
- invalid transport state may update Service-local error state

The transport Driver must not:

- write Registry
- know provider IDs
- know Logic rules
- own stale/lost policy

## 3. WirelessTemperatureDevice Integration

`WirelessTemperatureDevice` converts a validated wireless `CAP_TEMPERATURE` packet into a standard `CapabilityPayload`.

WirelessService calls:

```text
device.updateFromPacket(now_ms, header, value, diagnostics)
device.readPayload(payload)
device.nodeRecord(node_record)
```

Device responsibilities:

- validate packet fields relevant to the Device
- store latest compact node diagnostics
- produce standard `CAP_TEMPERATURE` payload

WirelessService responsibilities around the Device:

- decide whether a packet should be processed
- call Device only after packet shape is acceptable
- write Device payload to Registry provider state
- update provider status and last-seen facts

Device must not write Registry directly.

## 4. Provider Registration Flow

Provider registration happens before packet processing can update provider state.

Expected flow:

```text
Wireless PNP Discovery
-> Wireless PNP Registration
-> Registry ModuleRecord
-> Registry DeviceRecord
-> canonical CapabilityRecord or existing CAP_TEMPERATURE
-> CapabilityProviderRecord for wireless provider
```

First wireless temperature provider:

```text
provider_id = "provider-wireless-temperature-001"
capability_id = CAP_TEMPERATURE
provider_type = WIRELESS
status = STALE or AVAILABLE after first value packet
priority = configured wireless priority
latest_payload = initial stale/unavailable payload
```

Rules:

- WirelessService does not write Registry arrays directly.
- WirelessService may not bypass PNP Registration.
- Duplicate provider IDs must be rejected by Registry.
- Duplicate canonical capability IDs require the multi-provider model.

## 5. Provider Update Flow

When a valid wireless capability packet arrives:

```text
SimEspNowTransportDriver
-> WirelessService::processPackets(now_ms)
-> WirelessTemperatureDevice::updateFromPacket(...)
-> WirelessTemperatureDevice::readPayload(...)
-> Registry provider update API
-> Registry provider selection helper
-> Registry active provider mapping / selected payload update in future phase
```

Required future Registry APIs:

```text
updateCapabilityProviderPayload(provider_id, payload)
updateCapabilityProviderStatus(provider_id, status)
selectBestProvider(capability_id, out_provider)
setActiveProvider(capability_id, provider_id)
```

Current foundation already supports:

```text
registerCapabilityProviderWithResult(...)
getCapabilityProvider(...)
selectBestProvider(...)
setActiveProvider(...)
```

Until provider payload update APIs exist, WirelessService implementation must stop before pretending provider selection updates canonical payloads.

## 6. Node Record Update Flow

Wireless node state is compact and latest-only.

On valid packet:

```text
node_id = header.node_id
last_seen_ms = local now_ms
last_sequence_id = header.sequence_id
status = AVAILABLE
battery_present = diagnostics.battery_present
battery_level_percent = diagnostics.battery_level_percent
battery_voltage = diagnostics.battery_voltage
signal_quality_percent = diagnostics.signal_quality_percent
```

On invalid packet:

```text
do not update capability payload
do not update provider payload
optionally store compact Service error
```

On stale timeout:

```text
status = STALE
provider status = STALE
payload stale = STALE
```

On lost timeout:

```text
status = LOST
provider status = LOST
payload available = UNAVAILABLE
```

No raw packet history is stored.

## 7. Active Provider Selection Flow

WirelessService may ask Registry to select the best provider, but Registry owns the selection decision.

After a wireless provider update:

```text
Registry::selectBestProvider(CAP_TEMPERATURE, selected)
```

If selection succeeds:

```text
Registry::setActiveProvider(CAP_TEMPERATURE, selected.provider_id)
```

Future selected-payload update:

```text
Registry copies selected provider latest_payload into canonical CapabilityRecord.latest_payload
```

Rules:

- WirelessService may trigger evaluation.
- Registry chooses the provider.
- Logic remains provider-blind.
- Runtime does not choose providers.
- Provider selection must scan only bounded provider storage.

## 8. Last Seen Updates

`last_seen_ms` is calculated locally.

Rules:

- remote node timestamp is not authoritative
- `last_seen_ms = now_ms` when a valid packet is accepted
- duplicate packets may refresh diagnostics only if duplicate policy allows it
- invalid packets must not refresh last-seen
- `last_seen_ms` may be exposed through API diagnostics later

Suggested fields:

```text
node_id
last_seen_ms
last_sequence_id
missed_heartbeat_count
status
```

`last_seen_ms` is diagnostic state and not necessarily a `CAP_*` payload in v1.

## 9. Stale Provider Handling

Stale means the last known provider payload exists but is old.

Suggested timeout:

```text
WIRELESS_STALE_TIMEOUT_MS = bounded configured value
```

On stale timeout:

```text
node status = STALE
provider status = STALE
provider latest_payload.stale = STALE
provider latest_payload.quality = "stale"
provider latest_payload.error_code = "none" or compact stale error if added later
```

Selection behavior:

- fresh available providers should beat stale providers
- stale providers may be fallback only if no fresh provider exists
- Logic must never treat stale as fresh

## 10. Lost Node Handling

Lost means the node has exceeded the lost timeout and should not be treated as an active provider.

Suggested timeout:

```text
WIRELESS_LOST_TIMEOUT_MS > WIRELESS_STALE_TIMEOUT_MS
```

On lost timeout:

```text
node status = LOST
provider status = LOST
provider latest_payload.available = UNAVAILABLE
provider latest_payload.stale = STALE
provider latest_payload.value_type = NONE
provider latest_payload.error_code = ERR_CAPABILITY_UNAVAILABLE or ERR_DEVICE_TIMEOUT
```

Selection behavior:

- lost providers are not eligible
- Registry should fail over to an available provider if one exists
- if no provider exists, canonical capability becomes unavailable in a future selected-payload phase

Wireless actuator note:

```text
future wireless actuator node loss must fail safe
```

No wireless actuator command behavior is allowed in the first WirelessService slice.

## 11. Runtime Integration

Runtime schedules WirelessService tasks only.

Required task contexts:

```text
WirelessServiceProcessTaskContext
WirelessServiceTimeoutTaskContext
```

Required Runtime tasks:

```text
task.wireless_service.process_packets
task.wireless_service.check_timeouts
```

Task behavior:

```text
process task -> WirelessService::processPackets(now_ms)
timeout task -> WirelessService::checkTimeouts(now_ms)
```

Runtime must not:

- parse packets
- inspect `CAP_TEMPERATURE`
- select providers
- update Registry wireless state
- call Drivers directly
- call Devices directly

## 12. Future Wireless Capabilities

### Temperature

First slice:

```text
CAP_TEMPERATURE
value_type = FLOAT
unit = "degree_celsius"
```

Logic reads `CAP_TEMPERATURE` only.

### Humidity

Future sensor:

```text
CAP_HUMIDITY
value_type = FLOAT
unit = "percent"
```

Requires capability ID and payload schema before implementation.

### Battery

Diagnostics:

```text
CAP_BATTERY_LEVEL
CAP_BATTERY_VOLTAGE
```

Rules:

- unavailable battery is not `0`
- battery does not replace main sensor payload
- Logic may use battery capability only by `CAP_*` ID

### Signal

Diagnostics:

```text
CAP_SIGNAL_STRENGTH
```

Rules:

- signal may be unknown
- signal quality is diagnostic
- poor signal may influence stale/degraded state

## 13. Future Wireless Actuators

Wireless actuator support is explicitly future work.

Required before wireless actuators:

- trusted node model
- command request packet
- command acknowledgement packet
- command result packet
- bounded pending command slot
- timeout enforcement
- duplicate command protection
- Runtime READY/RUNNING gating
- SAFE_MODE blocking
- emergency OFF/STOP behavior
- lost-node fail-safe behavior

Rules:

- API commands must go through Services.
- Runtime must remain scheduler only.
- Registry must not execute commands.
- WirelessService must not bypass actuator Services.
- unknown wireless actuator nodes must not auto-register as controllable.

## 14. Validation Requirements

Validation must prove the simulated wireless path without real radio hardware.

Required cases:

1. Simulated wireless packet is injected into `SimEspNowTransportDriver`.
2. Runtime task calls `WirelessService::processPackets(now_ms)`.
3. WirelessService reads exactly the bounded pending packet.
4. Valid `CAP_TEMPERATURE` packet updates `WirelessTemperatureDevice`.
5. Device produces standard `CAP_TEMPERATURE` payload.
6. Registry provider record is updated through public APIs.
7. Registry `selectBestProvider()` can select the wireless provider.
8. Active provider mapping can be set without changing Logic.
9. `last_seen_ms` updates to local `now_ms`.
10. Battery diagnostics are stored when present.
11. Signal diagnostics are stored or marked unknown.
12. Invalid packet fails closed and does not update Registry.
13. Stale timeout marks provider stale.
14. Lost timeout marks provider lost/unavailable.
15. Logic still queries `CAP_TEMPERATURE` only.
16. Existing wired/simulated temperature validation remains intact.

Validation must not:

- use real ESP-NOW
- require WiFi
- modify `src/main.cpp`
- make Logic depend on provider or node IDs
- read private Registry internals
- use dynamic allocation

## 15. Stop Conditions

Stop implementation and review architecture if:

- WirelessService needs dynamic allocation
- WirelessService needs Arduino `String`
- WirelessService needs STL containers
- packet storage becomes unbounded
- raw packet history is stored
- Registry parses packets
- Runtime parses packets
- API reads raw packets
- Logic sees node ID or provider ID
- provider selection moves out of Registry
- Provider updates bypass Registry public APIs
- PNP Registration is bypassed
- real ESP-NOW is required before simulated validation passes
- wireless actuator support is requested before sensor stale/lost behavior is validated

## Final Assessment

WirelessService should be a bounded packet-processing Service.

Its job is:

```text
read simulated transport packet
validate packet
update virtual Device
write provider facts to Registry
ask Registry to select providers
mark stale/lost through timeout policy
```

The capability contract remains unchanged:

```text
Logic sees CAP_TEMPERATURE
not wireless_temperature_node
```

This keeps Cyber32 wireless support aligned with the existing capability-first architecture while preparing for future diagnostics and, much later, carefully gated wireless actuators.
