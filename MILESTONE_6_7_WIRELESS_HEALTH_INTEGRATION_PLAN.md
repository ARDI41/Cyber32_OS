# Milestone 6.7 Wireless Health Integration Plan

## Goal

Design how `WirelessService::checkTimeouts()` integrates provider health scanning and best-provider payload updates.

This document is documentation only. It does not change source code, does not modify `src/main.cpp`, and does not change current Registry, WirelessService, Logic, Runtime, API, or validation behavior.

## Reviewed Inputs

- `CYBER32_BIBLE.md`
- `MILESTONE_6_6_PROVIDER_HEALTH_TIMEOUTS_PLAN.md`
- `MILESTONE_6_5_PROVIDER_FAILOVER_RECOVERY_PLAN.md`
- `MILESTONE_6_3_WIRELESS_SERVICE_PLAN.md`

## Problem

Registry can now own provider health scanning and provider failover/recovery orchestration:

```text
updateProviderHealth(now_ms)
updateBestCapabilityPayload(capability_id)
```

`WirelessService::checkTimeouts()` does not yet call those Registry methods.

The next step is to wire the wireless timeout task into the Registry-owned provider health model while preserving the Cyber32 architecture:

```text
WirelessService coordinates
Registry owns provider state and selection
Runtime schedules only
Logic remains provider-blind
```

## 1. WirelessService::checkTimeouts() Responsibility

`WirelessService::checkTimeouts(uint32_t now_ms)` is responsible for coordinating wireless provider timeout processing.

It may:

- verify required attachments exist
- call Registry provider health scanning
- ask Registry to update the best canonical capability payload
- return compact success/failure
- store compact last error state if needed

It must not:

- scan Registry provider arrays
- choose providers
- copy provider payloads into canonical capabilities itself
- parse packets
- call Logic
- expose API
- call Runtime
- know dashboard or Mobile Studio behavior

For the first wireless slice, the only capability affected is:

```text
CAP_TEMPERATURE
```

Future wireless capabilities should extend the same pattern capability by capability.

## 2. Required Registry Calls

`WirelessService::checkTimeouts(now_ms)` should call:

```cpp
registry_->updateProviderHealth(now_ms);
registry_->updateBestCapabilityPayload(CAP_TEMPERATURE);
```

Required order:

```text
1. updateProviderHealth(now_ms)
2. updateBestCapabilityPayload(CAP_TEMPERATURE)
```

Reason:

- health scan updates provider eligibility
- best-provider update uses the latest provider health
- canonical payload changes only through Registry public API

## 3. Behavior

### Missing Registry

If Registry is not attached:

```text
return false
```

Recommended compact error:

```text
ERR_CAPABILITY_UNAVAILABLE
```

or future wireless-specific error if added later.

### Health Update Failure

Call:

```text
updateProviderHealth(now_ms)
```

If result is not `RegistryResult::OK`:

```text
return false
```

Current planned Registry behavior returns `OK` for a bounded scan, even when no providers exist.

### Best Provider Payload Update

Call:

```text
updateBestCapabilityPayload(CAP_TEMPERATURE)
```

If result is `RegistryResult::OK`:

```text
return true
```

If result is `RegistryResult::NOT_FOUND`:

```text
return false
keep system safe
canonical payload remains last known value
```

If result is another failure:

```text
return false
canonical payload remains controlled by Registry failure behavior
```

The Service must not attempt to manually repair provider state or canonical payloads.

## 4. Canonical Payload Policy

`checkTimeouts()` may update canonical payload only through Registry public API:

```text
updateBestCapabilityPayload(CAP_TEMPERATURE)
```

WirelessService must not:

- call `getCapabilityProvider()` and copy payloads itself
- call `updateCapabilityPayload()` with a provider payload
- write Registry internals
- transform provider payloads during timeout processing

This preserves the split:

```text
WirelessService -> coordination
Registry -> provider health, selection, active mapping, canonical payload copy
Logic -> provider-blind CAP_* reads
```

## 5. Runtime Task Future

Future Runtime task:

```text
task.wireless_service.check_timeouts
```

Task callback:

```text
WirelessService::checkTimeouts(now_ms)
```

Runtime remains scheduler-only.

Runtime must not:

- know `CAP_TEMPERATURE`
- own provider timeout thresholds
- inspect provider status
- choose active providers
- copy canonical payloads
- parse wireless packets

Runtime only calls the registered Service callback with a bounded task context.

## 6. Expected Flow

### Normal Healthy Wireless Provider

```text
wireless provider AVAILABLE
wireless provider fresh
checkTimeouts(now_ms)
  -> updateProviderHealth(now_ms) returns OK
  -> updateBestCapabilityPayload(CAP_TEMPERATURE) returns OK
canonical CAP_TEMPERATURE remains wireless value
return true
```

### Wireless Provider Times Out

```text
wireless provider active at 23.5F
wireless last_update_ms exceeds lost timeout
sim provider AVAILABLE at 22.4F
checkTimeouts(now_ms)
  -> updateProviderHealth(now_ms) marks wireless LOST
  -> updateBestCapabilityPayload(CAP_TEMPERATURE) selects sim provider
canonical CAP_TEMPERATURE becomes 22.4F
return true
```

### No Eligible Provider

```text
wireless provider LOST
sim provider UNAVAILABLE
checkTimeouts(now_ms)
  -> updateProviderHealth(now_ms) returns OK
  -> updateBestCapabilityPayload(CAP_TEMPERATURE) returns NOT_FOUND
canonical payload remains last known value
return false
```

### Wireless Provider Recovers

```text
valid packet arrives with value 24.0F
processPackets(now_ms)
  -> updates provider payload
  -> provider status AVAILABLE
checkTimeouts(now_ms) or future best update flow
  -> updateBestCapabilityPayload(CAP_TEMPERATURE)
canonical CAP_TEMPERATURE returns to wireless 24.0F
```

## 7. Validation Plan

Validation should be added after `WirelessService::checkTimeouts()` is implemented.

### Timeout Failover

Setup:

```text
provider-wireless-temperature-001
  status = AVAILABLE
  priority = 20
  payload = 23.5F
  last_update_ms older than PROVIDER_LOST_TIMEOUT_MS

provider-sim-temperature-001
  status = AVAILABLE
  priority = 10
  payload = 22.4F
```

Call:

```text
wireless_service_.checkTimeouts(now_ms)
```

Expect:

```text
return true
wireless provider status == LOST
active provider == provider-sim-temperature-001
canonical CAP_TEMPERATURE == 22.4F
```

### No Eligible Provider

Setup:

```text
wireless provider LOST
sim provider UNAVAILABLE
canonical CAP_TEMPERATURE last known 23.5F or 24.0F
```

Call:

```text
wireless_service_.checkTimeouts(now_ms)
```

Expect:

```text
return false
canonical CAP_TEMPERATURE remains last known value
no direct Service payload copy
```

### Recovery

Setup:

```text
wireless provider LOST
sim provider active at 22.4F
```

Call:

```text
inject valid wireless packet with CAP_TEMPERATURE = 24.0F
wireless_service_.processPackets(now_ms)
wireless_service_.checkTimeouts(now_ms)
```

Expect:

```text
wireless provider status == AVAILABLE
active provider == provider-wireless-temperature-001
canonical CAP_TEMPERATURE == 24.0F
```

### Attachment Failure

Setup:

```text
WirelessService without Registry attachment
```

Call:

```text
checkTimeouts(now_ms)
```

Expect:

```text
return false
no Registry access
```

### Existing Validation Preservation

Confirm existing validation remains intact:

- wired/simulated temperature
- distance
- servo
- motor
- relay
- wireless provider payload update
- provider storage
- active provider mapping
- selected provider payload update
- best provider failover/recovery
- provider health timeout validation

## 8. Error And Result Handling

Recommended v1 mapping:

```text
RegistryResult::OK        -> checkTimeouts returns true
RegistryResult::NOT_FOUND -> checkTimeouts returns false, safe no-provider state
other RegistryResult      -> checkTimeouts returns false
```

If `WirelessService` stores compact diagnostics:

```text
last_registry_result_
last_error_code_
```

No dynamic strings are allowed.

## 9. Stop Conditions

Stop and review architecture if any of these occur:

- WirelessService scans Registry arrays
- WirelessService chooses provider itself
- WirelessService copies provider payload into canonical capability directly
- WirelessService writes canonical capability payload with `updateCapabilityPayload()` for timeout failover
- Runtime owns timeout policy
- Runtime knows `CAP_TEMPERATURE`
- Logic sees provider ID
- Logic sees provider health
- canonical payload is written directly by Service
- provider timeout processing requires dynamic allocation
- Arduino `String` or STL containers are introduced in core paths

## Final Assessment

Milestone 6.7 should make `WirelessService::checkTimeouts()` a narrow coordinator:

```text
updateProviderHealth(now_ms)
updateBestCapabilityPayload(CAP_TEMPERATURE)
```

That keeps the architecture intact:

```text
Registry owns provider health and selection
WirelessService coordinates wireless timeout task behavior
Runtime schedules only
Logic remains provider-blind
canonical payload updates happen only through Registry public APIs
```

