#pragma once

#include "../../core/types/wireless_node_allowlist_records.h"

namespace Cyber32 {

class MacDiscoveryHelper {
public:
    MacDiscoveryHelper();

    bool begin();
    const uint8_t* macAddress() const;
    bool hasMacAddress() const;
    void printBaseNodeMacToSerial() const;

private:
    uint8_t mac_address_[WIRELESS_MAC_ADDRESS_SIZE];
    bool has_mac_address_;
};

}  // namespace Cyber32
