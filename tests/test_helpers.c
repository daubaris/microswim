#include "test_helpers.h"
#include "member.h"
#include "update.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void test_helper_init_ms(microswim_t* ms, const char* uuid, int port) {
    memset(ms, 0, sizeof(*ms));
    microswim_socket_setup(ms, "127.0.0.1", port);
    strncpy((char*)ms->self.uuid, uuid, UUID_SIZE);

    microswim_member_t self_member = { 0 };
    strncpy((char*)self_member.uuid, uuid, UUID_SIZE);
    self_member.addr = ms->self.addr;
    self_member.status = ALIVE;
    self_member.incarnation = 0;

    microswim_member_add(ms, self_member);
    microswim_index_add(ms);
    microswim_update_add(ms, &ms->members[0]);
}

microswim_member_t test_helper_make_member(const char* uuid, int port) {
    microswim_member_t m = { 0 };
    strncpy((char*)m.uuid, uuid, UUID_SIZE);
    m.addr.sin_family = AF_INET;
    m.addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &m.addr.sin_addr);
    m.status = ALIVE;
    m.incarnation = 0;
    return m;
}

microswim_member_t* test_helper_add_member(microswim_t* ms, const char* uuid, int port) {
    microswim_member_t m = test_helper_make_member(uuid, port);
    microswim_member_t* added = microswim_member_add(ms, m);
    if (added) {
        microswim_index_add(ms);
        microswim_update_add(ms, added);
    }
    return added;
}

void test_helper_seed_uuid(char* buf, int seed) {
    snprintf(buf, UUID_SIZE, "00000000-0000-0000-0000-%012d", seed);
}

void test_helper_teardown_ms(microswim_t* ms) {
    if (ms->socket > 0) {
        close(ms->socket);
        ms->socket = -1;
    }
}
