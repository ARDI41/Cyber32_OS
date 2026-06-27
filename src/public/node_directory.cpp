#include "../../include/cyber32/public/node_directory.h"

namespace Cyber32 {

NodeDirectory::NodeDirectory() {
    reset();
}

void NodeDirectory::reset() {
    count_ = 0;
    for (PublicNodeIndex index = 0; index < NODE_DIRECTORY_MAX_PUBLIC_NODES; ++index) {
        records_[index].reset();
    }
}

PublicNodeIndex NodeDirectory::count() const {
    return count_;
}

PublicNodeIndex NodeDirectory::capacity() const {
    return NODE_DIRECTORY_MAX_PUBLIC_NODES;
}

bool NodeDirectory::isEmpty() const {
    return count_ == 0;
}

bool NodeDirectory::readByIndex(PublicNodeIndex index, PublicNodeRecord& out_record) const {
    out_record.reset();
    if (index >= count_) {
        return false;
    }

    out_record = records_[index];
    return true;
}

bool NodeDirectory::addNode(const PublicNodeRecord& record) {
    if (!record.valid ||
        record.node_id == 0 ||
        record.lifecycle_state == PublicLifecycleState::NONE ||
        record.visibility_state == PublicVisibilityState::NONE) {
        return false;
    }

    if (count_ >= NODE_DIRECTORY_MAX_PUBLIC_NODES) {
        return false;
    }

    for (PublicNodeIndex index = 0; index < count_; ++index) {
        if (records_[index].valid && records_[index].node_id == record.node_id) {
            return false;
        }
    }

    records_[count_] = record;
    ++count_;
    return true;
}

}  // namespace Cyber32
