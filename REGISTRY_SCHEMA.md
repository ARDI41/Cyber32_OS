# Cyber32 Registry Schema

Registry is the central catalog for Cyber32 system objects.

Registry stores registered Modules, Devices, Services, Capabilities, and Metadata. It makes objects discoverable through stable IDs and capabilities.

Registry does not perform plug-and-play, execute drivers, control hardware, evaluate logic, or render UI.

## Registry Collections

```text
modules
devices
services
capabilities
metadata
```

## Common Record Fields

All registry records should include:

```text
id
type
status
created_at
updated_at
metadata_id
```

### id

Stable registry identifier.

### type

Record type or category.

### status

Allowed values:

```text
registered
available
unavailable
degraded
disabled
removed
unknown
```

### created_at

Timestamp when the record was first registered.

### updated_at

Timestamp when the record was last changed.

### metadata_id

Reference to a metadata registry record.

## Module Record

Required fields:

```text
id
type
status
metadata_id
device_ids
capability_ids
pnp_level
```

Example:

```text
id: module-navigation-001
type: navigation
status: available
metadata_id: metadata-module-navigation-001
device_ids:
  - device-gps-001
capability_ids:
  - CAP_POSITION
  - CAP_SPEED
  - CAP_TIME
pnp_level: 2
```

## Device Record

Required fields:

```text
id
type
status
metadata_id
module_id
driver_ids
capability_ids
```

Example:

```text
id: device-gps-001
type: sensor
status: available
metadata_id: metadata-device-gps-001
module_id: module-navigation-001
driver_ids:
  - driver-uart-gps
capability_ids:
  - CAP_POSITION
  - CAP_SPEED
  - CAP_TIME
```

## Service Record

Required fields:

```text
id
type
status
metadata_id
capability_ids
dependencies
```

Example:

```text
id: telemetry-manager
type: telemetry
status: available
metadata_id: metadata-service-telemetry-manager
capability_ids:
  - CAP_TELEMETRY_STREAM
dependencies:
  - network-manager
```

## Capability Record

Required fields are defined in `CAPABILITY_SCHEMA.md`.

Registry must be able to search capabilities by:

- capability ID
- owner ID
- source
- category
- status

## Metadata Record

Required fields:

```text
id
owner_id
owner_type
name
description
version
manufacturer
raw_metadata
```

Example:

```text
id: metadata-module-navigation-001
owner_id: module-navigation-001
owner_type: module
name: GPS Module
description: Navigation module providing position, speed, and time.
version: 1.0.0
manufacturer: Cyber32
raw_metadata: <module metadata payload>
```

## Lookup Requirements

Registry must support these conceptual lookups:

```text
find module by id
find device by id
find service by id
find capability by id
find devices by capability id
find modules by capability id
find services by capability id
find metadata by owner id
```

## Capability-First Rule

Any higher layer that needs functionality should query by capability ID first.

Examples:

```text
find providers of CAP_POSITION
find providers of CAP_BATTERY_LEVEL
find providers of CAP_MOTOR_CONTROL
```

Higher layers must not require a specific module name when an equivalent capability exists.

## Registration Rule

PNP is responsible for creating registry records after discovery, identification, and compatibility checks.

Registry is responsible for storing and exposing records, not for deciding how hardware is discovered.

## Removal Rule

When a module is removed, Registry records should be marked unavailable or removed in a controlled way.

Logic and Services must receive capability availability changes through documented events or lookups.
