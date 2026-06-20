#include "first_wireless_temperature_base_test.h"

#include "../../core/ids/capability_ids.h"
#include "../../core/types/wireless_node_security_diagnostics.h"
#include "../../services/wireless/wireless_packet_transport.h"

namespace Cyber32 {

static const uint32_t FIRST_WIRELESS_TEMPERATURE_NODE_ID = 1001;

static void fillFirstWirelessTemperatureSenderMac(
    uint8_t mac_address[WIRELESS_MAC_ADDRESS_SIZE]) {
    mac_address[0] = 0x00;
    mac_address[1] = 0x00;
    mac_address[2] = 0x00;
    mac_address[3] = 0x00;
    mac_address[4] = 0x00;
    mac_address[5] = 0x00;
}

FirstWirelessTemperatureBaseTest::FirstWirelessTemperatureBaseTest()
    : registry_(),
      espnow_transport_(),
      wireless_temperature_device_(),
      wireless_service_(),
      api_(),
      initialized_(false) {
}

bool FirstWirelessTemperatureBaseTest::begin() {
    initialized_ = false;

    registry_.begin();

    const bool transport_started = espnow_transport_.begin();
    if (!transport_started) {
        return false;
    }

    if (!wireless_temperature_device_.begin(FIRST_WIRELESS_TEMPERATURE_NODE_ID)) {
        return false;
    }

    wireless_service_.begin();
    wireless_service_.attachRegistry(&registry_);
    wireless_service_.attachWirelessTemperatureDevice(&wireless_temperature_device_);

    const WirelessPacketTransportAdapter adapter =
        makeEspNowTransportAdapter(&espnow_transport_);
    if (!wireless_service_.attachTransportAdapter(adapter)) {
        return false;
    }

    if (!registerAllowlistRecord()) {
        return false;
    }
    if (!registerWirelessProvider()) {
        return false;
    }
    if (!registerSecurityDiagnostic()) {
        return false;
    }

    initialized_ = true;
    return true;
}

bool FirstWirelessTemperatureBaseTest::run(uint32_t now_ms) {
    if (!initialized_) {
        return false;
    }

    if (espnow_transport_.hasRawPayload()) {
        espnow_transport_.decodePendingRawPayload();
    }

    wireless_service_.processPackets(now_ms);
    return true;
}

bool FirstWirelessTemperatureBaseTest::registerAllowlistRecord() {
    WirelessNodeAllowlistRecord record;
    record.node_id = FIRST_WIRELESS_TEMPERATURE_NODE_ID;
    record.allow_state = WirelessNodeAllowState::ALLOWED;
    record.trust_state = WirelessTrustState::TRUSTED;
    record.added_at_ms = 0;
    record.last_seen_ms = 0;
    record.has_mac_address = true;
    fillFirstWirelessTemperatureSenderMac(record.mac_address);

    return registry_.registerWirelessNodeAllowlistRecordWithResult(record).result ==
           RegistryResult::OK;
}

bool FirstWirelessTemperatureBaseTest::registerWirelessProvider() {
    CapabilityPayload payload;
    payload.capability_id = CAP_TEMPERATURE;
    payload.schema_version = 1;
    payload.timestamp_ms = 0;
    payload.available = Availability::AVAILABLE;
    payload.stale = StaleState::STALE;
    payload.value_type = PayloadValueType::NONE;
    payload.value_float = 0.0F;
    payload.value_int = 0;
    payload.unit = "degree_celsius";
    payload.quality = "stale";
    payload.error_code = "none";

    CapabilityProviderRecord provider;
    provider.provider_id = WirelessService::WIRELESS_TEMPERATURE_PROVIDER_ID;
    provider.capability_id = CAP_TEMPERATURE;
    provider.owner_module_index = 0;
    provider.owner_device_index = 0;
    provider.provider_type = CapabilityProviderType::WIRELESS;
    provider.status = CapabilityProviderStatus::STALE;
    provider.priority = 20;
    provider.last_update_ms = 0;
    provider.latest_payload = payload;

    return registry_.registerCapabilityProviderWithResult(provider).result ==
           RegistryResult::OK;
}

bool FirstWirelessTemperatureBaseTest::registerSecurityDiagnostic() {
    WirelessNodeSecurityDiagnosticRecord record;
    record.node_id = FIRST_WIRELESS_TEMPERATURE_NODE_ID;
    record.has_mac_address = true;
    fillFirstWirelessTemperatureSenderMac(record.mac_address);
    record.allow_state = WirelessNodeAllowState::ALLOWED;
    record.trust_state = WirelessTrustState::TRUSTED;
    record.last_seen_ms = 0;
    record.last_accepted_sequence_id = 0;
    record.last_rejected_sequence_id = 0;
    record.last_error_code = "none";
    record.checksum_reject_count = 0;
    record.mac_not_allowed_reject_count = 0;
    record.mac_node_mismatch_reject_count = 0;
    record.blocked_reject_count = 0;
    record.not_allowed_reject_count = 0;
    record.untrusted_reject_count = 0;
    record.duplicate_sequence_reject_count = 0;
    record.invalid_packet_reject_count = 0;
    record.accepted_packet_count = 0;

    return registry_.registerWirelessNodeSecurityDiagnosticWithResult(record).result ==
           RegistryResult::OK;
}

}  // namespace Cyber32
