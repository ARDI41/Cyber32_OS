# Cyber32 Module Metadata Schema

Module metadata describes a Cyber32 module without requiring the system to know the module implementation.

PNP reads metadata, checks compatibility, registers Devices and Capabilities, and makes the module available through Registry.

## PNP Levels

### Level 1: Passive Identification

Passive identification uses a resistor, capacitor, pin pattern, voltage divider, or similar passive signal.

Level 1 metadata is minimal and may require a built-in lookup table.

Required result:

```text
passive_id
module_type
metadata_level
```

### Level 2: I2C Metadata

I2C metadata allows a module to describe itself over I2C.

Required result:

```text
module_id
name
version
manufacturer
module_type
capabilities
devices
metadata_level
```

### Level 3: Smart Module EEPROM

Smart module EEPROM stores persistent module metadata on the module.

Level 3 may include calibration, serial number, security data, manufacturing data, configuration defaults, and compatibility constraints.

Required result:

```text
module_id
serial_number
name
version
manufacturer
module_type
capabilities
devices
metadata_level
```

## Required Metadata Fields

Every complete module metadata record must define:

```text
module_id
metadata_level
name
version
manufacturer
module_type
capabilities
devices
compatibility
```

## Field Definitions

### module_id

Stable module identifier.

For Level 1 modules, this may be derived from a passive lookup table.

### metadata_level

PNP level used to identify the module.

Allowed values:

```text
1
2
3
```

### name

Human-readable module name.

This is display metadata only. Logic must not depend on it.

### version

Module metadata or firmware version.

### manufacturer

Manufacturer or project name.

This is metadata only and must not be used by Logic.

### module_type

Broad module classification.

Suggested values:

```text
sensing
motion
navigation
power
communication
storage
display
automation
custom
```

### capabilities

List of capability IDs provided by the module or its devices.

Example:

```text
CAP_POSITION
CAP_SPEED
CAP_TIME
```

### devices

List of device descriptions included in the module.

Each device should include:

```text
device_id
device_type
driver_requirements
capabilities
```

### compatibility

Requirements that must be checked before registration.

Suggested fields:

```text
min_os_version
protocol_version
required_services
required_capabilities
required_buses
power_requirements
```

## Optional Metadata Fields

Level 2 and Level 3 modules may define:

```text
description
model
serial_number
supported_protocols
configuration
calibration
security
resources
dashboard_hints
documentation_url
```

`dashboard_hints` may help presentation, but must not change capability behavior.

## Example

```text
module_id: module-navigation-001
metadata_level: 2
name: GPS Module
version: 1.0.0
manufacturer: Cyber32
module_type: navigation
capabilities:
  - CAP_POSITION
  - CAP_SPEED
  - CAP_TIME
devices:
  - device_id: gps-device-001
    device_type: sensor
    driver_requirements:
      - uart
    capabilities:
      - CAP_POSITION
      - CAP_SPEED
      - CAP_TIME
compatibility:
  min_os_version: 0.1.0
  protocol_version: 0.1
  required_buses:
    - uart
```

## Registration Rule

PNP must not register a module as usable until metadata has been read and compatibility has passed.

Registration must create or update Registry entries for:

- Module
- Devices
- Capabilities
- Metadata

## Logic Isolation Rule

Metadata may contain module names and manufacturer names.

Logic must ignore those names and bind only to capability IDs.
