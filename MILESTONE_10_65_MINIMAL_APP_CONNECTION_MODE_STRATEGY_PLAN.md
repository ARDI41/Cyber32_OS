# Milestone 10.65 - Minimal App Connection Mode Strategy Plan

## 1. Scope

Milestone 10.65 plans the future Minimal App connection mode strategy only.

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

The purpose is to plan connection mode UX and architecture while keeping normal user onboarding simple and keeping Cyber32 Core OS transport-independent.

## 2. Background

Current state:

* Core API Snapshot Contract exists as a future read-only data shape.
* Minimal App has no live transport yet.
* Core OS must remain transport-independent.
* future app may receive the same snapshot through different connection modes.
* user wants client-selectable modes without technical setup complexity.

The next transport-facing work needs an app strategy before any local, cloud, BLE, WiFi, HTTP, or gateway implementation begins.

## 3. Core Principle

Core OS must not know whether data is delivered through Local, Cloud, Gateway/LoRa, BLE, WiFi, or HTTP.

Core exposes public API / snapshot data.

Transport adapters deliver it.

Minimal App consumes the same logical snapshot shape.

The data path should remain:

```text
Cyber32 Core OS
-> Cyber32Api
-> Snapshot Contract
-> Transport Adapter
-> Minimal App
```

Connection mode must not change Core semantics.

## 4. User-Facing Connection Modes

Future user-facing modes:

* Auto
* Local only
* Cloud
* Gateway / LoRa later
* Advanced technical mode, hidden by default

Default mode:

```text
Auto
```

Auto should be recommended and should work for most users without manual configuration.

Normal users should not need to understand transport details to use a Cyber32 kit.

## 5. Auto Mode Behavior

Planned Auto mode behavior:

* first try known paired device identity
* try local discovery if available
* if local connection is available, use local
* if local is unavailable, use cloud if configured
* if Gateway/LoRa is configured later, treat it as gateway-side transport
* if nothing works, show simple onboarding/help
* never require normal user to type IP, URL, or port

Auto mode should prefer the fastest available trusted connection without changing snapshot data semantics.

## 6. Local Mode Behavior

Planned Local mode behavior:

* phone/app connects to a nearby Cyber32 base station or local network device
* intended for home, workshop, farm, field, and local offline usage
* should use discovery/pairing rather than manual address entry
* should work without internet when already paired if local network/device is reachable
* should consume the same snapshot contract

Implementation technology is not chosen in this milestone.

Possible future local technologies remain separate contracts and may include local network discovery, setup mode, or other approved local mechanisms.

## 7. Cloud Mode Behavior

Planned Cloud mode behavior:

* base station or gateway syncs read-only snapshots to cloud later
* app reads from cloud when user is remote
* useful for remote monitoring and multi-user access
* must not change Core data shape
* cloud account/auth/sync are future contracts
* offline cache is a future contract

Cloud mode should extend access, not redefine Core data.

Core should initiate outbound cloud connections when cloud behavior is eventually approved. Router port forwarding should not be required for normal users.

## 8. Gateway / LoRa Mode Behavior

Planned Gateway/LoRa behavior:

* LoRa is primarily node-to-gateway or sensor-to-gateway communication
* phone should not normally speak LoRa directly
* gateway/base station converts LoRa-side data into owner-backed Core/public snapshot data
* app still consumes the same snapshot shape
* LoRa transport contract is deferred

Gateway/LoRa mode should appear to the app as another connection source for the same logical snapshot, not a separate data model.

## 9. Normal User Onboarding

Target onboarding:

1. user powers on Cyber32 kit
2. user opens app
3. user taps Add Device
4. user scans QR code or uses guided local discovery
5. app pairs with device
6. Auto mode is selected by default
7. device appears in app

Normal user should not handle:

* IP address
* URL
* port
* endpoint
* MAC address
* mDNS name
* server URL
* raw token
* firmware debug output

Setup should feel like pairing a product, not configuring a server.

## 10. Advanced Settings

Hidden advanced options may include:

* manual local address
* custom server URL
* port
* diagnostic connection info
* local discovery status
* cloud sync status
* gateway status
* export/import pairing info if approved later

Rules:

* advanced settings hidden by default
* not required for normal setup
* should not be needed for plug-and-play kits

Advanced settings are diagnostic and recovery tools, not the primary UX.

## 11. Pairing Identity Concept

High-level pairing identity concepts:

* `device_id`
* public identity
* pairing token or key later
* local discovery name later
* cloud binding later
* gateway binding later

This milestone does not implement crypto or authentication.

This milestone does not define final security format.

Pairing/security requires a future approved contract.

## 12. Snapshot Compatibility

All connection modes should deliver the same logical snapshot contract:

* nodes
* capabilities
* node-capability mappings
* optional per-node capability summaries
* status metadata

Connection mode must not change data semantics.

Local, cloud, and future gateway paths may differ in delivery mechanics, but the app should interpret the snapshot the same way.

## 13. Offline Behavior

Planned offline behavior:

* Local mode should work without internet when device is reachable.
* Cloud mode may show last known snapshot if future cache exists.
* Auto mode should clearly indicate offline/local/cloud state.
* adding new data offline is deferred.
* offline cache/sync conflict handling are future contracts.

Offline state should be visible to users in simple language.

## 14. Minimal App UX Rules

Minimal App UX rules:

* simple default Auto mode
* clear status indicator: Local / Cloud / Offline / Gateway later
* user can manually choose mode if desired
* app should explain mode in simple words
* no technical jargon in main UX
* no forced manual address entry
* failed connection should show guided recovery

Main app UX should stay product-like and friendly. Technical connection details belong in advanced settings.

## 15. Transport Separation

This plan does not implement:

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

These require later milestones.

## 16. Core OS Safety Boundary

Core OS safety boundary:

* no Core OS behavior change
* no source code change
* no `src/main.cpp` change
* no Registry access
* no packet parsing
* no HAL, Driver, Device, Service, Logic, or Runtime calls
* no mutable owner store exposure
* no fake data

Core OS remains API-first and transport-independent.

## 17. Future Architecture Direction

Future layers:

```text
Cyber32 Core OS
-> Cyber32Api
-> Snapshot Builder / Snapshot Contract
-> Transport Adapter
-> Minimal App Connection Manager
-> Minimal App UI
```

Transport adapters may include:

* Local adapter
* Cloud adapter
* Gateway adapter later

Transport adapters deliver data. They do not define Core ownership, mutate Core state, infer mappings, or create fake data.

## 18. Validation Plan For Future Milestones

Future validation should prove:

* app can switch between mocked Auto/Local/Cloud states
* same snapshot shape is accepted from all modes
* normal setup does not require manual IP, URL, or port
* advanced settings remain optional
* offline/local/cloud status display works
* no fake Core data required
* no mutable owner access required
* transport failures do not mutate Core state

Validation should begin with mock connection modes before live transport is introduced.

## 19. Recommended Next Milestone

Recommended next milestone:

```text
Milestone 10.66 - Minimal App Connection Mode Strategy Validation Audit
```

Purpose:

Audit the connection mode strategy before any transport or app implementation.

Alternative:

```text
Milestone 10.66 - Core API Snapshot Struct Plan
```

## 20. Stop Conditions

Stop future work if it tries to:

* implement transport in this strategy milestone
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
