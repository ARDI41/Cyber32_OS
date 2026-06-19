#include "espnow_transport_driver.h"

#include <WiFi.h>
#include <esp_now.h>

namespace Cyber32 {

static EspNowTransportDriver* active_espnow_transport_driver = 0;

EspNowTransportDriver::EspNowTransportDriver()
    : initialized_(false),
      pending_received_packet_(false),
      pending_header_(),
      pending_value_(),
      pending_diagnostics_(),
      pending_source_mac_(),
      pending_has_source_mac_(false),
      callback_received_(false),
      last_received_length_(0),
      last_source_mac_(),
      pending_raw_payload_(),
      pending_raw_length_(0),
      pending_raw_payload_available_(false) {
}

bool EspNowTransportDriver::begin() {
    clearReceivedPacket();
    clearCallbackState();
    initialized_ = false;
    active_espnow_transport_driver = 0;

    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK) {
        initialized_ = false;
        return false;
    }

    active_espnow_transport_driver = this;
    if (esp_now_register_recv_cb(&EspNowTransportDriver::receiveCallback) != ESP_OK) {
        active_espnow_transport_driver = 0;
        initialized_ = false;
        return false;
    }

    initialized_ = true;
    return true;
}

bool EspNowTransportDriver::initialized() const {
    return initialized_;
}

bool EspNowTransportDriver::hasReceivedPacket() const {
    return initialized_ && pending_received_packet_;
}

void EspNowTransportDriver::clearReceivedPacket() {
    pending_received_packet_ = false;
    pending_header_ = WirelessPacketHeader();
    pending_value_ = WirelessCapabilityValue();
    pending_diagnostics_ = WirelessNodeDiagnostics();
    clearWirelessMacAddress(pending_source_mac_);
    pending_has_source_mac_ = false;
    clearRawPayloadState();
}

bool EspNowTransportDriver::callbackReceived() const {
    return callback_received_;
}

uint16_t EspNowTransportDriver::lastReceivedLength() const {
    return last_received_length_;
}

bool EspNowTransportDriver::hasRawPayload() const {
    return pending_raw_payload_available_;
}

uint16_t EspNowTransportDriver::rawPayloadLength() const {
    return pending_raw_length_;
}

bool EspNowTransportDriver::readRawPayload(
    uint8_t out_buffer[WIRELESS_MAX_PACKET_SIZE],
    uint16_t& out_length,
    uint8_t out_source_mac[WIRELESS_MAC_ADDRESS_SIZE],
    bool& out_has_source_mac) {
    if (!pending_raw_payload_available_ || out_buffer == 0) {
        return false;
    }

    for (uint16_t i = 0; i < pending_raw_length_; ++i) {
        out_buffer[i] = pending_raw_payload_[i];
    }
    out_length = pending_raw_length_;

    out_has_source_mac = pending_has_source_mac_;
    if (out_source_mac != 0) {
        if (pending_has_source_mac_) {
            for (uint8_t i = 0; i < WIRELESS_MAC_ADDRESS_SIZE; ++i) {
                out_source_mac[i] = pending_source_mac_[i];
            }
        } else {
            clearWirelessMacAddress(out_source_mac);
        }
    }

    clearRawPayloadState();
    return true;
}

bool EspNowTransportDriver::injectRawPayloadForTest(
    const uint8_t source_mac[WIRELESS_MAC_ADDRESS_SIZE],
    const uint8_t* data,
    uint16_t length) {
    recordReceiveCallback(source_mac, data, static_cast<int>(length));
    return pending_raw_payload_available_;
}

void EspNowTransportDriver::receiveCallback(
    const uint8_t* source_mac,
    const uint8_t* data,
    int data_len) {
    if (active_espnow_transport_driver == 0) {
        return;
    }

    active_espnow_transport_driver->recordReceiveCallback(source_mac, data, data_len);
}

void EspNowTransportDriver::clearCallbackState() {
    callback_received_ = false;
    last_received_length_ = 0;
    clearWirelessMacAddress(last_source_mac_);
}

void EspNowTransportDriver::clearRawPayloadState() {
    pending_raw_length_ = 0;
    pending_raw_payload_available_ = false;
    clearWirelessMacAddress(pending_source_mac_);
    pending_has_source_mac_ = false;
    for (uint16_t i = 0; i < WIRELESS_MAX_PACKET_SIZE; ++i) {
        pending_raw_payload_[i] = 0;
    }
}

void EspNowTransportDriver::recordReceiveCallback(
    const uint8_t* source_mac,
    const uint8_t* data,
    int data_len) {
    callback_received_ = true;
    if (data_len <= 0) {
        last_received_length_ = 0;
    } else if (data_len > 65535) {
        last_received_length_ = 65535;
    } else {
        last_received_length_ = static_cast<uint16_t>(data_len);
    }

    if (source_mac == 0) {
        clearWirelessMacAddress(last_source_mac_);
    } else {
        for (uint8_t i = 0; i < WIRELESS_MAC_ADDRESS_SIZE; ++i) {
            last_source_mac_[i] = source_mac[i];
        }
    }

    if (data == 0 || data_len <= 0 || data_len > WIRELESS_MAX_PACKET_SIZE) {
        clearRawPayloadState();
        return;
    }

    pending_raw_length_ = static_cast<uint16_t>(data_len);
    for (uint16_t i = 0; i < pending_raw_length_; ++i) {
        pending_raw_payload_[i] = data[i];
    }
    pending_raw_payload_available_ = true;

    if (source_mac == 0) {
        clearWirelessMacAddress(pending_source_mac_);
        pending_has_source_mac_ = false;
        return;
    }

    for (uint8_t i = 0; i < WIRELESS_MAC_ADDRESS_SIZE; ++i) {
        pending_source_mac_[i] = source_mac[i];
    }
    pending_has_source_mac_ = true;
}

bool EspNowTransportDriver::readReceivedPacket(
    WirelessPacketHeader& out_header,
    WirelessCapabilityValue& out_value,
    WirelessNodeDiagnostics& out_diagnostics) {
    if (!hasReceivedPacket()) {
        return false;
    }

    out_header = pending_header_;
    out_value = pending_value_;
    out_diagnostics = pending_diagnostics_;
    clearReceivedPacket();
    return true;
}

bool EspNowTransportDriver::readReceivedPacket(
    WirelessPacketHeader& out_header,
    WirelessCapabilityValue& out_value,
    WirelessNodeDiagnostics& out_diagnostics,
    uint8_t out_source_mac[WIRELESS_MAC_ADDRESS_SIZE],
    bool& out_has_source_mac) {
    if (!hasReceivedPacket()) {
        return false;
    }

    out_header = pending_header_;
    out_value = pending_value_;
    out_diagnostics = pending_diagnostics_;
    out_has_source_mac = pending_has_source_mac_;
    if (out_source_mac != 0) {
        if (pending_has_source_mac_) {
            for (uint8_t i = 0; i < WIRELESS_MAC_ADDRESS_SIZE; ++i) {
                out_source_mac[i] = pending_source_mac_[i];
            }
        } else {
            clearWirelessMacAddress(out_source_mac);
        }
    }

    clearReceivedPacket();
    return true;
}

}  // namespace Cyber32
