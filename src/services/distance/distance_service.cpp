#include "distance_service.h"

#include "../../core/ids/capability_ids.h"
#include "../../core/ids/service_ids.h"

namespace Cyber32 {

DistanceService::DistanceService()
    : registry_(0),
      device_(0) {
}

bool DistanceService::begin(Registry* registry, SimDistanceDevice* device) {
    if (registry == 0 || device == 0) {
        return false;
    }

    registry_ = registry;
    device_ = device;
    return true;
}

bool DistanceService::update(uint32_t now_ms) {
    if (registry_ == 0 || device_ == 0) {
        return false;
    }

    CapabilityPayload payload;
    const bool read_success = device_->readPayload(now_ms, payload);
    const bool registry_success = registry_->updateCapabilityPayload(CAP_DISTANCE, payload);

    return read_success && registry_success;
}

const char* DistanceService::id() const {
    return SERVICE_DISTANCE;
}

}  // namespace Cyber32
