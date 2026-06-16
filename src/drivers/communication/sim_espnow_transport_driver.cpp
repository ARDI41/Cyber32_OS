#include "sim_espnow_transport_driver.h"

namespace Cyber32 {

SimEspNowTransportDriver::SimEspNowTransportDriver()
    : initialized_(false),
      failure_mode_(false),
      has_received_packet_(false),
      pending_header_(),
      pending_value_(),
      pending_diagnostics_() {
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

void SimEspNowTransportDriver::clearReceivedPacket() {
    has_received_packet_ = false;
    pending_header_ = WirelessPacketHeader();
    pending_value_ = WirelessCapabilityValue();
    pending_diagnostics_ = WirelessNodeDiagnostics();
}

}  // namespace Cyber32
