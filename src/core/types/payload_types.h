#pragma once

namespace Cyber32 {

enum class PayloadValueType : unsigned char {
    NONE,
    BOOLEAN,
    INTEGER,
    FLOAT,
    STRING,
    ENUM,
    OBJECT
};

}  // namespace Cyber32
