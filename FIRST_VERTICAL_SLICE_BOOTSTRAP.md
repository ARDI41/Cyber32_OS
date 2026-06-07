# Cyber32 First Vertical Slice Bootstrap

## Step-By-Step Startup Sequence

1. Create fixed objects.
   - `EventBus event_bus`
   - `Registry registry`
   - `Runtime runtime`
   - `HalTime hal_time`
   - `SimTemperatureDriver driver`
   - `SimTemperatureDevice device`
   - `PnpDiscovery pnp_discovery`
   - `PnpRegistration pnp_registration`
   - `TemperatureService temperature_service`
   - `TemperatureLogic temperature_logic`
   - `Cyber32Api api`

2. Initialize EventBus.
   - Call `event_bus.clear()`.
   - Expected queue count: `0`.
   - Expected dropped count: `0`.

3. Initialize Registry.
   - Call `registry.begin()`.
   - Expected module count: `0`.
   - Expected device count: `0`.
   - Expected capability count: `0`.

4. Wire EventBus to Registry.
   - Call `registry.attachEventBus(&event_bus)`.
   - Registry may now emit capability registration and payload update events.

5. Initialize Runtime.
   - Call `runtime.begin()`.
   - Expected Runtime state: `RuntimeState::BOOTING`.

6. Wire EventBus and Registry to Runtime.
   - Call `runtime.attachEventBus(&event_bus)`.
   - Call `runtime.attachRegistry(&registry)`.
   - Runtime may now process bounded EventBus items.

7. Initialize HAL time.
   - Call `hal_time.begin()`.
   - Use `hal_time.uptimeMs()` for hardware uptime when running on ESP32.

8. Initialize simulated temperature driver.
   - Call `driver.begin()`.
   - Expected driver read value after begin: `22.4` degrees Celsius.

9. Initialize simulated temperature device.
   - Call `device.begin(&driver)`.
   - Expected result: `true`.

10. Wire EventBus to PNP discovery.
    - Call `pnp_discovery.attachEventBus(&event_bus)`.

11. Discover the simulated temperature module.
    - Create `PnpModuleInfo module_info`.
    - Call `pnp_discovery.discoverSimulatedTemperatureModule(module_info)`.
    - Expected result: `true`.
    - Expected `module_info.valid`: `true`.
    - Expected capability: `CAP_TEMPERATURE`.
    - Expected EventBus event: `EVT_MODULE_DISCOVERED`.

12. Initialize PNP registration.
    - Call `pnp_registration.begin(&registry)`.
    - Expected result: `true`.

13. Register discovered module information.
    - Call `pnp_registration.registerModuleInfo(module_info)`.
    - Registration order:
      - `ModuleRecord`
      - `DeviceRecord`
      - `CapabilityRecord`
    - Expected result: `true`.
    - Expected Registry module count: `1`.
    - Expected Registry device count: `1`.
    - Expected Registry capability count: `1`.
    - Expected capability: `CAP_TEMPERATURE`.
    - Expected EventBus event: `EVT_CAPABILITY_REGISTERED`.

14. Initialize Temperature Service.
    - Call `temperature_service.begin(&registry, &device)`.
    - Expected result: `true`.
    - Service ID: `SERVICE_TEMPERATURE`.

15. Initialize Temperature Logic.
    - Call `temperature_logic.begin(&registry)`.
    - Expected result: `true`.
    - Logic receives Registry access only.
    - Logic must not receive module ID or device ID.

16. Initialize internal API.
    - Call `api.begin(&registry, &runtime)`.
    - Expected result: `true`.
    - API reads Registry and Runtime only.

17. Run first service update.
    - Get current time:
      - `uint32_t now_ms = hal_time.uptimeMs()`
    - Call `temperature_service.update(now_ms)`.
    - Expected result: `true`.
    - Expected Registry payload for `CAP_TEMPERATURE`:
      - `available = Availability::AVAILABLE`
      - `stale = StaleState::FRESH`
      - `value_type = PayloadValueType::FLOAT`
      - `value_float = 22.4`
      - `unit = "degree_celsius"`
      - `quality = "valid"`
      - `error_code = "none"`
    - Expected EventBus event: `EVT_CAPABILITY_VALUE_UPDATED`.

18. Run first Logic evaluation.
    - Call `temperature_logic.evaluate()`.
    - Expected result: `true`.
    - Expected Logic status: `LogicStatus::TEMPERATURE_SEEN`.
    - Expected last temperature: `22.4`.

19. Run first API temperature query.
    - Create `ApiCapabilityState temperature_state`.
    - Call `api.getTemperatureState(temperature_state)`.
    - Expected result: `true`.
    - Expected `temperature_state.ok`: `true`.
    - Expected payload capability: `CAP_TEMPERATURE`.
    - Expected payload value: `22.4`.

20. Run first API system status query.
    - Create `ApiSystemStatus system_status`.
    - Call `api.getSystemStatus(system_status)`.
    - Expected result: `true`.
    - Expected `system_status.ok`: `true`.
    - Expected module count: `1`.
    - Expected device count: `1`.
    - Expected capability count: `1`.
    - Expected latest error code: `"none"`.

21. Optional bounded Runtime update.
    - Call `runtime.update(now_ms)`.
    - Runtime may pop up to `4` queued events.
    - Runtime does not execute behavior based on event content in the current slice.

22. Expected EventBus event order before Runtime consumes events.
    - `EVT_MODULE_DISCOVERED`
    - `EVT_CAPABILITY_REGISTERED`
    - `EVT_CAPABILITY_VALUE_UPDATED`

23. Expected final Registry counts.
    - Modules: `1`
    - Devices: `1`
    - Capabilities: `1`

24. Expected final vertical slice state.
    - Simulated module discovered.
    - Simulated module registered.
    - `CAP_TEMPERATURE` registered.
    - Temperature Service updated `CAP_TEMPERATURE`.
    - Registry stores latest temperature payload.
    - Logic queried `CAP_TEMPERATURE` without module-name dependency.
    - API returned `CAP_TEMPERATURE` state.
