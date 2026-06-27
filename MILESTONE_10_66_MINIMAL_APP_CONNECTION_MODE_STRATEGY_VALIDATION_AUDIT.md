# Milestone 10.66 - Minimal App Connection Mode Strategy Validation Audit

## 1. Scope

Milestone 10.66 audits the Milestone 10.65 Minimal App Connection Mode Strategy Plan only.

This milestone is documentation only.

It does not:

* modify firmware source code
* modify Minimal App code
* modify `src/main.cpp`
* implement app code
* add HTTP
* add WebServer
* add JSON transport
* add BLE
* add WiFi transport
* add cloud transport
* add local transport
* add LoRa transport
* add gateway transport
* add pairing, authentication, or QR implementation
* connect to Minimal App
* add production records
* add fake nodes, capabilities, mappings, values, or diagnostics
* add provider, discovery, or Registry bridges
* parse packets
* call HAL, Drivers, Devices, Modules, Services, Logic, or Runtime
* expose private Registry arrays
* expose mutable owner stores
* change existing API behavior

No implementation and no transport are approved by this audit.

## 2. Files Changed In 10.65

Milestone 10.65 modified:

* `MILESTONE_10_65_MINIMAL_APP_CONNECTION_MODE_STRATEGY_PLAN.md`

No firmware source files, app files, or `src/main.cpp` were changed.

## 3. Strategy Summary

Milestone 10.65 defines the future Minimal App connection mode strategy.

Strategy summary:

* future Minimal App supports selectable connection modes.
* default mode is Auto.
* future modes include Local only, Cloud, Gateway/LoRa later, and hidden Advanced mode.
* normal users should not manually enter IP, URL, port, endpoint, MAC, server URL, or raw token.
* Core OS remains transport-independent.
* the same snapshot shape should be consumed regardless of connection mode.

The strategy keeps connection UX product-like while preserving the Core API / snapshot boundary.

## 4. Core Boundary

Core boundary remains clear:

* Core OS must not know whether data is delivered through Local, Cloud, Gateway/LoRa, BLE, WiFi, or HTTP.
* Core exposes public API / snapshot data.
* transport adapters deliver it.
* Minimal App consumes the same logical snapshot shape.
* no Core OS behavior changes were made.
* no source code was changed.

Architecture direction remains transport-independent.

## 5. Auto Mode Audit

Auto mode should:

* be default and recommended.
* first try known paired device identity.
* try local discovery if available.
* use local connection when available.
* use cloud when local is unavailable and cloud is configured.
* support Gateway/LoRa later as gateway-side transport.
* show simple onboarding/help if nothing works.
* never require normal user to type IP, URL, or port.

Auto mode is the normal user path.

## 6. Local Mode Audit

Local mode should:

* connect to a nearby Cyber32 base station or local network device.
* support home, workshop, farm, field, and offline local use.
* use discovery/pairing rather than manual address entry.
* work without internet when already paired and the local device is reachable.
* consume the same snapshot contract.
* not choose implementation technology yet.

Local mode is a user-facing mode, not a selected transport implementation in this milestone.

## 7. Cloud Mode Audit

Cloud mode should:

* use future base station/gateway snapshot sync.
* allow remote monitoring.
* support future multi-user access.
* preserve the same data semantics.
* defer account, auth, sync, and offline cache contracts.
* not change Core data shape.

Cloud mode extends reach. It must not redefine Core data.

## 8. Gateway / LoRa Audit

Gateway/LoRa later should:

* treat LoRa mainly as node-to-gateway or sensor-to-gateway communication.
* avoid phone speaking LoRa directly in normal mode.
* use gateway/base station to convert LoRa-side data into owner-backed public snapshot data.
* keep the app consuming the same snapshot shape.
* defer LoRa transport contract.

Gateway/LoRa is a future transport family and should remain hidden behind the same snapshot semantics.

## 9. Onboarding Audit

Target onboarding:

1. user powers on Cyber32 kit
2. user opens app
3. user taps Add Device
4. user scans QR code or uses guided local discovery
5. app pairs with device
6. Auto mode is selected by default
7. device appears in app

Normal user does not handle:

* IP address
* URL
* port
* endpoint
* MAC address
* mDNS name
* server URL
* raw token
* firmware debug output

Setup should remain guided and non-technical.

## 10. Advanced Settings Audit

Advanced settings are:

* hidden by default
* optional
* not required for normal setup
* for diagnostics/manual override only
* not part of plug-and-play setup

Possible future advanced items:

* manual local address
* custom server URL
* port
* diagnostic connection info
* local discovery status
* cloud sync status
* gateway status
* export/import pairing info if approved later

Advanced settings are escape hatches, not the main UX.

## 11. Pairing Identity Audit

Pairing identity is only high-level planned:

* `device_id`
* public identity
* pairing token or key later
* local discovery name later
* cloud binding later
* gateway binding later

Confirmed:

* no crypto/auth format was implemented.
* no final security format was defined.
* pairing/security require a future approved contract.

Milestone 10.65 intentionally avoided defining security implementation details.

## 12. Snapshot Compatibility Audit

All connection modes should deliver the same logical snapshot:

* nodes
* capabilities
* node-capability mappings
* optional per-node capability summaries
* status metadata

Connection mode must not change data semantics.

The app should interpret Local, Cloud, and future Gateway snapshots the same way.

## 13. Offline Behavior Audit

Offline behavior plan:

* Local mode should work without internet when device is reachable.
* Cloud mode may later show last known snapshot if future cache exists.
* Auto mode should clearly indicate Local / Cloud / Offline / Gateway later state.
* adding new data offline is deferred.
* offline cache/sync conflict handling are future contracts.

Offline states must be visible and understandable without exposing transport internals.

## 14. Transport Separation Audit

Milestone 10.65 does not implement:

* HTTP
* JSON
* BLE
* WiFi
* cloud sync
* LoRa
* gateway
* pairing
* auth
* encryption
* offline cache
* retry logic

Each transport family requires a later approved milestone.

## 15. Safety Boundary

Safety boundary remains intact:

* no fake nodes
* no fake capabilities
* no fake mappings
* no fake sensor values
* no fake diagnostics
* no private Registry arrays exposed
* no packet parsing
* no HAL calls
* no Driver calls
* no Device calls
* no Module calls
* no Service calls
* no Logic calls
* no Runtime calls
* no mutable owner access
* no source code change
* no app code change
* no transport
* no WebServer, HTTP, JSON, BLE, WiFi, or LoRa

The plan remains architecture-only.

## 16. Compatibility Statement

Milestone 10.65 defines future Minimal App connection strategy, but does not implement app connection, transport, pairing, or live data.

The architecture direction remains:

```text
Cyber32 Core OS
-> Cyber32Api
-> Snapshot Builder / Snapshot Contract
-> Transport Adapter
-> Minimal App Connection Manager
-> Minimal App UI
```

Never:

```text
Core OS -> direct transport-specific behavior
```

Core OS remains API-first and transport-independent.

## 17. Known Limitations

Known limitations:

* no app implementation yet
* no connection manager yet
* no local discovery
* no cloud sync
* no gateway/LoRa
* no QR/pairing
* no authentication/encryption
* no offline cache
* no retry logic
* no live Minimal App data
* no transport adapter
* no user-facing device setup yet

These are intentional deferrals until transport and app milestones are approved.

## 18. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.67 - Core API Snapshot Struct Plan
```

Purpose:

Plan the bounded C/C++ snapshot structs and buffer model for read-only Core snapshot data without implementing transport, JSON, HTTP, BLE, WiFi, or fake data.

Alternative:

```text
Milestone 10.67 - Minimal App Mock Connection Manager Plan
```

## 19. Stop Conditions

Stop future work if it tries to:

* implement transport in this audit milestone
* add HTTP, WebServer, JSON, BLE, WiFi, or LoRa
* connect Minimal App directly
* require normal users to type IP, URL, or port
* expose mutable owner stores
* create fake app data
* parse packets
* read Registry arrays
* call HAL, Drivers, Devices, Services, Logic, or Runtime
* modify `src/main.cpp`
* add provider/discovery/value/diagnostics before approved contracts
