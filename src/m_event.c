#include "m_event.h"
#include "microswim.h"
#include "microswim_log.h"

void microswim_event_register(microswim_t* ms, microswim_event_t event) {
    if (event.type >= MAXIMUM_EVENTS) {
        MICROSWIM_LOG_WARN(
            "Unable to register event: type (%d) exceeds the maximum (%d). "
            "Event types must be in the range [0, %d).",
            event.type, MAXIMUM_EVENTS, MAXIMUM_EVENTS);
        return;
    }

    // events[] is a sparse table indexed by the caller-defined event type; only
    // count a type the first time it is bound so event_count stays accurate.
    if (ms->events[event.type].handler == NULL) {
        ms->event_count++;
    }
    ms->events[event.type] = event;
}

void microswim_event_dispatch(microswim_t* ms, uint8_t type, void* data, size_t length) {
    if (type >= MAXIMUM_EVENTS) {
        return;
    }

    microswim_event_t* event = &ms->events[type];
    if (event->handler != NULL) {
        event->handler(ms, data, length);
    }
}
