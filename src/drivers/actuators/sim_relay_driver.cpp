#include "sim_relay_driver.h"

namespace Cyber32 {

SimRelayDriver::SimRelayDriver()
    : initialized_(false),
      failure_mode_(false),
      enabled_(false) {
}

void SimRelayDriver::begin() {
    initialized_ = true;
    enabled_ = false;
}

bool SimRelayDriver::setEnabled(bool enabled) {
    if (!initialized_ || failure_mode_) {
        return false;
    }

    enabled_ = enabled;
    return true;
}

bool SimRelayDriver::disable() {
    return setEnabled(false);
}

bool SimRelayDriver::enabled() const {
    return enabled_;
}

void SimRelayDriver::setFailureMode(bool enabled) {
    failure_mode_ = enabled;
}

bool SimRelayDriver::initialized() const {
    return initialized_;
}

}  // namespace Cyber32
