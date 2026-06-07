# Cyber32 Mobile Studio Vision

This document defines the future Cyber32 mobile app and visual logic builder vision.

Mobile Studio is future v2+.

This document does not expand ESP32 v1 scope.

## Scope Rule

Cyber32 ESP32 v1 must only expose capability-first API contracts.

Mobile Studio must use those contracts later.

ESP32 v1 does not need to implement the mobile app, rich visual logic builder, app marketplace, cloud sync, or advanced UI features.

## Purpose

Cyber32 Mobile Studio is a future user-facing app for:

- viewing Cyber32 systems
- seeing connected modules, devices, and capabilities
- building automation visually
- sending safe commands through API
- configuring capability-based logic
- monitoring telemetry
- translating technical capability data into human-friendly views

Mobile Studio is a user-facing layer. It is not part of HAL, Drivers, Devices, Modules, PNP, Registry, Runtime, Services, or Logic execution.

## Fixed Architecture Relationship

The fixed Cyber32 architecture remains:

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

Mobile Studio belongs at the Dashboard/App layer.

Mobile Studio must use API.

Mobile Studio must never access Devices directly.

Mobile Studio must never bypass Services.

## Core Rules

- Mobile app is future v2+.
- ESP32 v1 must only expose capability-first API contracts.
- Mobile app must never depend on module names.
- Visual blocks must compile into capability-based Logic rules.
- Dashboard/App is only a user-facing layer.
- App must use API, not direct Device access.
- App may show friendly names, icons, and translations.
- Core system must preserve `CAP_*` IDs underneath.

## Visual-Code Concept

Mobile Studio should let users build robotics and automation behavior without writing code.

User-facing blocks may look like:

```text
Distance sensor < 30 cm -> Stop motor
```

Internal rule:

```text
IF CAP_DISTANCE.value < 0.30
THEN CAP_MOTOR_CONTROL command stop
```

The app may show friendly names. The saved rule must use capability IDs.

## Capability Block Model

A capability block represents a Cyber32 capability.

Block display may include:

- friendly name
- icon
- translated label
- unit display
- current value
- availability state
- provider display name

Block internal data must preserve:

```text
capability_id
schema_version
payload_field
unit
operator
value
command
params
```

Example user-facing block:

```text
Battery level
```

Internal reference:

```text
CAP_BATTERY_LEVEL.value
```

The app may show `Battery level`, `Aku tase`, or an icon. The rule must store `CAP_BATTERY_LEVEL`.

## Trigger Blocks

Trigger blocks define when Logic starts evaluating.

Examples:

User-facing:

```text
When distance changes
When battery drops below threshold
When system starts
When capability becomes unavailable
```

Internal:

```text
TRIGGER CAP_DISTANCE value_changed
TRIGGER CAP_BATTERY_LEVEL threshold_crossed
TRIGGER EVT_BOOT_READY
TRIGGER EVT_CAPABILITY_UNAVAILABLE
```

Trigger blocks must not bind to module names.

## Condition Blocks

Condition blocks compare capability payload fields or system state.

Examples:

User-facing:

```text
Distance sensor < 30 cm
Battery level < 20%
Temperature > 50 C
Position fix is available
```

Internal:

```text
CAP_DISTANCE.value < 0.30
CAP_BATTERY_LEVEL.value < 20.0
CAP_TEMPERATURE.value > 50.0
CAP_POSITION.value.fix == true
```

Condition blocks must use canonical units internally.

Dashboard/App may translate units for display.

## Action Blocks

Action blocks request commands through capability IDs.

Examples:

User-facing:

```text
Stop motor
Move servo to 90 degrees
Show text
Play warning tone
```

Internal:

```text
CAP_MOTOR_CONTROL command stop
CAP_SERVO_POSITION command set_position position_degree 90.0
CAP_DISPLAY_TEXT command write text "Warning"
CAP_AUDIO_OUTPUT command tone frequency_hz 1000 duration_ms 500
```

Action blocks must compile into command payloads defined by capability payload schemas and command dispatch contracts.

## Example Visual Rules

### Obstacle Stop

User sees:

```text
Distance sensor < 30 cm -> Stop motor
```

Internal rule:

```text
IF CAP_DISTANCE.value < 0.30
THEN CAP_MOTOR_CONTROL command stop
```

### Low Battery Warning

User sees:

```text
Battery level < 20% -> Play warning tone
```

Internal rule:

```text
IF CAP_BATTERY_LEVEL.value < 20.0
THEN CAP_AUDIO_OUTPUT command tone frequency_hz 1000 duration_ms 500
```

### Temperature Display

User sees:

```text
Temperature > 40 C -> Show "Hot"
```

Internal rule:

```text
IF CAP_TEMPERATURE.value > 40.0
THEN CAP_DISPLAY_TEXT command write text "Hot"
```

### Lost Capability Fallback

User sees:

```text
If distance sensor is unavailable -> Stop motor
```

Internal rule:

```text
IF CAP_DISTANCE.available == false
THEN CAP_MOTOR_CONTROL command stop
```

## What ESP32 v1 Requires Now

ESP32 v1 only needs to provide the contracts that a future Mobile Studio can use.

Required now:

- stable `CAP_*` capability IDs
- capability payload schemas
- capability-first API endpoints
- machine-readable error codes
- bounded Registry state
- event model for capability changes
- command dispatch contract
- Logic rules that bind to capability IDs
- Dashboard/API preservation of capability IDs

ESP32 v1 does not need:

- native mobile app
- visual block editor
- cloud synchronization
- account system
- project sharing
- app marketplace
- drag-and-drop UI
- rich charts
- mobile push notifications

## Deferred To v2+

Deferred Mobile Studio features:

- iOS app
- Android app
- desktop visual editor
- visual block editor
- rule simulation
- rule debugger
- block templates
- reusable automation recipes
- icon libraries
- translations
- cloud backups
- multi-robot management
- remote access
- marketplace for modules and blocks
- visual dashboard builder
- firmware update UI

Deferred features must still respect capability-first architecture when implemented.

## How Current Docs Support The Future App

Current Cyber32 docs already provide the foundation:

| Document | Future App Support |
|---|---|
| `ARCHITECTURE.md` | Keeps Dashboard/App as user-facing layer. |
| `CAPABILITY_SCHEMA.md` | Defines stable capability IDs. |
| `CAPABILITY_CATALOG.md` | Defines first official capability list. |
| `CAPABILITY_PAYLOAD_SCHEMA.md` | Defines machine-readable payload fields. |
| `COMMAND_DISPATCH_CONTRACT.md` | Defines safe command request path. |
| `EVENT_MODEL.md` | Defines capability and system change notifications. |
| `ERROR_MODEL.md` | Defines machine-readable error codes for display and recovery. |
| `REGISTRY_IMPLEMENTATION_PLAN.md` | Defines where current capability state lives. |
| `RUNTIME_IMPLEMENTATION_PLAN.md` | Defines bounded execution and safe mode behavior. |
| `ESP32_V1_SCOPE.md` | Keeps v1 realistic and prevents app scope creep. |

## API Relationship

Mobile Studio must use API.

Allowed conceptual flow:

```text
Mobile Studio
-> API
-> Services
-> Runtime
-> Devices
-> Drivers
-> HAL
```

State observation:

```text
Registry
-> API
-> Mobile Studio
```

Command request:

```text
Mobile Studio
-> API command by capability_id
-> Service validation
-> Runtime coordination
-> Device execution
-> Registry command state
-> API status
-> Mobile Studio display
```

Not allowed:

```text
Mobile Studio -> Device
Mobile Studio -> Driver
Mobile Studio -> HAL
Mobile Studio -> module-name-based Logic
```

## Friendly Names And Translation

Mobile Studio may show:

- friendly names
- translated labels
- icons
- module display names
- device display names
- grouped categories
- human units

Mobile Studio must store and send:

- `capability_id`
- canonical payload field
- canonical unit
- command name
- command params
- schema version

Friendly display names are never the core rule identity.

## Future Visual Rule Storage

Future visual rules should preserve both display and machine-readable forms.

Conceptual stored rule:

```text
rule_id: rule-obstacle-stop
display_name: Obstacle stop
trigger:
  capability_id: CAP_DISTANCE
  event: value_changed
condition:
  capability_id: CAP_DISTANCE
  field: value
  operator: <
  value: 0.30
  unit: meter
action:
  capability_id: CAP_MOTOR_CONTROL
  command: stop
schema_version: 1
```

The display name can change. The capability IDs remain stable.

## Success Criteria

The Mobile Studio vision is successful when future app features:

1. Use API only.
2. Preserve `CAP_*` IDs underneath friendly UI.
3. Compile visual blocks into capability-based Logic rules.
4. Never depend on module names.
5. Never access Devices directly.
6. Keep Dashboard/App as a user-facing layer only.
7. Do not expand ESP32 v1 implementation scope.
