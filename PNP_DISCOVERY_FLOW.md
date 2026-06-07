# Cyber32 PNP Discovery Flow

PNP turns connected modules into registered Cyber32 capabilities.

The flow is fixed:

```text
Detect
-> Identify
-> Read Metadata
-> Validate
-> Register
-> Expose Capabilities
```

PNP must respect the fixed architecture:

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

## PNP Levels

Cyber32 supports three PNP levels:

- Level 1: Passive identification
- Level 2: I2C metadata
- Level 3: Smart module EEPROM

## 1. Detect

Detect determines that something has been connected, removed, or changed.

Possible detection sources:

- Passive ID read
- I2C scan
- UART signal
- CAN message
- Smart module EEPROM presence
- Future Cyber32 Bus mechanism

Output:

```text
detection_event
physical_location
bus_or_channel
raw_identifier
pnp_level_candidate
```

Detect does not read full metadata, register modules, or expose capabilities.

## 2. Identify

Identify determines what kind of module may be present.

Level-specific behavior:

- Level 1 maps passive identification to a known module type or lookup record.
- Level 2 reads enough I2C information to locate the metadata record.
- Level 3 locates the smart module EEPROM and prepares metadata access.

Output:

```text
module_identity
metadata_source
metadata_level
module_type_candidate
```

Identify does not decide final compatibility or expose the module to Logic.

## 3. Read Metadata

Read Metadata obtains the module description.

Metadata should include:

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

Level-specific metadata:

- Level 1 may use a built-in lookup table from passive ID to metadata.
- Level 2 reads metadata over I2C.
- Level 3 reads metadata from smart module EEPROM.

Metadata may include display names, manufacturer names, model numbers, serial numbers, calibration, security data, and dashboard hints.

Logic must not depend on those names. Logic must use capability IDs.

## 4. Validate

Validate checks whether the module can safely enter the system.

Validation checks:

- Required metadata fields exist
- Capability IDs are valid
- Device descriptions are valid
- Required buses are available
- Required services are available
- Protocol version is supported
- OS version is compatible
- Power requirements are acceptable
- Capability conflicts are handled
- Security or trust requirements pass when required

Output:

```text
validation_status
warnings
errors
registration_plan
```

If validation fails, the module must not be registered as available.

## 5. Register

Register writes accepted module data into Registry.

Registration creates or updates:

- Module record
- Device records
- Capability records
- Metadata records

Registry status should reflect the real state:

```text
registered
available
degraded
disabled
unavailable
```

Register does not evaluate Logic and does not render Dashboard UI.

## 6. Expose Capabilities

Expose Capabilities makes registered capabilities available to higher layers through Registry, Services, API, and Dashboard.

Capability exposure enables:

- Runtime lifecycle management
- Services to coordinate system behavior
- Logic to bind to capability IDs
- API to expose capabilities externally
- Dashboard to show user-friendly names

Logic must only see stable capability IDs and capability state.

## Complete Flow

```text
Module connected
-> Detect physical or bus presence
-> Identify module or metadata source
-> Read metadata by PNP level
-> Validate metadata, compatibility, power, and security
-> Register Module, Devices, Capabilities, and Metadata
-> Expose capabilities to Runtime, Services, Logic, API, and Dashboard
```

## Removal Flow

When a module is removed:

```text
Removal detected
-> Mark module unavailable or removed
-> Mark related devices unavailable
-> Mark related capabilities unavailable
-> Notify Runtime and Services
-> Notify Logic of capability availability change
-> Notify API and Dashboard
```

Removal must not leave Logic bound to a missing module name. Logic should respond to capability availability.

## Boundary Rule

PNP owns discovery, identification, metadata reading, validation, and registration coordination.

PNP does not own:

- Driver internals
- Runtime scheduling
- Service policies
- Logic decisions
- API contracts
- Dashboard rendering
