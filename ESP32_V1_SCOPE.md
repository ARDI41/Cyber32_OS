# Cyber32 ESP32 v1 Scope

This document defines the implementation limits for Cyber32 v1.

Cyber32 v1 targets ESP32 only. Do not optimize v1 for Raspberry Pi, Linux, Android, desktop simulation, or multi-platform hosts.

The architecture remains fixed:

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

The v1 implementation must be small, bounded, and realistic for ESP32.

## Supported ESP32 Boards

Cyber32 v1 supports the PlatformIO `esp32dev` target first.

Primary board:

```text
esp32dev
```

Acceptable compatible boards for v1, if they match the same resource assumptions:

```text
ESP32-WROOM-32
ESP32 DevKit V1
ESP32 DevKitC
```

Not targeted in v1:

```text
ESP32-S3
ESP32-C3
ESP32-C6
ESP8266
Raspberry Pi
Linux
Android
desktop simulation
```

Other ESP32 variants may be added later only after v1 is stable.

## PlatformIO Assumptions

Cyber32 v1 uses PlatformIO.

Initial environment:

```text
platform = espressif32
board = esp32dev
framework = arduino
```

Assumptions:

- USB serial monitor is available during development.
- Firmware is built and uploaded through PlatformIO.
- Dependencies are explicitly listed in `platformio.ini`.
- Build flags are used for feature control.
- Partition layout must be documented before OTA is enabled.

No hidden dependency downloads should be required during normal builds after dependencies are declared.

## Arduino vs ESP-IDF Decision

Cyber32 v1 uses Arduino on ESP32.

Decision:

```text
v1 framework: Arduino
v1 build tool: PlatformIO
v1 operating model: Arduino setup() / loop() with lightweight Cyber32 runtime
```

ESP-IDF is deferred.

Reason:

- The current project already uses Arduino/PlatformIO.
- Arduino is faster for the first working ESP32 vertical slice.
- The v1 goal is architecture proof, not full platform depth.

Constraints:

- Do not assume a full operating system.
- Do not create desktop-style services.
- Avoid blocking Arduino library calls in timing-sensitive paths.
- Avoid unbounded use of Arduino `String`.
- Keep FreeRTOS-specific usage minimal unless required.

ESP-IDF may be reconsidered in v2+ if Cyber32 needs deeper partition, task, networking, or power control.

## RAM Budget

ESP32 RAM is limited. Cyber32 v1 must use bounded memory.

Target runtime budget:

```text
Cyber32 core runtime:       <= 48 KB RAM
Registry records:           <= 16 KB RAM
PNP metadata buffers:        <= 4 KB RAM
API request/response buffers <= 12 KB RAM
Dashboard runtime buffers:   <= 8 KB RAM
Telemetry buffers:           <= 8 KB RAM
Safety reserve:              >= 40 KB free heap after boot
```

Rules:

- Prefer static allocation or fixed-size pools.
- Avoid unbounded dynamic allocation.
- Avoid storing full text metadata in RAM.
- Avoid loading large Dashboard assets into RAM.
- Avoid large JSON documents.
- Avoid long-lived temporary strings.
- Track free heap in debug builds.

If RAM limits are exceeded, disable optional features before increasing scope.

## Flash Budget

Cyber32 v1 must fit comfortably in common ESP32 flash layouts.

Target flash budget:

```text
Firmware image:        <= 1.2 MB
Dashboard assets:      <= 128 KB
Configuration storage: <= 32 KB
Metadata cache:        <= 32 KB
Logs on flash:         disabled by default
OTA:                   deferred until partition plan exists
```

Rules:

- Keep Dashboard assets minimal.
- Do not store telemetry history in flash by default.
- Avoid large embedded HTML/CSS/JS bundles.
- Avoid embedding long schema text in firmware.
- Use compact IDs and enums internally.
- Define partition strategy before OTA implementation.

## Maximum Object Counts

Cyber32 v1 uses fixed upper bounds.

Recommended v1 limits:

```text
maximum modules:       8
maximum devices:       16
maximum capabilities:  48
maximum services:      8
maximum logic rules:   16
maximum logic flows:   4
maximum variables:     16
maximum API clients:   2
maximum WebSocket clients: 1
```

These are implementation limits, not permanent architecture limits.

If a project needs more than these limits, it should be treated as a v2+ requirement.

## Maximum Metadata Size

PNP metadata must be compact.

Limits:

```text
Level 1 passive ID payload:        <= 16 bytes
Level 1 resolved metadata record:  <= 256 bytes
Level 2 I2C metadata payload:      <= 512 bytes
Level 3 EEPROM metadata payload:   <= 1024 bytes
metadata read buffer:              <= 1024 bytes
metadata string field:             <= 32 bytes
capability IDs per module:         <= 12
devices per module:                <= 4
```

Metadata should include:

```text
magic
schema_version
module_id
metadata_level
module_type
version
capability_ids
device_descriptors
compatibility
checksum
```

Metadata should not include large descriptions, documentation text, images, dashboard layouts, or verbose JSON in v1.

## Enabled Features For v1

Cyber32 v1 enables only the features needed for the first architecture-proving ESP32 implementation.

Enabled:

- ESP32 HAL contracts for GPIO, PWM, I2C, UART, storage, and time
- Driver layer for a small number of concrete drivers
- Device layer with capability descriptions
- Module layer with compact metadata
- PNP boot-time discovery
- Level 1 passive identification
- Level 2 I2C metadata
- Optional Level 3 EEPROM metadata read
- Compact Registry
- Lightweight Runtime loop
- Basic lifecycle states
- Device Manager
- Module Manager
- minimal Telemetry Manager
- minimal Storage Manager for configuration
- minimal Logic rules using capability IDs
- minimal REST-style API
- minimal ESP32-hosted Dashboard
- serial logging
- validation of capability IDs and metadata

## Deferred Features For v2+

Deferred:

- multi-platform support
- ESP-IDF native implementation
- full hot-plug under active operation
- full WebSocket telemetry streaming
- rich Dashboard frontend
- Dashboard widget system
- browser-based visual Logic builder
- OTA updates
- secure module certificates
- encrypted module metadata
- cloud integration
- long-term telemetry history
- dynamic plugin loading
- large SDK generators
- CAN-first distributed robotics
- full AI inference on device
- desktop simulator
- Android host mode
- Linux host mode

Deferred does not mean rejected. It means not part of ESP32 v1.

## Dashboard Limits

The ESP32 v1 Dashboard must be minimal.

Allowed:

- system status page
- module list
- device list
- capability list
- simple controls for enabled actuator capabilities
- simple telemetry values
- basic settings

Limits:

```text
static assets: <= 128 KB flash
served pages: <= 4
simultaneous clients: 1
refresh model: polling by default
WebSocket: optional and single-client only
```

Not allowed in v1:

- large JavaScript framework bundles
- heavy charts
- full visual Logic builder
- dynamic widget marketplace
- asset-heavy UI
- multi-user sessions

Dashboard must use API and capability IDs. It must not directly control hardware.

## API Limits

The ESP32 v1 API must be small and safe.

Allowed endpoints:

```text
GET /system
GET /modules
GET /devices
GET /capabilities
GET /telemetry
POST /capabilities/{id}/command
GET /config
PUT /config
POST /system/restart
```

Limits:

```text
request body size:  <= 1024 bytes
response body size: <= 4096 bytes
simultaneous clients: <= 2
command rate: bounded per capability
telemetry polling: rate limited
```

Rules:

- API must validate capability IDs.
- API must reject unknown module-name-based commands.
- API must not expose raw HAL access.
- API must not block control loops.
- API must return degraded state when services are unavailable.

WebSocket is optional in v1 and should be disabled by default until memory and stability are measured.

## PNP Limits

PNP v1 focuses on boot-time discovery.

Allowed:

- boot-time passive ID detection
- boot-time I2C metadata scan
- boot-time EEPROM metadata read
- manual rescan command when system is idle
- metadata validation
- compatibility validation
- registry registration
- capability exposure

Limits:

```text
maximum PNP scan time:       <= 1000 ms at boot
maximum I2C metadata devices <= 8
maximum metadata retries:    2
maximum metadata payload:    1024 bytes
```

Rules:

- Full I2C scans should happen at boot or controlled rescan only.
- PNP must not run long blocking scans during active motion.
- Unknown modules must be ignored or marked unsupported.
- Invalid metadata must not block boot.
- Invalid modules must not expose capabilities.

## Hot-Plug Limits

True hot-plug is limited in v1.

Supported:

- boot-time module discovery
- manual rescan while safe or idle
- detection of missing modules during health checks
- marking capabilities unavailable when a module disappears

Not supported in v1:

- guaranteed safe live insertion during active motion
- automatic continuous I2C scanning
- dynamic driver loading
- dynamic memory expansion
- complex dependency re-resolution during active workflows

Hot-plug v1 rule:

```text
Detect changes conservatively.
Do not destabilize active control.
Prefer safe unavailable states over risky live reconfiguration.
```

## Runtime Limits

Cyber32 v1 Runtime must be lightweight.

Allowed:

- cooperative scheduler
- fixed task slots
- simple lifecycle states
- bounded event queue
- watchdog-aware loop
- basic resource counters

Limits:

```text
maximum scheduled tasks: 16
maximum event queue entries: 32
maximum event payload size: 128 bytes
maximum blocking operation target: 10 ms
main loop watchdog safety: required
```

Avoid:

- unbounded task creation
- desktop-style service threads
- large event payloads
- blocking network calls in control paths
- heap-heavy callback chains

## Compile-Time Feature Flags

Cyber32 v1 should use compile-time feature flags to keep firmware small.

Recommended flags:

```text
CYBER32_FEATURE_PNP_LEVEL1
CYBER32_FEATURE_PNP_LEVEL2
CYBER32_FEATURE_PNP_LEVEL3
CYBER32_FEATURE_API_REST
CYBER32_FEATURE_API_WEBSOCKET
CYBER32_FEATURE_DASHBOARD
CYBER32_FEATURE_LOGGING
CYBER32_FEATURE_TELEMETRY
CYBER32_FEATURE_STORAGE
CYBER32_FEATURE_LOGIC
CYBER32_FEATURE_OTA
CYBER32_FEATURE_SECURITY
CYBER32_FEATURE_DEBUG_MEMORY
CYBER32_FEATURE_DEBUG_TIMING
```

Default v1 feature profile:

```text
CYBER32_FEATURE_PNP_LEVEL1      enabled
CYBER32_FEATURE_PNP_LEVEL2      enabled
CYBER32_FEATURE_PNP_LEVEL3      optional
CYBER32_FEATURE_API_REST        enabled
CYBER32_FEATURE_API_WEBSOCKET   disabled
CYBER32_FEATURE_DASHBOARD       enabled minimal
CYBER32_FEATURE_LOGGING         enabled serial
CYBER32_FEATURE_TELEMETRY       enabled minimal
CYBER32_FEATURE_STORAGE         enabled config only
CYBER32_FEATURE_LOGIC           enabled minimal
CYBER32_FEATURE_OTA             disabled
CYBER32_FEATURE_SECURITY        minimal
CYBER32_FEATURE_DEBUG_MEMORY    debug only
CYBER32_FEATURE_DEBUG_TIMING    debug only
```

Feature flags must not change the architecture. They only include or exclude v1 implementations of documented responsibilities.

## Safety Requirements

Cyber32 v1 must boot safely.

Rules:

- Actuators default to safe inactive state.
- Invalid metadata must not activate devices.
- Missing capabilities must be reported as unavailable.
- Logic must not run until Registry and Runtime are ready.
- API must not accept commands until the system is ready.
- Dashboard must show degraded state if services are unavailable.
- Watchdog recovery must return to safe outputs.
- Brownout or reboot must not leave actuators active.

Safe boot states:

```text
BOOTING
INITIALIZING
READY
WARNING
ERROR
RECOVERY
SHUTDOWN
```

## V1 Success Criteria

Cyber32 v1 is successful when an ESP32 can:

1. Boot safely.
2. Initialize HAL and selected Drivers.
3. Discover or load a small set of Modules.
4. Register Devices and Capabilities.
5. Expose capabilities through Registry.
6. Run simple capability-first Logic.
7. Serve a minimal API.
8. Serve a minimal Dashboard.
9. Recover safely from invalid metadata, missing modules, WiFi loss, and watchdog reset.

The goal is a small, reliable capability-first robotics OS core on ESP32, not a full desktop-style operating system.
