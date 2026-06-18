#include "sim_espnow_transport_driver.h"

namespace Cyber32 {

SimEspNowTransportDriver::SimEspNowTransportDriver()
    : initialized_(false),
      failure_mode_(false),
      has_received_packet_(false),
      pending_header_(),
      pending_value_(),
      pending_diagnostics_(),
      pending_source_mac_(),
      pending_has_source_mac_(false) {
}

void SimEspNowTransportDriver::begin() {
    initialized_ = true;
    clearReceivedPacket();
}

bool SimEspNowTransportDriver::initialized() const {
    return initialized_;
}

void SimEspNowTransportDriver::setFailureMode(bool enabled) {
    failure_mode_ = enabled;
}

bool SimEspNowTransportDriver::injectReceivedCapabilityValue(
    const WirelessPacketHeader& header,
    const WirelessCapabilityValue& value,
    const WirelessNodeDiagnostics& diagnostics) {
    if (!initialized_ || failure_mode_ || has_received_packet_) {
        return false;
    }

    pending_header_ = header;
    pending_value_ = value;
    pending_diagnostics_ = diagnostics;
    clearWirelessMacAddress(pending_source_mac_);
    pending_has_source_mac_ = false;
    has_received_packet_ = true;
    return true;
}

bool SimEspNowTransportDriver::injectReceivedCapabilityValueWithMac(
    const uint8_t source_mac[WIRELESS_MAC_ADDRESS_SIZE],
    const WirelessPacketHeader& header,
    const WirelessCapabilityValue& value,
    const WirelessNodeDiagnostics& diagnostics) {
    if (!initialized_ || failure_mode_ || has_received_packet_ || source_mac == 0) {
        return false;
    }

    pending_header_ = header;
    pending_value_ = value;
    pending_diagnostics_ = diagnostics;
    for (uint8_t i = 0; i < WIRELESS_MAC_ADDRESS_SIZE; ++i) {
        pending_source_mac_[i] = source_mac[i];
    }
    pending_has_source_mac_ = true;
    has_received_packet_ = true;
    return true;
}

bool SimEspNowTransportDriver::hasReceivedPacket() const {
    return initialized_ && !failure_mode_ && has_received_packet_;
}

bool SimEspNowTransportDriver::readReceivedPacket(
    WirelessPacketHeader& out_header,
    WirelessCapabilityValue& out_value,
    WirelessNodeDiagnostics& out_diagnostics) {
    if (!initialized_ || failure_mode_ || !has_received_packet_) {
        return false;
    }

    out_header = pending_header_;
    out_value = pending_value_;
    out_diagnostics = pending_diagnostics_;
    clearReceivedPacket();
    return true;
}

bool SimEspNowTransportDriver::readReceivedPacket(
    WirelessPacketHeader& out_header,
    WirelessCapabilityValue& out_value,
    WirelessNodeDiagnostics& out_diagnostics,
    uint8_t out_source_mac[WIRELESS_MAC_ADDRESS_SIZE],
    bool& out_has_source_mac) {
    if (!initialized_ || failure_mode_ || !has_received_packet_) {
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

void SimEspNowTransportDriver::clearReceivedPacket() {
    has_received_packet_ = false;
    pending_header_ = WirelessPacketHeader();
    pending_value_ = WirelessCapabilityValue();
    pending_diagnostics_ = WirelessNodeDiagnostics();
    clearWirelessMacAddress(pending_source_mac_);
    pending_has_source_mac_ = false;
}

}  // namespace Cyber32
