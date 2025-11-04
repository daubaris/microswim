#ifndef MICROSWIM_H
#define MICROSWIM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <sys/socket.h>

#include "constants.h"

#ifdef CUSTOM_CONFIGURATION
#include "configuration.h"
#else
#include "microswim_configuration.h"
#endif

typedef enum {
    ALIVE = 0,
    SUSPECT,
    CONFIRMED,
} microswim_member_status_t;

typedef enum {
    PING_MESSAGE = 0,
    PING_REQ_MESSAGE,
    ACK_MESSAGE,
    ALIVE_MESSAGE,
    SUSPECT_MESSAGE,
    CONFIRM_MESSAGE,
    EVENT_MESSAGE,
    UNKOWN_MESSAGE,
    MALFORMED_MESSAGE
} microswim_message_type_t;

typedef struct {
    uint8_t uuid[UUID_SIZE];
    struct sockaddr_in addr;
    microswim_member_status_t status;
    size_t incarnation;
    uint64_t timeout; // NOTE: Suspicion timeout
} microswim_member_t;

typedef struct {
    microswim_member_t* member;
    uint64_t ping_req_deadline;
    uint64_t suspect_deadline;
    bool ping_req;
} microswim_ping_t;

typedef struct {
    microswim_member_t* target;
    microswim_member_t* source;
    uint64_t timeout;
} microswim_ping_req_t;

typedef struct {
    microswim_member_t* member;
    size_t count;
} microswim_update_t;

typedef struct {
    microswim_message_type_t type;
    uint8_t uuid[UUID_SIZE];
    struct sockaddr_in addr;
    microswim_member_status_t status;
    size_t incarnation;
    microswim_member_t mu[MAXIMUM_UPDATES];
    size_t update_count;
} microswim_message_t;

typedef size_t (*microswim_event_encoder_t)(void* output, void* input, size_t size);
typedef void (*microswim_event_decoder_t)(void* output, void* input, size_t size);
typedef void (*microswim_event_handler_t)(void* ms, void* buffer, size_t length);

typedef struct {
    uint8_t type;
    size_t size;
    microswim_event_encoder_t encoder;
    microswim_event_decoder_t decoder;
    microswim_event_handler_t handler;
} microswim_event_t;

typedef struct {
    microswim_message_type_t type;
    void* payload;
} microswim_event_message_t;

typedef struct {
    int socket;
    microswim_member_t self;
    microswim_member_t members[MAXIMUM_MEMBERS];
    microswim_member_t confirmed[MAXIMUM_MEMBERS];
    microswim_update_t updates[MAXIMUM_UPDATES];
    microswim_ping_t pings[MAXIMUM_MEMBERS];
    microswim_ping_req_t ping_reqs[MAXIMUM_MEMBERS];
    microswim_event_t events[MAXIMUM_EVENTS];
    size_t indices[MAXIMUM_MEMBERS];
    size_t member_count;
    size_t confirmed_count;
    size_t update_count;
    size_t ping_count;
    size_t ping_req_count;
    size_t event_count;
    size_t round_robin_index;
} microswim_t;

void microswim_initialize(microswim_t* ms);
void microswim_socket_setup(microswim_t* ms, char* addr, int port);

void microswim_index_add(microswim_t* ms);
void microswim_index_remove(microswim_t* ms);
void microswim_indices_shuffle(microswim_t* ms);
size_t* microswim_indices_shift(microswim_t* ms, size_t index);

int microswim_compare_by_count(const void* a, const void* b);

#ifdef __cplusplus
}
#endif

#endif // MICROSWIM_H
