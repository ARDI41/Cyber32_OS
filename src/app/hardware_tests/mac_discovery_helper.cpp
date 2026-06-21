#include "mac_discovery_helper.h"

#include <Arduino.h>
#include <WiFi.h>

namespace Cyber32 {

namespace {

void printMacByte(uint8_t value) {
    if (value < 0x10U) {
        Serial.print('0');
    }
    Serial.print(value, HEX);
}

}  // namespace

MacDiscoveryHelper::MacDiscoveryHelper()
    : mac_address_(),
      has_mac_address_(false) {
}

bool MacDiscoveryHelper::begin() {
    has_mac_address_ = false;

    WiFi.mode(WIFI_STA);

    uint8_t mac[WIRELESS_MAC_ADDRESS_SIZE];
    if (WiFi.macAddress(mac) == 0) {
        return false;
    }

    for (uint8_t i = 0; i < WIRELESS_MAC_ADDRESS_SIZE; ++i) {
        mac_address_[i] = mac[i];
    }

    has_mac_address_ = true;
    return true;
}

const uint8_t* MacDiscoveryHelper::macAddress() const {
    return mac_address_;
}

bool MacDiscoveryHelper::hasMacAddress() const {
    return has_mac_address_;
}

void MacDiscoveryHelper::printBaseNodeMacToSerial() const {
    Serial.print("Cyber32 Base Node MAC: ");

    for (uint8_t i = 0; i < WIRELESS_MAC_ADDRESS_SIZE; ++i) {
        if (i > 0U) {
            Serial.print(':');
        }
        printMacByte(mac_address_[i]);
    }

    Serial.println();
}

}  // namespace Cyber32
