# Milestone 7.9 Real ESP-NOW Integration Checklist

## Goal

Create the final readiness checklist before implementing real ESP-NOW transport.

This document is documentation only. It does not change source code, does not modify `src/main.cpp`, and does not add real ESP-NOW/WiFi behavior.

## Reviewed Inputs

- `CYBER32_BIBLE.md`
- `MILESTONE_7_8_ESPNOW_TRANSPORT_BOUNDARY_PLAN.md`
- `MILESTONE_7_7_ESPNOW_IDENTITY_PAIRING_PLAN.md`
- `MILESTONE_7_5_WIRELESS_SECURITY_AUDIT.md`

## Overall Readiness

**WARNING**

Cyber32 is architecturally ready to begin a tightly bounded real ESP-NOW driver slice, but not production-ready for wireless security.

The next real ESP-NOW milestone may proceed only if it keeps real radio code isolated inside the driver boundary and continues using simulated validation as the default path.

## 1. Driver Boundary Readiness

Status: **PASS**

Checklist:

- Real driver location defined:
  - `src/drivers/communication/espnow_transport_driver.h`
  - `src/drivers/communication/espnow_transport_driver.cpp`
- `WiFi.h` must be included only inside the real driver boundary.
- `esp_now.h` must be included only inside the real driver boundary.
- No ESP-NOW includes are allowed in Registry, Runtime, Logic, API, Services, Devices, PNP, or validation.
- Real driver must expose bounded packet read behavior.
- Real driver must not parse capability semantics.
- Real driver must not update Registry.
- Real driver must not choose providers.

Stop if:

- `WiFi.h` or `esp_now.h` appears outside the real driver files.

## 2. Packet Contract Readiness

Status: **PASS**

Required packet structs exist:

- `WirelessPacketHeader`
- `WirelessCapabilityValue`
- `WirelessNodeDiagnostics`

Required packet fields exist:

- `checksum`
- `sequence_id`
- `node_id`
- `packet_type`
- `capability_id`
- payload type/value fields
- diagnostics fields

Checklist:

- Packet stays bounded.
- No JSON.
- No heap allocation.
- Checksum is available.
- Sequence ID is available.
- Packet size must remain within ESP-NOW limit.

Warning:

- Current checksum is simple and suitable for v1 corruption detection, not production authentication.

## 3. MAC Identity Readiness

Status: **PASS with warning**

Ready:

- `WIRELESS_MAC_ADDRESS_SIZE = 6`
- `WirelessNodeAllowlistRecord` includes bounded `mac_address[6]`
- `has_mac_address` field exists
- `wirelessMacAddressEquals(...)` helper exists
- `clearWirelessMacAddress(...)` helper exists
- Registry can read allowlist records by MAC
- simulated transport can inject/read optional source MAC

Still required for real driver:

- capture source MAC from ESP-NOW receive callback
- expose source MAC with packet read
- map source MAC to allowlist record before provider update

Warning:

- MAC identity is not strong authentication.

## 4. Security Gate Readiness

Status: **PASS with warning**

Security gates currently implemented in simulated flow:

- checksum validation
- allowlist validation by node ID
- trust validation
- duplicate sequence protection

Required before real ESP-NOW provider updates:

- MAC-to-node allowlist mapping must be used
- unknown MAC must be rejected
- blocked MAC must be rejected
- valid MAC must map to node ID

Warning:

- No keyed message integrity yet.
- No bounded replay window beyond last-sequence duplicate.

## 5. Runtime Readiness

Status: **PASS**

Runtime tasks exist conceptually and in validation:

- `task.wireless_service.process_packets`
- `task.wireless_service.check_timeouts`

Runtime remains scheduler-only.

Runtime must not:

- call ESP-NOW directly
- parse packets
- inspect MAC addresses
- choose providers
- own allowlist/trust policy

## 6. Provider Readiness

Status: **PASS**

Provider system supports:

- provider payload update
- selected payload update
- best provider update
- failover/recovery
- health timeouts
- active provider mapping

Wireless flow supports:

- wireless provider latest payload update
- canonical payload update through Registry methods
- failover to simulated provider when wireless provider is lost
- recovery to wireless provider when it returns

## 7. Diagnostics Readiness

Status: **PASS with warning**

Ready:

- provider diagnostics API
- wireless node diagnostics API
- provider summary
- wireless node summary

Warning:

- Rejection counters are future-ready fields but not fully backed by Registry state yet.
- Real ESP-NOW MAC diagnostics should be added before production use.

## 8. Real Hardware Test Plan

Status: **WARNING**

Minimum hardware:

- one ESP32 base station running Cyber32
- one ESP32 wireless node

Initial test setup:

- fixed `node_id`
- known source MAC
- allowlist record for known MAC
- `CAP_TEMPERATURE` packet
- valid checksum
- incrementing sequence ID

Expected result:

- base station receives packet
- real driver captures source MAC
- WirelessService maps MAC to node
- security gates pass
- wireless provider payload updates
- canonical payload updates only through existing provider flow

## 9. Failure Tests

Status: **WARNING**

Required real-driver failure tests:

- unknown MAC rejected
- blocked node rejected
- bad checksum rejected
- duplicate sequence rejected
- stale/lost timeout causes failover
- recovered node causes provider recovery
- malformed packet rejected
- packet larger than bounded size rejected
- transport failure mode does not corrupt Registry state

Expected failure behavior:

- provider payload unchanged
- canonical payload unchanged
- packet consumed or safely dropped
- compact error visible in diagnostics where supported

## 10. Stop Conditions

Status: **BLOCKER if violated**

Stop real ESP-NOW implementation immediately if:

- `WiFi.h` leaks outside real driver boundary
- `esp_now.h` leaks outside real driver boundary
- Registry parses ESP-NOW packets
- Runtime parses ESP-NOW packets
- Logic sees MAC address
- Logic sees node ID
- Logic sees provider ID
- Logic sees packet type/checksum/sequence ID
- real driver updates Registry
- real driver chooses providers
- dynamic allocation is introduced in core packet path
- Arduino `String` is introduced in core packet path
- STL containers are introduced in core packet path
- unknown nodes update providers
- blocked nodes update providers

## Final Checklist

| Area | Status |
| --- | --- |
| Driver boundary | PASS |
| Packet contract | PASS |
| MAC identity foundation | PASS with warning |
| Security gates | PASS with warning |
| Runtime integration | PASS |
| Provider integration | PASS |
| Diagnostics | PASS with warning |
| Hardware test plan | WARNING |
| Production security | WARNING |
| Stop conditions | BLOCKER if violated |

## Final Assessment

**WARNING**

Cyber32 is ready for a limited real ESP-NOW driver prototype behind the strict driver boundary.

Cyber32 is not yet ready for production wireless deployment until MAC diagnostics, persistence, stronger message integrity, and bounded replay policy are completed.
