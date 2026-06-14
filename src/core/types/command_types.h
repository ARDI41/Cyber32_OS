#pragma once

namespace Cyber32 {

enum class CommandState : unsigned char {
    REQUEST,
    ACCEPTED,
    EXECUTING,
    COMPLETED,
    FAILED,
    TIMED_OUT,
    CANCELLED
};

}  // namespace Cyber32
