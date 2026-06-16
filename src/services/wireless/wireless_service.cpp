#include "wireless_service.h"

namespace Cyber32 {

const char* const WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID =
    "provider-wireless-temperature-001";

WirelessService::WirelessService()
    : registry_(0),
      transport_(0),
      temperature_device_(0),
      last_process_result_(false),
      last_error_code_("not_started") {
}

void WirelessService::begin() {
    registry_ = 0;
    transport_ = 0;
    temperature_device_ = 0;
    last_process_result_ = false;
    last_error_code_ = "none";
}

void WirelessService::attachRegistry(Registry* registry) {
    registry_ = registry;
}

void WirelessService::attachTransportDriver(SimEspNowTransportDriver* transport) {
    transport_ = transport;
}

void WirelessService::attachWirelessTemperatureDevice(WirelessTemperatureDevice* device) {
    temperature_device_ = device;
}

bool WirelessService::processPackets(uint32_t now_ms) {
    last_process_result_ = false;

    if (!hasRequiredAttachments()) {
        last_error_code_ = "not_attached";
        return false;
    }
    if (!transport_->initialized()) {
        last_error_code_ = "transport_not_initialized";
        return false;
    }
    if (!transport_->hasReceivedPacket()) {
        last_error_code_ = "no_packet";
        return false;
    }

    WirelessPacketHeader header;
    WirelessCapabilityValue value;
    WirelessNodeDiagnostics diagnostics;
    if (!transport_->readReceivedPacket(header, value, diagnostics)) {
        last_error_code_ = "packet_read_failed";
        return false;
    }

    if (!temperature_device_->updateFromPacket(now_ms, header, value, diagnostics)) {
        last_error_code_ = "device_update_failed";
        return false;
    }

    CapabilityPayload payload;
    if (!temperature_device_->readPayload(payload)) {
        last_error_code_ = "device_payload_unavailable";
        return false;
    }

    const RegistryResult provider_result = registry_->updateCapabilityProviderPayload(
        WIRELESS_TEMPERATURE_PROVIDER_ID,
        payload,
        CapabilityProviderStatus::AVAILABLE,
        now_ms);
    if (provider_result != RegistryResult::OK) {
        last_error_code_ = "provider_update_failed";
        return false;
    }

    last_process_result_ = true;
    last_error_code_ = "none";
    return true;
}

bool WirelessService::checkTimeouts(uint32_t now_ms) {
    (void)now_ms;
    return hasRequiredAttachments();
}

bool WirelessService::lastProcessResult() const {
    return last_process_result_;
}

const char* WirelessService::lastErrorCode() const {
    return last_error_code_;
}

bool WirelessService::hasRequiredAttachments() const {
    return registry_ != 0 &&
           transport_ != 0 &&
           temperature_device_ != 0;
}

}  // namespace Cyber32
