#pragma once

#include <stdint.h>

namespace Cyber32 {

enum class RegistryResult : unsigned char {
    OK,
    NOT_ATTACHED,
    INVALID_RECORD,
    INVALID_ID,
    UNSUPPORTED_CAPABILITY,
    DUPLICATE_ID,
    TABLE_FULL,
    NOT_FOUND,
    UNAVAILABLE,
    STALE,
    TYPE_MISMATCH,
    INTERNAL_ERROR
};

struct RegistryWriteResult {
    RegistryResult result;
    uint8_t index;
};

}  // namespace Cyber32
