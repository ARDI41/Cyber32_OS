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
      last_source_mac_() {
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
}

bool EspNowTransportDriver::callbackReceived() const {
    return callback_received_;
}

uint16_t EspNowTransportDriver::lastReceivedLength() const {
    return last_received_length_;
}

void EspNowTransportDriver::receiveCallback(
    const uint8_t* source_mac,
    const uint8_t* data,
    int data_len) {
    (void)data;

    if (active_espnow_transport_driver == 0) {
        return;
    }

    active_espnow_transport_driver->recordReceiveCallback(source_mac, data_len);
}

void EspNowTransportDriver::clearCallbackState() {
    callback_received_ = false;
    last_received_length_ = 0;
    clearWirelessMacAddress(last_source_mac_);
}

void EspNowTransportDriver::recordReceiveCallback(const uint8_t* source_mac, int data_len) {
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
        return;
    }

    for (uint8_t i = 0; i < WIRELESS_MAC_ADDRESS_SIZE; ++i) {
        last_source_mac_[i] = source_mac[i];
    }
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
