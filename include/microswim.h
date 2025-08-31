#ifndef MICROSWIM_H
#define MICROSWIM_H

#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "constants.h"

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
    CONFIRM_MESSAGE
} microswim_message_type_t;

typedef struct {
    char uuid[UUID_SIZE];
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
    char uuid[UUID_SIZE];
    struct sockaddr_in addr;
    microswim_member_status_t status;
    int incarnation;
    microswim_member_t mu[MAXIMUM_UPDATES];
    int update_count;
} microswim_message_t;

typedef struct {
    int socket;
    microswim_member_t self;
    microswim_member_t members[MAXIMUM_MEMBERS];
    microswim_member_t confirmed[MAXIMUM_MEMBERS];
    microswim_update_t updates[MAXIMUM_UPDATES];
    microswim_ping_t pings[MAXIMUM_MEMBERS];
    microswim_ping_req_t ping_reqs[MAXIMUM_MEMBERS];
    size_t indices[MAXIMUM_MEMBERS];
    size_t member_count;
    size_t confirmed_count;
    size_t update_count;
    size_t ping_count;
    size_t ping_req_count;
    size_t round_robin_index;
    pthread_mutex_t mutex;
} microswim_t;

void microswim_initialize(microswim_t* ms);
void microswim_socket_setup(microswim_t* ms, char* addr, int port);

void microswim_index_add(microswim_t* ms);
void microswim_index_remove(microswim_t* ms);

void microswim_indices_shuffle(microswim_t* ms);
size_t* microswim_indices_shift(microswim_t* ms, size_t index);

int microswim_compare_by_count(const void* a, const void* b);

#endif
