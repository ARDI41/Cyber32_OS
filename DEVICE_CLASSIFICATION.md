# Cyber32 Device Classification

This document defines the relationship between Drivers, Devices, Modules, and Capabilities.

Cyber32 must remain capability-first. Hardware-specific behavior belongs in lower layers, while higher layers discover and use functionality through capability IDs.

## Relationship Overview

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

The classification chain is:

```text
Driver
-> Device
-> Module
-> Capability
```

## Drivers

Drivers communicate with concrete hardware components through HAL.

Drivers know how to talk to hardware, but they do not define user automation behavior.

Examples:

- Servo driver
- GPS driver
- Battery monitor driver
- OLED display driver
- EEPROM driver
- CAN communication driver

Driver responsibilities:

- Use HAL interfaces
- Hide hardware-specific details
- Provide reusable low-level behavior to Devices
- Convert hardware data into forms usable by Devices

Drivers do not own:

- Plug-and-play registration
- Registry records
- Logic rules
- API endpoints
- Dashboard rendering

## Devices

Devices combine one or more Drivers into a usable system object.

A Device is the first layer where raw hardware behavior becomes a Cyber32 object with state and capabilities.

Examples:

- Servo Device
- GPS Device
- Battery Device
- Relay Device
- Display Device
- Storage Device

Device responsibilities:

- Manage device state
- Coordinate one or more Drivers
- Describe device capabilities
- Provide a stable interface to Modules and Services

Devices do not own:

- Module identity
- Plug-and-play discovery
- Global registry storage
- Automation logic
- Dashboard layout

## Modules

Modules combine one or more Devices into a physical or logical plug-and-play unit.

A Module is what PNP discovers and registers.

Examples:

- Navigation Module
- Motion Module
- Power Module
- Sensor Module
- Communication Module
- Display Module

Module responsibilities:

- Group related Devices
- Provide module metadata
- Declare module-level capabilities
- Participate in PNP discovery and registration
- Expose a coherent functional unit to the system

Modules do not own:

- Low-level hardware control
- Driver internals
- Registry storage
- Logic decisions
- API or Dashboard implementation

## Capabilities

Capabilities describe what Devices, Modules, Services, or the System can do.

Capabilities are the stable contracts used by Registry, Logic, API, Dashboard, and AI tools.

Examples:

- `CAP_POSITION`
- `CAP_DISTANCE`
- `CAP_BATTERY_LEVEL`
- `CAP_MOTOR_CONTROL`
- `CAP_DISPLAY_TEXT`
- `CAP_TELEMETRY_STREAM`

Capability responsibilities:

- Describe a reusable behavior or observable value
- Provide stable IDs for higher layers
- Allow modules to be replaced without breaking Logic
- Allow Registry lookup by function instead of name

Capabilities do not own:

- Hardware implementation
- Driver code
- Module identity
- Dashboard display names

## Classification Examples

### Servo Motion

```text
PWM HAL
-> Servo Driver
-> Servo Device
-> Motion Module
-> CAP_SERVO_POSITION
-> CAP_MOTOR_CONTROL
```

Logic must use `CAP_SERVO_POSITION` or `CAP_MOTOR_CONTROL`, not a specific servo model or pin.

### GPS Position

```text
UART HAL
-> GPS Driver
-> GPS Device
-> Navigation Module
-> CAP_POSITION
-> CAP_SPEED
-> CAP_TIME
```

Logic must use `CAP_POSITION`, not `GPS Module`.

### Battery Monitoring

```text
ADC HAL
-> Battery Monitor Driver
-> Battery Device
-> Power Module
-> CAP_BATTERY_LEVEL
-> CAP_BATTERY_VOLTAGE
```

Logic must use `CAP_BATTERY_LEVEL`, not a specific battery board.

## Ownership Rule

Drivers provide hardware behavior.

Devices provide usable Cyber32 objects.

Modules provide plug-and-play units.

Capabilities provide stable functional contracts.

## Replacement Rule

Any Module may be replaced by another Module if it exposes compatible capabilities.

For example, Logic using `CAP_POSITION` must continue to work when the position provider changes from GPS to RTK, UWB, visual odometry, or another future module.

## Registry Rule

Registry stores relationships between Modules, Devices, Services, Metadata, and Capabilities.

Higher layers should query Registry by capability ID when they need functionality.
