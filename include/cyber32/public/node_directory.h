#pragma once

#include <stdint.h>

#include "public_owner_types.h"

namespace Cyber32 {

// Provisional public-node owner bound for the empty skeleton.
// Final capacity can be revisited when real owner-backed data is introduced.
static const PublicNodeIndex NODE_DIRECTORY_MAX_PUBLIC_NODES = 16;

class NodeDirectory {
public:
    NodeDirectory();

    void reset();
    PublicNodeIndex count() const;
    PublicNodeIndex capacity() const;
    bool isEmpty() const;
    bool readByIndex(PublicNodeIndex index, PublicNodeRecord& out_record) const;

private:
    PublicNodeRecord records_[NODE_DIRECTORY_MAX_PUBLIC_NODES];
    PublicNodeIndex count_;
};

}  // namespace Cyber32
