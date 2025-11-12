#include "encode.h"
#include "log.h"
#include "lwip/dhcp.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "member.h"
#include "message.h"
#include "microswim.h"
#include "ping.h"
#include "ping_req.h"
#include "random.h"
#include "thread.h"
#include "update.h"
#include "utils.h"
#include "ztimer.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/select.h>
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

        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(ms->socket, &rfds);

        struct timeval tv = { 0, 0 };
        int ready = select(ms->socket + 1, &rfds, NULL, NULL, &tv);

        if (ready > 0 && FD_ISSET(ms->socket, &rfds)) {
            ssize_t bytes =
                recvfrom(ms->socket, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&from, &len);
            if (bytes > 0) {
                buffer[bytes] = '\0';
                microswim_message_handle(ms, buffer, bytes, NULL);
            }
        }

        pthread_mutex_unlock(&mutex);
        ztimer_sleep(ZTIMER_SEC, 1);
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

        printf("ms->ping_count: %zu\r\n", ms->ping_count);
        printf("ms->ping_req_count: %zu\r\n", ms->ping_req_count);
        printf("ms->update_count: %zu\r\n", ms->update_count);
        printf("ms->member_count: %zu\r\n", ms->member_count);
        printf("ms->confirmed_count: %zu\r\n", ms->confirmed_count);
        printf("[DEBUG] ms->indices: [");
        for (size_t i = 0; i < ms->member_count; i++) {
            if (i < ms->member_count - 1) {
                printf("%zu ", ms->indices[i]);
            } else {
                printf("%zu]\r\n", ms->indices[i]);
            }
        }

        pthread_mutex_unlock(&mutex);
        ztimer_sleep(ZTIMER_SEC, PROTOCOL_PERIOD);
    }
    return NULL;
}

int main(void) {
    ztimer_sleep(ZTIMER_SEC, 20);

    random_init(42);

    pthread_mutex_init(&mutex, NULL);

    microswim_t ms;
    printf("sizeof(microswim_t): %d\r\n", sizeof(microswim_t));
    printf("sizeof THREAD_STACKSIZE_MAIN: %u\r\n", THREAD_STACKSIZE_MAIN);

    memset(&ms, 0, sizeof(ms));

    microswim_socket_setup(&ms, NULL, atoi("8000"));

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
    member.addr.sin_port = htons(atoi("8000"));
    member.status = ALIVE;
    member.incarnation = 0;

    if (inet_pton(AF_INET, "192.168.1.120", &(member.addr.sin_addr)) != 1) {
        LOG_ERROR("Invalid IP address: %s", "192.168.1.120");
        return 1;
    }

    microswim_member_t* remote = microswim_member_add(&ms, member);
    if (remote) {
        microswim_index_add(&ms);
        microswim_update_add(&ms, remote);
    }

    pthread_t fd_thread, pl_thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 8192);
    pthread_create(&fd_thread, &attr, failure_detection, (void*)&ms);
    pthread_create(&pl_thread, &attr, listener, (void*)&ms);
    pthread_join(fd_thread, NULL);
    pthread_join(pl_thread, NULL);

    close(ms.socket);
    pthread_mutex_destroy(&mutex);

    return 0;
}
