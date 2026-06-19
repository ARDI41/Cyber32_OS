#pragma once

#include "../../core/types/wireless_node_allowlist_records.h"
#include "../../core/types/wireless_packet_types.h"
#include "../../drivers/communication/espnow_transport_driver.h"
#include "../../drivers/communication/sim_espnow_transport_driver.h"

namespace Cyber32 {

struct WirelessPacketTransportAdapter {
    void* context;
    bool (*has_received_packet)(void* context);
    bool (*read_received_packet)(
        void* context,
        WirelessPacketHeader& out_header,
        WirelessCapabilityValue& out_value,
        WirelessNodeDiagnostics& out_diagnostics,
        uint8_t out_source_mac[WIRELESS_MAC_ADDRESS_SIZE],
        bool& out_has_source_mac);
    void (*clear_received_packet)(void* context);
};

inline bool wirelessPacketTransportAdapterValid(
    const WirelessPacketTransportAdapter& adapter) {
    return adapter.context != 0 &&
           adapter.has_received_packet != 0 &&
           adapter.read_received_packet != 0 &&
           adapter.clear_received_packet != 0;
}

inline bool simEspNowTransportHasReceivedPacket(void* context) {
    if (context == 0) {
        return false;
    }

    SimEspNowTransportDriver* driver =
        static_cast<SimEspNowTransportDriver*>(context);
    return driver->hasReceivedPacket();
}

inline bool simEspNowTransportReadReceivedPacket(
    void* context,
    WirelessPacketHeader& out_header,
    WirelessCapabilityValue& out_value,
    WirelessNodeDiagnostics& out_diagnostics,
    uint8_t out_source_mac[WIRELESS_MAC_ADDRESS_SIZE],
    bool& out_has_source_mac) {
    if (context == 0) {
        return false;
    }

    SimEspNowTransportDriver* driver =
        static_cast<SimEspNowTransportDriver*>(context);
    return driver->readReceivedPacket(
        out_header,
        out_value,
        out_diagnostics,
        out_source_mac,
        out_has_source_mac);
}

inline void simEspNowTransportClearReceivedPacket(void* context) {
    if (context == 0) {
        return;
    }

    SimEspNowTransportDriver* driver =
        static_cast<SimEspNowTransportDriver*>(context);
    driver->clearReceivedPacket();
}

inline WirelessPacketTransportAdapter makeSimEspNowTransportAdapter(
    SimEspNowTransportDriver* driver) {
    WirelessPacketTransportAdapter adapter;
    adapter.context = driver;
    adapter.has_received_packet = 0;
    adapter.read_received_packet = 0;
    adapter.clear_received_packet = 0;

    if (driver == 0) {
        adapter.context = 0;
        return adapter;
    }

    adapter.has_received_packet = &simEspNowTransportHasReceivedPacket;
    adapter.read_received_packet = &simEspNowTransportReadReceivedPacket;
    adapter.clear_received_packet = &simEspNowTransportClearReceivedPacket;
    return adapter;
}

inline bool espNowTransportHasReceivedPacket(void* context) {
    if (context == 0) {
        return false;
    }

    EspNowTransportDriver* driver =
        static_cast<EspNowTransportDriver*>(context);
    return driver->hasReceivedPacket();
}

inline bool espNowTransportReadReceivedPacket(
    void* context,
    WirelessPacketHeader& out_header,
    WirelessCapabilityValue& out_value,
    WirelessNodeDiagnostics& out_diagnostics,
    uint8_t out_source_mac[WIRELESS_MAC_ADDRESS_SIZE],
    bool& out_has_source_mac) {
    if (context == 0) {
        return false;
    }

    EspNowTransportDriver* driver =
        static_cast<EspNowTransportDriver*>(context);
    return driver->readReceivedPacket(
        out_header,
        out_value,
        out_diagnostics,
        out_source_mac,
        out_has_source_mac);
}

inline void espNowTransportClearReceivedPacket(void* context) {
    if (context == 0) {
        return;
    }

    EspNowTransportDriver* driver =
        static_cast<EspNowTransportDriver*>(context);
    driver->clearReceivedPacket();
}

inline WirelessPacketTransportAdapter makeEspNowTransportAdapter(
    EspNowTransportDriver* driver) {
    WirelessPacketTransportAdapter adapter;
    adapter.context = driver;
    adapter.has_received_packet = 0;
    adapter.read_received_packet = 0;
    adapter.clear_received_packet = 0;

    if (driver == 0) {
        adapter.context = 0;
        return adapter;
    }

    adapter.has_received_packet = &espNowTransportHasReceivedPacket;
    adapter.read_received_packet = &espNowTransportReadReceivedPacket;
    adapter.clear_received_packet = &espNowTransportClearReceivedPacket;
    return adapter;
}

}  // namespace Cyber32
