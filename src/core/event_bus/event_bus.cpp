#include "event_bus.h"

namespace Cyber32 {

EventBus::EventBus()
    : head_(0),
      tail_(0),
      count_(0),
      dropped_count_(0) {
}

bool EventBus::publish(const EventRecord& event) {
    if (count_ >= QUEUE_SIZE) {
        ++dropped_count_;
        return false;
    }

    queue_[tail_] = event;
    tail_ = static_cast<uint8_t>((tail_ + 1) % QUEUE_SIZE);
    ++count_;
    return true;
}

bool EventBus::hasEvents() const {
    return count_ > 0;
}

bool EventBus::pop(EventRecord& out_event) {
    if (count_ == 0) {
        return false;
    }

    out_event = queue_[head_];
    head_ = static_cast<uint8_t>((head_ + 1) % QUEUE_SIZE);
    --count_;
    return true;
}

void EventBus::clear() {
    head_ = 0;
    tail_ = 0;
    count_ = 0;
}

uint32_t EventBus::droppedCount() const {
    return dropped_count_;
}

uint8_t EventBus::queuedCount() const {
    return count_;
}

}  // namespace Cyber32
