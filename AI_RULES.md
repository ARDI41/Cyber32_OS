# Cyber32 AI Rules

Cyber32 is AI ready, but AI must work inside the architecture instead of redesigning it.

These rules apply to AI agents, generated code, generated documentation, and automated development tools.

## Prime Directive

Do not redesign the architecture.

The fixed layer order is:

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

## Documentation First

Before implementation, update or create the relevant documentation.

Implementation must follow documented contracts.

If a contract is missing, document the contract before writing code.

## Capability First

AI must model system behavior through capabilities.

Use capability IDs such as:

```text
CAP_POSITION
CAP_DISTANCE
CAP_BATTERY_LEVEL
CAP_MOTOR_CONTROL
```

Do not bind Logic to:

- module names
- device names
- driver names
- manufacturer names
- board names
- pins

## Logic Rule

Logic must never depend on specific module names.

Correct:

```text
IF CAP_BATTERY_LEVEL < 20
THEN CAP_SEND_NOTIFICATION
```

Incorrect:

```text
IF Battery Module < 20
THEN GPS Module sends notification
```

## Hardware Agnostic Rule

Generated work must keep Cyber32 portable across hardware platforms.

Platform-specific behavior belongs in Platform, HAL, or Drivers, depending on the responsibility.

## Plug-and-Play Rule

PNP behavior must respect the three levels:

- Level 1: Passive identification
- Level 2: I2C metadata
- Level 3: Smart module EEPROM

AI must not skip PNP by hardcoding a module directly into Logic, API, or Dashboard.

## Registry Rule

Registry is the source of truth for registered objects.

AI-generated higher-layer behavior should discover functionality through Registry and capability IDs.

## Boundary Rule

Do not move responsibilities across layers.

Examples:

- HAL must not know Logic.
- Drivers must not render Dashboard UI.
- Devices must not perform PNP registration.
- Modules must not evaluate automation rules.
- Logic must not control GPIO, I2C, PWM, UART, CAN, or SPI directly.
- Dashboard must not bypass API to control hardware.

## Naming Rule

Human-readable names are display metadata.

Stable behavior must use IDs:

- capability IDs
- module IDs
- device IDs
- service IDs
- metadata IDs

## Safety Rule

When uncertain, prefer:

1. Update documentation.
2. Define the interface.
3. Add validation rules.
4. Implement the smallest layer-correct slice.

Do not add broad abstractions or shortcuts that weaken the fixed architecture.

## Current Prototype Rule

Existing direct hardware prototypes are allowed as historical or temporary demos.

They must not be treated as the target architecture.

When moving prototype behavior into Cyber32, place each responsibility in the correct layer.
