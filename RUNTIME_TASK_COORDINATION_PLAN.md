# Cyber32 Runtime Task Coordination Plan

## Goal

Define how Cyber32 Runtime will coordinate Service and Logic execution for the existing first vertical slice.

This document does not introduce new features.

It preserves the fixed architecture:

```text
HAL
-> Drivers
-> Devices
-> Modules
-> PNP
-> Registry
-> Runtime
-> Services
-> Logic
-> API
-> Dashboard
```

Runtime schedules tasks only.

Runtime does not execute business logic.

Services own update policy.

Logic owns evaluation policy.

## Existing RuntimeTask Structure

The current Runtime task contract is:

```text
task_id
enabled
period_ms
next_run_ms
last_run_ms
callback
context
```

Execution model:

```text
Runtime::update(now_ms)
-> check enabled task slots
-> run due task callback(context)
-> update last_run_ms
-> schedule next_run_ms = now_ms + period_ms
-> process bounded EventBus items
```

This plan uses that structure without changing it.

## Current Validation Flow

The current `VerticalSliceValidation` flow directly coordinates the slice:

1. Create fixed objects.
2. Clear EventBus.
3. Begin Registry.
4. Attach EventBus to Registry.
5. Begin Runtime.
6. Attach EventBus and Registry to Runtime.
7. Begin HAL time.
8. Begin simulated temperature Driver.
9. Begin simulated temperature Device.
10. Discover simulated Module through PNP.
11. Register discovered Module through PNP Registration.
12. Begin Temperature Service.
13. Begin Temperature Logic.
14. Begin internal API.
15. Call `temperature_service.update(now_ms)` directly.
16. Call `temperature_logic.evaluate()` directly.
17. Query API directly.
18. Validate Registry, Logic, and API state.

Current limitation:

```text
Runtime exists, but the validation harness directly calls Service and Logic.
```

This is valid for Phase 14 validation, but the next Runtime step should move Service and Logic execution into Runtime-owned task slots.

## Target Runtime-Owned Flow

Target flow:

1. Bootstrap still creates and wires fixed objects.
2. PNP still discovers and registers the simulated module before normal Runtime task execution.
3. Temperature Service is initialized with Registry and Device.
4. Temperature Logic is initialized with Registry.
5. API is initialized with Registry and Runtime.
6. Runtime registers a Temperature Service task.
7. Runtime registers a Temperature Logic task.
8. Optional validation/API query remains outside Runtime for now.
9. Main loop or validation runner calls `runtime.update(now_ms)`.
10. Runtime calls due task callbacks.
11. Service task callback calls `TemperatureService::update(now_ms)`.
12. Logic task callback calls `TemperatureLogic::evaluate()`.
13. Runtime processes bounded EventBus entries.
14. API queries Registry state after Runtime update.

Runtime still does not know the business meaning of temperature.

Runtime only knows:

```text
task_id
period_ms
callback
context
enabled
timing
```

## Task Contexts

Because `RuntimeTask` supports only `void (*callback)(void* context)`, each task should receive a compact static context struct.

### Temperature Service Task Context

Required fields:

```text
TemperatureService* service
uint32_t last_now_ms
bool last_result
const char* last_error
```

Callback behavior:

```text
cast context
if context or service is null: record failure in context and return
call service->update(now_ms-equivalent)
store result in context
```

Important note:

The current `RuntimeTask` callback signature does not pass `now_ms`.

Therefore the first implementation must choose one of these options:

1. Store the current Runtime time in the task context before invoking callback.
2. Add a small Runtime-owned time context object shared by all task contexts.
3. Later revise `RuntimeTask` callback signature only after architecture review.

Recommended first step:

```text
Use context-stored now_ms supplied by the validation runner or Runtime coordination wrapper.
```

This avoids changing the existing `RuntimeTask` structure.

### Temperature Logic Task Context

Required fields:

```text
TemperatureLogic* logic
bool last_result
const char* last_error
```

Callback behavior:

```text
cast context
if context or logic is null: record failure in context and return
call logic->evaluate()
store result in context
```

Logic remains capability-first.

Logic queries:

```text
CAP_TEMPERATURE
```

Logic must not receive:

```text
module_id
device_id
driver pointer
HAL pointer
```

## Task Registration Sequence

Task registration happens after PNP registration and after Service/Logic initialization.

Step-by-step:

1. Begin Runtime.
2. Attach EventBus to Runtime.
3. Attach Registry to Runtime.
4. Complete PNP discovery.
5. Complete PNP registration into Registry.
6. Begin Temperature Service.
7. Begin Temperature Logic.
8. Prepare Temperature Service task context.
9. Prepare Temperature Logic task context.
10. Create `RuntimeTask` for Temperature Service.
11. Create `RuntimeTask` for Temperature Logic.
12. Register Temperature Service task with Runtime.
13. Register Temperature Logic task with Runtime.
14. Set Runtime state to `READY` or `RUNNING` after required task registration succeeds.

If any required task registration fails:

```text
Runtime must not enter RUNNING.
Validation must fail with a compact error.
```

## Task Definitions

### Temperature Service Update Task

Task ID:

```text
task.temperature_service.update
```

Purpose:

```text
Update CAP_TEMPERATURE payload through TemperatureService.
```

Period:

```text
1000 ms
```

Initial scheduling:

```text
next_run_ms = 0
```

This allows the first `runtime.update(now_ms)` call to run the Service task immediately.

Callback:

```text
temperature_service_task_callback
```

Context:

```text
TemperatureServiceTaskContext
```

Allowed behavior:

```text
TemperatureService::update(now_ms)
```

Not allowed:

```text
Registry direct table writes
Logic evaluation
API calls
Device calls outside the Service
Driver calls outside the Device
```

### Temperature Logic Evaluation Task

Task ID:

```text
task.temperature_logic.evaluate
```

Purpose:

```text
Evaluate capability-first temperature logic.
```

Period:

```text
1000 ms
```

Initial scheduling:

```text
next_run_ms = 1
```

This makes the Logic task run after the first Service task when both are scheduled during the same startup window.

Callback:

```text
temperature_logic_task_callback
```

Context:

```text
TemperatureLogicTaskContext
```

Allowed behavior:

```text
TemperatureLogic::evaluate()
```

Not allowed:

```text
module name access
device name access
Driver calls
Device calls
HAL calls
API calls
Registry writes
```

## Task Execution Order

For the first vertical slice, task registration order determines execution order when tasks are due at the same time.

Required order:

1. Temperature Service Update
2. Temperature Logic Evaluation

Reason:

```text
Service updates Registry payload first.
Logic evaluates the latest Registry payload second.
```

If both tasks are due during one `Runtime::update(now_ms)` call, Runtime should run them in stored slot order.

Recommended registration order:

```text
register temperature service task first
register temperature logic task second
```

## Update Periods

Recommended first-slice periods:

| Task | Period | Reason |
|---|---:|---|
| Temperature Service Update | 1000 ms | Simple deterministic first slice update |
| Temperature Logic Evaluation | 1000 ms | Evaluate after each expected temperature update |
| Runtime Event Processing | every Runtime update | Keep queue bounded |
| API Query | external/on demand | API reads current Registry state only |

The service period may later follow capability freshness rules from `CAPABILITY_PAYLOAD_SCHEMA.md`.

For `CAP_TEMPERATURE`, the documented freshness window is:

```text
5000 ms
```

The 1000 ms service period keeps the payload comfortably fresh.

## Runtime Update Loop

Target first-slice loop:

```text
now_ms = hal_time.uptimeMs()
update shared task time context with now_ms
runtime.update(now_ms)
api.getTemperatureState(...)
api.getSystemStatus(...)
```

Inside `runtime.update(now_ms)`:

```text
for each enabled task:
  if task.next_run_ms <= now_ms:
    run task.callback(task.context)
    task.last_run_ms = now_ms
    task.next_run_ms = now_ms + task.period_ms

pop up to 4 EventBus events
discard or handle according to current minimal Runtime behavior
```

Current Runtime processes events after task execution.

This is acceptable for the first slice because:

- Service writes current state into Registry.
- Logic reads current state from Registry.
- API reads current state from Registry.
- Events announce changes but do not carry source-of-truth state.

## Event Processing Interaction

Expected events:

```text
EVT_MODULE_DISCOVERED
EVT_CAPABILITY_REGISTERED
EVT_CAPABILITY_VALUE_UPDATED
```

Event producers:

- PNP emits `EVT_MODULE_DISCOVERED`.
- Registry emits `EVT_CAPABILITY_REGISTERED`.
- Registry emits `EVT_CAPABILITY_VALUE_UPDATED`.

Runtime interaction:

1. Runtime owns bounded event processing.
2. Runtime pops up to `4` events per update.
3. Runtime must not store event history.
4. Runtime must not treat EventBus as Registry.
5. Consumers needing state must query Registry.

Current first-slice behavior:

```text
Runtime pops and discards events.
```

Future behavior:

```text
Runtime dispatches events to bounded consumers.
```

This future dispatch must not allow Logic to bind to module names. Logic may react to capability events only.

## Registry Interaction

Runtime may hold a Registry pointer for coordination.

Runtime may:

- attach Registry
- pass Registry-owned context indirectly to tasks during setup
- allow Services and Logic to use Registry through their own contracts

Runtime must not:

- store duplicate Registry facts
- update capability payloads directly
- register modules directly outside PNP registration
- evaluate capability values
- expose API state

Service Registry interaction:

```text
TemperatureService
-> Registry::updateCapabilityPayload(CAP_TEMPERATURE, payload)
```

Logic Registry interaction:

```text
TemperatureLogic
-> Registry::getCapabilityPayload(CAP_TEMPERATURE, payload)
```

API Registry interaction:

```text
Cyber32Api
-> Registry::moduleCount()
-> Registry::deviceCount()
-> Registry::capabilityCount()
-> Registry::getCapabilityPayload(CAP_TEMPERATURE, payload)
```

## Failure Behavior

### Task Registration Failure

If `Runtime::registerTask()` returns false:

```text
validation fails
Runtime must not be considered RUNNING
last_error = runtime_task_registration_failed
```

Possible causes:

- task table full
- invalid setup order
- duplicate registration policy added later

### Service Task Failure

If `TemperatureService::update(now_ms)` returns false:

Possible causes:

- Device read failed
- Registry update failed
- Service was not initialized

Required behavior:

1. Service still attempts to update Registry with unavailable/error payload when Device read fails.
2. Task context stores `last_result = false`.
3. Validation may fail for the happy-path test.
4. Registry should expose unavailable/error payload if update succeeded.
5. Logic must not treat unavailable as zero.

Future Runtime behavior:

```text
raise or retain compact service failure state
emit EVT_SERVICE_ERROR or EVT_ERROR_RAISED when implemented
```

### Logic Task Failure

If `TemperatureLogic::evaluate()` returns false:

Possible causes:

- Registry query failed
- capability unavailable
- payload datatype mismatch

Required behavior:

1. Logic status becomes `TEMPERATURE_UNAVAILABLE`.
2. Task context stores `last_result = false`.
3. Runtime does not reinterpret the Logic result.
4. API continues to expose Registry state.

### Event Queue Overflow

Current EventBus behavior:

```text
publish fails when queue is full
dropped counter increments
```

Current Runtime behavior:

```text
pop up to 4 events per update
```

First-slice failure handling:

- validation should inspect dropped count later when a public check exists
- no dynamic recovery is required for the first slice

Future behavior:

- preserve critical events
- coalesce repeated low-priority updates
- raise `EVT_RUNTIME_OVERLOAD` or `EVT_ERROR_RAISED`

## API Query Timing

API should be queried after at least one Runtime update that runs the Service task.

Minimum happy path:

```text
runtime.update(now_ms)
api.getTemperatureState(state)
api.getSystemStatus(status)
```

Expected API result:

```text
state.ok = true
state.payload.capability_id = CAP_TEMPERATURE
state.payload.available = AVAILABLE
state.payload.value_type = FLOAT
state.payload.value_float = 22.4
```

API must not trigger Service update.

API must not trigger Logic evaluation.

API reads current Registry and Runtime state only.

## Migration Plan From Current Validation Harness

### Step 1 - Keep Existing Validation Harness

Do not remove current direct validation yet.

It remains useful for checking:

- object wiring
- PNP discovery
- PNP registration
- Registry counts
- direct Service update
- direct Logic evaluation
- API query

### Step 2 - Add Task Contexts

Add fixed task context structs for:

- Temperature Service task
- Temperature Logic task

No dynamic allocation.

No STL containers.

No Arduino `String`.

### Step 3 - Add Static Task Callback Functions

Add callbacks compatible with:

```text
void (*callback)(void* context)
```

Callbacks should only delegate:

```text
service context -> TemperatureService::update(now_ms)
logic context -> TemperatureLogic::evaluate()
```

Callbacks must not contain policy beyond null checks and compact result storage.

### Step 4 - Register Tasks After Service And Logic Begin

In validation setup:

1. Begin Service.
2. Begin Logic.
3. Fill task contexts.
4. Create `RuntimeTask` records.
5. Register tasks with Runtime.

### Step 5 - Replace Direct Service/Logic Calls In Runtime Validation Path

Create a second validation mode or update `runOnce()` to use:

```text
runtime.update(now_ms)
```

instead of:

```text
temperature_service.update(now_ms)
temperature_logic.evaluate()
```

Keep the old direct checks only until the Runtime-owned path passes.

### Step 6 - Validate Results Through Registry, Logic, And API

After `runtime.update(now_ms)`:

1. Check Registry module count.
2. Check Registry device count.
3. Check Registry capability count.
4. Check `CAP_TEMPERATURE` payload.
5. Check Logic status.
6. Check API temperature state.

Do not validate by reading Driver or Device directly.

### Step 7 - Document Any Remaining First-Slice Shortcuts

Expected first-slice shortcuts:

- Runtime does not yet dispatch events to consumers.
- Runtime does not yet enforce task time budgets.
- Runtime does not yet raise Runtime errors.
- Service still receives a concrete simulated Device pointer.
- PNP registration still uses single-slice indexes.

These must remain documented until removed.

## Future Multi-Service Scalability

The Runtime task model should scale by fixed slots.

Future services may register tasks such as:

```text
task.power_service.update
task.storage_service.update
task.telemetry_service.update
task.network_service.update
```

Rules:

1. Each Service owns its policy.
2. Runtime owns only scheduling.
3. Each task uses a fixed context.
4. Each task has a bounded period.
5. Each task returns quickly.
6. Service failures are stored as compact result state.
7. Registry remains source of truth for exposed state.
8. Logic consumes capabilities, not service names or module names.

Recommended future task ordering:

1. Safety-critical Services
2. Required sensor/input Services
3. Logic evaluation
4. API servicing
5. Telemetry or low-priority Services
6. Event processing within budget

For the first vertical slice, only these tasks are needed:

```text
task.temperature_service.update
task.temperature_logic.evaluate
```

## Stop Conditions

Stop and review architecture if any implementation step causes:

- Runtime to call Driver directly
- Runtime to call Device directly outside a Service-owned callback
- Runtime to evaluate `CAP_TEMPERATURE` values itself
- Logic to receive module ID
- Logic to receive device ID
- API to trigger Device reads
- API to call Service update directly
- Registry to execute task callbacks
- EventBus to store current state
- dynamic allocation in Runtime task registration
- unbounded task creation

## Success Criteria

Runtime task coordination is successful when:

1. Temperature Service update runs from a Runtime task.
2. Temperature Logic evaluation runs from a Runtime task.
3. Runtime does not know temperature policy.
4. Runtime does not evaluate capability values.
5. Service updates Registry through public API.
6. Logic reads `CAP_TEMPERATURE` through Registry.
7. API reads the updated Registry payload.
8. EventBus remains compact transport only.
9. No dynamic allocation is introduced.
10. The first vertical slice still passes with one simulated `CAP_TEMPERATURE` provider.
