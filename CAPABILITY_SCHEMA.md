# Cyber32 Capability Schema

Capabilities describe what a system, module, device, or service can do.

Cyber32 is capability-first. Stable capability IDs are the contract used by Registry, Logic, API, Dashboard, and AI tools.

## Capability ID

A capability ID is the internal stable identifier.

Format:

```text
CAP_<DOMAIN>_<ACTION_OR_VALUE>
```

Examples:

```text
CAP_POSITION
CAP_SPEED
CAP_TIME
CAP_TEMPERATURE
CAP_DISTANCE
CAP_BATTERY_LEVEL
CAP_VIDEO_STREAM
CAP_MOTOR_CONTROL
CAP_RELAY_CONTROL
CAP_SEND_NOTIFICATION
```

Rules:

- Must start with `CAP_`
- Must use uppercase ASCII letters, numbers, and underscores
- Must be stable across translations
- Must not include module names
- Must not include manufacturer names
- Must not include board-specific or pin-specific details

## Required Fields

Every capability record must define:

```text
id
name
description
category
kind
data_type
access
source
owner_id
status
```

## Field Definitions

### id

Stable system identifier.

Example:

```text
CAP_BATTERY_LEVEL
```

### name

Human-readable display name.

Example:

```text
Battery Level
```

The name may be translated. Logic must not use this field as an identifier.

### description

Short human-readable explanation of what the capability means.

### category

High-level grouping.

Suggested values:

```text
sensing
motion
power
communication
storage
display
automation
system
security
custom
```

### kind

The behavior shape of the capability.

Allowed values:

```text
sensor
actuator
stream
event
command
state
service
```

### data_type

The value type exposed by the capability.

Allowed values:

```text
boolean
integer
float
string
object
array
binary
none
```

Use `none` for command capabilities that do not return a value.

### unit

Optional measurement unit.

Examples:

```text
percent
degree_celsius
meter
meter_per_second
volt
ampere
second
```

### access

How the capability may be used.

Allowed values:

```text
read
write
execute
read_write
stream
```

### source

Where the capability is provided from.

Allowed values:

```text
device
module
service
system
virtual
```

### owner_id

The registered object that owns or provides this capability.

Examples:

```text
device:device-001
module:module-001
service:telemetry-manager
```

### status

Current availability state.

Allowed values:

```text
available
unavailable
degraded
disabled
unknown
```

## Optional Fields

Capability records may also define:

```text
min_value
max_value
default_value
precision
sample_rate_hz
permissions
metadata
version
deprecated
replacement_id
```

## Example

```text
id: CAP_BATTERY_LEVEL
name: Battery Level
description: Current battery charge level.
category: power
kind: sensor
data_type: float
unit: percent
access: read
source: device
owner_id: device:battery-001
status: available
min_value: 0
max_value: 100
```

## Logic Binding Rule

Logic must bind to `id`.

Correct:

```text
IF CAP_DISTANCE < 0.10
THEN CAP_MOTOR_CONTROL stop
```

Incorrect:

```text
IF Front Ultrasonic Sensor < 10cm
THEN Servo Module stop
```

## Dashboard Rule

Dashboard may show `name`, `description`, `unit`, and translated labels.

Dashboard must keep `id` attached to every logic rule, flow, API command, and saved configuration.
