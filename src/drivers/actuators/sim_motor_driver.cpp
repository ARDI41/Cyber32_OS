#include "sim_motor_driver.h"

namespace Cyber32 {

SimMotorDriver::SimMotorDriver()
    : initialized_(false),
      failure_mode_(false),
      current_direction_(MotorDirection::STOP),
      current_speed_percent_(0.0F) {
}

void SimMotorDriver::begin() {
    initialized_ = true;
    current_direction_ = MotorDirection::STOP;
    current_speed_percent_ = 0.0F;
}

bool SimMotorDriver::setMotor(MotorDirection direction, float speed_percent) {
    if (!initialized_ || failure_mode_) {
        return false;
    }

    if (speed_percent < 0.0F || speed_percent > 100.0F) {
        return false;
    }

    if (direction == MotorDirection::STOP && speed_percent != 0.0F) {
        return false;
    }

    current_direction_ = direction;
    current_speed_percent_ = speed_percent;
    return true;
}

bool SimMotorDriver::stop() {
    if (!initialized_ || failure_mode_) {
        return false;
    }

    current_direction_ = MotorDirection::STOP;
    current_speed_percent_ = 0.0F;
    return true;
}

MotorDirection SimMotorDriver::currentDirection() const {
    return current_direction_;
}

float SimMotorDriver::currentSpeedPercent() const {
    return current_speed_percent_;
}

void SimMotorDriver::setFailureMode(bool enabled) {
    failure_mode_ = enabled;
}

bool SimMotorDriver::initialized() const {
    return initialized_;
}

}  // namespace Cyber32
