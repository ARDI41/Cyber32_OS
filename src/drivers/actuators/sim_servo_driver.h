#pragma once

namespace Cyber32 {

class SimServoDriver {
public:
    SimServoDriver();

    void begin();
    bool setPosition(float position_degrees);
    float currentPosition() const;
    void setFailureMode(bool enabled);
    bool initialized() const;

private:
    bool initialized_;
    bool failure_mode_;
    float current_position_degrees_;
};

}  // namespace Cyber32
