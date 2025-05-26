#include "constants.h"
#include "log.h"
#include "member.h"
#include "microswim.h"
#include "unity.h"
#include "update.h"
#include "utils.h"
#include <stdlib.h>

void setUp(void) {
}

void tearDown(void) {
}

microswim_member_t* add_member_mock(microswim_t* ms, char* addr, int port) {
    char uuid[UUID_SIZE];
    microswim_uuid_generate(uuid);

    microswim_member_t member;
    strncpy(member.uuid, uuid, UUID_SIZE);
    member.status = 0;
    member.incarnation = 0;
    member.addr.sin_family = AF_INET;
    member.addr.sin_port = htons(port);
    member.addr.sin_addr.s_addr = inet_addr(addr);
    member.timeout = (microswim_milliseconds() + (uint64_t)(SUSPECT_TIMEOUT * 1000));

    return microswim_member_add(ms, member);
}

void print_members_and_updates(microswim_t* ms) {
    for (int i = 0; i < ms->member_count; i++) {
        LOG_INFO("member: %s, address: %p", ms->members[i].uuid, &ms->members[i]);
    }

    for (int i = 0; i < ms->update_count; i++) {
        LOG_INFO("update: %s, count: %zu", ms->members[i].uuid, ms->updates[i].count);
    }
    printf("\n");
}

void test_add_update(void) {
    microswim_t ms;
    microswim_initialize(&ms);

    srand(time(NULL));

    microswim_member_t* m1 = add_member_mock(&ms, "192.168.0.1", 8000);
    microswim_update_t* u1 = microswim_update_add(&ms, m1);
    u1->count = 99;

    TEST_ASSERT_EQUAL_CHAR_ARRAY(m1->uuid, ms.members[0].uuid, UUID_SIZE);
    TEST_ASSERT_EQUAL_CHAR_ARRAY(u1->member->uuid, ms.members[0].uuid, UUID_SIZE);

    microswim_member_t* m2 = add_member_mock(&ms, "192.168.0.2", 8001);
    microswim_update_t* u2 = microswim_update_add(&ms, m2);
    u2->count = 1;
    TEST_ASSERT_EQUAL_CHAR_ARRAY(m2->uuid, ms.members[1].uuid, UUID_SIZE);
    TEST_ASSERT_EQUAL_CHAR_ARRAY(u2->member->uuid, ms.members[1].uuid, UUID_SIZE);

    microswim_member_t* m3 = add_member_mock(&ms, "192.168.0.3", 8002);
    microswim_update_t* u3 = microswim_update_add(&ms, m3);
    u3->count = 2;
    TEST_ASSERT_EQUAL_CHAR_ARRAY(m3->uuid, ms.members[2].uuid, UUID_SIZE);
    TEST_ASSERT_EQUAL_CHAR_ARRAY(u3->member->uuid, ms.members[2].uuid, UUID_SIZE);

    microswim_member_t* m4 = add_member_mock(&ms, "192.168.0.4", 8003);
    microswim_update_t* u4 = microswim_update_add(&ms, m4);
    u4->count = 3;
    TEST_ASSERT_EQUAL_CHAR_ARRAY(m4->uuid, ms.members[3].uuid, UUID_SIZE);
    TEST_ASSERT_EQUAL_CHAR_ARRAY(u4->member->uuid, ms.members[3].uuid, UUID_SIZE);
}

int main() {
    RUN_TEST(test_add_update);
}
