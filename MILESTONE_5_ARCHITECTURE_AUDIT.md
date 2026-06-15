# Milestone 5 Architecture Audit

## Goal

Audit the completed `CAP_RELAY_CONTROL` implementation against Cyber32 architecture and safety rules.

Reviewed:

- `ARCHITECTURE.md`
- `PRODUCTION_BOOTSTRAP_PLAN.md`
- `MILESTONE_5_IMPLEMENTATION_PLAN.md`
- `ACTUATOR_SAFETY_POLICY.md`
- `COMMAND_DISPATCH_CONTRACT.md`
- Relay implementation files under `src/drivers/actuators`, `src/devices/actuators`, `src/modules/actuators`, `src/pnp`, `src/services/relay`, `src/api`, and `src/app/validation`

Assessment scale:

- Pass: implementation matches current architecture and safety rules.
- Warning: implementation is acceptable for the simulated milestone but needs hardening before production or real hardware.
- Fail: architecture or safety rule violation.

## 1. Architecture Compliance

Assessment: Pass

Milestone 5 follows the fixed layer order:

```text
Drivers
-> Devices
-> Modules
-> PNP
-> Registry
-> Runtime
-> Services
-> API
```

The validation harness wires the slice in the expected bootstrap pattern and keeps relay behavior inside the proper layers. `src/main.cpp` modification is not required.

`CAP_RELAY_CONTROL` is introduced as a capability, registered through PNP Registration, stored in Registry, updated by RelayService, exposed by API, and validated through the validation harness.

## 2. Layer Dependency Audit

Assessment: Pass

Layer dependency findings:

- `SimRelayDriver` is simulated and hardware-free. It does not depend on Registry, Runtime, EventBus, API, Device, Module, or PNP.
- `SimRelayDevice` depends on `SimRelayDriver` and shared payload/error/capability types. It does not write Registry or expose API.
- `SimRelayModule` is metadata-only and exposes identity plus `CAP_RELAY_CONTROL`.
- PNP Discovery reads module metadata only.
- PNP Registration uses Registry public result APIs and does not write Registry arrays directly.
- Registry stores capability payload and command-state facts only.
- Runtime schedules relay state and command tasks only.
- RelayService owns relay update, validation, pending command, and safety policy.
- API reads Registry and routes relay commands through RelayService.

No lower layer depends on a higher layer.

## 3. Capability-First Compliance

Assessment: Pass

`CAP_RELAY_CONTROL` is the canonical system contract. Relay state, command state, PNP registration, Registry records, Service updates, and API state methods use the capability ID.

Friendly module metadata remains display/identity data only. No Logic dependency on relay module name exists in this milestone.

## 4. Runtime Responsibility Audit

Assessment: Pass

Runtime remains scheduler-only. Relay command execution is triggered by:

```text
task.relay_service.execute_command
```

Relay state refresh is triggered by:

```text
task.relay_service.update_state
```

Runtime does not inspect `CAP_RELAY_CONTROL`, parse relay commands, choose ON/OFF policy, or call Device/Driver directly.

## 5. Registry Responsibility Audit

Assessment: Pass

Registry stores:

- relay ModuleRecord
- relay DeviceRecord
- relay CapabilityRecord
- latest `CAP_RELAY_CONTROL` payload
- latest relay command-state record

Registry does not discover relay hardware, execute relay commands, validate relay policy, call RelayService, call RelayDevice, or call RelayDriver.

## 6. RelayService Responsibility Audit

Assessment: Pass

RelayService owns the correct policy surface:

- Runtime gating
- ON/OFF command validation
- timeout validation
- one-slot pending command storage
- command acceptance
- command execution via `executePendingCommand()`
- Registry payload updates
- Registry command-state writes
- error-code mapping for relay command failures

Device calls occur from `updateState()` and `executePendingCommand()`, not from API directly.

## 7. API Responsibility Audit

Assessment: Pass

Cyber32Api exposes relay state and command methods without adding external transport:

- `attachRelayService()`
- `getRelayControlState()`
- `commandRelayControl()`
- `commandRelayOff()`
- `getRelayCommandState()`

State reads come from Registry. Commands route through RelayService. API does not call RelayDevice, RelayDriver, HAL, Runtime update, or private Registry internals.

## 8. SAFE_MODE Audit

Assessment: Pass

Implemented behavior matches the relay plan:

- `SAFE_MODE` blocks relay ON.
- `SAFE_MODE` allows relay OFF.
- blocked ON returns `ERR_SAFE_MODE_BLOCKED`.
- OFF remains the fail-safe command path.

Validation covers SAFE_MODE ON rejection and SAFE_MODE OFF acceptance/execution through the Runtime command task.

## 9. Pending Command Audit

Assessment: Pass

RelayService uses a single bounded `RelayPendingCommand` slot. There is no queue and no dynamic allocation.

Accepted commands write `CommandState::ACCEPTED` to Registry command state and do not call Device immediately. Device execution is deferred to `executePendingCommand()` through the Runtime task.

OFF may replace a pending command by occupying the single slot. ON while a pending command exists is rejected with `ERR_PENDING_COMMAND_EXISTS`.

## 10. Timeout Audit

Assessment: Pass

RelayService validates timeout range as `1..10000` milliseconds. Invalid timeout fails before pending command acceptance and returns `ERR_COMMAND_INVALID_TIMEOUT`.

Pending command timeout is checked before Device execution. Timed-out commands write `CommandState::TIMED_OUT`, clear the pending slot, and do not call Device.

Warning: validation currently covers invalid timeout, but a delayed pending relay timeout execution case should be added before production use.

## 11. Command-State Audit

Assessment: Pass

RelayService writes compact command state to Registry with:

- `capability_id = CAP_RELAY_CONTROL`
- `command_state`
- `timestamp_ms`
- `registry_result`
- `error_code`
- compact value fields

API exposes latest relay command state through `getRelayCommandState()`. Registry remains latest-state storage only and does not keep command history.

## 12. Validation Coverage Audit

Assessment: Pass

Validation covers:

- relay driver/device/service setup
- PNP discovery
- PNP registration
- Registry counts increased to 5 modules, 5 devices, and 5 capabilities
- `CAP_RELAY_CONTROL` exists
- initial relay OFF state
- API relay state read
- READY ON accepted, then completed through Runtime task
- payload remains OFF before command task execution
- READY OFF accepted, then completed through Runtime task
- invalid timeout rejection
- driver failure rejection at execution
- failed ON does not store ON payload
- SAFE_MODE ON blocked
- SAFE_MODE OFF allowed
- ERROR_STATE ON blocked
- existing temperature, distance, servo, and motor validation preserved

Warning: relay validation has not been compiled in this environment because `pio` is not available on PATH.

## 13. Memory and Allocation Audit

Assessment: Pass

The relay implementation uses fixed objects, pointers, enums, primitive fields, and `const char*` IDs/errors.

No dynamic allocation, Arduino `String`, STL containers, heap queues, or unbounded command history were introduced in the relay path.

The pending command model is one-slot and ESP32-friendly.

## 14. Future Real Hardware Readiness

Assessment: Warning

The simulated relay slice is architecturally ready for the next design step, but not ready for direct real relay hardware until additional hardware safety contracts are documented and implemented.

Before GPIO/real relay support:

- define HAL GPIO safe default behavior
- define boot pin state and relay polarity handling
- define active-high vs active-low metadata
- define brownout/watchdog relay OFF enforcement
- define hardware debounce or relay settling assumptions if needed
- define power-domain risk for loads controlled by relay
- add validation for timeout-to-`TIMED_OUT` on pending relay commands
- add validation for relay ON pending command followed by SAFE_MODE transition before execution

## 15. Passes

- Pass: `CAP_RELAY_CONTROL` is capability-first.
- Pass: Runtime does not own relay policy.
- Pass: Registry stores state only.
- Pass: API routes commands through RelayService.
- Pass: RelayService owns command validation and safety policy.
- Pass: Device does not write Registry.
- Pass: Driver is hardware-free and has no GPIO writes.
- Pass: SAFE_MODE blocks ON.
- Pass: SAFE_MODE allows OFF.
- Pass: OFF is the default state after driver begin.
- Pass: pending command storage is bounded to one slot.
- Pass: command execution is separated from command acceptance.
- Pass: command state is stored in Registry and exposed through API.
- Pass: no dynamic allocation is used.
- Pass: no Arduino `String` is used.
- Pass: no STL containers are used.
- Pass: no `src/main.cpp` modification is required.

## 16. Warnings

- Warning: relay pending timeout execution should be explicitly validated, not only invalid timeout request rejection.
- Warning: relay pending command behavior during Runtime transition to `SAFE_MODE` or `ERROR_STATE` before execution should be validated like motor transition safety.
- Warning: real relay hardware requires a HAL GPIO safety plan before implementation.
- Warning: relay polarity and fail-safe electrical behavior are not yet modeled.
- Warning: command-state history is intentionally absent; this is correct for v1 memory limits but limits debugging.

## 17. Recommended Improvements

1. Add relay timeout execution validation:
   - accept ON with a short timeout
   - delay Runtime command task past timeout
   - expect `CommandState::TIMED_OUT`
   - verify payload remains OFF

2. Add relay Runtime transition validation:
   - accept ON in READY
   - enter SAFE_MODE before execution
   - run command task
   - expect FAILED and payload OFF

3. Add relay ERROR_STATE pending transition validation:
   - accept ON in READY
   - switch Runtime to ERROR_STATE before execution
   - run command task
   - expect FAILED and payload OFF

4. Document real relay electrical safety before hardware:
   - default GPIO level
   - active-high/active-low behavior
   - boot strap pin restrictions
   - brownout/watchdog OFF behavior

5. Consider adding a relay-specific hardware readiness checklist before any GPIO implementation.

## 18. Critical Violations

Assessment: Pass

No critical architecture violations were found in the completed simulated relay slice.

Specifically:

- Fail check: API-to-Device bypass was not found.
- Fail check: Runtime relay policy ownership was not found.
- Fail check: Registry command execution was not found.
- Fail check: Device Registry writes were not found.
- Fail check: Driver hardware/GPIO writes were not found.
- Fail check: unbounded relay command storage was not found.
- Fail check: dynamic allocation was not found.
- Fail check: Arduino `String` or STL containers were not found.
- Fail check: `src/main.cpp` changes were not required.

## 19. Final Assessment

Overall assessment: Pass with warnings

Milestone 5 successfully extends Cyber32 with a simulated `CAP_RELAY_CONTROL` actuator slice while preserving the fixed architecture.

The implementation is capability-first, bounded, Service-owned, Runtime-coordinated, Registry-backed, and API-safe. The important safety policy is present: relay ON is blocked in `SAFE_MODE`, relay OFF remains allowed, and OFF is the fail-safe default.

The remaining work is production hardening, not architecture repair. Before real relay hardware is added, Cyber32 should document and validate hardware-specific safety behavior, especially GPIO default state, relay polarity, watchdog/brownout behavior, and pending-command Runtime transition safety.
