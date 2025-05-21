#include "constants.h"
#include "member.h"
#include "microswim.h"
#include "unity.h"
#include "utils.h"

void setUp(void) {
}

void tearDown(void) {
}

void test_add_member(void) {
    microswim_t ms;
    microswim_initialize(&ms);

    char uuid[UUID_SIZE];
    microswim_uuid_generate(uuid);

    microswim_member_t member;
    strncpy(member.uuid, uuid, UUID_SIZE);
    member.status = 0;
    member.incarnation = 2;
    member.addr.sin_family = AF_INET;
    member.addr.sin_port = htons(8000);
    member.addr.sin_addr.s_addr = inet_addr("192.168.0.1");
    member.timeout = (microswim_milliseconds() + (uint64_t)(SUSPECT_TIMEOUT * 1000));

    microswim_member_add(&ms, member);

    TEST_ASSERT_EQUAL(1, ms.member_count);
    TEST_ASSERT_EQUAL(2, ms.members[0].incarnation);
    TEST_ASSERT_EQUAL_CHAR_ARRAY(ms.members[0].uuid, member.uuid, UUID_SIZE);
}

int main() {
    RUN_TEST(test_add_member);
}
