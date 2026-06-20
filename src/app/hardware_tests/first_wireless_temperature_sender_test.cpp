#include "first_wireless_temperature_sender_test.h"

#include <WiFi.h>
#include <esp_now.h>

#include "../../core/ids/capability_ids.h"

namespace Cyber32 {

static const uint32_t FIRST_WIRELESS_TEMPERATURE_SENDER_NODE_ID = 1001;

FirstWirelessTemperatureSenderTest::FirstWirelessTemperatureSenderTest()
    : base_mac_(),
      initialized_(false) {
}

bool FirstWirelessTemperatureSenderTest::begin() {
    initialized_ = false;
    fillKnownBaseMac();

    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK) {
        return false;
    }

    esp_now_peer_info_t peer_info = {};
    for (uint8_t i = 0; i < WIRELESS_MAC_ADDRESS_SIZE; ++i) {
        peer_info.peer_addr[i] = base_mac_[i];
    }
    peer_info.channel = 0;
    peer_info.encrypt = false;

    if (!esp_now_is_peer_exist(base_mac_)) {
        if (esp_now_add_peer(&peer_info) != ESP_OK) {
            return false;
        }
    }

    initialized_ = true;
    return true;
}

bool FirstWirelessTemperatureSenderTest::sendTemperature(
    float temperature,
    uint32_t sequence_id) {
    if (!initialized_) {
        return false;
    }

    uint8_t packet[WIRELESS_MAX_PACKET_SIZE];
    uint16_t packet_length = 0;
    if (!packTemperaturePacket(temperature, sequence_id, packet, packet_length)) {
        return false;
    }

    return esp_now_send(base_mac_, packet, packet_length) == ESP_OK;
}

void FirstWirelessTemperatureSenderTest::fillKnownBaseMac() {
    // Replace this placeholder with the measured base ESP32 MAC for hardware testing.
    base_mac_[0] = 0x00;
    base_mac_[1] = 0x00;
    base_mac_[2] = 0x00;
    base_mac_[3] = 0x00;
    base_mac_[4] = 0x00;
    base_mac_[5] = 0x00;
}

void FirstWirelessTemperatureSenderTest::copyBoundedText(
    char* destination,
    uint8_t destination_size,
    const char* source) const {
    if (destination == 0 || destination_size == 0) {
        return;
    }

    uint8_t index = 0;
    while (index < static_cast<uint8_t>(destination_size - 1U) &&
           source != 0 &&
           source[index] != '\0') {
        destination[index] = source[index];
        ++index;
    }

    destination[index] = '\0';
    ++index;
    while (index < destination_size) {
        destination[index] = '\0';
        ++index;
    }
}

bool FirstWirelessTemperatureSenderTest::packTemperaturePacket(
    float temperature,
    uint32_t sequence_id,
    uint8_t out_buffer[WIRELESS_MAX_PACKET_SIZE],
    uint16_t& out_length) const {
    enum {
        EXPECTED_PACKET_SIZE =
            sizeof(WirelessPacketHeader) +
            sizeof(WirelessCapabilityValue) +
            sizeof(WirelessNodeDiagnostics)
    };

    if (out_buffer == 0 || EXPECTED_PACKET_SIZE > WIRELESS_MAX_PACKET_SIZE) {
        out_length = 0;
        return false;
    }

    WirelessPacketHeader header;
    header.magic = WIRELESS_PACKET_MAGIC;
    header.protocol_version = WIRELESS_PROTOCOL_VERSION;
    header.packet_type = WirelessPacketType::CAPABILITY_VALUE;
    header.flags = 0;
    header.sequence_id = static_cast<uint16_t>(sequence_id);
    header.node_id = FIRST_WIRELESS_TEMPERATURE_SENDER_NODE_ID;
    header.payload_length = static_cast<uint8_t>(
        sizeof(WirelessCapabilityValue) + sizeof(WirelessNodeDiagnostics));
    header.checksum = 0;

    WirelessCapabilityValue value;
    copyBoundedText(value.capability_id, WIRELESS_CAPABILITY_ID_SIZE, CAP_TEMPERATURE);
    value.payload_type = WirelessPayloadType::FLOAT;
    value.value_float = temperature;
    value.value_int = 0;
    copyBoundedText(value.error_code, WIRELESS_ERROR_CODE_SIZE, "none");

    WirelessNodeDiagnostics diagnostics;
    diagnostics.battery_present = false;
    diagnostics.battery_level_percent = 0.0F;
    diagnostics.battery_voltage = 0.0F;
    diagnostics.signal_quality_percent = 0.0F;

    header.checksum = calculateWirelessPacketChecksum(header, value, diagnostics);

    uint16_t offset = 0;
    const uint8_t* header_bytes = reinterpret_cast<const uint8_t*>(&header);
    for (uint16_t i = 0; i < sizeof(WirelessPacketHeader); ++i) {
        out_buffer[offset] = header_bytes[i];
        ++offset;
    }

    const uint8_t* value_bytes = reinterpret_cast<const uint8_t*>(&value);
    for (uint16_t i = 0; i < sizeof(WirelessCapabilityValue); ++i) {
        out_buffer[offset] = value_bytes[i];
        ++offset;
    }

    const uint8_t* diagnostics_bytes = reinterpret_cast<const uint8_t*>(&diagnostics);
    for (uint16_t i = 0; i < sizeof(WirelessNodeDiagnostics); ++i) {
        out_buffer[offset] = diagnostics_bytes[i];
        ++offset;
    }

    out_length = offset;
    return true;
}

}  // namespace Cyber32
