#pragma once

namespace Cyber32 {

enum class LogicStatus : unsigned char {
    IDLE,
    TEMPERATURE_SEEN,
    TEMPERATURE_HIGH,
    TEMPERATURE_UNAVAILABLE
};

}  // namespace Cyber32
