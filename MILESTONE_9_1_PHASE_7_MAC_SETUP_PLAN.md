# Milestone 9.1 Phase 7 MAC Setup Plan

## Goal

Define the manual MAC discovery and configuration process for the first real Cyber32 wireless node test.

This plan is documentation only. It does not modify source code and does not modify `src/main.cpp`.

## Required MAC Addresses

The two-board test requires:

- Base ESP32 MAC
- Sender ESP32 MAC

These values must be recorded before the first real wireless `CAP_TEMPERATURE` packet test.

## MAC Discovery Procedure

### Temporary Serial Output Method

For each ESP32 board:

1. Load a temporary MAC discovery sketch or temporary test code.
2. Initialize WiFi STA mode.
3. Print `WiFi.macAddress()` to serial.
4. Record the MAC address.
5. Remove the temporary code after discovery.

Temporary code must not become part of the Cyber32 production bootstrap.

Temporary code must not remain in `src/main.cpp`.

### Alternative Methods

If already available, MAC address may also be read from:

- existing ESP32 board tooling
- bootloader/monitor output
- a dedicated hardware-test utility
- ESP-IDF/Arduino helper sketch outside Cyber32 production source

The selected method must produce the exact MAC bytes used by ESP-NOW.

## Configuration Mapping

### Sender MAC To Base Allowlist

The sender ESP32 MAC maps to the base node allowlist record:

```text
sender MAC
-> Base allowlist record
-> node_id = 1001
```

### Base MAC To Sender Peer

The base ESP32 MAC maps to the sender ESP-NOW peer setup:

```text
base MAC
-> sender ESP-NOW peer registration
```

## Required Records

Base node allowlist record must contain:

```text
node_id = 1001
allow_state = ALLOWED
trust_state = TRUSTED
sender MAC associated with node_id 1001
```

The sender packet must contain:

```text
header.node_id = 1001
```

The ESP-NOW source MAC observed by the base must match the sender MAC in the allowlist.

## Validation

Verify:

1. received source MAC equals expected sender MAC
2. Registry allowlist lookup by MAC succeeds
3. MAC record `node_id` equals packet `header.node_id`
4. node ID allowlist lookup succeeds
5. trust validation succeeds
6. MAC-to-node validation succeeds

Expected successful path:

```text
source MAC
-> allowlist lookup by MAC
-> node_id match
-> node allowlist lookup
-> trust check
-> sequence check
-> provider update
```

## Failure Cases

### Unknown MAC

Condition:

- packet source MAC is not present in base allowlist

Expected:

- packet rejected
- provider payload unchanged
- `mac_not_allowed_reject_count` increments
- `last_error_code = wireless_mac_not_allowed`

### Wrong MAC

Condition:

- source MAC belongs to a different sender or stale test board

Expected:

- packet rejected
- provider payload unchanged
- `mac_not_allowed_reject_count` increments if MAC is not registered
- `mac_node_mismatch_reject_count` increments if MAC is registered to a different node

### MAC / Node Mismatch

Condition:

- source MAC maps to one node
- packet claims a different `node_id`

Expected:

- packet rejected
- provider payload unchanged
- `mac_node_mismatch_reject_count` increments
- `last_error_code = wireless_mac_node_mismatch`

## Stop Conditions

Stop if MAC setup requires:

- architecture changes
- Runtime changes
- Registry redesign
- WirelessService redesign
- Dashboard
- Mobile Studio
- cloud
- persistence
- actuator behavior
- `src/main.cpp` changes
- dynamic allocation
- Arduino `String`
- STL containers

## Final Assessment

Milestone 9.1 Phase 7 should produce a reliable manual identity setup for the first real Cyber32 wireless temperature test.

The base node should trust only the sender MAC explicitly mapped to `node_id = 1001`.
