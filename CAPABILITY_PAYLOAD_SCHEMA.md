# Cyber32 Capability Payload Schema

This document defines canonical payload structures for Cyber32 capabilities.

Cyber32 is capability-first. All layers above Registry must interact through capability IDs and capability payloads.

Registry stores current payload state. Events announce payload changes.

Logic must never depend on module names.

## Purpose

Capability payloads define the machine-readable values, commands, and state exchanged through Cyber32.

Payloads are used by:

- Registry
- Runtime
- Services
- Logic
- API
- Dashboard

Payloads must be compact enough for ESP32 v1.

## Design Principles

- Use capability IDs as the stable contract.
- Keep payloads compact and bounded.
- Use machine-readable fields.
- Use explicit units.
- Use timestamps for sampled values.
- Represent unavailable and stale values consistently.
- Avoid module names, device names, driver names, and manufacturer names in payload logic.
- Keep display formatting out of core payloads.
- Keep large metadata out of payloads.
- Keep API payloads safe to validate.

## Common Field Naming Rules

Field names must:

- use lowercase ASCII
- use `snake_case`
- be stable across translations
- describe data, not UI labels
- avoid module names
- avoid manufacturer names

Correct:

```text
value
unit
timestamp_ms
available
stale
accuracy_m
```

Incorrect:

```text
gps_value
prettyTemperature
AcmeBatteryStatus
dashboard_label
```

## Common Payload Envelope

Read and read-write capabilities should use this common envelope:

```text
capability_id
schema_version
timestamp_ms
available
stale
value
unit
quality
error_code
```

Command/write capabilities should use:

```text
capability_id
schema_version
command
params
request_id
timeout_ms
```

Fields may be omitted only when the capability schema says they are not used.

## Datatype Rules

Allowed datatypes:

```text
boolean
integer
float
string
enum
object
array
none
```

Rules:

- Use `float` for measured numeric values.
- Use `integer` for counts, byte sizes, and timestamps.
- Use `enum` for bounded states and modes.
- Use `object` only when multiple named fields are required.
- Avoid arrays unless the maximum length is documented.
- Avoid arbitrary nested objects in ESP32 v1.
- Avoid large strings.

## Units And Measurement Rules

Units must be explicit and stable.

Canonical units:

```text
degree_celsius
percent
pascal
meter
meter_per_second
degree
volt
byte
millisecond
```

Rules:

- Internal numeric values should use canonical units.
- Dashboard may translate units for humans.
- API must expose units in machine-readable form.
- Logic must compare canonical values, not translated display strings.

## Timestamps

Use system uptime milliseconds:

```text
timestamp_ms
```

Rules:

- `timestamp_ms` is required for sampled read values.
- `timestamp_ms` is optional for pure commands.
- Do not use formatted date/time strings in v1 payloads.
- If real-world time exists, expose it through a separate time capability.

## Precision Rules

Each numeric payload should define expected precision.

Rules:

- Store only useful precision.
- Avoid excessive decimal precision in API responses.
- Use capability-specific precision for comparisons.
- Logic thresholds should account for sensor noise.

Recommended examples:

```text
temperature: 0.1 degree_celsius
humidity: 0.1 percent
pressure: 1 pascal
distance: 0.001 meter
position latitude/longitude: 0.000001 degree
battery level: 0.1 percent
voltage: 0.001 volt
```

## Null And Unavailable Handling

Unavailable data must be explicit.

Rules:

- If a capability is unavailable, set `available` to `false`.
- If `available` is `false`, `value` may be omitted or set to `null`.
- Include `error_code` when unavailable due to known failure.
- Logic must not treat unavailable as zero.
- Dashboard may display unavailable in a human-friendly way.

Example:

```text
capability_id: CAP_DISTANCE
schema_version: 1
timestamp_ms: 12000
available: false
stale: false
value: null
unit: meter
quality: unavailable
error_code: ERR_DEVICE_TIMEOUT
```

## Stale Value Handling

Stale means the last value exists but is older than the capability's allowed freshness window.

Rules:

- If a value is stale, set `stale` to `true`.
- Keep `available` true only if the provider is still present.
- Logic must treat stale safety-critical values conservatively.
- Events may announce stale transitions with `EVT_CAPABILITY_VALUE_UPDATED` or `EVT_CAPABILITY_UNAVAILABLE`.

Each capability should define a freshness expectation.

Example:

```text
capability_id: CAP_BATTERY_LEVEL
schema_version: 1
timestamp_ms: 50000
available: true
stale: true
value: 31.2
unit: percent
quality: stale
error_code: none
```

## Versioning Rules

Every payload schema must include:

```text
schema_version: 1
```

Rules:

- Additive optional fields may keep the same major version.
- Renamed fields require a new schema version.
- Unit changes require a new schema version.
- Datatype changes require a new schema version.
- Logic should validate schema version before binding.
- API should reject unsupported payload versions.

## Registry And Event Rules

Registry stores:

- latest payload state
- availability
- stale state
- timestamp
- error state

Events announce:

- payload value updated
- capability registered
- capability unavailable
- capability error
- stale state change

Event payloads must remain compact and may include only a small inline value. Consumers needing full state should query Registry.

## Capability Payload Schemas

### CAP_TEMPERATURE

Capability ID:

```text
CAP_TEMPERATURE
```

Direction:

```text
read
```

Datatype:

```text
float
```

Payload fields:

```text
capability_id
schema_version
timestamp_ms
available
stale
value
unit
quality
error_code
```

Units:

```text
degree_celsius
```

Valid range:

```text
-40.0 to 125.0
```

Precision:

```text
0.1 degree_celsius
```

Update behavior:

Temperature updates when the provider samples a new value or when availability/stale state changes. Recommended freshness window is 5000 ms.

### CAP_HUMIDITY

Capability ID:

```text
CAP_HUMIDITY
```

Direction:

```text
read
```

Datatype:

```text
float
```

Payload fields:

```text
capability_id
schema_version
timestamp_ms
available
stale
value
unit
quality
error_code
```

Units:

```text
percent
```

Valid range:

```text
0.0 to 100.0
```

Precision:

```text
0.1 percent
```

Update behavior:

Humidity updates when sampled or when availability/stale state changes. Recommended freshness window is 5000 ms.

### CAP_PRESSURE

Capability ID:

```text
CAP_PRESSURE
```

Direction:

```text
read
```

Datatype:

```text
float
```

Payload fields:

```text
capability_id
schema_version
timestamp_ms
available
stale
value
unit
quality
error_code
```

Units:

```text
pascal
```

Valid range:

```text
30000 to 120000
```

Precision:

```text
1 pascal
```

Update behavior:

Pressure updates when sampled or when availability/stale state changes. Recommended freshness window is 5000 ms.

### CAP_DISTANCE

Capability ID:

```text
CAP_DISTANCE
```

Direction:

```text
read
```

Datatype:

```text
float
```

Payload fields:

```text
capability_id
schema_version
timestamp_ms
available
stale
value
unit
quality
error_code
```

Units:

```text
meter
```

Valid range:

```text
0.0 to 100.0
```

Precision:

```text
0.001 meter
```

Update behavior:

Distance updates when sampled or when availability/stale state changes. Recommended freshness window is 1000 ms for robotics use.

### CAP_POSITION

Capability ID:

```text
CAP_POSITION
```

Direction:

```text
read
```

Datatype:

```text
object
```

Payload fields:

```text
capability_id
schema_version
timestamp_ms
available
stale
value.latitude
value.longitude
value.altitude
value.fix
value.accuracy_m
unit
quality
error_code
```

Units:

```text
latitude: degree
longitude: degree
altitude: meter
accuracy_m: meter
fix: boolean
```

Valid range:

```text
latitude: -90.0 to 90.0
longitude: -180.0 to 180.0
altitude: -500.0 to 10000.0
accuracy_m: 0.0 to 10000.0
fix: true or false
```

Precision:

```text
latitude/longitude: 0.000001 degree
altitude: 0.1 meter
accuracy_m: 0.1 meter
```

Update behavior:

Position updates when a new valid position sample is available, when fix state changes, or when stale/unavailable state changes. Recommended freshness window is 2000 ms for moving robotics.

### CAP_SPEED

Capability ID:

```text
CAP_SPEED
```

Direction:

```text
read
```

Datatype:

```text
float
```

Payload fields:

```text
capability_id
schema_version
timestamp_ms
available
stale
value
unit
quality
error_code
```

Units:

```text
meter_per_second
```

Valid range:

```text
0.0 to 100.0
```

Precision:

```text
0.01 meter_per_second
```

Update behavior:

Speed updates when sampled or calculated, or when availability/stale state changes. Recommended freshness window is 1000 ms.

### CAP_HEADING

Capability ID:

```text
CAP_HEADING
```

Direction:

```text
read
```

Datatype:

```text
float
```

Payload fields:

```text
capability_id
schema_version
timestamp_ms
available
stale
value
unit
quality
error_code
```

Units:

```text
degree
```

Valid range:

```text
0.0 to less than 360.0
```

Precision:

```text
0.1 degree
```

Update behavior:

Heading updates when sampled or calculated, or when availability/stale state changes. Recommended freshness window is 1000 ms.

### CAP_BATTERY_LEVEL

Capability ID:

```text
CAP_BATTERY_LEVEL
```

Direction:

```text
read
```

Datatype:

```text
float
```

Payload fields:

```text
capability_id
schema_version
timestamp_ms
available
stale
value
unit
quality
error_code
```

Units:

```text
percent
```

Valid range:

```text
0.0 to 100.0
```

Precision:

```text
0.1 percent
```

Update behavior:

Battery level updates on periodic power sampling or when threshold-relevant changes occur. Recommended freshness window is 10000 ms.

### CAP_BATTERY_VOLTAGE

Capability ID:

```text
CAP_BATTERY_VOLTAGE
```

Direction:

```text
read
```

Datatype:

```text
float
```

Payload fields:

```text
capability_id
schema_version
timestamp_ms
available
stale
value
unit
quality
error_code
```

Units:

```text
volt
```

Valid range:

```text
0.0 to 60.0
```

Precision:

```text
0.001 volt
```

Update behavior:

Battery voltage updates on periodic power sampling or when threshold-relevant changes occur. Recommended freshness window is 5000 ms.

### CAP_STORAGE_USAGE

Capability ID:

```text
CAP_STORAGE_USAGE
```

Direction:

```text
read
```

Datatype:

```text
object
```

Payload fields:

```text
capability_id
schema_version
timestamp_ms
available
stale
value.used_bytes
value.total_bytes
value.free_bytes
value.used_percent
unit
quality
error_code
```

Units:

```text
used_bytes: byte
total_bytes: byte
free_bytes: byte
used_percent: percent
```

Valid range:

```text
used_bytes: 0 to total_bytes
total_bytes: 0 to platform maximum
free_bytes: 0 to total_bytes
used_percent: 0.0 to 100.0
```

Precision:

```text
bytes: integer
used_percent: 0.1 percent
```

Update behavior:

Storage usage updates after storage write/delete operations or periodic storage checks. Recommended freshness window is 30000 ms.

### CAP_DISPLAY_TEXT

Capability ID:

```text
CAP_DISPLAY_TEXT
```

Direction:

```text
write
```

Datatype:

```text
object
```

Payload fields:

```text
capability_id
schema_version
command
params.text
params.row
params.column
params.clear_before_write
request_id
timeout_ms
```

Units:

```text
none
```

Valid range:

```text
text: 0 to 64 ASCII or UTF-8 bytes for v1
row: 0 to display row maximum
column: 0 to display column maximum
clear_before_write: true or false
timeout_ms: 0 to 5000
```

Precision:

```text
not applicable
```

Update behavior:

Display text writes execute on command. Registry may store last command status, not large display buffers. Events may announce command accepted, completed, or failed.

### CAP_AUDIO_OUTPUT

Capability ID:

```text
CAP_AUDIO_OUTPUT
```

Direction:

```text
write
```

Datatype:

```text
object
```

Payload fields:

```text
capability_id
schema_version
command
params.mode
params.frequency_hz
params.duration_ms
params.volume_percent
request_id
timeout_ms
```

Units:

```text
frequency_hz: hertz
duration_ms: millisecond
volume_percent: percent
```

Valid range:

```text
mode: tone, beep, stop
frequency_hz: 20 to 20000
duration_ms: 0 to 10000
volume_percent: 0.0 to 100.0
timeout_ms: 0 to 10000
```

Precision:

```text
frequency_hz: integer
duration_ms: integer
volume_percent: 1 percent
```

Update behavior:

Audio output writes execute on command. Registry may store last output command status. Events may announce accepted, completed, stopped, or failed.

### CAP_MOTOR_CONTROL

Capability ID:

```text
CAP_MOTOR_CONTROL
```

Direction:

```text
write
```

Datatype:

```text
object
```

Payload fields:

```text
capability_id
schema_version
command
params.mode
params.speed_percent
params.direction
params.duration_ms
request_id
timeout_ms
```

Units:

```text
speed_percent: percent
duration_ms: millisecond
```

Valid range:

```text
mode: stop, coast, brake, set_speed
speed_percent: -100.0 to 100.0
direction: forward, reverse, left, right, none
duration_ms: 0 to 60000
timeout_ms: 0 to 60000
```

Precision:

```text
speed_percent: 0.1 percent
duration_ms: integer
```

Update behavior:

Motor control writes execute on command and should fail safe. Stop/brake commands are safety-relevant and should be prioritized. Registry may store last command status and current requested motor state.

### CAP_SERVO_POSITION

Capability ID:

```text
CAP_SERVO_POSITION
```

Direction:

```text
read-write
```

Datatype:

```text
float
```

Payload fields for read:

```text
capability_id
schema_version
timestamp_ms
available
stale
value
unit
quality
error_code
```

Payload fields for write:

```text
capability_id
schema_version
command
params.position_degree
params.duration_ms
request_id
timeout_ms
```

Units:

```text
degree
```

Valid range:

```text
position_degree: 0.0 to 180.0
duration_ms: 0 to 10000
timeout_ms: 0 to 10000
```

Precision:

```text
0.1 degree
```

Update behavior:

Servo position read updates when the requested or measured position changes. Servo writes request a target position. Registry may store current requested position and last command status.

## Dashboard Rule

Dashboard may translate payloads for humans.

Dashboard may display:

- friendly names
- translated units
- formatted values
- warnings
- charts

Dashboard must preserve:

- `capability_id`
- `schema_version`
- canonical units
- machine-readable values

## Logic Rule

Logic must bind to capability IDs and canonical payload fields only.

Correct:

```text
IF CAP_BATTERY_LEVEL.value < 20.0
THEN CAP_AUDIO_OUTPUT mode beep
```

Incorrect:

```text
IF Battery Module is low
THEN Buzzer Board beep
```

## ESP32 Compactness Rule

ESP32 v1 payloads should avoid verbose JSON internally.

Implementation may represent payloads as structs, enums, compact IDs, and fixed-size fields. API may serialize them into JSON only at the boundary.

Large payloads are not allowed in v1 capability state.
