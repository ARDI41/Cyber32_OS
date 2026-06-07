#pragma once

#include <stdint.h>

namespace Cyber32 {

class HalTime {
public:
    void begin();
    uint32_t uptimeMs() const;
};

}  // namespace Cyber32
