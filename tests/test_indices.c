#include "member.h"
#include "microswim.h"
#include "unity.h"
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

void print_indices(microswim_t* ms) {
    for (size_t i = 0; i < ms->member_count; i++) {
        printf("ms->indices: %zu\n", ms->indices[i]);
    }
    printf("\n");
}

void test_add_index(void) {
    microswim_t ms;
    microswim_initialize(&ms);
    srand(time(NULL));

    microswim_member_t* m1 = add_member_mock(&ms, "192.168.0.1", 8000);
    microswim_index_add(&ms);
    TEST_ASSERT_EQUAL(0, ms.indices[0]);

    TEST_ASSERT_EQUAL(1, ms.member_count);
    TEST_ASSERT_EQUAL(0, ms.members[0].incarnation);
    TEST_ASSERT_EQUAL_CHAR_ARRAY(m1->uuid, ms.members[0].uuid, UUID_SIZE);

    microswim_member_t* m2 = add_member_mock(&ms, "192.168.0.2", 8001);
    microswim_index_add(&ms);
    TEST_ASSERT_EQUAL(2, ms.member_count);
    TEST_ASSERT_EQUAL_CHAR_ARRAY(m2->uuid, ms.members[1].uuid, UUID_SIZE);

    microswim_member_t* m3 = add_member_mock(&ms, "192.168.0.3", 8002);
    microswim_index_add(&ms);
    TEST_ASSERT_EQUAL(3, ms.member_count);
    TEST_ASSERT_EQUAL_CHAR_ARRAY(m3->uuid, ms.members[2].uuid, UUID_SIZE);

    microswim_indices_shuffle(&ms);
}

void test_remove_index(void) {
    microswim_t ms;
    microswim_initialize(&ms);
    srand(time(NULL));

    microswim_member_t* m1 = add_member_mock(&ms, "192.168.0.1", 8000);
    microswim_index_add(&ms);
    TEST_ASSERT_EQUAL(0, ms.indices[0]);

    TEST_ASSERT_EQUAL(1, ms.member_count);
    TEST_ASSERT_EQUAL(0, ms.members[0].incarnation);
    TEST_ASSERT_EQUAL_CHAR_ARRAY(m1->uuid, ms.members[0].uuid, UUID_SIZE);

    microswim_member_t* m2 = add_member_mock(&ms, "192.168.0.2", 8001);
    microswim_index_add(&ms);
    TEST_ASSERT_EQUAL(2, ms.member_count);
    TEST_ASSERT_EQUAL_CHAR_ARRAY(m2->uuid, ms.members[1].uuid, UUID_SIZE);

    microswim_member_t* m3 = add_member_mock(&ms, "192.168.0.3", 8002);
    microswim_index_add(&ms);
    TEST_ASSERT_EQUAL(3, ms.member_count);
    TEST_ASSERT_EQUAL_CHAR_ARRAY(m3->uuid, ms.members[2].uuid, UUID_SIZE);

    microswim_indices_shuffle(&ms);

    microswim_member_move(&ms, m1);
    microswim_index_remove(&ms);

    microswim_member_move(&ms, m2);
    microswim_index_remove(&ms);
    TEST_ASSERT_EQUAL(0, ms.indices[0]);
}

int main() {
    RUN_TEST(test_add_index);
    RUN_TEST(test_remove_index);
}
