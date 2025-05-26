#ifndef MICROSWIM_MESSAGE_H
#define MICROSWIM_MESSAGE_H

#include "microswim.h"

void microswim_message_construct(
    microswim_t* ms, microswim_message_t* message, microswim_message_type_t type,
    microswim_update_t* updates[MAXIMUM_MEMBERS_IN_AN_UPDATE], size_t update_count);

size_t microswim_encode_message(microswim_message_t* message, unsigned char* buffer, size_t size);
void microswim_decode_message(microswim_message_t* message, unsigned char* buffer, ssize_t len);

size_t microswim_json_encode_message(microswim_message_t* message, unsigned char* buffer, size_t size);
void microswim_json_decode_message(microswim_message_t* message, const char* buffer, ssize_t len);

void microswim_cbor_decode_message(microswim_message_t* message, unsigned char* buffer, ssize_t len);
size_t microswim_cbor_encode_message(microswim_message_t* message, unsigned char* buffer, size_t size);

void microswim_ping_message_send(microswim_t* ms, microswim_member_t* member);

void microswim_status_message_send(microswim_t* ms, microswim_message_t* message);
void microswim_status_message_construct(
    microswim_t* ms, microswim_message_t* message, microswim_message_type_t type, microswim_member_t* member);
void microswim_message_handle(microswim_t* ms, unsigned char* buffer, ssize_t len);

#endif
