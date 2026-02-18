#include "microswim.h"
#include "ping_req.h"
#include "test_helpers.h"
#include "unity.h"
#include "utils.h"
#include <string.h>

void test_ping_req_add(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-PR-01", 14000);

    char uuid_s[UUID_SIZE], uuid_t[UUID_SIZE];
    test_helper_seed_uuid(uuid_s, 1);
    test_helper_seed_uuid(uuid_t, 2);
    microswim_member_t* src = test_helper_add_member(&ms, uuid_s, 14001);
    microswim_member_t* tgt = test_helper_add_member(&ms, uuid_t, 14002);

    microswim_ping_req_add(&ms, src, tgt);
    TEST_ASSERT_EQUAL_UINT(1, ms.ping_req_count);

    test_helper_teardown_ms(&ms);
}

void test_ping_req_find(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-PR-02", 14010);

    char uuid_s[UUID_SIZE], uuid_t[UUID_SIZE];
    test_helper_seed_uuid(uuid_s, 3);
    test_helper_seed_uuid(uuid_t, 4);
    microswim_member_t* src = test_helper_add_member(&ms, uuid_s, 14011);
    microswim_member_t* tgt = test_helper_add_member(&ms, uuid_t, 14012);

    microswim_ping_req_add(&ms, src, tgt);

    microswim_ping_req_t* found = microswim_ping_req_find(&ms, src, tgt);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_PTR(src, found->source);
    TEST_ASSERT_EQUAL_PTR(tgt, found->target);

    test_helper_teardown_ms(&ms);
}

void test_ping_req_remove(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-PR-03", 14020);

    char uuid_s[UUID_SIZE], uuid_t[UUID_SIZE];
    test_helper_seed_uuid(uuid_s, 5);
    test_helper_seed_uuid(uuid_t, 6);
    microswim_member_t* src = test_helper_add_member(&ms, uuid_s, 14021);
    microswim_member_t* tgt = test_helper_add_member(&ms, uuid_t, 14022);

    microswim_ping_req_add(&ms, src, tgt);
    microswim_ping_req_t* pr = microswim_ping_req_find(&ms, src, tgt);
    TEST_ASSERT_NOT_NULL(pr);

    microswim_ping_req_remove(&ms, pr);
    TEST_ASSERT_EQUAL_UINT(0, ms.ping_req_count);

    test_helper_teardown_ms(&ms);
}

/* Duplicate replacement */
void test_ping_req_add_duplicate_replaces(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-PR-04", 14030);

    char uuid_s[UUID_SIZE], uuid_t[UUID_SIZE];
    test_helper_seed_uuid(uuid_s, 7);
    test_helper_seed_uuid(uuid_t, 8);
    microswim_member_t* src = test_helper_add_member(&ms, uuid_s, 14031);
    microswim_member_t* tgt = test_helper_add_member(&ms, uuid_t, 14032);

    microswim_ping_req_add(&ms, src, tgt);
    microswim_ping_req_add(&ms, src, tgt); /* duplicate */

    TEST_ASSERT_EQUAL_UINT(1, ms.ping_req_count);

    test_helper_teardown_ms(&ms);
}

/* Empty UUID rejected */
void test_ping_req_add_empty_uuid_rejected(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-PR-05", 14040);

    microswim_member_t src = test_helper_make_member("", 14041);
    char uuid_t[UUID_SIZE];
    test_helper_seed_uuid(uuid_t, 9);
    microswim_member_t* tgt = test_helper_add_member(&ms, uuid_t, 14042);

    microswim_ping_req_add(&ms, &src, tgt);
    TEST_ASSERT_EQUAL_UINT(0, ms.ping_req_count);

    test_helper_teardown_ms(&ms);
}

/* ping_reqs_check removes expired */
void test_ping_reqs_check_removes_expired(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-PR-06", 14050);

    char uuid_s[UUID_SIZE], uuid_t[UUID_SIZE];
    test_helper_seed_uuid(uuid_s, 10);
    test_helper_seed_uuid(uuid_t, 11);
    microswim_member_t* src = test_helper_add_member(&ms, uuid_s, 14051);
    microswim_member_t* tgt = test_helper_add_member(&ms, uuid_t, 14052);

    microswim_ping_req_add(&ms, src, tgt);
    ms.ping_reqs[0].timeout = 1; /* already expired */

    microswim_ping_reqs_check(&ms);
    TEST_ASSERT_EQUAL_UINT(0, ms.ping_req_count);

    test_helper_teardown_ms(&ms);
}

/* ping_reqs_check keeps unexpired */
void test_ping_reqs_check_keeps_unexpired(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-PR-07", 14060);

    char uuid_s[UUID_SIZE], uuid_t[UUID_SIZE];
    test_helper_seed_uuid(uuid_s, 12);
    test_helper_seed_uuid(uuid_t, 13);
    microswim_member_t* src = test_helper_add_member(&ms, uuid_s, 14061);
    microswim_member_t* tgt = test_helper_add_member(&ms, uuid_t, 14062);

    microswim_ping_req_add(&ms, src, tgt);
    ms.ping_reqs[0].timeout = microswim_milliseconds() + 60000;

    microswim_ping_reqs_check(&ms);
    TEST_ASSERT_EQUAL_UINT(1, ms.ping_req_count);

    test_helper_teardown_ms(&ms);
}
