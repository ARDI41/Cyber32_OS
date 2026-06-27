#pragma once

#include <stdint.h>

#include "public_owner_types.h"

namespace Cyber32 {

// Provisional public-capability owner bound for the empty skeleton.
// Final capacity can be revisited when real owner-backed data is introduced.
static const PublicCapabilityIndex CAPABILITY_DIRECTORY_MAX_PUBLIC_CAPABILITIES = 16;

class CapabilityDirectory {
public:
    CapabilityDirectory();

    void reset();
    PublicCapabilityIndex count() const;
    PublicCapabilityIndex capacity() const;
    bool isEmpty() const;
    bool readByIndex(PublicCapabilityIndex index, PublicCapabilityRecord& out_record) const;
    bool addCapability(const PublicCapabilityRecord& record);

private:
    PublicCapabilityRecord records_[CAPABILITY_DIRECTORY_MAX_PUBLIC_CAPABILITIES];
    PublicCapabilityIndex count_;
};

}  // namespace Cyber32
