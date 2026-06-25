# Milestone 9.4 Sensor Discovery Multi-Capability Plan

## Goal

Plan how Cyber32 Base Node will represent wired and wireless sensors in one capability/provider model before minimal app development.

This document is planning only.

It does not change source code, does not modify `src/main.cpp`, does not update Registry behavior, and does not ingest decoded packets into providers.

Reviewed:

- `CYBER32_BIBLE.md`
- `MILESTONE_9_3_PACKET_DECODE_PLAN.md`

## 1. Sensor Classes

Cyber32 sensors may be wired or wireless, but both must become visible through the same capability/provider model.

### Wired Sensor

A wired sensor is local to the Core.

Characteristics:

- local Driver / Device path
- no MAC address
- no radio diagnostics
- `provider_type = WIRED`
- source may be GPIO, I2C, SPI, UART, ADC, or other local hardware

Examples:

- wired temperature probe
- wired distance sensor
- local button
- local light sensor

### Wireless Sensor

A wireless sensor is a remote node that reports capabilities to the Core.

Characteristics:

- ESP-NOW node
- source MAC address
- `node_id`
- radio diagnostics
- `provider_type = WIRELESS`
- packet sequence state
- battery/signal diagnostics when available

Examples:

- wireless temperature node
- wireless battery sensor
- wireless motion sensor
- future wireless weather node

## 2. Common Sensor Display Fields

All sensors should eventually expose common capability/provider display fields.

Common fields:

- node/device name
- `capability_id`
- capability value
- unit
- provider type
- status `AVAILABLE` / `STALE` / `LOST` / `UNAVAILABLE`
- `last_update_ms`
- update interval / report interval
- quality
- `error_code`

These fields allow Mini App, Dev Panel, Mission Control, Dashboard, and future custom pages to display wired and wireless sensors consistently.

Logic must remain capability-first and must not depend on wired/wireless origin.

## 3. Wireless-Only Fields

Wireless sensors may expose additional diagnostic fields.

Wireless-only fields:

- MAC address
- `node_id`
- `sequence_id`
- battery percent
- battery voltage
- signal strength / RSSI
- accepted packets
- rejected packets
- `last_error_code`

These fields are diagnostics, not normal Logic dependencies.

UI may show them through API-approved diagnostics views.

## 4. Plug-And-Play Provisioning Flow

Cyber32 should support a bounded plug-and-play provisioning flow for wireless sensors.

Flow:

```text
Unknown wireless node sends announce or first packet
-> Base captures source MAC
-> Base decodes node / capability metadata
-> Base creates PENDING discovery record
-> Minimal App shows "New sensor found"
-> User accepts
-> Base creates allowlist record
-> Base creates provider record
-> Sensor becomes visible as capability provider
```

Accept action creates allowlist state:

```text
source MAC
node_id
allow_state = ALLOWED
trust_state = TRUSTED
```

Provider creation:

```text
capability_id
provider_type = WIRELESS
status = AVAILABLE or STALE
latest payload placeholder or decoded value
diagnostics defaults
```

Unknown nodes must not update providers before acceptance.

Blocked nodes must not update providers.

Provisioning must be API-first and capability-first.

## 5. Minimal App Readiness

The Minimal App must not talk to Drivers.

The Minimal App must use Cyber32 API only.

Minimum app screens:

- Node list
- Sensor detail
- Capability value card
- Battery/signal card
- Pairing request screen
- Diagnostics screen

App responsibilities:

- display discovered nodes
- show sensor capability values
- show provider status
- show battery/signal diagnostics
- show pairing requests
- allow user acceptance later through API-approved provisioning flow

App must not:

- call Drivers
- call Devices
- call Modules
- call HAL
- access Registry arrays directly
- parse raw ESP-NOW packets
- bypass allowlist/trust/security policy

## 6. Stop Conditions

Stop implementation if any of these are required in Milestone 9.4 planning:

- implement app
- implement Dashboard
- update Registry behavior
- ingest provider payload
- change packet decode behavior
- call WirelessService from decode visibility
- expose raw packet data through API
- allow UI to talk to Drivers
- allow unknown wireless nodes to update providers
- dynamic allocation in core paths
- Arduino `String` in core paths
- STL containers in core paths

## Final Recommendation

Before minimal app development, implement these intermediate milestones:

1. decoded packet -> discovery visibility
2. discovery record -> API read-only
3. manual accept simulation or API accept later

Recommended order:

```text
Milestone 9.3
-> decode received Sensor Firmware packet
-> print decoded capability value

Milestone 9.4
-> define discovery/provider model for wired and wireless sensors

Future milestone
-> decoded packet creates bounded discovery visibility
-> API exposes read-only discovery records
-> Minimal App displays "New sensor found"
-> later API accept creates allowlist and provider records
```

The architecture goal is a single user-visible model:

```text
wired sensor -> capability provider
wireless sensor -> capability provider
Logic reads CAP_* only
UI reads API only
```
