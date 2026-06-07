#pragma once

#include <stdint.h>

namespace Cyber32 {

struct RuntimeTask {
    const char* task_id;
    bool enabled;
    uint32_t period_ms;
    uint32_t next_run_ms;
    uint32_t last_run_ms;
    void (*callback)(void* context);
    void* context;
};

}  // namespace Cyber32
