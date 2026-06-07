#include "hal_time.h"

#if defined(ARDUINO)
#include <Arduino.h>
#endif

namespace Cyber32 {

void HalTime::begin() {
}

uint32_t HalTime::uptimeMs() const {
#if defined(ARDUINO)
    return static_cast<uint32_t>(millis());
#else
    return 0;
#endif
}

}  // namespace Cyber32
