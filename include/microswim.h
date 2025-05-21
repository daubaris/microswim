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
    microswim_member_t* member;
    size_t count;
} microswim_update_t;

typedef struct {
    int socket;
    microswim_member_t self;
    microswim_member_t members[MAXIMUM_MEMBERS];
    microswim_member_t confirmed[MAXIMUM_MEMBERS];
    microswim_update_t updates[MAXIMUM_UPDATES];
    microswim_ping_t pings[MAXIMUM_MEMBERS];
    // TODO: microswim_ping_req_t ping_reqs[MAXIMUM_MEMBERS];
    size_t member_count;
    size_t confirmed_count;
    size_t update_count;
    size_t ping_count;
    size_t ping_req_count;
    size_t round_robin_index;
    pthread_mutex_t mutex;
} microswim_t;

#endif
