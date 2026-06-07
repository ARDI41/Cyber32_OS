#pragma once

#include <stdint.h>

#include "event_record.h"

namespace Cyber32 {

class EventBus {
public:
    static constexpr uint8_t QUEUE_SIZE = 32;

    EventBus();

    bool publish(const EventRecord& event);
    bool hasEvents() const;
    bool pop(EventRecord& out_event);
    void clear();
    uint32_t droppedCount() const;
    uint8_t queuedCount() const;

private:
    EventRecord queue_[QUEUE_SIZE];
    uint8_t head_;
    uint8_t tail_;
    uint8_t count_;
    uint32_t dropped_count_;
};

}  // namespace Cyber32
