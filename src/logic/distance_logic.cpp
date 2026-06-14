#include "distance_logic.h"

#include "../core/ids/capability_ids.h"
#include "logic_status.h"

namespace Cyber32 {

DistanceLogic::DistanceLogic()
    : registry_(0),
      status_(LogicStatus::IDLE),
      last_distance_(0.0F) {
}

bool DistanceLogic::begin(Registry* registry) {
    if (registry == 0) {
        return false;
    }

    registry_ = registry;
    return true;
}

bool DistanceLogic::evaluate() {
    if (registry_ == 0) {
        status_ = LogicStatus::DISTANCE_UNAVAILABLE;
        return false;
    }

    CapabilityPayload payload;
    if (!registry_->getCapabilityPayload(CAP_DISTANCE, payload)) {
        status_ = LogicStatus::DISTANCE_UNAVAILABLE;
        return false;
    }
    if (payload.available != Availability::AVAILABLE) {
        status_ = LogicStatus::DISTANCE_UNAVAILABLE;
        return false;
    }
    if (payload.value_type != PayloadValueType::FLOAT) {
        status_ = LogicStatus::DISTANCE_UNAVAILABLE;
        return false;
    }

    last_distance_ = payload.value_float;
    if (last_distance_ < 0.30F) {
        status_ = LogicStatus::DISTANCE_NEAR;
    } else {
        status_ = LogicStatus::DISTANCE_SEEN;
    }

    return true;
}

LogicStatus DistanceLogic::status() const {
    return status_;
}

float DistanceLogic::lastDistance() const {
    return last_distance_;
}

}  // namespace Cyber32
