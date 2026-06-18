#include "espnow_transport_driver.h"

namespace Cyber32 {

EspNowTransportDriver::EspNowTransportDriver()
    : initialized_(false),
      pending_received_packet_(false),
      pending_header_(),
      pending_value_(),
      pending_diagnostics_(),
      pending_source_mac_(),
      pending_has_source_mac_(false) {
}

bool EspNowTransportDriver::begin() {
    initialized_ = false;
    clearReceivedPacket();
    return false;
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
