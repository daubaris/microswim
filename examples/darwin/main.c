#include "encode.h"
#include "log.h"
#include "member.h"
#include "message.h"
#include "microswim.h"
#include "ping.h"
#include "ping_req.h"
#include "update.h"
#include "utils.h"
#include <stdlib.h>
#include <unistd.h>

pthread_mutex_t mutex;

void* listener(void* params) {
    microswim_t* ms = (microswim_t*)params;

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
            microswim_message_handle(ms, buffer, bytes, NULL);
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

        LOG_DEBUG("ms->ping_count: %zu", ms->ping_count);
        LOG_DEBUG("ms->ping_req_count: %zu", ms->ping_req_count);
        LOG_DEBUG("ms->update_count: %zu", ms->update_count);
        LOG_DEBUG("ms->member_count: %zu", ms->member_count);
        LOG_DEBUG("ms->confirmed_count: %zu", ms->confirmed_count);
        printf("[DEBUG] ms->indices: [");
        for (int i = 0; i < ms->member_count; i++) {
            if (i < ms->member_count - 1) {
                printf("%zu ", ms->indices[i]);
            } else {
                printf("%zu]\n", ms->indices[i]);
            }
        }

        pthread_mutex_unlock(&mutex);
        usleep(PROTOCOL_PERIOD * 1000000);
    }
}

int main(int argc, char** argv) {
    srand(time(NULL));
    pthread_mutex_init(&mutex, NULL);

    microswim_t ms;
    microswim_initialize(&ms);
    microswim_socket_setup(&ms, argv[1], atoi(argv[2]));

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
        LOG_ERROR("Invalid IP address: %s\n", argv[3]);
        return 1;
    }

    microswim_member_t* remote = microswim_member_add(&ms, member);
    if (remote) {
        microswim_index_add(&ms);
        microswim_update_add(&ms, remote);
    }

    pthread_t fd_thread, pl_thread;
    pthread_create(&fd_thread, NULL, failure_detection, (void*)&ms);
    pthread_create(&pl_thread, NULL, listener, (void*)&ms);

    pthread_join(fd_thread, NULL);
    pthread_join(pl_thread, NULL);

    close(ms.socket);
    pthread_mutex_destroy(&mutex);

    return 0;
}
