# Cyber32 Architecture

Cyber32 is a capability-first robotics operating system.

The architecture is fixed. It must not be redesigned, reordered, or bypassed.

## Fixed Layer Order

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

Each layer may depend only on the contracts exposed by lower layers. Higher layers must not leak implementation details back into lower layers.

## Core Principles

- Documentation first
- Capability first
- Plug and play
- Hardware agnostic
- AI ready

## Layer Responsibilities

### HAL

HAL abstracts hardware access such as GPIO, I2C, SPI, UART, CAN, PWM, ADC, storage, and time.

HAL does not know devices, modules, plug-and-play, logic, API, or dashboard behavior.

### Drivers

Drivers use HAL to communicate with concrete hardware components.

Drivers hide hardware-specific details and expose reusable behavior for Devices.

Drivers do not own plug-and-play, registry, services, logic, API, or dashboard behavior.

### Devices

Devices combine one or more Drivers into a usable system object.

Devices describe capabilities and state in a hardware-agnostic way.

Devices do not perform plug-and-play registration, global registry management, or application logic.

### Modules

Modules combine one or more Devices into a physical or logical plug-and-play unit.

Modules carry identity, metadata, and capability descriptions.

Modules do not control low-level hardware directly and do not contain user automation logic.

### PNP

PNP discovers, identifies, validates, and registers modules.

PNP levels are:

- Level 1: Passive identification
- Level 2: I2C metadata
- Level 3: Smart module EEPROM

PNP does not execute drivers, own application logic, render UI, or directly control hardware.

### Registry

Registry is the central catalog of registered Modules, Devices, Services, Capabilities, and Metadata.

Registry stores what exists and how it can be found.

Registry does not discover modules, control hardware, execute runtime behavior, or evaluate logic.

### Runtime

Runtime manages component lifecycle, scheduling, tasks, state machines, and resources.

Runtime turns registered components into a controlled running system.

Runtime does not perform plug-and-play, own registry data, or contain application logic.

### Services

Services coordinate higher-level system functions such as device management, module management, storage, power, network, telemetry, OTA, and security.

Services may use Core, Registry, Runtime, Devices, and Modules through their contracts.

Services do not implement low-level drivers, HAL behavior, or dashboard rendering.

### Logic

Logic is the automation and decision layer.

Logic must operate on capabilities, not specific module names, device names, manufacturers, or driver names.

Correct:

```text
IF CAP_BATTERY_LEVEL < 20
THEN CAP_SEND_NOTIFICATION
```

Incorrect:

```text
IF Battery Module < 20
THEN GPS Module sends warning
```

Logic does not control hardware directly, discover modules, manage the registry, or render UI.

### API

API exposes Cyber32 functionality to internal components, external clients, REST clients, WebSocket clients, and the Dashboard.

API exposes system state, registered objects, capabilities, commands, telemetry, and logic controls through stable contracts.

API does not own hardware behavior, plug-and-play, registry storage, or UI rendering.

### Dashboard

Dashboard is the user interface for viewing state, managing modules, building logic, and controlling the system without programming.

Dashboard may display translated or friendly capability names.

Dashboard must preserve capability IDs in all system-facing logic and API operations.

Dashboard does not control hardware directly, run drivers, perform plug-and-play, or manage the registry.

## Dependency Rule

The system must flow upward through the fixed architecture.

Lower layers must stay generic and reusable. Higher layers may coordinate lower layers only through documented contracts.

No layer may depend on a concrete module name unless that layer is documentation, examples, tests, or dashboard display text.

## Capability Rule

Capabilities are stable system contracts.

Logic, API, Registry, and Dashboard integrations must use capability IDs such as `CAP_POSITION`, `CAP_DISTANCE`, and `CAP_BATTERY_LEVEL`.

Human-readable names are display metadata only.

## Hardware Agnostic Rule

Cyber32 must allow a module to be replaced by another module with equivalent capabilities without breaking Logic.

For example, a rule using `CAP_POSITION` must keep working if the position source changes from GPS to RTK, indoor UWB, visual odometry, or a future navigation module.
