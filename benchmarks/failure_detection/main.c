#include "configuration.h"
#include "encode.h"
#include "member.h"
#include "message.h"
#include "microswim.h"
#include "microswim_log.h"
#include "ping.h"
#include "ping_req.h"
#include "update.h"
#include "utils.h"
#include <fcntl.h>
#include <hiredis/hiredis.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define TRACKED_MAX (MAXIMUM_MEMBERS * 2)

typedef struct {
    uint8_t uuid[UUID_SIZE];
    int port;
    microswim_member_status_t status;
} tracked_t;

static tracked_t tracked[TRACKED_MAX];
static size_t tracked_count = 0;
static int packet_drop_pct = 0;

pthread_mutex_t mutex;

static void check_transitions(microswim_t* ms, redisContext* ctx, int self_port) {
    uint64_t ts = microswim_milliseconds();

    // Scan active (ALIVE / SUSPECT) members.
    for (size_t i = 0; i < ms->member_count; i++) {
        microswim_member_t* m = &ms->members[i];

        if (m->uuid[0] == '\0')
            continue;

        if (strncmp((char*)m->uuid, (char*)ms->self.uuid, UUID_SIZE) == 0)
            continue;

        int port = ntohs(m->addr.sin_port);

        tracked_t* found = NULL;
        for (size_t j = 0; j < tracked_count; j++) {
            if (strncmp((char*)tracked[j].uuid, (char*)m->uuid, UUID_SIZE) == 0) {
                found = &tracked[j];
                break;
            }
        }

        if (found == NULL) {
            if (tracked_count < TRACKED_MAX) {
                strncpy((char*)tracked[tracked_count].uuid, (char*)m->uuid, UUID_SIZE);
                tracked[tracked_count].port = port;
                tracked[tracked_count].status = m->status;
                tracked_count++;
            }
        } else if (found->status != m->status) {
            redisCommand(ctx, "RPUSH event:%d target=%d:state=%d:ts=%llu", self_port, port, (int)m->status, (unsigned long long)ts);
            found->status = m->status;
            found->port = port;
        }
    }

    for (size_t i = 0; i < ms->confirmed_count; i++) {
        microswim_member_t* m = &ms->confirmed[i];

        if (m->uuid[0] == '\0')
            continue;
        if (strncmp((char*)m->uuid, (char*)ms->self.uuid, UUID_SIZE) == 0)
            continue;

        int port = ntohs(m->addr.sin_port);

        tracked_t* found = NULL;
        for (size_t j = 0; j < tracked_count; j++) {
            if (strncmp((char*)tracked[j].uuid, (char*)m->uuid, UUID_SIZE) == 0) {
                found = &tracked[j];
                break;
            }
        }

        if (found == NULL) {
            if (tracked_count < TRACKED_MAX) {
                strncpy((char*)tracked[tracked_count].uuid, (char*)m->uuid, UUID_SIZE);
                tracked[tracked_count].port = port;
                tracked[tracked_count].status = CONFIRMED;
                tracked_count++;
                redisCommand(
                    ctx, "RPUSH event:%d target=%d:state=%d:ts=%llu", self_port, port,
                    (int)CONFIRMED, (unsigned long long)ts);
            }
        } else if (found->status != CONFIRMED) {
            redisCommand(ctx, "RPUSH event:%d target=%d:state=%d:ts=%llu", self_port, port, (int)CONFIRMED, (unsigned long long)ts);
            found->status = CONFIRMED;
            found->port = port;
        }
    }
}

void* listener(void* params) {
    microswim_t* ms = (microswim_t*)params;
    bool converged = false;

    redisContext* ctx = redisConnect("127.0.0.1", 6379);
    if (ctx->err) {
        MICROSWIM_LOG_ERROR("Redis error: %s", ctx->errstr);
        exit(-1);
    } else {
        MICROSWIM_LOG_INFO("Successfully connected to Redis!");
        MICROSWIM_LOG_INFO(
            "MAXIMUM_MEMBERS: %d, GOSSIP_FANOUT: %d, MAXIMUM_MEMBERS_IN_AN_UPDATE: %d, "
            "packet_drop_pct: %d",
            MAXIMUM_MEMBERS, GOSSIP_FANOUT, MAXIMUM_MEMBERS_IN_AN_UPDATE, packet_drop_pct);
    }

    int port = ntohs(ms->self.addr.sin_port);

    for (;;) {
        pthread_mutex_lock(&mutex);
        microswim_ping_reqs_check(ms);
        microswim_pings_check(ms);
        microswim_members_check_suspects(ms);

        unsigned char buffer[BUFFER_SIZE] = { 0 };
        struct sockaddr_in from;
        socklen_t len = sizeof(from);

        ssize_t bytes = recvfrom(ms->socket, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&from, &len);

        if (bytes > 0) {
            buffer[bytes] = '\0';
            // Simulate packet drop at the receiver.
            if (packet_drop_pct > 0 && (rand() % 100) < packet_drop_pct) {
                // Discard — simulated network packet loss.
            } else {
                microswim_message_handle(ms, buffer, bytes, NULL);
            }
        }

        check_transitions(ms, ctx, port);
        // Detect convergence: first time member_count reaches MAXIMUM_MEMBERS.
        if (!converged && ms->member_count >= MAXIMUM_MEMBERS) {
            converged = true;
            uint64_t ts = microswim_milliseconds();
            redisReply* reply = redisCommand(ctx, "SET convergence:%d %llu", port, (unsigned long long)ts);
            if (reply == NULL) {
                MICROSWIM_LOG_ERROR("SET convergence command failed.");
                redisFree(ctx);
                exit(-2);
            }
            freeReplyObject(reply);
            MICROSWIM_LOG_INFO("Convergence reached at port %d, ts=%llu", port, (unsigned long long)ts);
        }

        pthread_mutex_unlock(&mutex);
        usleep(100 * 10);
    }
}

void* failure_detection(void* params) {
    microswim_t* ms = (microswim_t*)params;

    for (;;) {
        pthread_mutex_lock(&mutex);

        for (int i = 0; i < GOSSIP_FANOUT; i++) {
            microswim_member_t* member = microswim_member_retrieve(ms);
            if (member != NULL) {
                unsigned char buffer[BUFFER_SIZE] = { 0 };
                microswim_message_t message = { 0 };
                microswim_update_t* updates[MAXIMUM_MEMBERS_IN_AN_UPDATE] = { 0 };
                size_t update_count = microswim_updates_retrieve(ms, updates);
                microswim_message_construct(ms, &message, PING_MESSAGE, updates, update_count);
                size_t length = microswim_encode_message(&message, buffer, BUFFER_SIZE);

                microswim_message_send(ms, member, (const char*)buffer, length);
                microswim_ping_add(ms, member);
            }
        }

        MICROSWIM_LOG_DEBUG("ms->member_count: %zu, ms->confirmed_count: %zu", ms->member_count, ms->confirmed_count);

        pthread_mutex_unlock(&mutex);
        usleep(PROTOCOL_PERIOD * 1000000);
    }
}

int main(int argc, char** argv) {
    srand(time(NULL) + (unsigned int)atoi(argv[2]));

    if (argc >= 6) {
        packet_drop_pct = atoi(argv[5]);
        if (packet_drop_pct < 0)
            packet_drop_pct = 0;
        if (packet_drop_pct > 100)
            packet_drop_pct = 100;
    }

    microswim_t ms;
    memset(&ms, 0, sizeof(ms));
    microswim_socket_setup(&ms, argv[1], atoi(argv[2]));

    int flags = fcntl(ms.socket, F_GETFL, 0);
    if (fcntl(ms.socket, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("Failed to set non-blocking");
        close(ms.socket);
    }

    char uuid[UUID_SIZE];
    microswim_uuid_generate(uuid);
    strncpy((char*)ms.self.uuid, uuid, UUID_SIZE);

    microswim_member_t* self = microswim_member_add(&ms, ms.self);
    if (self) {
        microswim_index_add(&ms);
        microswim_update_add(&ms, self);
    }

    microswim_member_t member;
    member.uuid[0] = '\0';
    member.addr.sin_family = AF_INET;
    member.addr.sin_port = htons(atoi(argv[4]));
    member.status = ALIVE;
    member.incarnation = 0;

    if (inet_pton(AF_INET, argv[3], &(member.addr.sin_addr)) != 1) {
        MICROSWIM_LOG_ERROR("Invalid IP address: %s\n", argv[3]);
        return 1;
    }

    microswim_member_t* remote = microswim_member_add(&ms, member);
    if (remote) {
        microswim_index_add(&ms);
        microswim_update_add(&ms, remote);
    }

    pthread_t _thread, listener_thread;
    pthread_create(&_thread, NULL, failure_detection, (void*)&ms);
    pthread_create(&listener_thread, NULL, listener, (void*)&ms);

    pthread_join(_thread, NULL);
    pthread_join(listener_thread, NULL);

    close(ms.socket);
    pthread_mutex_destroy(&mutex);

    return 0;
}
