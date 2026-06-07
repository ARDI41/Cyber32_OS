#include "runtime.h"

namespace Cyber32 {

Runtime::Runtime()
    : state_(RuntimeState::BOOTING),
      task_count_(0),
      event_bus_(0),
      registry_(0) {
}

void Runtime::begin() {
    state_ = RuntimeState::BOOTING;
    task_count_ = 0;
}

void Runtime::setState(RuntimeState state) {
    state_ = state;
}

RuntimeState Runtime::state() const {
    return state_;
}

void Runtime::attachEventBus(EventBus* bus) {
    event_bus_ = bus;
}

void Runtime::attachRegistry(Registry* registry) {
    registry_ = registry;
}

bool Runtime::registerTask(const RuntimeTask& task) {
    if (task_count_ >= MAX_TASKS) {
        return false;
    }

    tasks_[task_count_] = task;
    ++task_count_;
    return true;
}

void Runtime::update(uint32_t now_ms) {
    for (uint8_t i = 0; i < task_count_; ++i) {
        RuntimeTask& task = tasks_[i];
        if (!task.enabled || task.callback == 0) {
            continue;
        }

        if (task.next_run_ms <= now_ms) {
            task.callback(task.context);
            task.last_run_ms = now_ms;
            task.next_run_ms = now_ms + task.period_ms;
        }
    }

    processEvents();
}

uint8_t Runtime::taskCount() const {
    return task_count_;
}

void Runtime::processEvents() {
    if (event_bus_ == 0) {
        return;
    }

    EventRecord event;
    for (uint8_t i = 0; i < EVENTS_PER_UPDATE; ++i) {
        if (!event_bus_->pop(event)) {
            return;
        }
    }
}

}  // namespace Cyber32
