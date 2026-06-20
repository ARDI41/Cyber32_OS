# Milestone 9.1 Phase 9 MAC Discovery Helper Plan

## Goal

Define a temporary MAC discovery helper for the first real Cyber32 wireless test.

This plan is documentation only. It does not modify source code and does not modify `src/main.cpp`.

## Purpose

The helper exists only to read and record:

- Base ESP32 MAC
- Sender ESP32 MAC

These MAC addresses must be known before the first real over-the-air `CAP_TEMPERATURE` packet test.

## Temporary Helper

Recommended files:

```text
src/app/hardware_tests/
    mac_discovery_helper.h
    mac_discovery_helper.cpp
```

Recommended public interface:

```cpp
bool begin();
const uint8_t* macAddress() const;
```

The helper should:

- initialize WiFi STA mode
- read the ESP32 station MAC
- store the MAC in a fixed-size array
- optionally print the MAC during temporary bench testing

## Output

Temporary serial output is allowed for this helper.

Example output:

```text
BASE MAC:
AA:BB:CC:DD:EE:FF

SENDER MAC:
11:22:33:44:55:66
```

The output label should match the board being tested.

Serial output is temporary and should not become a production logging requirement.

## Usage

Recommended workflow:

1. Flash helper to base ESP32.
2. Record base MAC.
3. Flash helper to sender ESP32.
4. Record sender MAC.
5. Update base allowlist with sender MAC.
6. Update sender peer configuration with base MAC.
7. Remove helper from the active testing path.
8. Continue to the first real packet test.

## Rules

The helper must not change:

- Registry behavior
- WirelessService behavior
- Runtime behavior
- Logic behavior
- API behavior
- Provider behavior
- packet processing behavior

The helper must not:

- process packets
- update providers
- update diagnostics
- run production bootstrap
- require Dashboard
- require WebServer
- require Mobile Studio
- require cloud

## Success Criteria

Success means:

- known Base MAC
- known Sender MAC
- recorded MAC values are available for allowlist and peer setup

No packet transmission is required for this phase.

No provider update is required for this phase.

## Stop Conditions

Stop implementation if the helper requires:

- permanent architecture changes
- production bootstrap changes
- `src/main.cpp` changes
- Registry changes
- WirelessService changes
- Runtime changes
- Logic changes
- API changes
- packet processing
- provider updates
- dynamic allocation
- Arduino `String`
- STL containers

## Final Assessment

Milestone 9.1 Phase 9 should create a temporary, removable MAC discovery path for bench setup only.

It should provide the two MAC addresses needed for the real wireless temperature packet test and nothing more.
