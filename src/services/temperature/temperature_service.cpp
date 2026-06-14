#include "temperature_service.h"

#include "../../core/ids/capability_ids.h"
#include "../../core/ids/service_ids.h"

namespace Cyber32 {

TemperatureService::TemperatureService()
    : registry_(0),
      device_(0),
      last_registry_result_(RegistryResult::NOT_ATTACHED) {
}

bool TemperatureService::begin(Registry* registry, SimTemperatureDevice* device) {
    if (registry == 0 || device == 0) {
        return false;
    }

    registry_ = registry;
    device_ = device;
    return true;
}

bool TemperatureService::update(uint32_t now_ms) {
    if (registry_ == 0 || device_ == 0) {
        last_registry_result_ = RegistryResult::NOT_ATTACHED;
        return false;
    }

    CapabilityPayload payload;
    const bool read_success = device_->readPayload(now_ms, payload);
    last_registry_result_ = registry_->updateCapabilityPayloadWithResult(CAP_TEMPERATURE, payload);

    return read_success && last_registry_result_ == RegistryResult::OK;
}

const char* TemperatureService::id() const {
    return SERVICE_TEMPERATURE;
}

RegistryResult TemperatureService::lastRegistryResult() const {
    return last_registry_result_;
}

}  // namespace Cyber32
