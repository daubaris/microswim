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

void print_members(microswim_t* ms) {
    for (int i = 0; i < ms->member_count; i++) {
        LOG_INFO("member: %s, address: %p", ms->members[i].uuid, &ms->members[i]);
    }
    printf("\n");
}

void test_add_member(void) {
    microswim_t ms;
    microswim_initialize(&ms);
    srand(time(NULL));

    microswim_member_t* m1 = add_member_mock(&ms, "192.168.0.1", 8000);

    TEST_ASSERT_EQUAL(1, ms.member_count);
    TEST_ASSERT_EQUAL(0, ms.members[0].incarnation);
    TEST_ASSERT_EQUAL_CHAR_ARRAY(m1->uuid, ms.members[0].uuid, UUID_SIZE);

    microswim_member_t* m2 = add_member_mock(&ms, "192.168.0.2", 8001);
    TEST_ASSERT_EQUAL(2, ms.member_count);
    TEST_ASSERT_EQUAL_CHAR_ARRAY(m2->uuid, ms.members[1].uuid, UUID_SIZE);

    microswim_member_t* m3 = add_member_mock(&ms, "192.168.0.3", 8002);
    TEST_ASSERT_EQUAL(3, ms.member_count);
    TEST_ASSERT_EQUAL_CHAR_ARRAY(m3->uuid, ms.members[2].uuid, UUID_SIZE);
}

void test_move_to_confirmed(void) {
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

    microswim_member_move(&ms, m2);

    TEST_ASSERT_EQUAL(3, ms.member_count);
    TEST_ASSERT_EQUAL(1, ms.confirmed_count);
    TEST_ASSERT_EQUAL_CHAR_ARRAY(u2->member->uuid, ms.confirmed[0].uuid, UUID_SIZE);
}

void test_round_robin(void) {
    microswim_t ms;
    microswim_initialize(&ms);
    srand(time(NULL));

    microswim_member_t* m1 = add_member_mock(&ms, "192.168.0.1", 8000);
    microswim_index_add(&ms);
    microswim_member_t* m2 = add_member_mock(&ms, "192.168.0.2", 8001);
    microswim_index_add(&ms);
    microswim_member_t* m3 = add_member_mock(&ms, "192.168.0.3", 8002);
    microswim_index_add(&ms);
    microswim_member_t* m4 = add_member_mock(&ms, "192.168.0.4", 8003);
    microswim_index_add(&ms);

    for (size_t i = 0; i < ms.member_count - 1; i++) {
        microswim_member_t* member = microswim_member_retrieve(&ms);
        TEST_ASSERT_EQUAL_CHAR_ARRAY(member->uuid, ms.members[ms.indices[i]].uuid, UUID_SIZE);
    }
}

void test_update_self_incarnation_on_suspect_status(void) {
    microswim_t ms;
    microswim_initialize(&ms);
    char uuid[UUID_SIZE];
    microswim_uuid_generate(uuid);
    strncpy(ms.self.uuid, uuid, UUID_SIZE);
    microswim_member_t* member = add_member_mock(&ms, "192.168.0.1", 8000);

    TEST_ASSERT_EQUAL(0, ms.self.incarnation); // Updated to new->incarnation + 1
    TEST_ASSERT_EQUAL(ALIVE, member->status);

    ms.self = *member;
    member->status = SUSPECT;

    TEST_ASSERT_EQUAL(0, ms.self.incarnation); // Updated to new->incarnation + 1
    TEST_ASSERT_EQUAL(SUSPECT, member->status);

    microswim_member_t* existing = microswim_member_find(&ms, member);
    microswim_member_update(&ms, existing, member);

    TEST_ASSERT_EQUAL(1, ms.self.incarnation); // Updated to new->incarnation + 1
    TEST_ASSERT_EQUAL(1, member->incarnation); // Updated to new->incarnation + 1
    TEST_ASSERT_EQUAL(ALIVE, ms.self.status);
    TEST_ASSERT_EQUAL(ALIVE, member->status);
}

void test_alive_with_higher_incarnation_overrides_suspect(void) {
    microswim_t ms;
    microswim_initialize(&ms);

    microswim_member_t* member = add_member_mock(&ms, "192.168.0.1", 8000);
    microswim_member_t new = *member;

    member->status = SUSPECT;
    member->incarnation = 1;

    new.status = ALIVE;
    new.incarnation = 2;

    microswim_member_update(&ms, member, &new);

    TEST_ASSERT_EQUAL(ALIVE, member->status);
    TEST_ASSERT_EQUAL(2, member->incarnation);
}

void test_suspect_with_equal_incarnation_overrides_alive(void) {
    microswim_t ms;
    microswim_initialize(&ms);

    microswim_member_t* member = add_member_mock(&ms, "192.168.0.1", 8000);
    microswim_member_t new = *member;

    member->status = SUSPECT;
    member->incarnation = 1;

    new.status = ALIVE;
    new.incarnation = 1;

    microswim_member_update(&ms, member, &new);

    TEST_ASSERT_EQUAL(SUSPECT, member->status);
    TEST_ASSERT_EQUAL(1, member->incarnation);
}

void test_confirmed_overrides_any_status(void) {
    microswim_t ms;
    microswim_initialize(&ms);

    microswim_member_t* member = add_member_mock(&ms, "192.168.0.1", 8000);
    microswim_member_t new = *member;

    member->status = ALIVE;
    member->incarnation = 1;

    new.status = CONFIRMED;
    new.incarnation = 2;

    microswim_member_update(&ms, member, &new);

    TEST_ASSERT_EQUAL(CONFIRMED, member->status);
    TEST_ASSERT_EQUAL(2, member->incarnation);
    TEST_ASSERT_EQUAL(0, ms.member_count);
    TEST_ASSERT_EQUAL(1, ms.confirmed_count);

    new = *member;
    new.status = ALIVE;
    new.incarnation = 5;

    microswim_member_update(&ms, member, &new);

    TEST_ASSERT_EQUAL(CONFIRMED, member->status);
    TEST_ASSERT_EQUAL(2, member->incarnation);
}

void test_no_update_on_lower_incarnation(void) {
    microswim_t ms;
    microswim_initialize(&ms);

    microswim_member_t* member = add_member_mock(&ms, "192.168.0.1", 8000);
    microswim_member_t new = *member;

    member->status = ALIVE;
    member->incarnation = 3;

    new.status = ALIVE;
    new.incarnation = 2;

    microswim_member_update(&ms, member, &new);

    TEST_ASSERT_EQUAL(ALIVE, member->status);  // No change
    TEST_ASSERT_EQUAL(3, member->incarnation); // No change
}

void test_ping_req_candidates(void) {
    srand(time(NULL));
    microswim_t ms;
    microswim_initialize(&ms);

    microswim_member_t* m1 = add_member_mock(&ms, "192.168.0.1", 8000);
    microswim_index_add(&ms);
    microswim_member_t* m2 = add_member_mock(&ms, "192.168.0.2", 8001);
    microswim_index_add(&ms);
    microswim_member_t* m3 = add_member_mock(&ms, "192.168.0.3", 8002);
    microswim_index_add(&ms);
    microswim_member_t* m4 = add_member_mock(&ms, "192.168.0.4", 8003);
    microswim_index_add(&ms);
    microswim_member_t* m5 = add_member_mock(&ms, "192.168.0.5", 8004);
    microswim_index_add(&ms);
    microswim_member_t* m6 = add_member_mock(&ms, "192.168.0.6", 8005);
    microswim_index_add(&ms);

    size_t members[FAILURE_DETECTION_GROUP];
    size_t count = microswim_get_ping_req_candidates(&ms, members);

    // TODO: verify each member is unique.
    printf("count: %zu\n", count);
    for (size_t i = 0; i < count; i++) {
        printf("members[%zu]: %zu\n", i, members[i]);
    }
}

int main() {
    RUN_TEST(test_add_member);
    RUN_TEST(test_move_to_confirmed);
    RUN_TEST(test_round_robin);
    RUN_TEST(test_update_self_incarnation_on_suspect_status);
    RUN_TEST(test_alive_with_higher_incarnation_overrides_suspect);
    RUN_TEST(test_suspect_with_equal_incarnation_overrides_alive);
    RUN_TEST(test_confirmed_overrides_any_status);
    RUN_TEST(test_confirmed_overrides_any_status);
    RUN_TEST(test_no_update_on_lower_incarnation);
    RUN_TEST(test_ping_req_candidates);
}
