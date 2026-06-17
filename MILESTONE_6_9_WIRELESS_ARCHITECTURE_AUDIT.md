# Milestone 6.9 Wireless Architecture Audit

## Goal

Audit the completed simulated wireless provider flow against the Cyber32 architecture.

This document is documentation only. It does not change source code, does not modify `src/main.cpp`, and does not add real ESP-NOW, WiFi, WebServer, Dashboard, or Mobile Studio behavior.

## Reviewed Inputs

- `CYBER32_BIBLE.md`
- `MILESTONE_6_3_WIRELESS_SERVICE_PLAN.md`
- `MILESTONE_6_6_PROVIDER_HEALTH_TIMEOUTS_PLAN.md`
- `MILESTONE_6_8_WIRELESS_RUNTIME_TASK_PLAN.md`

## Overall Assessment

Pass with warnings.

The simulated wireless provider flow follows the Cyber32 capability-first architecture:

```text
SimEspNowTransportDriver
-> WirelessTemperatureDevice
-> WirelessService
-> Registry provider record
-> Registry provider health and selection
-> canonical CAP_TEMPERATURE payload
-> Logic/API provider-blind reads
```

The slice remains simulation-only and is not ready for real ESP-NOW until packet checksum/security, peer handling, production bootstrap, and real radio boundaries are implemented and audited.

## 1. Layer Compliance

Assessment: Pass

Observed architecture:

- simulated transport driver stores one bounded received packet
- virtual wireless Device converts validated packet data into `CapabilityPayload`
- wireless Module remains metadata-only
- PNP discovery/registration remain metadata and Registry API based
- WirelessService coordinates packet processing and timeout checks
- Registry stores provider records, health, active provider mapping, and canonical payloads
- Runtime invokes WirelessService through callbacks only
- Logic continues reading `CAP_TEMPERATURE`

No layer should bypass its public contract.

Pass criteria met:

- Device does not write Registry
- WirelessService uses Registry public APIs
- Runtime does not parse packets
- Logic does not know provider IDs
- API does not read raw packets

## 2. Runtime Scheduler-Only Rule

Assessment: Pass

Runtime task integration uses:

```text
task.wireless_service.process_packets
task.wireless_service.check_timeouts
```

Runtime responsibilities remain limited to:

- storing task records
- invoking callbacks
- passing context pointers
- tracking task execution

Runtime does not:

- parse wireless packets
- know wireless packet schema
- inspect `CAP_TEMPERATURE`
- select providers
- scan provider records
- call Registry provider APIs directly
- update canonical payloads directly

The validation-level task contexts keep `now_ms` and execution facts outside Runtime internals.

## 3. Registry Provider Ownership

Assessment: Pass

Registry owns:

- provider storage
- provider status
- provider latest payload
- provider health timeout transitions
- active provider mapping
- provider selection
- selected provider payload copy into canonical capability payload

Implemented Registry-owned operations include:

```text
registerCapabilityProviderWithResult()
updateCapabilityProviderPayload()
setActiveProvider()
getActiveProvider()
selectBestProvider()
updateSelectedCapabilityPayload()
updateBestCapabilityPayload()
updateProviderHealth()
```

This matches the Cyber32 Bible:

```text
Registry owns state.
Registry owns provider selection.
Logic remains provider-blind.
```

## 4. WirelessService Responsibilities

Assessment: Pass

WirelessService responsibilities are correctly scoped.

WirelessService may:

- read one pending simulated packet through the transport driver
- call `WirelessTemperatureDevice::updateFromPacket(...)`
- read Device payload
- update provider payload through Registry public API
- coordinate provider health scan through Registry public API
- coordinate best-provider canonical update through Registry public API

WirelessService does not:

- scan Registry arrays
- choose providers directly
- copy canonical payloads directly
- expose API
- run Logic
- include real ESP-NOW or WiFi headers

This preserves the Service layer role:

```text
Services own policy and coordination.
Registry owns state and selection.
Runtime schedules only.
```

## 5. Provider Selection

Assessment: Pass

Provider selection is Registry-owned and bounded.

Selection behavior:

- only matching `capability_id` providers are considered
- only `AVAILABLE` providers are selected
- highest priority wins
- priority tie uses newest `last_update_ms`
- `LOST`, `UNAVAILABLE`, `DISABLED`, `STALE`, and `UNKNOWN` are not selected by `selectBestProvider()`

Provider selection does not mutate active provider mapping by itself.

Canonical payload changes require explicit Registry calls:

```text
updateSelectedCapabilityPayload(capability_id)
updateBestCapabilityPayload(capability_id)
```

## 6. Failover And Recovery

Assessment: Pass

Failover path:

```text
wireless provider LOST
sim provider AVAILABLE
updateBestCapabilityPayload(CAP_TEMPERATURE)
-> active provider becomes provider-sim-temperature-001
-> canonical CAP_TEMPERATURE becomes 22.4F
```

Recovery path:

```text
wireless provider receives valid update
provider becomes AVAILABLE
updateBestCapabilityPayload(CAP_TEMPERATURE)
-> active provider becomes provider-wireless-temperature-001
-> canonical CAP_TEMPERATURE becomes recovered wireless value
```

Validation covers:

- wireless priority wins
- lost wireless fails over to simulated
- wireless recovery returns active provider to wireless
- no eligible provider leaves canonical payload at last known value

This is consistent with the v1 policy.

## 7. Timeout Behavior

Assessment: Pass

Provider timeout scanning is Registry-owned.

Timeout constants:

```text
PROVIDER_STALE_TIMEOUT_MS
PROVIDER_LOST_TIMEOUT_MS
```

Health transitions:

```text
AVAILABLE -> STALE
STALE -> LOST
LOST -> LOST
UNAVAILABLE -> UNAVAILABLE
DISABLED -> DISABLED
UNKNOWN -> UNKNOWN
```

Canonical payload policy is preserved:

```text
updateProviderHealth(now_ms)
```

does not rewrite canonical payloads by itself.

Canonical failover requires:

```text
updateBestCapabilityPayload(CAP_TEMPERATURE)
```

WirelessService timeout integration correctly coordinates:

```text
updateProviderHealth(now_ms)
updateBestCapabilityPayload(CAP_TEMPERATURE)
```

through Registry public APIs.

## 8. Capability-First Behavior

Assessment: Pass

The wireless path preserves the same capability contract as wired/simulated providers.

Both examples resolve to:

```text
CAP_TEMPERATURE
```

Logic and API do not need to know:

- wired provider
- simulated provider
- wireless provider
- transport
- node ID
- provider ID

The canonical capability payload remains the public state surface.

## 9. Logic Provider-Blindness

Assessment: Pass

Logic continues to query:

```text
getCapabilityPayload(CAP_TEMPERATURE)
```

Logic does not inspect:

- provider IDs
- provider types
- wireless node IDs
- transport names
- packet data
- signal or battery diagnostics

This satisfies the Cyber32 provider-blind Logic rule.

## 10. Memory And Allocation Rules

Assessment: Pass

The simulated wireless flow remains bounded.

Bounded structures:

- one-slot simulated transport packet storage
- fixed-size wireless packet structs
- fixed provider table
- fixed active provider mapping table
- fixed Runtime task table
- fixed validation task contexts

No design requirement introduces:

- dynamic allocation
- Arduino `String`
- STL containers
- unbounded packet queues
- provider history
- packet history

Warning: A real ESP-NOW implementation must preserve these constraints when integrating vendor radio APIs.

## 11. Readiness For Real ESP-NOW

Assessment: Warning

The architecture is ready for the next simulated validation phase, but not ready for real ESP-NOW hardware.

Missing before real ESP-NOW:

- real ESP-NOW transport driver boundary
- `WiFi.h` and `esp_now.h` isolation plan
- peer registration policy
- packet checksum or CRC validation
- duplicate sequence handling beyond schema design
- replay protection policy
- trust model enforcement
- node allowlist or pairing model
- radio failure handling
- RSSI or signal quality source integration
- battery diagnostics provider registration
- production bootstrap wiring
- PlatformIO build verification in target environment

Real ESP-NOW must not enter the codebase until these items are planned, implemented, and validated in bounded form.

## 12. Validation Coverage Audit

Assessment: Pass with warning

Covered:

- provider storage
- active provider mapping
- provider selection
- selected provider payload copy
- best provider payload update
- provider failover
- provider recovery
- provider health timeout transitions
- WirelessService timeout integration
- Runtime task invocation for wireless process path
- Runtime task invocation for wireless timeout path
- canonical payload remains explicit and provider-blind

Warning:

The environment still lacks a successful PlatformIO build because `pio` is not available on PATH in the current workspace. Final milestone completion still requires a real build pass.

## 13. API Visibility

Assessment: Warning

Normal API capability reads remain compatible because canonical payload state is updated in Registry.

Missing future diagnostics:

- active provider ID
- provider status
- provider type
- provider priority
- provider last update timestamp
- wireless battery diagnostics
- wireless signal diagnostics
- lost/stale provider visibility

This is not an architecture violation, but provider diagnostics API should be added before production wireless deployments.

## 14. Security And Trust

Assessment: Warning

The simulated flow assumes trusted test packets.

Before real ESP-NOW:

- node identity must be validated
- packet checksum or CRC must fail closed
- unknown nodes must be rejected or quarantined
- sequence IDs must detect duplicates
- replay handling must be defined
- provider updates must not be accepted from untrusted nodes

Security remains a future milestone.

## 15. Passes

- Capability-first contract preserved.
- Runtime remains scheduler-only.
- Registry owns provider state, selection, health, and canonical payload copy.
- WirelessService coordinates through Registry public APIs.
- WirelessService does not scan Registry arrays.
- WirelessService does not choose providers directly.
- Logic remains provider-blind.
- Canonical payload updates are explicit and testable.
- Provider failover and recovery are validated.
- Timeout health transitions are validated.
- Runtime wireless process and timeout tasks are validated.
- No dynamic allocation, Arduino `String`, or STL containers are required by the simulated flow.
- No real ESP-NOW, WiFi, WebServer, Dashboard, cloud, or Mobile Studio behavior was introduced.

## 16. Warnings

- PlatformIO build still needs to be verified in an environment with `pio` available.
- Provider diagnostics API is still missing.
- Wireless battery and signal diagnostic capabilities are not yet implemented.
- Real ESP-NOW security and trust model is not implemented.
- Packet checksum/CRC is not implemented.
- Production bootstrap is not wired in `src/main.cpp`.
- Real ESP-NOW radio integration is not ready.

## 17. Failures

No architecture failures found in the simulated wireless provider flow.

## 18. Recommended Fixes

Before real ESP-NOW:

1. Add provider diagnostics API.
2. Add battery and signal provider/capability handling.
3. Define and implement packet checksum/CRC validation.
4. Define and implement node trust/allowlist behavior.
5. Add duplicate sequence handling validation.
6. Add real ESP-NOW transport plan before including ESP-NOW headers.
7. Add production bootstrap plan update for wireless tasks.
8. Verify PlatformIO build.

## 19. Final Assessment

Pass with warnings.

The simulated wireless provider flow is architecturally sound and consistent with Cyber32's master rules:

```text
capability-first
local-first
bounded by default
simulation before hardware
Runtime scheduler-only
Registry owns state
Services coordinate policy
Logic provider-blind
```

The next safe steps are diagnostics, security, checksum validation, and production bootstrap planning before any real ESP-NOW hardware work.

