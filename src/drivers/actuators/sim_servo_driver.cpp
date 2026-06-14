#include "sim_servo_driver.h"

namespace Cyber32 {

SimServoDriver::SimServoDriver()
    : initialized_(false),
      failure_mode_(false),
      current_position_degrees_(90.0F) {
}

void SimServoDriver::begin() {
    initialized_ = true;
    current_position_degrees_ = 90.0F;
}

bool SimServoDriver::setPosition(float position_degrees) {
    if (!initialized_ || failure_mode_) {
        return false;
    }

    if (position_degrees < 0.0F || position_degrees > 180.0F) {
        return false;
    }

    current_position_degrees_ = position_degrees;
    return true;
}

float SimServoDriver::currentPosition() const {
    return current_position_degrees_;
}

void SimServoDriver::setFailureMode(bool enabled) {
    failure_mode_ = enabled;
}

bool SimServoDriver::initialized() const {
    return initialized_;
}

}  // namespace Cyber32
