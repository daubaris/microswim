#include "encode.h"
#include "ipso.h"
#include "member.h"
#include "message.h"
#include "microswim.h"
#include "microswim_log.h"
#include "ping.h"
#include "ping_req.h"
#include "update.h"
#include "utils.h"
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

pthread_mutex_t mutex;

/* Static backing storage for IPSO resources */
static int64_t temperature = 25;
static int64_t humidity = 60;

/* IPSO resource definitions */
static ipso_resource_t temp_resources[] = {
    { .rid = 5700, .type = IPSO_T_I64, .storage = &temperature, .storage_len = sizeof(temperature), .set = NULL },
};

static ipso_resource_t hum_resources[] = {
    { .rid = 5700, .type = IPSO_T_I64, .storage = &humidity, .storage_len = sizeof(humidity), .set = NULL },
};

/* IPSO instances: temperature 3303/0, humidity 3304/0 */
static ipso_instance_t instances[] = {
    { .oid = 3303, .iid = 0, .resources = temp_resources, .resource_count = 1 },
    { .oid = 3304, .iid = 0, .resources = hum_resources, .resource_count = 1 },
};

static ipso_registry_t registry = {
    .instances = instances,
    .instance_count = 2,
};

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

        MICROSWIM_LOG_DEBUG("ms->member_count: %zu", ms->member_count);
        for (size_t i = 0; i < ms->member_count; i++) {
            microswim_member_t* m = &ms->members[i];
            printf("[DEBUG] Member: %s (status=%d, ipso_objects=%zu)\n", m->uuid, m->status, m->ipso_object_count);
            for (size_t j = 0; j < m->ipso_object_count; j++) {
                printf(
                    "[DEBUG]   IPSO: oid=%u, iid=%u\n", m->ipso_objects[j].oid, m->ipso_objects[j].iid);
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

    ms.registry = &registry;

    /* Populate self's IPSO objects from the local registry so they
       appear in the members list and get gossiped via updates. */
    for (size_t i = 0; i < registry.instance_count && i < MAXIMUM_IPSO_OBJECTS; i++) {
        ms.self.ipso_objects[i].oid = registry.instances[i].oid;
        ms.self.ipso_objects[i].iid = registry.instances[i].iid;
    }
    ms.self.ipso_object_count = registry.instance_count < MAXIMUM_IPSO_OBJECTS
        ? registry.instance_count : MAXIMUM_IPSO_OBJECTS;

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
    member.ipso_object_count = 0;

    if (inet_pton(AF_INET, argv[3], &(member.addr.sin_addr)) != 1) {
        MICROSWIM_LOG_ERROR("Invalid IP address: %s\n", argv[3]);
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
