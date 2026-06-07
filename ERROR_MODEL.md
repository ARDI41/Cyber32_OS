# Cyber32 Error Model

This document defines the canonical Cyber32 v1 error model for ESP32.

Cyber32 v1 is ESP32-only, memory-bounded, and capability-first.

Errors are referenced by `error_code` in capability payloads, events, registry records, PNP, Runtime, and API responses.

## Purpose

The error model defines how errors are:

- named
- structured
- emitted
- stored
- recovered from
- exposed through API
- displayed by Dashboard

The goal is a compact, stable, machine-readable error system that is safe for ESP32 RAM limits.

## What An Error Is

An error is a compact record that describes a fault, invalid state, failed operation, unsafe condition, or degraded capability.

An error should answer:

```text
what failed
where it failed
how severe it is
whether recovery is possible
which object is affected
what safe fallback is required
```

An error must not contain:

- large text messages
- large metadata blobs
- raw API requests
- full registry records
- Dashboard-only UI data
- stack traces in core runtime
- heap-owned dynamic strings

## Error Severity Levels

Allowed severity levels:

```text
info
warning
error
critical
fatal
```

### info

Non-failing diagnostic condition.

Example:

```text
configuration fallback used
```

### warning

A degraded condition that does not stop core operation.

Example:

```text
optional module unavailable
```

### error

A failed operation or unavailable capability that may be recoverable.

Example:

```text
device read timeout
```

### critical

A safety-relevant or system-relevant error requiring immediate fallback.

Example:

```text
actuator communication lost while active
```

### fatal

The system cannot continue normal operation.

Fatal errors must enter safe mode.

Example:

```text
repeated watchdog reset during boot
```

## Error Code Naming Rules

Error codes must:

- start with `ERR_`
- use uppercase ASCII letters, numbers, and underscores
- include a category prefix
- describe the failure in stable machine-readable form
- avoid module names
- avoid manufacturer names
- avoid Dashboard/UI-only terminology
- remain stable across translations

Correct:

```text
ERR_DEVICE_TIMEOUT
ERR_REGISTRY_FULL
ERR_METADATA_INVALID
```

Incorrect:

```text
ERR_GPS_BROKEN
ERR_ACME_BOARD_FAILED
ERR_RED_CARD_VISIBLE
```

## Required Error Categories

Required categories:

```text
ERR_HAL_*
ERR_DRIVER_*
ERR_DEVICE_*
ERR_MODULE_*
ERR_PNP_*
ERR_REGISTRY_*
ERR_RUNTIME_*
ERR_SERVICE_*
ERR_LOGIC_*
ERR_API_*
ERR_SECURITY_*
ERR_STORAGE_*
ERR_CONFIG_*
ERR_WATCHDOG_*
```

Required v1 error examples:

```text
ERR_DEVICE_TIMEOUT
ERR_REGISTRY_FULL
ERR_METADATA_INVALID
ERR_PNP_TIMEOUT
ERR_CAPABILITY_UNAVAILABLE
ERR_DRIVER_INIT_FAILED
ERR_CONFIG_INVALID
ERR_STORAGE_WRITE_FAILED
ERR_API_UNAUTHORIZED
ERR_WATCHDOG_RESET
```

Additional recommended v1 codes:

```text
ERR_HAL_BUS_FAULT
ERR_DRIVER_READ_FAILED
ERR_DEVICE_UNAVAILABLE
ERR_MODULE_UNSUPPORTED
ERR_REGISTRY_DUPLICATE_ID
ERR_RUNTIME_OVERLOAD
ERR_SERVICE_DEGRADED
ERR_LOGIC_BINDING_FAILED
ERR_SECURITY_TOKEN_INVALID
ERR_STORAGE_FULL
ERR_CONFIG_CHECKSUM_FAILED
ERR_WATCHDOG_BOOT_LOOP
```

## Error Record Fields

Every error record must fit in a fixed-size slot.

Required fields:

```text
error_code
severity
recoverable
timestamp_ms
source_layer
source_id
target_id
state
retry_count
safe_action
```

Optional compact fields:

```text
detail_code
related_capability_id
related_event_id
```

Field definitions:

### error_code

Stable compact machine-readable error code.

Example:

```text
ERR_DEVICE_TIMEOUT
```

### severity

One of:

```text
info
warning
error
critical
fatal
```

### recoverable

Boolean.

```text
true
false
```

### timestamp_ms

System uptime timestamp in milliseconds.

### source_layer

Layer that created the error.

Allowed values:

```text
hal
drivers
devices
modules
pnp
registry
runtime
services
logic
api
dashboard
```

Dashboard should not create core runtime errors in v1, but API may report Dashboard-facing request errors.

### source_id

ID of the source that detected or created the error.

Examples:

```text
driver:battery-adc
device:device-battery-001
service:power-manager
```

### target_id

ID of the affected object.

May reference:

- module ID
- device ID
- capability ID
- service ID
- registry record ID

### state

Current error lifecycle state.

Allowed values:

```text
active
recovering
cleared
fatal
suppressed
```

### retry_count

Small integer retry count.

### safe_action

Compact enum describing required fallback.

Allowed values:

```text
none
mark_unavailable
disable_capability
stop_motion
disable_output
enter_warning
enter_error
enter_safe_mode
restart_required
```

## Recommended Fixed Error Slot

Cyber32 v1 should target an error slot no larger than 128 bytes.

Recommended logical shape:

```text
error_code:             uint16 or compact enum
severity:               uint8
recoverable:            bool
timestamp_ms:           uint32
source_layer:           uint8
source_id:              compact ID or fixed 32-byte field
target_id:              compact ID or fixed 32-byte field
state:                  uint8
retry_count:            uint8
safe_action:            uint8
detail_code:            uint16 optional
related_capability_id:  compact ID optional
related_event_id:       uint16 optional
```

String error codes are acceptable in documentation and API output. Compact enum values are preferred inside ESP32 runtime.

## Which Layers May Create Errors

All core layers may create errors when they own the failing operation.

| Layer | May Create Errors | Examples |
|---|---|---|
| HAL | Yes | bus fault, timer fault, storage interface unavailable |
| Drivers | Yes | init failed, read failed, write failed, timeout |
| Devices | Yes | device timeout, unavailable device, invalid state |
| Modules | Yes | unsupported module, missing required device |
| PNP | Yes | metadata invalid, discovery timeout, validation failed |
| Registry | Yes | registry full, duplicate ID, missing record |
| Runtime | Yes | overload, task failed, lifecycle transition invalid |
| Services | Yes | service degraded, network unavailable, telemetry overflow |
| Logic | Yes | capability binding failed, invalid rule, unavailable action |
| API | Yes | unauthorized, invalid request, command rejected |
| Dashboard | No core errors in v1 | Dashboard may display API errors but should not create core runtime errors |

## Which Layers May Store Errors

Storage responsibility is limited.

Registry may store:

- latest error state per module
- latest error state per device
- latest error state per capability
- latest error state per service

Runtime may store:

- current system error state
- boot error counters
- watchdog recovery state

Services may store:

- service-local latest error
- retry counters
- degraded state

Storage Manager may persist:

- boot failure counter
- safe boot flag
- last fatal error code
- minimal recovery diagnostics

Event Bus must not store errors long-term.

Dashboard must not be the source of truth for errors.

## Errors And Events

Events announce errors.

Error events should use:

```text
EVT_ERROR_RAISED
EVT_ERROR_RECOVERED
EVT_ERROR_FATAL
```

Layer-specific events may also reference `error_code`, such as:

```text
EVT_DEVICE_ERROR
EVT_CAPABILITY_ERROR
EVT_SERVICE_ERROR
EVT_PNP_VALIDATION_FAILED
```

Event rule:

```text
Events announce error changes.
Registry, Runtime, or owning Services store current error state.
```

Events must include compact error codes, not large messages.

Example event:

```text
event_id: EVT_ERROR_RAISED
priority: high
source_layer: devices
source_id: device:device-distance-001
target_id: capability:CAP_DISTANCE
value_type: error_code
value: ERR_DEVICE_TIMEOUT
```

## Errors And Capability Payloads

Capability payloads may reference `error_code`.

Rules:

- If a capability is unavailable due to an error, set `available` to `false`.
- If the last value is stale, set `stale` to `true`.
- Include `error_code` when the cause is known.
- Do not treat unavailable numeric values as zero.
- Logic must react to capability/error state, not module names.

Example:

```text
capability_id: CAP_DISTANCE
schema_version: 1
timestamp_ms: 24000
available: false
stale: false
value: null
unit: meter
quality: unavailable
error_code: ERR_DEVICE_TIMEOUT
```

## Recoverable Vs Fatal Errors

### Recoverable Errors

Recoverable errors allow Cyber32 to continue after fallback or retry.

Examples:

```text
ERR_DEVICE_TIMEOUT
ERR_PNP_TIMEOUT
ERR_STORAGE_WRITE_FAILED
ERR_CAPABILITY_UNAVAILABLE
```

Allowed recovery actions:

- retry with limit
- mark capability unavailable
- mark service degraded
- use cached stale value if safe
- request controlled rescan
- enter warning state

### Fatal Errors

Fatal errors prevent normal operation.

Examples:

```text
ERR_WATCHDOG_BOOT_LOOP
ERR_REGISTRY_FULL during required boot registration
ERR_CONFIG_INVALID with no safe defaults
```

Fatal behavior:

- stop safety-critical outputs
- disable actuator capabilities
- reject API commands
- enter safe mode
- expose machine-readable fatal error state
- require restart, recovery, or safe boot

## Safe Fallback Behavior

Safe fallback is mandatory.

Rules:

- Safety-critical actuator errors must fail safe.
- Motor control errors must stop, brake, or disable output according to capability safety policy.
- Servo errors must stop updates or return to safe neutral if configured.
- Invalid metadata must not expose capabilities.
- Registry full must reject new registrations cleanly.
- Config invalid must use validated defaults or enter safe mode.
- Storage write failure must not corrupt active configuration.
- API unauthorized must reject the command without side effects.
- Watchdog reset must boot into recovery logic if repeated.

Safe fallback examples:

| Error | Safe Action |
|---|---|
| `ERR_DEVICE_TIMEOUT` | `mark_unavailable` |
| `ERR_REGISTRY_FULL` | `enter_error` |
| `ERR_METADATA_INVALID` | `disable_capability` |
| `ERR_PNP_TIMEOUT` | `mark_unavailable` |
| `ERR_CAPABILITY_UNAVAILABLE` | `disable_capability` |
| `ERR_DRIVER_INIT_FAILED` | `mark_unavailable` |
| `ERR_CONFIG_INVALID` | `enter_safe_mode` if defaults fail |
| `ERR_STORAGE_WRITE_FAILED` | `enter_warning` |
| `ERR_API_UNAUTHORIZED` | `none` |
| `ERR_WATCHDOG_RESET` | `enter_safe_mode` if repeated |

## Error Clearing Rules

Errors must be cleared deliberately.

Rules:

- Clear only after the underlying condition is resolved.
- Do not clear fatal errors automatically during the same boot.
- Do not clear repeated boot errors until safe boot or recovery completes.
- Do not clear an unavailable capability error while the provider is still missing.
- Clearing an error should emit `EVT_ERROR_RECOVERED`.
- Registry should update latest error state to `cleared`.
- API should expose cleared state until overwritten or compact history expires.

Allowed clear sources:

- owning layer
- Runtime recovery
- Service recovery
- controlled user action through API
- successful revalidation

Manual clear must not hide active unsafe conditions.

## API Exposure Rules

API must expose machine-readable error codes.

API error response fields:

```text
ok
error_code
severity
recoverable
target_id
safe_action
timestamp_ms
```

Optional fields:

```text
message_key
detail_code
```

Rules:

- API must not expose large internal logs by default.
- API must not expose raw stack traces.
- API must not translate error_code.
- API may include `message_key` for Dashboard translation.
- API must return stable error codes for validation failures.
- Unauthorized requests must return `ERR_API_UNAUTHORIZED`.

Example:

```text
ok: false
error_code: ERR_CAPABILITY_UNAVAILABLE
severity: error
recoverable: true
target_id: capability:CAP_DISTANCE
safe_action: disable_capability
timestamp_ms: 84000
message_key: capability_unavailable
```

## Dashboard Display Rules

Dashboard may translate errors for humans.

Dashboard may display:

- translated message
- severity
- affected capability
- affected device or module display name
- recovery suggestion

Dashboard must preserve:

- `error_code`
- `severity`
- `target_id`
- `recoverable`
- `safe_action`

Dashboard must not:

- treat display messages as stable IDs
- hide fatal errors
- issue hardware commands directly to recover
- store canonical error state separately from Registry/API

## ESP32 Memory Limits

Cyber32 v1 must keep errors compact.

Limits:

```text
maximum active error records: 16
maximum error slot size:      128 bytes
maximum persisted fatal errors: 4
maximum error_code length for API: 32 bytes
maximum message_key length:   32 bytes
dynamic allocation:           not allowed for core error records
```

Core runtime should use compact error enums internally and serialize to string codes at API boundaries.

No large error messages in core runtime.

## Logging Limits

Logging is diagnostic, not the error source of truth.

Rules:

- Serial logging is allowed in v1.
- Flash logging is disabled by default.
- Log lines must be bounded.
- Logs may include error_code and compact IDs.
- Logs should not include large metadata blobs.
- Logs should not include raw API bodies by default.
- Logging failure must not affect safety behavior.

Recommended limits:

```text
log line buffer: 256 bytes
in-memory recent error log: 8 entries
flash error log: disabled by default
```

## Repeated Boot Error Handling

Repeated boot errors must trigger recovery or safe boot.

Tracked data:

```text
boot_attempt_count
last_boot_error_code
last_fatal_error_code
safe_boot_requested
watchdog_reset_count
```

Rules:

- If watchdog reset repeats, raise `ERR_WATCHDOG_RESET` or `ERR_WATCHDOG_BOOT_LOOP`.
- If boot fails repeatedly, enter safe mode.
- Safe mode should disable optional PNP rescans, Dashboard extras, and actuator outputs.
- API may expose recovery state.
- Dashboard may show recovery state.

## Required Example Definitions

### ERR_DEVICE_TIMEOUT

Category:

```text
ERR_DEVICE_*
```

Typical severity:

```text
error
```

Recoverable:

```text
true
```

Safe action:

```text
mark_unavailable
```

Meaning:

A device did not respond within its timeout.

### ERR_REGISTRY_FULL

Category:

```text
ERR_REGISTRY_*
```

Typical severity:

```text
critical
```

Recoverable:

```text
false for required records, true for optional records
```

Safe action:

```text
enter_error
```

Meaning:

A fixed-size Registry table has no free slot.

### ERR_METADATA_INVALID

Category:

```text
ERR_PNP_*
```

Typical severity:

```text
error
```

Recoverable:

```text
true
```

Safe action:

```text
disable_capability
```

Meaning:

Module metadata failed validation, version, length, magic, or checksum checks.

### ERR_PNP_TIMEOUT

Category:

```text
ERR_PNP_*
```

Typical severity:

```text
warning
```

Recoverable:

```text
true
```

Safe action:

```text
mark_unavailable
```

Meaning:

PNP discovery or metadata read timed out.

### ERR_CAPABILITY_UNAVAILABLE

Category:

```text
ERR_SERVICE_*
```

Typical severity:

```text
error
```

Recoverable:

```text
true
```

Safe action:

```text
disable_capability
```

Meaning:

A requested capability is not currently available.

### ERR_DRIVER_INIT_FAILED

Category:

```text
ERR_DRIVER_*
```

Typical severity:

```text
error
```

Recoverable:

```text
true if optional, false if required for safe boot
```

Safe action:

```text
mark_unavailable
```

Meaning:

A driver failed initialization.

### ERR_CONFIG_INVALID

Category:

```text
ERR_CONFIG_*
```

Typical severity:

```text
critical
```

Recoverable:

```text
true if safe defaults exist
```

Safe action:

```text
enter_safe_mode
```

Meaning:

Configuration failed validation.

### ERR_STORAGE_WRITE_FAILED

Category:

```text
ERR_STORAGE_*
```

Typical severity:

```text
warning
```

Recoverable:

```text
true
```

Safe action:

```text
enter_warning
```

Meaning:

Storage write failed. Active runtime state should remain safe and existing valid config should not be corrupted.

### ERR_API_UNAUTHORIZED

Category:

```text
ERR_API_*
```

Typical severity:

```text
warning
```

Recoverable:

```text
true
```

Safe action:

```text
none
```

Meaning:

API request was not authorized.

### ERR_WATCHDOG_RESET

Category:

```text
ERR_WATCHDOG_*
```

Typical severity:

```text
critical
```

Recoverable:

```text
true if isolated, false if repeated
```

Safe action:

```text
enter_safe_mode if repeated
```

Meaning:

System restarted due to watchdog reset.

## Capability-First Rule

Logic must react to capability and error state, not module names.

Correct:

```text
IF CAP_DISTANCE.error_code == ERR_DEVICE_TIMEOUT
THEN CAP_MOTOR_CONTROL stop
```

Incorrect:

```text
IF Front Sensor Module timed out
THEN Servo Board stop
```

## V1 Success Criteria

The Cyber32 v1 Error Model is successful when:

1. Error codes are compact and stable.
2. Runtime stores no large error messages.
3. Registry can expose latest error state.
4. Events can announce errors with `EVT_ERROR_*`.
5. Capability payloads can reference `error_code`.
6. API exposes machine-readable error codes.
7. Dashboard can translate errors without changing their IDs.
8. Fatal errors enter safe mode.
9. Safety-critical actuator errors fail safe.
10. Repeated boot errors trigger recovery or safe boot.
