#include "encode.h"
#include "event/thread.h"
#include "event/timeout.h"
#include "member.h"
#include "message.h"
#include "microswim.h"
#include "microswim_configuration.h"
#include "microswim_log.h"
#include "mutex.h"
#include "net/sock.h"
#include "net/sock/async/event.h"
#include "net/sock/udp.h"
#include "net/utils.h"
#include "ping.h"
#include "ping_req.h"
#include "random.h"
#include "thread.h"
#include "time_units.h"
#include "update.h"
#include "utils.h"
#include "ztimer.h"
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/select.h>
#include <unistd.h>

#define DEADLINE_DETECTION_PERIOD 1

mutex_t mutex;

static microswim_t ms;

static void _failure_detection_cb(event_t* arg);
static event_t _failure_detection_step = { .handler = _failure_detection_cb };
static event_timeout_t _failure_detection_step_event_timeout;

static void _deadline_detection_cb(event_t* arg);
static event_t _deadline_detection_step = { .handler = _deadline_detection_cb };
static event_timeout_t _deadline_detection_step_event_timeout;

static void _udp_event_handler(sock_udp_t* sock, sock_async_flags_t type, void* arg) {
    mutex_lock(&mutex);
    microswim_t* ms = (microswim_t*)arg;

    sock_udp_ep_t remote;

    if (type & SOCK_ASYNC_MSG_RECV) {
        uint8_t buffer[BUFFER_SIZE];
        ssize_t bytes = sock_udp_recv(sock, buffer, sizeof(buffer), 0, &remote);
        if (bytes <= 0) {
            MICROSWIM_LOG_DEBUG("sock_udp_recv failure: %i\n", (int)bytes);
            mutex_unlock(&mutex);
            return;
        }
        buffer[bytes] = '\0';

        microswim_message_handle(ms, buffer, bytes, NULL);
    }
    mutex_unlock(&mutex);
}

static void _failure_detection_cb(event_t* arg) {
    (void)arg;
    time_t next_step = PROTOCOL_PERIOD;

    mutex_lock(&mutex);
    for (int i = 0; i < GOSSIP_FANOUT; i++) {
        microswim_member_t* member = microswim_member_retrieve(&ms);
        if (member) {
            unsigned char buffer[BUFFER_SIZE] = { 0 };
            microswim_message_t message = { 0 };
            microswim_update_t* updates[MAXIMUM_MEMBERS_IN_AN_UPDATE] = { 0 };

            size_t update_count = microswim_updates_retrieve(&ms, updates);
            microswim_message_construct(&ms, &message, PING_MESSAGE, updates, update_count);
            size_t length = microswim_encode_message(&message, buffer, BUFFER_SIZE);

            microswim_message_send(&ms, member, (const char*)buffer, length);
            MICROSWIM_LOG_DEBUG("Sending PING message to %s", member->uuid);
            microswim_ping_add(&ms, member);
        }
    }

    MICROSWIM_LOG_DEBUG("ms->ping_count: %u", ms.ping_count);
    MICROSWIM_LOG_DEBUG("ms->ping_req_count: %u", ms.ping_req_count);
    MICROSWIM_LOG_DEBUG("ms->update_count: %u", ms.update_count);
    MICROSWIM_LOG_DEBUG("ms->member_count: %u", ms.member_count);
    MICROSWIM_LOG_DEBUG("ms->confirmed_count: %u", ms.confirmed_count);
    printf("[DEBUG] ms->indices: [");
    for (size_t i = 0; i < ms.member_count; i++) {
        if (i < ms.member_count - 1) {
            printf("%u ", ms.indices[i]);
        } else {
            printf("%u]\r\n", ms.indices[i]);
        }
    }
    mutex_unlock(&mutex);

    event_timeout_set(&_failure_detection_step_event_timeout, next_step * US_PER_SEC);
}

static void _deadline_detection_cb(event_t* arg) {
    (void)arg;
    time_t next_step = DEADLINE_DETECTION_PERIOD;

    mutex_lock(&mutex);
    microswim_ping_reqs_check(&ms);
    microswim_pings_check(&ms);
    microswim_members_check_suspects(&ms);
    mutex_unlock(&mutex);

    event_timeout_set(&_deadline_detection_step_event_timeout, next_step * US_PER_SEC);
}

int main(void) {
    mutex_init(&mutex);
    ztimer_sleep(ZTIMER_SEC, 20);

    memset(&ms, 0, sizeof(ms));
    microswim_socket_setup(&ms, NULL, 8000);

    char buffer[64];
    microswim_sockaddr_to_uri(&ms.self.addr, buffer, 64);
    MICROSWIM_LOG_INFO("SELF.ADDR: %s", buffer);

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
    member.addr.family = AF_INET;
    member.addr.port = 8000;
    member.addr.netif = SOCK_ADDR_ANY_NETIF;
    member.status = ALIVE;
    member.incarnation = 0;

    netutils_get_ipv4((ipv4_addr_t*)&member.addr.addr.ipv4, "192.168.0.102");

    memset(buffer, 0, 64);
    microswim_sockaddr_to_uri(&member.addr, buffer, 64);
    MICROSWIM_LOG_INFO("MEMBER.ADDR: %s", buffer);

    microswim_member_t* remote = microswim_member_add(&ms, member);
    if (remote) {
        microswim_index_add(&ms);
        microswim_update_add(&ms, remote);
    }

    sock_udp_event_init(&ms.socket, EVENT_PRIO_MEDIUM, _udp_event_handler, &ms);
    event_timeout_init(&_failure_detection_step_event_timeout, EVENT_PRIO_MEDIUM, &_failure_detection_step);
    event_timeout_set(&_failure_detection_step_event_timeout, PROTOCOL_PERIOD);

    event_timeout_init(&_deadline_detection_step_event_timeout, EVENT_PRIO_MEDIUM, &_deadline_detection_step);
    event_timeout_set(&_deadline_detection_step_event_timeout, DEADLINE_DETECTION_PERIOD);

    return 0;
}
