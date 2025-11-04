#ifndef MICROSWIM_MESSAGE_H
#define MICROSWIM_MESSAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "microswim.h"

void microswim_message_construct(
    microswim_t* ms, microswim_message_t* message, microswim_message_type_t type,
    microswim_update_t* updates[MAXIMUM_MEMBERS_IN_AN_UPDATE], size_t update_count);

void microswim_status_message_construct(
    microswim_t* ms, microswim_message_t* message, microswim_message_type_t type, microswim_member_t* member);
void microswim_message_handle(
    microswim_t* ms, unsigned char* buffer, ssize_t len,
    void (*event_handler)(microswim_t*, unsigned char*, ssize_t));
void microswim_message_send(microswim_t* ms, microswim_member_t* member, const char* buffer, size_t length);
void microswim_ping_message_send(microswim_t* ms, microswim_member_t* member);

#ifdef __cplusplus
}
#endif

#endif // MICROSWIM_MESSAGE_H
