#ifndef MICROSWIM_EVENT_H
#define MICROSWIM_EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "microswim.h"

void microswim_event_register(microswim_t* ms, microswim_event_t event);
void microswim_event_dispatch(microswim_t* ms, uint8_t type, void* data, size_t length);

#ifdef __cplusplus
}
#endif

#endif
