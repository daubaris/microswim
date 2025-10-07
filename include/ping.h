#ifndef MICROSWIM_PING_H
#define MICROSWIM_PING_H

#ifdef __cplusplus
extern "C" {
#endif

#include "microswim.h"

microswim_ping_t* microswim_ping_add(microswim_t* ms, microswim_member_t* member);
microswim_ping_t* microswim_ping_find(microswim_t* ms, microswim_member_t* member);
void microswim_ping_remove(microswim_t* ms, microswim_ping_t* ping);

void microswim_pings_check(microswim_t* ms);

#ifdef __cplusplus
}
#endif

#endif // MICROSWIM_PING_H
