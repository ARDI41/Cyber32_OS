#include "wireless_service.h"

#include "../../core/ids/capability_ids.h"

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

    if (!wirelessPacketChecksumValid(header, value, diagnostics)) {
        last_error_code_ = "wireless_checksum_invalid";
        return false;
    }

    WirelessNodeAllowlistRecord allowlist_record;
    const RegistryResult allowlist_result =
        registry_->getWirelessNodeAllowlistRecord(header.node_id, allowlist_record);
    if (allowlist_result == RegistryResult::NOT_FOUND) {
        last_error_code_ = "wireless_node_not_allowed";
        return false;
    }
    if (allowlist_result != RegistryResult::OK) {
        last_error_code_ = "wireless_node_not_allowed";
        return false;
    }
    if (allowlist_record.allow_state == WirelessNodeAllowState::BLOCKED) {
        last_error_code_ = "wireless_node_blocked";
        return false;
    }
    if (allowlist_record.allow_state != WirelessNodeAllowState::ALLOWED) {
        last_error_code_ = "wireless_node_not_allowed";
        return false;
    }

    if (!wirelessTrustAllowsPayloadUpdate(temperature_device_->trustState())) {
        last_error_code_ = "wireless_untrusted";
        return false;
    }

    if (!temperature_device_->sequenceAccepted(header.sequence_id)) {
        last_error_code_ = "wireless_duplicate_sequence";
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

    temperature_device_->markSequenceAccepted(header.sequence_id);
    last_process_result_ = true;
    last_error_code_ = "none";
    return true;
}

bool WirelessService::checkTimeouts(uint32_t now_ms) {
    last_process_result_ = false;

    if (registry_ == 0) {
        last_error_code_ = "registry_not_attached";
        return false;
    }

    RegistryResult result = registry_->updateProviderHealth(now_ms);
    if (result != RegistryResult::OK) {
        last_error_code_ = "provider_health_failed";
        return false;
    }

    result = registry_->updateBestCapabilityPayload(CAP_TEMPERATURE);
    if (result == RegistryResult::OK) {
        last_process_result_ = true;
        last_error_code_ = "none";
        return true;
    }
    if (result == RegistryResult::NOT_FOUND) {
        last_error_code_ = "no_eligible_provider";
        return false;
    }

    last_error_code_ = "best_provider_update_failed";
    return false;
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
