#include "sim_distance_driver.h"

namespace Cyber32 {

SimDistanceDriver::SimDistanceDriver()
    : initialized_(false),
      failure_mode_(false) {
}

void SimDistanceDriver::begin() {
    initialized_ = true;
}

bool SimDistanceDriver::readDistance(float& out_meters) {
    if (!initialized_ || failure_mode_) {
        return false;
    }

    out_meters = 1.25F;
    return true;
}

void SimDistanceDriver::setFailureMode(bool enabled) {
    failure_mode_ = enabled;
}

}  // namespace Cyber32
