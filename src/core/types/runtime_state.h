#pragma once

namespace Cyber32 {

enum class RuntimeState : unsigned char {
    BOOTING,
    INITIALIZING,
    DISCOVERING,
    REGISTERING,
    STARTING,
    READY,
    RUNNING,
    ERROR_STATE,
    SAFE_MODE
};

}  // namespace Cyber32
