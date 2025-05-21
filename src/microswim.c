#include "microswim.h"

void microswim_initialize(microswim_t* ms) {
    *ms = (microswim_t){ .socket = 0,
                         .self = { { 0 } },
                         .members = { { { 0 } } },
                         .confirmed = { { { 0 } } },
                         .updates = { { 0 } },
                         .pings = { { 0 } },
                         .member_count = 0,
                         .update_count = 0,
                         .ping_count = 0,
                         .round_robin_index = 0 };

    pthread_mutex_init(&ms->mutex, NULL);
}
