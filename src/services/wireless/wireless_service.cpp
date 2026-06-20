#include "wireless_service.h"

#include "../../core/ids/capability_ids.h"

namespace Cyber32 {

const char* const WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID =
    "provider-wireless-temperature-001";

WirelessService::WirelessService()
    : registry_(0),
      transport_(0),
      transport_adapter_(),
      temperature_device_(0),
      last_process_result_(false),
      last_error_code_("not_started") {
}

void WirelessService::begin() {
    registry_ = 0;
    transport_ = 0;
    transport_adapter_.context = 0;
    transport_adapter_.has_received_packet = 0;
    transport_adapter_.read_received_packet = 0;
    transport_adapter_.clear_received_packet = 0;
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

bool WirelessService::attachTransportAdapter(const WirelessPacketTransportAdapter& adapter) {
    if (!wirelessPacketTransportAdapterValid(adapter)) {
        return false;
    }

    transport_adapter_ = adapter;
    return true;
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

    WirelessPacketHeader header;
    WirelessCapabilityValue value;
    WirelessNodeDiagnostics diagnostics;
    uint8_t source_mac[WIRELESS_MAC_ADDRESS_SIZE];
    bool has_source_mac = false;

    if (wirelessPacketTransportAdapterValid(transport_adapter_)) {
        if (!transport_adapter_.has_received_packet(transport_adapter_.context)) {
            last_error_code_ = "no_packet";
            return false;
        }
        if (!transport_adapter_.read_received_packet(
                transport_adapter_.context,
                header,
                value,
                diagnostics,
                source_mac,
                has_source_mac)) {
            last_error_code_ = "packet_read_failed";
            return false;
        }
    } else {
        if (!transport_->initialized()) {
            last_error_code_ = "transport_not_initialized";
            return false;
        }
        if (!transport_->hasReceivedPacket()) {
            last_error_code_ = "no_packet";
            return false;
        }
        if (!transport_->readReceivedPacket(header, value, diagnostics)) {
            last_error_code_ = "packet_read_failed";
            return false;
        }
    }

    if (!wirelessPacketChecksumValid(header, value, diagnostics)) {
        last_error_code_ = "wireless_checksum_invalid";
        return false;
    }

    WirelessNodeAllowlistRecord allowlist_record;
    if (has_source_mac) {
        const RegistryResult mac_result =
            registry_->getWirelessNodeAllowlistRecordByMac(source_mac, allowlist_record);
        if (mac_result != RegistryResult::OK) {
            last_error_code_ = "wireless_mac_not_allowed";
            return false;
        }
        if (allowlist_record.allow_state == WirelessNodeAllowState::BLOCKED) {
            last_error_code_ = "wireless_node_blocked";
            return false;
        }
        if (allowlist_record.node_id != header.node_id) {
            last_error_code_ = "wireless_mac_node_mismatch";
            return false;
        }
        if (allowlist_record.allow_state != WirelessNodeAllowState::ALLOWED) {
            last_error_code_ = "wireless_mac_not_allowed";
            return false;
        }
    }

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
           (transport_ != 0 || wirelessPacketTransportAdapterValid(transport_adapter_)) &&
           temperature_device_ != 0;
}

}  // namespace Cyber32
