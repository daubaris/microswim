#ifndef MICROWSIM_PING_H
#define MICROWSIM_PING_H

#include "microswim.h"

microswim_ping_t* microswim_ping_add(microswim_t* ms, microswim_ping_t* ping);
microswim_ping_t* microswim_ping_find(microswim_t* ms, microswim_ping_t* ping);
microswim_ping_t* microswim_ping_update(microswim_t* ms, microswim_ping_t* ping);
microswim_ping_t* microswim_ping_remove(microswim_t* ms, microswim_ping_t* ping);

void microswim_ping_mark_alive(microswim_t* ms, microswim_ping_t* ping);
void microswim_ping_mark_suspect(microswim_t* ms, microswim_ping_t* ping);
void microswim_ping_mark_confirmed(microswim_t* ms, microswim_ping_t* ping);

#endif
