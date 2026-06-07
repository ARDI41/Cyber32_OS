#include "sim_temperature_driver.h"

namespace Cyber32 {

SimTemperatureDriver::SimTemperatureDriver()
    : initialized_(false),
      failure_mode_(false) {
}

void SimTemperatureDriver::begin() {
    initialized_ = true;
}

bool SimTemperatureDriver::readTemperature(float& out_celsius) {
    if (!initialized_ || failure_mode_) {
        return false;
    }

    out_celsius = 22.4F;
    return true;
}

void SimTemperatureDriver::setFailureMode(bool enabled) {
    failure_mode_ = enabled;
}

}  // namespace Cyber32
