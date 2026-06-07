#pragma once

namespace Cyber32 {

class SimTemperatureDriver {
public:
    SimTemperatureDriver();

    void begin();
    bool readTemperature(float& out_celsius);
    void setFailureMode(bool enabled);

private:
    bool initialized_;
    bool failure_mode_;
};

}  // namespace Cyber32
