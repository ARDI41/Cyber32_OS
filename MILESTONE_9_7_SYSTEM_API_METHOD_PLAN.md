# Milestone 9.7 - System API Method Plan

## Purpose

Milestone 9.7 plans the first read-only System API methods for Cyber32.

These methods will become the foundation for future UI clients:

- Dev Panel
- Minimal App
- Mission Control
- Dashboard
- Cloud Bridge
- Marketplace
- AI Assistant

The plan is documentation only. It does not implement API methods.

## 1. Scope

This milestone plans read-only System API methods that return the Phase 2 system API structs:

- `ApiSystemIdentity`
- `ApiSystemFirmware`
- `ApiSystemRuntime`
- `ApiSystemModes`
- `ApiSystemMemory`
- `ApiSystemSummary`

Planned method names:

- `getSystemIdentity(...)`
- `getSystemFirmware(...)`
- `getSystemRuntime(...)`
- `getSystemModes(...)`
- `getSystemMemory(...)`
- `getSystemSummary(...)`

The first implementation should remain internal C++ API only.

## 2. Non-Goals

Milestone 9.7 does not implement behavior.

Non-goals:

- No WebServer
- No HTTP
- No JSON
- No UI
- No App
- No Dashboard
- No Cloud
- No Marketplace
- No AI
- No Runtime architecture change
- No Registry ownership change
- No Service behavior change
- No command behavior
- No packet parsing
- No provider selection
- No hardware access
- No `src/main.cpp` change

## 3. Read-Only Philosophy

System API methods are observation methods only.

They may:

- report stable literals and placeholders
- report Runtime state already attached to `Cyber32Api`
- report bounded Registry summary information where public Registry APIs already support it
- report compact error codes
- compose existing system structs into `ApiSystemSummary`

They must not:

- mutate Registry state
- mutate Runtime state
- call Services for command behavior
- call Drivers
- call Devices
- call HAL
- parse packets
- start transports
- choose providers
- update providers
- change canonical payloads
- allocate memory
- build JSON

## 4. Method Naming Rules

System read methods must use the `get...` prefix.

Planned signatures should follow the existing API style:

```cpp
bool getSystemIdentity(ApiSystemIdentity& out_response);
bool getSystemFirmware(ApiSystemFirmware& out_response);
bool getSystemRuntime(ApiSystemRuntime& out_response);
bool getSystemModes(ApiSystemModes& out_response);
bool getSystemMemory(ApiSystemMemory& out_response);
bool getSystemSummary(ApiSystemSummary& out_response);
```

Naming rules:

- Use `get...` for read-only methods.
- Use `out_response` for output parameters where practical.
- Return `true` only when the response is filled successfully.
- Fill `ok` in the response struct.
- Fill `error_code` with `"none"` on success.
- Use compact stable literal error codes on failure.
- Do not use `command...` for read-only methods.
- Do not use `set...` for this phase.

## 5. Ownership

### Allowed Data Sources

System API methods may read:

- stable build/system literals
- existing `Cyber32Api` attachment state
- existing `Runtime` state
- existing Registry summaries where public Registry APIs already support the needed count
- local compile-time placeholders for fields not yet owned by a subsystem

### Forbidden Data Sources

System API methods must not read or call:

- Drivers
- Devices
- HAL
- raw packet buffers
- ESP-NOW callbacks
- packet parsers
- private Registry arrays
- provider selection logic
- Services for command behavior
- UI transport layers

### Ownership Notes By Method

`getSystemIdentity(...)`:

- may return stable literal placeholders for Core UUID, friendly name, owner state, provisioning state, and pairing state
- future owner: identity/provisioning subsystem

`getSystemFirmware(...)`:

- may return stable literal placeholders for firmware version, build version, hardware revision, and protocol version
- future owner: build metadata / firmware identity layer

`getSystemRuntime(...)`:

- may read attached `Runtime` state
- may report placeholders for uptime and readiness until a concrete owner exists

`getSystemModes(...)`:

- may return placeholders for setup, developer, local, and remote modes
- future owner: system mode manager

`getSystemMemory(...)`:

- may return placeholders until a memory/status owner exists
- may report Registry capacity summary only through existing public Registry summaries

`getSystemSummary(...)`:

- may compose the other read-only System API methods
- must not create extra side effects

## 6. Expected Return Contracts

| Method | Response struct | Notes |
| --- | --- | --- |
| `getSystemIdentity(...)` | `ApiSystemIdentity` | Core identity placeholders until identity owner exists. |
| `getSystemFirmware(...)` | `ApiSystemFirmware` | Firmware/build/hardware/protocol placeholders. |
| `getSystemRuntime(...)` | `ApiSystemRuntime` | Runtime state from attached Runtime where available. |
| `getSystemModes(...)` | `ApiSystemModes` | Setup/developer/local/remote mode placeholders. |
| `getSystemMemory(...)` | `ApiSystemMemory` | Memory and Registry capacity placeholders. |
| `getSystemSummary(...)` | `ApiSystemSummary` | Composed read-only aggregate of all System API views. |

Failure handling:

- Missing required attachment should return `false`.
- Failure responses should set `ok = false`.
- Failure responses should set compact `error_code`.
- Methods with placeholder-only data may return success if no required owner exists.
- No failure path may mutate system state.

## 7. Validation Strategy

Validation should be read-only.

Recommended validation cases:

- each method returns `true` when required attachments exist
- each response sets `ok == true`
- each success response sets `error_code == "none"`
- `getSystemRuntime(...)` reports the current Runtime state
- missing Runtime or Registry attachment returns a compact failure only where the method requires that attachment
- `getSystemSummary(...)` composes all child structs
- repeated reads do not mutate Runtime, Registry, providers, command state, or capability payloads
- normal capability reads remain unchanged
- existing provider, diagnostics, command-state, and wireless API validation remains unchanged

Validation must not require hardware.

## 8. Compile Strategy

Implementation should compile with:

```text
pio run
```

If `pio` is unavailable in the shell, report it explicitly.

Compile expectations:

- C++11 compatible
- no Arduino `String`
- no STL containers
- no heap allocation
- no WebServer
- no HTTP
- no JSON
- no app code
- no new source files unless an implementation phase explicitly requires them

## 9. Future Expansion Path

After System API methods are implemented and validated, future API method milestones may expand in this order:

1. Node read-only API method plan
2. Capability read-only API method plan
3. Provider and diagnostics read-only alignment methods
4. Discovery and pending pairing API plan
5. Project read-only placeholder methods
6. Logic Builder validation/read-only method plan
7. Template metadata read-only method plan
8. Command request method planning through Services only

Every expansion must remain API-first and capability-first.

## 10. Stop Conditions

Stop implementation and return to architecture review if a future System API method phase introduces:

- WebServer
- HTTP
- JSON
- UI
- App
- Dashboard
- Cloud
- Marketplace
- AI implementation
- Runtime architecture changes
- Registry ownership changes
- Service policy changes
- command execution
- Driver calls
- Device calls
- HAL calls
- packet parsing
- provider selection
- private Registry array access
- STL containers
- Arduino `String`
- heap allocation
- `src/main.cpp` changes

## 11. Recommended First Implementation Order

Recommended order:

1. `getSystemIdentity(...)`
2. `getSystemFirmware(...)`
3. `getSystemRuntime(...)`
4. `getSystemModes(...)`
5. `getSystemMemory(...)`
6. `getSystemSummary(...)`

Recommended first method:

```text
getSystemIdentity(ApiSystemIdentity& out_response)
```

Rationale:

`getSystemIdentity(...)` is the lowest-risk first method because it can return bounded stable literals/placeholders without touching providers, commands, packet parsing, hardware, Service policy, Registry ownership, or Runtime scheduling.
