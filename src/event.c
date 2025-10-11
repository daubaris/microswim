#include "event.h"
#include "constants.h"
#include "log.h"
#include "microswim.h"

void microswim_event_register(microswim_t* ms, microswim_event_t event) {
    if (ms->event_count < MAXIMUM_EVENTS) {
        ms->events[event.type].type = event.type;
        ms->events[event.type].size = event.size;
        ms->events[event.type].encoder = event.encoder;
        ms->events[event.type].decoder = event.decoder;
        ms->events[event.type].handler = event.handler;
    } else {
        LOG_WARN(
            "Unable to add a new event: the maximum limit (%d) has been "
            "reached. Consider increasing MAXIMUM_EVENTS to allow "
            "additional members.",
            MAXIMUM_EVENTS);
    }
}
