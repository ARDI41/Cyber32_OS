#pragma once

namespace Cyber32 {

class SimDistanceDriver {
public:
    SimDistanceDriver();

    void begin();
    bool readDistance(float& out_meters);
    void setFailureMode(bool enabled);

private:
    bool initialized_;
    bool failure_mode_;
};

}  // namespace Cyber32
