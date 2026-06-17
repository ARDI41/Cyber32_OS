# Milestone 7 Capability Provider Roadmap

## Goal

Define how capability providers will be used across the entire Cyber32 ecosystem.

This document is documentation only. It does not change source code, does not modify `src/main.cpp`, and does not implement new provider behavior.

## Reviewed Inputs

- `MILESTONE_6_1_MULTI_PROVIDER_CAPABILITY_PLAN.md`
- `MILESTONE_6_1_PROVIDER_SELECTION_PLAN.md`
- `MILESTONE_6_2_AUTOMATIC_PROVIDER_SELECTION_PLAN.md`
- `MILESTONE_6_3_WIRELESS_SERVICE_PLAN.md`
- `MILESTONE_6_4_SELECTED_PROVIDER_PAYLOAD_PLAN.md`

## Core Principle

Providers are internal sources for canonical Cyber32 capabilities.

Logic, API rules, Dashboard rules, Mobile Studio blocks, and future AI tools must bind to:

```text
CAP_*
```

They must not bind to:

- provider IDs
- module IDs
- device IDs
- node IDs
- MAC addresses
- transport names
- cloud endpoint names
- test fixture names

Provider selection may change the source of a capability, but it must not change the capability contract.

## Provider Model

Every provider represents one concrete source of a canonical capability.

Provider record concepts:

```text
provider_id
capability_id
owner_module_index
owner_device_index
provider_type
status
priority
last_update_ms
latest_payload
```

The canonical capability remains:

```text
CAP_TEMPERATURE
CAP_DISTANCE
CAP_SERVO_POSITION
CAP_MOTOR_CONTROL
CAP_RELAY_CONTROL
```

The selected provider supplies the payload copied into the canonical capability record.

## 1. Wired Providers

Wired providers are local physical providers connected directly to the controller.

Examples:

```text
I2C temperature sensor -> CAP_TEMPERATURE
UART GPS module -> CAP_GPS_POSITION
SPI display module -> CAP_DISPLAY_TEXT
GPIO relay output -> CAP_RELAY_CONTROL
```

### I2C

I2C providers are suited for compact sensors and small peripheral devices.

Examples:

- temperature
- humidity
- pressure
- distance
- IMU
- battery monitor

Provider properties:

```text
provider_type = WIRED
transport = i2c
priority = high by default
```

Rules:

- I2C driver owns bus transaction details.
- Device converts driver results into capability payloads.
- Service updates Registry through public APIs.
- Logic never sees I2C address.

### UART

UART providers are suited for stream-oriented modules.

Examples:

- GPS
- serial sensors
- modem-style devices
- barcode/identity modules

Rules:

- UART parsing belongs below Service/Device boundaries depending on packet complexity.
- Registry stores latest compact state only.
- Provider payloads must remain bounded.
- Logic must not know UART port names.

### SPI

SPI providers are suited for faster local peripherals.

Examples:

- displays
- cameras, future
- storage-backed sensors
- high-rate acquisition devices

Rules:

- SPI bus selection and chip-select details remain Driver/HAL concerns.
- Capability payloads must not become large binary streams without a separate bounded schema.
- Display/camera providers require explicit payload and memory plans before implementation.

### GPIO

GPIO providers are suited for simple digital inputs and outputs.

Examples:

- buttons
- relays
- limit switches
- LEDs
- simple power outputs

Rules:

- GPIO actuator providers must follow actuator safety policy.
- GPIO input providers may expose boolean capability payloads.
- GPIO pin numbers must not be visible to Logic.

## 2. Wireless Providers

Wireless providers are local or remote nodes that expose Cyber32 capabilities through a wireless transport.

Wireless provider rule:

```text
wireless changes the provider path, not the capability ID
```

### ESP-NOW

ESP-NOW is the first wireless transport.

Examples:

```text
wireless temperature node -> CAP_TEMPERATURE
wireless battery node -> CAP_BATTERY_LEVEL
wireless signal diagnostic -> CAP_SIGNAL_STRENGTH
```

Rules:

- Simulated ESP-NOW comes first.
- Real ESP-NOW comes after packet, stale/lost, and provider selection validation.
- WirelessService owns packet processing and timeout policy.
- Registry owns provider storage and provider selection.
- Logic never sees node ID or transport.

### Bluetooth

Bluetooth provider support is future work.

Potential uses:

- simple local sensors
- phone-adjacent diagnostics
- pairing-based configuration

Rules:

- Bluetooth pairing and trust state must be modeled before actuator use.
- Bluetooth provider payloads must map to canonical `CAP_*` IDs.
- Dashboard/Mobile Studio must use API, not direct Bluetooth access.

### BLE

BLE providers may expose low-power sensor values.

Potential uses:

- battery sensors
- environment sensors
- wearable or beacon-like inputs

Rules:

- BLE characteristics must be translated by Services/Devices into `CapabilityPayload`.
- BLE characteristic UUIDs must not leak into Logic.
- BLE stale/lost timing must be validated separately from ESP-NOW.

### LoRa Future

LoRa providers are long-range, low-bandwidth future providers.

Potential uses:

- outdoor sensors
- remote telemetry
- low-frequency status updates

Rules:

- LoRa payloads must be especially compact.
- Stale/lost thresholds will be longer than ESP-NOW.
- LoRa actuator control must be treated as high-risk future work.

## 3. Virtual Providers

Virtual providers are local software-defined sources for capabilities.

### Simulator

Simulator providers support development and validation.

Examples:

```text
simulated temperature -> CAP_TEMPERATURE
simulated distance -> CAP_DISTANCE
simulated servo -> CAP_SERVO_POSITION
```

Rules:

- simulated providers must use the same payload schemas as real providers
- simulated providers must not create special Logic branches
- simulated actuator providers must still respect command contracts and safety policy

### Test Devices

Test providers exist to prove behavior under controlled conditions.

Uses:

- invalid payload validation
- timeout validation
- provider failover validation
- actuator command failure validation

Rules:

- test providers should be bounded and deterministic
- test providers must not require dynamic allocation
- test-only IDs must not ship as production defaults unless explicitly documented

### Replay Devices

Replay providers may feed recorded or generated capability values.

Uses:

- repeatable sensor scenarios
- regression testing
- simulated environmental changes

Rules:

- replay history must not live in Registry
- replay buffers must be bounded
- replay provider output must still become normal `CapabilityPayload`

## 4. Cloud Providers

Cloud providers are future provider sources that bridge external data into Cyber32 capabilities.

Cloud providers are not ESP32 v1 core scope unless explicitly planned later.

### MQTT

MQTT providers may map topics into capabilities.

Example:

```text
topic: cyber32/site/temp
-> CAP_TEMPERATURE
```

Rules:

- MQTT topic names must not be Logic identifiers
- broker connection health becomes provider diagnostics
- cloud-origin actuator commands must use Services and safety gates

### REST

REST providers may poll bounded external endpoints.

Examples:

- weather
- remote sensor gateway
- external status source

Rules:

- REST payload parsing must be bounded
- API keys/secrets must not enter Registry payloads
- REST provider freshness must be explicit

### Future AI Services

AI providers may produce derived or interpreted capability-like state.

Examples:

```text
camera analysis -> CAP_OBJECT_DETECTED
audio analysis -> CAP_SOUND_LEVEL
prediction service -> CAP_ENVIRONMENT_RISK
```

Rules:

- AI output must use documented `CAP_*` IDs
- confidence/quality must be explicit
- AI providers must not replace safety-critical sensors without policy
- AI may recommend commands, but actuator commands must still go through Services

## 5. Selection Hierarchy

Provider selection must be deterministic and bounded.

Selection hierarchy:

```text
manual override
priority
freshness
failover
deterministic tie-break
```

### Manual

Manual selection is future work.

Rules:

- stored as bounded Registry state
- visible through API diagnostics
- must fail safely if selected provider becomes unavailable
- must not expose provider IDs to Logic

### Priority

Higher priority wins among eligible providers.

Suggested defaults:

| Provider Type | Priority |
|---|---:|
| trusted wired | 200 |
| trusted wireless | 150 |
| cloud | 100 |
| virtual/simulated | 50 |
| unknown | 0 |

### Freshness

Fresh provider payloads beat stale provider payloads.

Freshness inputs:

```text
status
last_update_ms
latest_payload.stale
latest_payload.available
```

### Failover

Failover occurs when active provider is no longer eligible.

Examples:

```text
wired lost -> wireless active
wireless lost -> wired active
cloud stale -> local active
```

Tie-breaker:

```text
newest last_update_ms, then lowest provider index
```

## 6. Provider Diagnostics

Provider diagnostics are separate from the main capability value.

Common diagnostics:

```text
provider_id
provider_type
status
priority
last_update_ms
owner_module_index
owner_device_index
last_error_code
quality
```

Wireless diagnostics:

```text
node_id
last_seen_ms
battery_present
battery_level_percent
battery_voltage
signal_quality_percent
missed_heartbeat_count
trust_state
```

Diagnostics may be exposed through API and Dashboard, but Logic must continue to use capability IDs.

## 7. Provider Health Model

Provider health states:

```text
UNKNOWN
AVAILABLE
STALE
UNAVAILABLE
LOST
DISABLED
```

Health transitions:

| From | To | Cause |
|---|---|---|
| `UNKNOWN` | `AVAILABLE` | first valid payload |
| `AVAILABLE` | `STALE` | stale timeout |
| `STALE` | `LOST` | lost timeout |
| `AVAILABLE` | `UNAVAILABLE` | explicit read/driver failure |
| `ANY` | `DISABLED` | manual or safety disable |
| `LOST` | `AVAILABLE` | valid recovery payload |

Rules:

- Service owns health transition policy.
- Registry stores resulting health state.
- Registry owns selection based on stored state.
- Logic sees resulting canonical capability availability, not provider health internals.

## 8. Provider Lifecycle

Provider lifecycle:

```text
discovered
registered
stale initial state
available after first valid payload
selected or not selected
stale
unavailable
lost
recovered
disabled
removed future
```

Registration flow:

```text
PNP Discovery
-> PNP Registration
-> ModuleRecord
-> DeviceRecord
-> canonical CapabilityRecord or existing capability
-> CapabilityProviderRecord
-> active provider selection
```

Provider removal is future work and must be bounded. Until removal exists, providers should be marked `DISABLED`, `UNAVAILABLE`, or `LOST`.

## 9. Provider Security Model

Security is provider-specific but must end in a common trust decision.

Trust states may include:

```text
UNKNOWN
TRUSTED
UNTRUSTED
BLOCKED
```

Rules:

- untrusted providers must not become active by default
- unknown wireless actuator providers must not register as controllable
- cloud providers must validate source/authentication
- manual override must not bypass safety policy
- provider security failures must be visible through diagnostics

Security-sensitive provider types:

- wireless actuators
- cloud actuators
- AI-generated command sources
- gateway providers

## 10. Future Actuator Providers

Actuator providers are higher risk than sensor providers.

Examples:

```text
wired relay -> CAP_RELAY_CONTROL
wireless relay -> CAP_RELAY_CONTROL
cloud-controlled relay provider -> future, high risk
```

Rules:

- actuator command execution must go through Services
- Registry stores state only
- provider selection must not execute commands
- Runtime remains scheduler only
- SAFE_MODE and runtime gating apply to actuator Services
- wireless/cloud actuator providers require acknowledgement, timeout, and fail-safe behavior

Actuator provider diagnostics must include:

```text
last_command_state
last_error_code
availability
safe_mode_blocked
timeout state
```

## 11. Future Dashboard Visibility

Dashboard may show provider diagnostics.

Dashboard can display:

- active provider
- provider list
- provider type
- provider health
- priority
- freshness
- last update time
- battery/signal diagnostics
- manual override controls, future

Dashboard must not:

- control Devices directly
- bypass API
- save rules using provider IDs
- send actuator commands outside Services

Dashboard rules must remain capability-first:

```text
IF CAP_TEMPERATURE > 40
THEN ...
```

## 12. Future Mobile Studio Integration

Mobile Studio should present friendly provider information without leaking provider identity into core rules.

User may see:

```text
Temperature Sensor
Wireless Temperature Node
Battery: 87%
Signal: 75%
```

Internal rule remains:

```text
IF CAP_TEMPERATURE.value > 40
THEN ...
```

Visual blocks may include provider diagnostics blocks in future:

```text
CAP_SIGNAL_STRENGTH < 30
CAP_BATTERY_LEVEL < 20
```

Rules:

- visual blocks compile to capability-based Logic rules
- provider IDs are diagnostic metadata only
- app uses API only
- no direct Device or transport access

## 13. Future AI Integration

AI may use provider diagnostics to explain system state or recommend configuration.

Allowed AI uses:

- summarize provider health
- identify likely failing provider
- suggest priority changes
- recommend replacing a weak wireless node
- generate capability-based Logic rules

Restricted AI uses:

- AI must not directly execute actuator commands
- AI must not bypass Services
- AI must not rewrite provider selection without explicit user/API policy
- AI must not make Logic depend on provider IDs

AI-generated rules must compile to:

```text
CAP_* trigger/condition/action model
```

## 14. Validation Strategy

Validation must be layered.

### Provider Storage

Validate:

- provider registration
- duplicate provider rejection
- provider record readback
- provider table bounds

### Active Mapping

Validate:

- set active provider
- update active provider
- missing provider rejection
- capability mismatch rejection

### Selection

Validate:

- priority winner
- freshness winner
- unavailable ignored
- missing capability returns `NOT_FOUND`

### Selected Payload

Validate:

- provider latest payload copied to canonical capability
- canonical `getCapabilityPayload()` returns selected payload
- Logic remains provider-blind

### Provider Health

Validate:

- stale transition
- lost transition
- recovery transition
- unavailable provider not selected

### Ecosystem Providers

Validate each provider family separately:

- wired provider slice
- wireless provider slice
- virtual provider slice
- cloud provider mock slice, future
- actuator provider safety slice

## 15. Production Roadmap

### Stage 1: Provider Foundation

Complete:

- provider record type
- fixed provider table
- active provider mapping
- selection helper
- selected payload update

### Stage 2: Wireless Sensor Provider

Complete:

- simulated ESP-NOW transport
- wireless temperature provider
- WirelessService packet processing
- provider payload update
- selected payload update
- stale/lost validation

### Stage 3: Provider Diagnostics API

Add:

- active provider diagnostics
- provider list summary
- node diagnostics
- provider health state

### Stage 4: Real Wired Providers

Add real local providers:

- I2C sensors
- UART GPS
- SPI display/camera planning
- GPIO inputs/outputs

### Stage 5: Real Wireless Providers

Add:

- real ESP-NOW sensor path
- BLE sensor path, future
- LoRa sensor path, future

### Stage 6: Actuator Provider Safety

Before wireless/cloud actuators:

- command acknowledgement
- command result
- timeout enforcement
- SAFE_MODE behavior
- loss fail-safe
- trust model

### Stage 7: Dashboard And Mobile Studio

Expose:

- provider health
- active provider
- diagnostics
- manual override future

Rules remain capability-first.

### Stage 8: AI-Assisted Provider Operations

Add:

- provider health summaries
- rule suggestions
- diagnostics explanation
- safe configuration recommendations

No direct command bypass.

## Stop Conditions

Stop implementation and review architecture if:

- Logic depends on provider IDs
- Dashboard rules store provider IDs
- Mobile Studio blocks compile to provider-specific rules
- Registry calls Drivers, Devices, Services, Runtime, API, or AI
- Runtime owns provider selection
- API bypasses Registry for provider state
- Services write Registry arrays directly
- provider storage becomes unbounded
- provider selection needs dynamic allocation
- Arduino `String` is introduced
- STL containers are introduced in core provider paths
- cloud or AI providers can command actuators without Services
- wireless actuator providers skip trust and timeout validation

## Final Assessment

Capability providers let Cyber32 support many physical and virtual sources while preserving one stable capability contract.

The long-term model is:

```text
many provider types
one canonical capability ID
one active selected provider
one canonical payload for Logic and API
```

This keeps Cyber32 expandable without making Logic, Dashboard, Mobile Studio, or AI depend on hardware, transport, cloud, or simulation details.
