# Milestone 4.3 Error Catalog Plan

## Goal

Define precise command and actuator error IDs before implementation.

This plan covers command validation, Runtime gating, pending command safety, timeout handling, and actuator execution failures for Cyber32 v1 on ESP32.

## Reviewed Documents

- `MILESTONE_4_1_ARCHITECTURE_AUDIT.md`
- `MILESTONE_4_2_RUNTIME_TRANSITION_SAFETY_PLAN.md`
- `COMMAND_DISPATCH_CONTRACT.md`
- `ERROR_MODEL.md`

## Scope

This is a documentation-only plan.

No source code, behavior, real hardware, PWM, H-bridge, WiFi, WebServer, or Dashboard changes are introduced.

## Error ID Principles

Command and actuator errors must:

- use compact stable `ERR_*` IDs
- avoid module names
- avoid human-only text
- be safe to store as `const char*`
- be usable in Registry command-state records
- be usable in API command responses
- be usable in capability payload `error_code`
- identify the failing condition precisely enough for Logic and API clients
- preserve capability-first behavior

Services own command validation and command policy. Runtime only provides state. Registry stores latest state only.

## Proposed Error IDs

| Error ID | Meaning | Used When |
|---|---|---|
| `ERR_COMMAND_INVALID` | Command request is structurally invalid or unsupported. | Generic command rejection when no more specific error applies. |
| `ERR_COMMAND_INVALID_SPEED` | Motor speed value is outside allowed range or incompatible with direction. | `CAP_MOTOR_CONTROL` speed validation fails. |
| `ERR_COMMAND_INVALID_DIRECTION` | Motor direction is unsupported or invalid. | `CAP_MOTOR_CONTROL` direction validation fails. |
| `ERR_COMMAND_INVALID_TIMEOUT` | Timeout is zero, too large, or outside v1 bounded limits. | Command request timeout validation fails. |
| `ERR_RUNTIME_NOT_READY` | Runtime state does not allow normal actuator commands. | Runtime is booting, initializing, discovering, registering, starting, or otherwise unavailable for commands. |
| `ERR_SAFE_MODE_BLOCKED` | Command is blocked because Runtime is in `SAFE_MODE`. | Normal actuator command is unsafe in safe mode. |
| `ERR_COMMAND_PENDING_EXISTS` | A bounded one-slot pending command already exists. | New motion command is rejected while another command is pending. |
| `ERR_COMMAND_TIMEOUT` | Accepted pending command expired before execution. | Runtime command execution task sees an expired pending command. |
| `ERR_ACTUATOR_UNAVAILABLE` | Actuator capability, service, device, or command path is unavailable. | API or Service cannot reach the actuator command path. |
| `ERR_ACTUATOR_EXECUTION_FAILED` | Device/Driver execution failed after command was accepted. | Simulated or real actuator execution returns failure. |

## Error Details

### `ERR_COMMAND_INVALID`

Meaning:

The command request cannot be accepted because it is malformed, unsupported, or fails a generic command validation rule.

Where used:

- Service command validation
- API command response mapping
- Registry command-state `error_code`

Layers that may emit it:

- Services
- API, only when mapping Service/API input validation failures

Validation expectation:

- command response `ok == false`
- `accepted == false`
- `executed == false`
- command state is `FAILED`
- payload remains unchanged

### `ERR_COMMAND_INVALID_SPEED`

Meaning:

The requested speed is outside the allowed `0.0F` to `100.0F` range, or `STOP` was requested with a nonzero speed.

Where used:

- `MotorService::setMotor()` validation
- `MotorService::stop()` validation if a future stop request carries speed
- API motor command response
- Registry command-state record

Layers that may emit it:

- Services
- API only if it performs bounded request-shape validation before Service dispatch

Validation expectation:

- `FORWARD / 999.0F` fails with `ERR_COMMAND_INVALID_SPEED`
- `STOP / 1.0F` fails with `ERR_COMMAND_INVALID_SPEED`
- Device is not called
- payload remains previous safe value

### `ERR_COMMAND_INVALID_DIRECTION`

Meaning:

The requested motor direction is not one of the supported v1 values:

- `STOP`
- `FORWARD`
- `REVERSE`

Where used:

- Motor command validation
- API response for invalid enum input
- Registry command-state record

Layers that may emit it:

- Services
- API only if invalid external values are rejected before Service dispatch

Validation expectation:

- invalid direction fails with command state `FAILED`
- `accepted == false`
- Device is not called
- payload remains unchanged

### `ERR_COMMAND_INVALID_TIMEOUT`

Meaning:

The requested command timeout is outside the v1 bounded range.

Where used:

- Service command validation before accepting a pending command
- API command response
- Registry command-state record

Layers that may emit it:

- Services
- API only if request-shape validation happens at API boundary

Validation expectation:

- timeout `0` fails for pending command requests that require a timeout
- timeout above v1 maximum fails
- pending slot is not occupied
- payload remains unchanged

### `ERR_RUNTIME_NOT_READY`

Meaning:

Runtime is in a state that does not allow normal actuator commands.

Blocked states include:

- `BOOTING`
- `INITIALIZING`
- `DISCOVERING`
- `REGISTERING`
- `STARTING`
- `ERROR_STATE`

Where used:

- Service Runtime gating
- Registry command-state record
- API command response

Layers that may emit it:

- Services

Validation expectation:

- valid motion command in blocked Runtime states fails
- `accepted == false`
- `executed == false`
- Device is not called
- payload remains unchanged

### `ERR_SAFE_MODE_BLOCKED`

Meaning:

Runtime is in `SAFE_MODE` and the requested actuator command is not allowed.

For `CAP_MOTOR_CONTROL`, `SAFE_MODE` allows only `STOP / 0.0F`.

Where used:

- Service Runtime gating
- pending command transition safety
- Registry command-state record
- API command response

Layers that may emit it:

- Services

Validation expectation:

- `FORWARD` or `REVERSE` in `SAFE_MODE` fails with `ERR_SAFE_MODE_BLOCKED`
- pending motion moved into `SAFE_MODE` fails before Device call
- `STOP / 0.0F` in `SAFE_MODE` remains allowed
- payload remains unchanged for blocked motion

### `ERR_COMMAND_PENDING_EXISTS`

Meaning:

The command cannot be accepted because the bounded one-slot pending command storage is already occupied.

Where used:

- `MotorService::setMotor()` when a pending command exists
- Registry command-state record
- API command response

Layers that may emit it:

- Services

Validation expectation:

- pending `FORWARD / 40.0F` exists
- new `FORWARD / 60.0F` fails with `ERR_COMMAND_PENDING_EXISTS`
- pending `STOP / 0.0F` replacement remains allowed by policy
- Device is not called during rejection
- payload remains unchanged

### `ERR_COMMAND_TIMEOUT`

Meaning:

An accepted pending command expired before the Runtime command execution task could execute it.

Where used:

- `MotorService::executePendingCommand()`
- Registry command-state record
- API command-state response

Layers that may emit it:

- Services

Validation expectation:

- command state becomes `TIMED_OUT`
- Device is not called
- pending slot is cleared
- payload remains unchanged

### `ERR_ACTUATOR_UNAVAILABLE`

Meaning:

The actuator command path is unavailable.

Examples:

- Service is not attached to API
- Registry is not attached to Service
- Device is not attached to Service
- capability is missing
- actuator payload is unavailable

Where used:

- API command response when Service is not attached
- Service command response when dependencies are missing
- Registry command-state record when possible
- capability payload `error_code` for unavailable actuator state

Layers that may emit it:

- API
- Services
- Devices may continue to use lower-level device errors until migrated

Validation expectation:

- API command without Service attachment fails with `ERR_ACTUATOR_UNAVAILABLE`
- missing Registry/Device attachment fails
- Device is not called when dependency is missing
- payload is unchanged or marked unavailable by state update path

### `ERR_ACTUATOR_EXECUTION_FAILED`

Meaning:

The actuator command was accepted, reached execution, and the Device/Driver failed to perform it.

Where used:

- `MotorService::executePendingCommand()`
- `ServoService::setPosition()`
- Registry command-state record
- API command-state response

Layers that may emit it:

- Services

Validation expectation:

- driver failure after accepted command results in command state `FAILED`
- command-state error is `ERR_ACTUATOR_EXECUTION_FAILED`
- requested unsafe value is not stored as successful payload
- pending slot is cleared

## Layer Usage Matrix

| Error ID | API | Service | Runtime | Registry | Device | Driver |
|---|---:|---:|---:|---:|---:|---:|
| `ERR_COMMAND_INVALID` | map/input only | emit | no | store only | no | no |
| `ERR_COMMAND_INVALID_SPEED` | map/input only | emit | no | store only | no | no |
| `ERR_COMMAND_INVALID_DIRECTION` | map/input only | emit | no | store only | no | no |
| `ERR_COMMAND_INVALID_TIMEOUT` | map/input only | emit | no | store only | no | no |
| `ERR_RUNTIME_NOT_READY` | map only | emit | state only | store only | no | no |
| `ERR_SAFE_MODE_BLOCKED` | map only | emit | state only | store only | no | no |
| `ERR_COMMAND_PENDING_EXISTS` | map only | emit | no | store only | no | no |
| `ERR_COMMAND_TIMEOUT` | read/map only | emit | schedule only | store only | no | no |
| `ERR_ACTUATOR_UNAVAILABLE` | emit/map | emit | no | store only | report through payload only | no |
| `ERR_ACTUATOR_EXECUTION_FAILED` | read/map only | emit | no | store only | failure source | failure source |

Registry never creates policy errors. It stores compact codes provided by owning layers.

Runtime never emits actuator-policy errors. It exposes Runtime state and schedules tasks.

## Command-State Expectations

| Condition | Command State | Error ID | Payload |
|---|---|---|---|
| invalid command | `FAILED` | `ERR_COMMAND_INVALID` | unchanged |
| invalid speed | `FAILED` | `ERR_COMMAND_INVALID_SPEED` | unchanged |
| invalid direction | `FAILED` | `ERR_COMMAND_INVALID_DIRECTION` | unchanged |
| invalid timeout | `FAILED` | `ERR_COMMAND_INVALID_TIMEOUT` | unchanged |
| Runtime blocked | `FAILED` | `ERR_RUNTIME_NOT_READY` | unchanged |
| SAFE_MODE motion blocked | `FAILED` | `ERR_SAFE_MODE_BLOCKED` | unchanged |
| pending motion exists | `FAILED` | `ERR_COMMAND_PENDING_EXISTS` | unchanged |
| command expired | `TIMED_OUT` | `ERR_COMMAND_TIMEOUT` | unchanged |
| actuator unavailable | `FAILED` | `ERR_ACTUATOR_UNAVAILABLE` | unchanged or unavailable payload |
| execution failed | `FAILED` | `ERR_ACTUATOR_EXECUTION_FAILED` | previous safe or unavailable payload |

## Migration Plan

1. Add the new IDs to `src/core/ids/error_ids.h`.
2. Replace generic `ERR_CAPABILITY_UNAVAILABLE` use in command validation with specific command errors.
3. Update `MotorService` validation paths:
   - invalid speed
   - invalid direction
   - invalid timeout
   - Runtime not ready
   - SAFE_MODE blocked
   - pending command exists
   - command timeout
   - execution failure
4. Update `ServoService` validation paths where equivalent errors apply.
5. Update API mapping without adding transport, JSON, HTTP, or WiFi.
6. Update validation harness expectations to check precise error codes.
7. Keep fallback compatibility where older code may still return `ERR_CAPABILITY_UNAVAILABLE`.

## Backward Compatibility

During migration, existing behavior may continue to accept generic `ERR_CAPABILITY_UNAVAILABLE` until each path is explicitly migrated.

After Milestone 4.3 implementation is complete:

- command validation failures should use `ERR_COMMAND_*`
- actuator availability/execution failures should use `ERR_ACTUATOR_*`
- Runtime gating should use `ERR_RUNTIME_NOT_READY`
- SAFE_MODE gating should use `ERR_SAFE_MODE_BLOCKED`

## Stop Conditions Before Implementation

Stop before implementing if:

- a new error ID requires dynamic allocation
- a new error ID requires human-readable runtime messages
- Registry would need to decide command policy
- Runtime would need to inspect command payloads
- API would bypass Service to classify actuator execution
- Device or Driver would need module-name knowledge
- validation cannot prove payload remains safe after command failure
- real hardware, PWM, H-bridge, WiFi, WebServer, or Dashboard work becomes necessary

## Success Criteria

Milestone 4.3 is ready for implementation when:

- every listed command and actuator failure has a compact error ID
- each error has a clear owning layer
- command-state expectations are explicit
- payload safety expectations are explicit
- Runtime remains scheduler/state provider only
- Registry remains state-only
- Services remain command policy owners
- validation expectations are specific enough to implement
