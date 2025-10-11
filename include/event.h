#ifndef MICROSWIM_EVENT_H
#define MICROSWIM_EVENT_H

#include "microswim.h"

void microswim_event_register(microswim_t* ms, microswim_event_t event);
void microswim_event_dispatch(microswim_t* ms, char* event_name, void* data);

#endif
