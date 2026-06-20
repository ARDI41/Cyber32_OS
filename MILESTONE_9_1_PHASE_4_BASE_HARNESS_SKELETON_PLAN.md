# Milestone 9.1 Phase 4 Base Harness Skeleton Plan

## Goal

Define the first hardware test harness skeleton for the Cyber32 base node.

This plan is documentation only. It does not modify source code and does not modify `src/main.cpp`.

## Proposed Files

```text
src/app/hardware_tests/
    first_wireless_temperature_base_test.h
    first_wireless_temperature_base_test.cpp
```

These files should be introduced only in the implementation phase for the base-node hardware test harness.

## Ownership

The hardware test harness may:

- construct `Registry`
- construct `EspNowTransportDriver`
- construct `WirelessService`
- construct `WirelessTemperatureDevice`
- attach `WirelessPacketTransportAdapter`
- register wireless allowlist records
- register wireless provider records
- register wireless security diagnostic records
- expose a simple `run(now_ms)` method

The hardware test harness must not:

- modify Runtime architecture
- modify Registry architecture
- modify API architecture
- modify Logic architecture
- bypass WirelessService packet policy
- write Registry arrays directly
- parse packets in Registry
- parse packets in Runtime

## Public Interface

The harness should expose:

```cpp
bool begin();
bool run(uint32_t now_ms);
```

### `begin()`

Initializes fixed test dependencies and returns success/failure.

### `run(uint32_t now_ms)`

Runs one bounded processing step and returns success/failure.

## Begin Responsibilities

`begin()` should:

1. initialize Registry
2. initialize `EspNowTransportDriver`
3. initialize `WirelessTemperatureDevice`
4. initialize `WirelessService`
5. register allowlist record:
   - `node_id = 1001`
   - sender MAC
   - `allow_state = ALLOWED`
   - `trust_state = TRUSTED`
6. register wireless provider:
   - `provider-wireless-temperature-001`
   - `CAP_TEMPERATURE`
   - provider type `WIRELESS`
7. register wireless security diagnostic record for node `1001`
8. create `WirelessPacketTransportAdapter` for `EspNowTransportDriver`
9. attach adapter to `WirelessService`
10. attach Registry and `WirelessTemperatureDevice` to `WirelessService`
11. return false on setup failure
12. return true when setup is ready

## Run Responsibilities

`run(now_ms)` should:

1. call `WirelessService::processPackets(now_ms)`
2. read provider diagnostics or provider record state
3. read wireless security diagnostics
4. return the processing result or a compact harness result

No printing requirements exist yet.

Serial output may be added in a later explicit hardware observation phase.

## Validation Goal

The first harness implementation should prove:

- harness files compile
- `begin()` can initialize fixed objects
- `run(now_ms)` can be called safely
- no real packet is required yet
- no sender node is required yet
- no production bootstrap wiring is required

The first skeleton validation should not require real ESP-NOW traffic.

## Boundary Rules

The harness is a hardware test surface, not production boot.

It should remain isolated under:

```text
src/app/hardware_tests/
```

It must not become the default app entry path.

It must not require `src/main.cpp` changes in this phase.

## Stop Conditions

Stop implementation if it requires:

- Dashboard
- Mobile Studio
- WebServer
- cloud
- persistence
- actuator behavior
- Runtime rewrite
- Registry architecture change
- API architecture change
- Logic architecture change
- direct Registry array access
- dynamic allocation
- Arduino `String`
- STL containers
- `src/main.cpp` changes

## Final Assessment

Milestone 9.1 Phase 4 should create only the plan for a minimal base-node hardware test harness skeleton.

The future implementation should compile a small isolated harness without requiring a real sender node or production bootstrap changes.
