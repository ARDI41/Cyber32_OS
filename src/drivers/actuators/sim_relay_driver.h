#pragma once

namespace Cyber32 {

class SimRelayDriver {
public:
    SimRelayDriver();

    void begin();
    bool setEnabled(bool enabled);
    bool disable();
    bool enabled() const;
    void setFailureMode(bool enabled);
    bool initialized() const;

private:
    bool initialized_;
    bool failure_mode_;
    bool enabled_;
};

}  // namespace Cyber32
