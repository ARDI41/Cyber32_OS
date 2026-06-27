#pragma once

#include "capability_directory.h"
#include "node_directory.h"

namespace Cyber32 {

class PublicOwnerStore {
public:
    PublicOwnerStore();

    void reset();

    const NodeDirectory& nodes() const;
    const CapabilityDirectory& capabilities() const;

    // Mutable access is reserved for approved Core bridge milestones.
    // UI/App/Transport and API read methods must not use these to create records.
    NodeDirectory& mutableNodes();
    CapabilityDirectory& mutableCapabilities();

private:
    NodeDirectory node_directory_;
    CapabilityDirectory capability_directory_;
};

}  // namespace Cyber32
