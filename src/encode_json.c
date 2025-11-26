#ifdef MICROSWIM_JSON

#include "microswim.h"
#include "microswim_log.h"
#include "utils.h"
#include <stdio.h>

size_t microswim_encode_message(microswim_message_t* message, unsigned char* buffer, size_t size) {
    (void)size;

    char uri_buffer[64];
    microswim_sockaddr_to_uri(&message->addr, uri_buffer, 64);
    int remainder = snprintf(
        (char*)buffer, BUFFER_SIZE,
        "{\"message\": %d, \"uuid\": \"%s\", \"uri\": \"%s\", \"status\": %d, \"incarnation\": "
        "%d, "
        "\"updates\": [",
        message->type, message->uuid, uri_buffer, message->status, message->incarnation);
    for (size_t i = 0; i < message->update_count; i++) {
        char ub[64] = { 0 };
        microswim_sockaddr_to_uri(&message->mu[i].addr, ub, 64);
        remainder += snprintf(
            (char*)buffer + remainder, BUFFER_SIZE - remainder,
            "{\"uuid\": \"%s\", \"uri\": \"%s\", \"status\": %d, \"incarnation\": %d}",
            message->mu[i].uuid, ub, message->mu[i].status, message->mu[i].incarnation);
        if (i < message->update_count - 1) {
            remainder += snprintf((char*)buffer + remainder, BUFFER_SIZE - remainder, ",");
        } else {
            remainder += snprintf((char*)buffer + remainder, BUFFER_SIZE - remainder, "]}");
        }
    }

    return remainder;
}

#endif
