#include "temperature_logic.h"

#include "../core/ids/capability_ids.h"
#include "logic_status.h"

namespace Cyber32 {

TemperatureLogic::TemperatureLogic()
    : registry_(0),
      status_(LogicStatus::IDLE),
      last_temperature_(0.0F) {
}

bool TemperatureLogic::begin(Registry* registry) {
    if (registry == 0) {
        return false;
    }

    registry_ = registry;
    return true;
}

bool TemperatureLogic::evaluate() {
    if (registry_ == 0) {
        status_ = LogicStatus::TEMPERATURE_UNAVAILABLE;
        return false;
    }

    CapabilityPayload payload;
    if (!registry_->getCapabilityPayload(CAP_TEMPERATURE, payload)) {
        status_ = LogicStatus::TEMPERATURE_UNAVAILABLE;
        return false;
    }
    if (payload.available != Availability::AVAILABLE) {
        status_ = LogicStatus::TEMPERATURE_UNAVAILABLE;
        return false;
    }
    if (payload.value_type != PayloadValueType::FLOAT) {
        status_ = LogicStatus::TEMPERATURE_UNAVAILABLE;
        return false;
    }

    last_temperature_ = payload.value_float;
    if (last_temperature_ > 40.0F) {
        status_ = LogicStatus::TEMPERATURE_HIGH;
    } else {
        status_ = LogicStatus::TEMPERATURE_SEEN;
    }

    return true;
}

LogicStatus TemperatureLogic::status() const {
    return status_;
}

float TemperatureLogic::lastTemperature() const {
    return last_temperature_;
}

}  // namespace Cyber32
