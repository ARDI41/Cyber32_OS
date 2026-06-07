# Cyber32 Boot Sequence

This document describes the Cyber32 startup flow.

The startup order follows the fixed architecture:

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

Boot must prepare each layer before higher layers depend on it.

## 1. HAL

Initialize hardware abstraction interfaces.

Examples:

- GPIO
- I2C
- SPI
- UART
- CAN
- PWM
- ADC
- Storage
- Time

Output:

```text
hal_ready
available_buses
available_timers
available_storage_interfaces
platform_capabilities
```

HAL does not create Devices, run PNP, or expose API.

## 2. Drivers

Initialize driver infrastructure and available driver definitions.

Drivers bind to HAL interfaces, not to Logic or Dashboard.

Output:

```text
driver_catalog_ready
driver_instances_ready
driver_errors
```

Drivers do not register modules or evaluate automation rules.

## 3. Devices

Prepare Device definitions and create built-in or statically configured Device objects when applicable.

Devices combine one or more Drivers into usable Cyber32 objects.

Output:

```text
device_layer_ready
device_definitions_ready
initial_device_candidates
```

Devices describe capabilities but do not own global Registry storage.

## 4. Modules

Prepare Module definitions and known module types.

Modules group Devices into physical or logical plug-and-play units.

Output:

```text
module_layer_ready
module_definitions_ready
module_type_catalog_ready
```

Modules do not bypass PNP or register themselves directly as available.

## 5. PNP

Start plug-and-play discovery.

PNP flow:

```text
Detect
-> Identify
-> Read Metadata
-> Validate
-> Register
-> Expose Capabilities
```

PNP levels:

- Level 1: Passive identification
- Level 2: I2C metadata
- Level 3: Smart module EEPROM

Output:

```text
detected_modules
validated_modules
registration_requests
pnp_warnings
pnp_errors
```

PNP does not evaluate Logic or render Dashboard UI.

## 6. Registry

Initialize Registry and store accepted PNP registration data.

Registry collections:

```text
modules
devices
services
capabilities
metadata
```

Output:

```text
registry_ready
registered_modules
registered_devices
registered_capabilities
registered_metadata
```

Registry is the source of truth for what exists and what capabilities are available.

## 7. Runtime

Start runtime coordination.

Runtime prepares:

- Lifecycle management
- Scheduler
- Tasks
- State machines
- Resource tracking

Output:

```text
runtime_ready
lifecycle_ready
scheduler_ready
resource_manager_ready
task_system_ready
```

Runtime does not own Registry data or application Logic definitions.

## 8. Services

Start system services.

Examples:

- Device Manager
- Module Manager
- Network Manager
- Storage Manager
- Power Manager
- Telemetry Manager
- OTA Manager
- Security Manager

Services use Registry and Runtime to coordinate system behavior.

Output:

```text
services_ready
service_registry_entries
service_status
service_capabilities
```

Services do not implement HAL behavior or render Dashboard UI.

## 9. Logic

Load and prepare capability-first Logic.

Logic may load:

- Triggers
- Conditions
- Actions
- Rules
- Flows
- Variables

Logic must bind to capability IDs, not module names.

Output:

```text
logic_ready
loaded_flows
loaded_rules
capability_bindings
logic_warnings
```

Logic must wait for Registry and Services before resolving capabilities.

## 10. API

Start API surfaces after system capabilities and services are known.

API surfaces may include:

- Internal API
- External API
- REST API
- WebSocket API

Output:

```text
api_ready
rest_ready
websocket_ready
internal_api_ready
external_api_ready
```

API exposes Registry, Services, Logic, telemetry, and system state through documented contracts.

API does not bypass Services or directly control hardware.

## 11. Dashboard

Start Dashboard after API is ready.

Dashboard may show:

- Modules
- Devices
- Capabilities
- Telemetry
- Logic builder
- Settings
- OTA status

Output:

```text
dashboard_ready
pages_ready
widgets_ready
realtime_updates_ready
```

Dashboard may show friendly or translated names, but saved Logic and API operations must preserve capability IDs.

## Complete Boot Flow

```text
Power on
-> Initialize HAL
-> Initialize Drivers
-> Prepare Devices
-> Prepare Modules
-> Run PNP discovery
-> Register accepted objects
-> Start Runtime
-> Start Services
-> Load Logic
-> Start API
-> Start Dashboard
-> System READY
```

## Failure Handling

If a layer fails, higher layers must not assume that layer is ready.

Examples:

- If HAL I2C fails, PNP Level 2 discovery may be unavailable.
- If Registry fails, Runtime and Logic must not resolve capabilities.
- If Services fail, API should expose degraded service state.
- If API fails, Dashboard should not claim live control.

System state should reflect:

```text
BOOTING
INITIALIZING
READY
WARNING
ERROR
RECOVERY
SHUTDOWN
```

## Capability Availability Rule

Boot is complete only when capabilities are discoverable through Registry and available to higher layers according to their status.

Logic must react to capability availability, not to specific module names.
