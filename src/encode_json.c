#ifdef MICROSWIM_JSON

#include "microswim.h"
#include "utils.h"
#include <stdio.h>

size_t microswim_encode_message(microswim_message_t* message, unsigned char* buffer, size_t size) {
    (void)size;
    char uri_buffer[INET6_ADDRSTRLEN];
    microswim_sockaddr_to_uri(&message->addr, uri_buffer, sizeof(uri_buffer));

    int remainder = snprintf(
        (char*)buffer, BUFFER_SIZE,
        "{\"message\": %d, \"uuid\": \"%s\", \"uri\": \"%s\", \"status\": %d, \"incarnation\": "
        "%d, "
        "\"updates\": [",
        message->type, message->uuid, uri_buffer, message->status, message->incarnation);

    for (size_t i = 0; i < message->update_count; i++) {
        char uri_buffer[INET6_ADDRSTRLEN];
        microswim_sockaddr_to_uri(&message->mu[i].addr, uri_buffer, sizeof(uri_buffer));
        remainder += snprintf(
            (char*)buffer + remainder, BUFFER_SIZE - remainder,
            "{\"uuid\": \"%s\", \"uri\": \"%s\", \"status\": %d, \"incarnation\": %d}",
            message->mu[i].uuid, uri_buffer, message->mu[i].status, message->mu[i].incarnation);

        if (i < message->update_count - 1) {
            remainder += snprintf((char*)buffer + remainder, BUFFER_SIZE - remainder, ",");
        } else {
            remainder += snprintf((char*)buffer + remainder, BUFFER_SIZE - remainder, "]}");
        }
    }

    return remainder;
}

#endif
