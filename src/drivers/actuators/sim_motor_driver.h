#pragma once

#include "../../core/types/motor_types.h"

namespace Cyber32 {

class SimMotorDriver {
public:
    SimMotorDriver();

    void begin();
    bool setMotor(MotorDirection direction, float speed_percent);
    bool stop();
    MotorDirection currentDirection() const;
    float currentSpeedPercent() const;
    void setFailureMode(bool enabled);
    bool initialized() const;

private:
    bool initialized_;
    bool failure_mode_;
    MotorDirection current_direction_;
    float current_speed_percent_;
};

}  // namespace Cyber32
