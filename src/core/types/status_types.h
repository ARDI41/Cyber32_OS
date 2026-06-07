#pragma once

namespace Cyber32 {

enum class Availability : unsigned char {
    UNAVAILABLE,
    AVAILABLE
};

enum class StaleState : unsigned char {
    FRESH,
    STALE
};

enum class RecordStatus : unsigned char {
    EMPTY,
    RESERVED,
    REGISTERED,
    AVAILABLE,
    UNAVAILABLE,
    REMOVED,
    DISABLED,
    ERROR
};

}  // namespace Cyber32
