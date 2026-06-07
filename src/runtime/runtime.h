#pragma once

#include <stdint.h>

#include "../core/event_bus/event_bus.h"
#include "../core/types/runtime_state.h"
#include "../registry/registry.h"
#include "runtime_task.h"

namespace Cyber32 {

class Runtime {
public:
    static constexpr uint8_t MAX_TASKS = 16;
    static constexpr uint8_t EVENTS_PER_UPDATE = 4;

    Runtime();

    void begin();
    void setState(RuntimeState state);
    RuntimeState state() const;
    void attachEventBus(EventBus* bus);
    void attachRegistry(Registry* registry);
    bool registerTask(const RuntimeTask& task);
    void update(uint32_t now_ms);
    uint8_t taskCount() const;

private:
    RuntimeState state_;
    RuntimeTask tasks_[MAX_TASKS];
    uint8_t task_count_;
    EventBus* event_bus_;
    Registry* registry_;

    void processEvents();
};

}  // namespace Cyber32
