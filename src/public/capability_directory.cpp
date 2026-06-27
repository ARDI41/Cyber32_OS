#include "../../include/cyber32/public/capability_directory.h"

namespace Cyber32 {

CapabilityDirectory::CapabilityDirectory() {
    reset();
}

void CapabilityDirectory::reset() {
    count_ = 0;
    for (PublicCapabilityIndex index = 0; index < CAPABILITY_DIRECTORY_MAX_PUBLIC_CAPABILITIES; ++index) {
        records_[index].reset();
    }
}

PublicCapabilityIndex CapabilityDirectory::count() const {
    return count_;
}

PublicCapabilityIndex CapabilityDirectory::capacity() const {
    return CAPABILITY_DIRECTORY_MAX_PUBLIC_CAPABILITIES;
}

bool CapabilityDirectory::isEmpty() const {
    return count_ == 0;
}

bool CapabilityDirectory::readByIndex(PublicCapabilityIndex index, PublicCapabilityRecord& out_record) const {
    out_record.reset();
    if (index >= count_) {
        return false;
    }

    out_record = records_[index];
    return true;
}

bool CapabilityDirectory::addCapability(const PublicCapabilityRecord& record) {
    if (!record.valid ||
        record.capability_id == 0 ||
        record.capability_instance_id == 0 ||
        record.lifecycle_state == PublicLifecycleState::NONE ||
        record.visibility_state == PublicVisibilityState::NONE) {
        return false;
    }

    if (count_ >= CAPABILITY_DIRECTORY_MAX_PUBLIC_CAPABILITIES) {
        return false;
    }

    for (PublicCapabilityIndex index = 0; index < count_; ++index) {
        if (records_[index].valid &&
            records_[index].capability_instance_id == record.capability_instance_id) {
            return false;
        }
    }

    records_[count_] = record;
    ++count_;
    return true;
}

}  // namespace Cyber32
