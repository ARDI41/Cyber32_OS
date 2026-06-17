# Milestone 6.8 Wireless Runtime Task Plan

## Goal

Design Runtime task integration for `WirelessService`.

This document is documentation only. It does not change source code, does not modify `src/main.cpp`, and does not change Runtime, WirelessService, Registry, API, Logic, or validation behavior.

## Required Tasks

Add future Runtime tasks:

```text
task.wireless_service.process_packets
task.wireless_service.check_timeouts
```

These tasks should be registered by the validation harness or future production bootstrap after:

1. Registry is initialized.
2. Wireless transport driver is initialized.
3. Wireless temperature device is initialized.
4. WirelessService is initialized.
5. WirelessService has attached Registry.
6. WirelessService has attached transport driver.
7. WirelessService has attached wireless temperature device.

## Runtime Role

Runtime remains scheduler-only.

Runtime may:

- store task records
- execute task callbacks
- pass task context pointers
- track task count and execution timing

Runtime must not:

- parse wireless packets
- inspect wireless packet headers
- inspect `CAP_TEMPERATURE`
- scan provider records
- own provider timeout policy
- select providers
- update canonical capability payloads
- call Drivers or Devices directly
- call Registry wireless/provider APIs directly for this flow

## WirelessService Role

WirelessService owns wireless coordination policy.

WirelessService task callbacks should call:

```text
process task -> WirelessService::processPackets(now_ms)
timeout task -> WirelessService::checkTimeouts(now_ms)
```

WirelessService may:

- read one pending simulated packet through the transport driver
- update the virtual wireless temperature device
- update provider payload through Registry public APIs
- coordinate provider health and failover through Registry public APIs

WirelessService must not:

- scan Registry arrays
- choose providers directly
- copy canonical payload directly
- expose API
- run Logic

## Task Contexts

Add fixed validation task contexts:

```cpp
struct WirelessServiceProcessTaskContext {
    WirelessService* service;
    uint32_t now_ms;
    bool ran;
    bool last_result;
    const char* last_error;
};

struct WirelessServiceTimeoutTaskContext {
    WirelessService* service;
    uint32_t now_ms;
    bool ran;
    bool last_result;
    const char* last_error;
};
```

Rules:

- fixed structs only
- no dynamic allocation
- no STL containers
- no Arduino `String`
- context owns no Service state
- context stores only execution facts for validation

## Task Callbacks

Add static callbacks compatible with:

```cpp
void (*callback)(void* context)
```

Process callback:

```text
cast context
validate context and service pointers
set ran = true
last_result = service->processPackets(now_ms)
last_error = last_result ? "none" : service->lastErrorCode()
```

Timeout callback:

```text
cast context
validate context and service pointers
set ran = true
last_result = service->checkTimeouts(now_ms)
last_error = last_result ? "none" : service->lastErrorCode()
```

Callbacks must not:

- parse packets
- call Registry directly
- select providers
- update canonical payloads directly
- run Logic

## Task Registration Order

Register wireless tasks after WirelessService attachments are complete.

Recommended order in validation:

```text
temperature service task
temperature logic task
distance service task
distance logic task
servo state task
motor state task
motor command task
relay state task
relay command task
wireless process task
wireless timeout task
```

Wireless process should run before wireless timeout when both are scheduled in the same Runtime cycle.

Reason:

```text
new packet updates provider first
timeout/best-provider update sees latest provider state
```

## Task Periods

Initial validation may run both tasks every Runtime update.

Future production periods:

```text
task.wireless_service.process_packets -> frequent, e.g. every Runtime tick or short interval
task.wireless_service.check_timeouts  -> periodic, e.g. 1000 ms
```

Exact production periods should remain configurable in future bootstrap code, but bounded and static.

## Validation Requirements

Validation should prove Runtime invokes the Service methods.

### Process Task Invocation

Setup:

```text
inject valid wireless CAP_TEMPERATURE packet
reset process task context ran/result fields
run Runtime update
```

Expect:

```text
WirelessServiceProcessTaskContext.ran == true
last_result == true
transport packet slot cleared
provider-wireless-temperature-001 payload updated
```

### Timeout Task Invocation

Setup:

```text
wireless provider AVAILABLE but old enough to become LOST
sim provider AVAILABLE
reset timeout task context ran/result fields
run Runtime update
```

Expect:

```text
WirelessServiceTimeoutTaskContext.ran == true
last_result == true
wireless provider status == LOST
active provider == provider-sim-temperature-001
canonical CAP_TEMPERATURE == 22.4F
```

### No Eligible Provider

Setup:

```text
wireless provider LOST
sim provider UNAVAILABLE
run Runtime timeout task
```

Expect:

```text
timeout task ran == true
last_result == false
last_error != "none"
canonical payload remains last known
```

### Runtime Boundary

Validation must confirm:

- Runtime task count includes wireless process and timeout tasks
- Runtime does not inspect `CAP_TEMPERATURE`
- Runtime does not parse packets
- Runtime does not call Registry provider APIs directly
- Logic still queries `CAP_TEMPERATURE` only

## Production Bootstrap Placement

Future production bootstrap should wire wireless tasks after:

```text
WirelessService begin
WirelessService attachRegistry
WirelessService attachTransportDriver
WirelessService attachWirelessTemperatureDevice
```

Do not modify `src/main.cpp` until a production bootstrap milestone explicitly requires it.

## Stop Conditions

Stop and review architecture if any of these occur:

- Runtime parses wireless packets
- Runtime knows wireless packet schema
- Runtime knows `CAP_TEMPERATURE`
- Runtime chooses providers
- Runtime owns provider timeout constants
- Runtime calls `updateProviderHealth()` directly for wireless flow
- Runtime calls `updateBestCapabilityPayload()` directly for wireless flow
- Runtime calls Drivers or Devices directly
- Wireless task contexts allocate memory dynamically
- Arduino `String` or STL containers are introduced in Runtime task context paths

## Final Assessment

Milestone 6.8 should add two Runtime-scheduled WirelessService callbacks:

```text
task.wireless_service.process_packets
task.wireless_service.check_timeouts
```

Runtime remains scheduler-only.

WirelessService remains the coordination layer.

Registry remains the owner of provider health, selection, and canonical payload state.

Logic remains provider-blind.

