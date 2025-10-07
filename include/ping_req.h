#ifndef MICROSWIM_PING_REQ_H
#define MICROSWIM_PING_REQ_H

#ifdef __cplusplus
extern "C" {
#endif

#include "microswim.h"

void microswim_ping_req_message_handle(microswim_t* ms, microswim_message_t* message);
void microswim_ping_req_add(microswim_t* ms, microswim_member_t* source, microswim_member_t* target);
void microswim_ping_req_remove(microswim_t* ms, microswim_ping_req_t* ping);

microswim_ping_req_t*
    microswim_ping_req_find(microswim_t* ms, microswim_member_t* source, microswim_member_t* target);
void microswim_ping_reqs_check(microswim_t* ms);

#ifdef __cplusplus
}
#endif

#endif // MICROSWIM_PING_REQ_H
