#include "wireless_temperature_device.h"

#include "../../core/ids/capability_ids.h"
#include "../../core/ids/error_ids.h"

namespace Cyber32 {

const char* const WirelessTemperatureDevice::DEVICE_ID = "device-wireless-temperature-001";
const char* const WirelessTemperatureDevice::DEVICE_TYPE = "wireless_sensor";

WirelessTemperatureDevice::WirelessTemperatureDevice()
    : initialized_(false),
      latest_payload_(),
      node_record_() {
    fillUnavailablePayload(0, ERR_CAPABILITY_UNAVAILABLE);
}

bool WirelessTemperatureDevice::begin(uint32_t node_id) {
    node_record_.node_id = node_id;
    node_record_.status = WirelessNodeStatus::UNKNOWN;
    node_record_.trust_state = WirelessTrustState::TRUSTED;
    node_record_.last_seen_ms = 0;
    node_record_.last_sequence_id = 0;
    node_record_.missed_heartbeat_count = 0;
    node_record_.battery_present = false;
    node_record_.battery_level_percent = 0.0F;
    node_record_.battery_voltage = 0.0F;
    node_record_.signal_quality_percent = 0;

    initialized_ = true;
    fillUnavailablePayload(0, ERR_CAPABILITY_UNAVAILABLE);
    return true;
}

bool WirelessTemperatureDevice::updateFromPacket(
    uint32_t now_ms,
    const WirelessPacketHeader& header,
    const WirelessCapabilityValue& value,
    const WirelessNodeDiagnostics& diagnostics) {
    if (!initialized_ || !isValidTemperaturePacket(header, value)) {
        fillUnavailablePayload(now_ms, ERR_CAPABILITY_UNAVAILABLE);
        node_record_.status = WirelessNodeStatus::UNAVAILABLE;
        return false;
    }

    node_record_.last_seen_ms = now_ms;
    node_record_.last_sequence_id = header.sequence_id;
    node_record_.status = WirelessNodeStatus::AVAILABLE;
    node_record_.battery_present = diagnostics.battery_present;
    node_record_.battery_level_percent = diagnostics.battery_level_percent;
    node_record_.battery_voltage = diagnostics.battery_voltage;
    node_record_.signal_quality_percent =
        signalQualityToUint8(diagnostics.signal_quality_percent);

    latest_payload_.capability_id = CAP_TEMPERATURE;
    latest_payload_.schema_version = 1;
    latest_payload_.timestamp_ms = now_ms;
    latest_payload_.available = Availability::AVAILABLE;
    latest_payload_.stale = StaleState::FRESH;
    latest_payload_.value_type = PayloadValueType::FLOAT;
    latest_payload_.value_float = value.value_float;
    latest_payload_.value_int = 0;
    latest_payload_.unit = "degree_celsius";
    latest_payload_.quality = "valid";
    latest_payload_.error_code = "none";
    return true;
}

bool WirelessTemperatureDevice::readPayload(CapabilityPayload& out_payload) const {
    out_payload = latest_payload_;
    return latest_payload_.available == Availability::AVAILABLE;
}

bool WirelessTemperatureDevice::nodeRecord(WirelessNodeRecord& out_record) const {
    if (!initialized_) {
        return false;
    }

    out_record = node_record_;
    return true;
}

const char* WirelessTemperatureDevice::id() const {
    return DEVICE_ID;
}

const char* WirelessTemperatureDevice::type() const {
    return DEVICE_TYPE;
}

void WirelessTemperatureDevice::fillUnavailablePayload(
    uint32_t now_ms,
    const char* error_code) {
    latest_payload_.capability_id = CAP_TEMPERATURE;
    latest_payload_.schema_version = 1;
    latest_payload_.timestamp_ms = now_ms;
    latest_payload_.available = Availability::UNAVAILABLE;
    latest_payload_.stale = StaleState::FRESH;
    latest_payload_.value_type = PayloadValueType::NONE;
    latest_payload_.value_float = 0.0F;
    latest_payload_.value_int = 0;
    latest_payload_.unit = "degree_celsius";
    latest_payload_.quality = "unavailable";
    latest_payload_.error_code = error_code;
}

bool WirelessTemperatureDevice::isValidTemperaturePacket(
    const WirelessPacketHeader& header,
    const WirelessCapabilityValue& value) const {
    if (header.magic != WIRELESS_PACKET_MAGIC) {
        return false;
    }
    if (header.protocol_version != WIRELESS_PROTOCOL_VERSION) {
        return false;
    }
    if (header.packet_type != WirelessPacketType::CAPABILITY_VALUE) {
        return false;
    }
    if (!isSameText(value.capability_id, CAP_TEMPERATURE)) {
        return false;
    }
    return value.payload_type == WirelessPayloadType::FLOAT;
}

bool WirelessTemperatureDevice::isSameText(const char* left, const char* right) const {
    if (left == 0 || right == 0) {
        return false;
    }

    for (uint8_t index = 0; index < WIRELESS_CAPABILITY_ID_SIZE; ++index) {
        if (right[index] == '\0') {
            return left[index] == '\0';
        }
        if (left[index] == '\0' || left[index] != right[index]) {
            return false;
        }
    }

    return right[WIRELESS_CAPABILITY_ID_SIZE - 1] == '\0';
}

uint8_t WirelessTemperatureDevice::signalQualityToUint8(float signal_quality_percent) const {
    if (signal_quality_percent <= 0.0F) {
        return 0;
    }
    if (signal_quality_percent >= 100.0F) {
        return 100;
    }
    return static_cast<uint8_t>(signal_quality_percent);
}

}  // namespace Cyber32
