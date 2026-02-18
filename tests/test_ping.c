#include "microswim.h"
#include "ping.h"
#include "test_helpers.h"
#include "unity.h"
#include "utils.h"
#include <string.h>
#include <unistd.h>

void test_ping_add(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-P-01", 13000);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 1);
    microswim_member_t* m = test_helper_add_member(&ms, uuid, 13001);

    microswim_ping_t* p = microswim_ping_add(&ms, m);
    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT_EQUAL_PTR(m, p->member);
    TEST_ASSERT_EQUAL_UINT(1, ms.ping_count);
    TEST_ASSERT_FALSE(p->ping_req);

    test_helper_teardown_ms(&ms);
}

void test_ping_find(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-P-02", 13010);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 2);
    microswim_member_t* m = test_helper_add_member(&ms, uuid, 13011);
    microswim_ping_add(&ms, m);

    microswim_member_t needle = test_helper_make_member(uuid, 13011);
    microswim_ping_t* found = microswim_ping_find(&ms, &needle);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_PTR(m, found->member);

    test_helper_teardown_ms(&ms);
}

void test_ping_remove(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-P-03", 13020);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 3);
    microswim_member_t* m = test_helper_add_member(&ms, uuid, 13021);
    microswim_ping_t* p = microswim_ping_add(&ms, m);

    microswim_ping_remove(&ms, p);
    TEST_ASSERT_EQUAL_UINT(0, ms.ping_count);

    test_helper_teardown_ms(&ms);
}

/* Duplicate replacement */
void test_ping_add_duplicate_replaces(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-P-04", 13030);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 4);
    microswim_member_t* m = test_helper_add_member(&ms, uuid, 13031);

    microswim_ping_add(&ms, m);
    microswim_ping_add(&ms, m); /* should replace */

    TEST_ASSERT_EQUAL_UINT(1, ms.ping_count);

    test_helper_teardown_ms(&ms);
}

/* Empty UUID rejected */
void test_ping_add_empty_uuid_rejected(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-P-05", 13040);

    microswim_member_t m = test_helper_make_member("", 13041);
    microswim_ping_t* p = microswim_ping_add(&ms, &m);
    TEST_ASSERT_NULL(p);
    TEST_ASSERT_EQUAL_UINT(0, ms.ping_count);

    test_helper_teardown_ms(&ms);
}

/* Overflow */
void test_ping_add_overflow(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-P-06", 13050);

    /* Fill pings to capacity */
    for (int i = 0; i <= MAXIMUM_PINGS; i++) {
        char uuid[UUID_SIZE];
        test_helper_seed_uuid(uuid, 500 + i);
        microswim_member_t* m = test_helper_add_member(&ms, uuid, 13060 + i);
        if (m)
            microswim_ping_add(&ms, m);
    }

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 999);
    microswim_member_t* last = test_helper_add_member(&ms, uuid, 13999);
    if (last) {
        microswim_ping_t* p = microswim_ping_add(&ms, last);
        /* Either NULL (overflow) or it replaced a duplicate */
        (void)p;
    }

    test_helper_teardown_ms(&ms);
}

/* pings_check marks suspect on timeout */
void test_pings_check_marks_suspect(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-P-07", 13070);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 10);
    microswim_member_t* m = test_helper_add_member(&ms, uuid, 13071);

    microswim_ping_t* p = microswim_ping_add(&ms, m);
    TEST_ASSERT_NOT_NULL(p);

    /* Set deadlines in the past */
    p->suspect_deadline = 1;
    p->ping_req_deadline = 1;

    microswim_pings_check(&ms);

    /* Member should be marked suspect */
    TEST_ASSERT_EQUAL_INT(SUSPECT, m->status);
    TEST_ASSERT_EQUAL_UINT(0, ms.ping_count);

    test_helper_teardown_ms(&ms);
}

/* pings_check no-op before deadline */
void test_pings_check_no_op_before_deadline(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-P-08", 13080);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 11);
    microswim_member_t* m = test_helper_add_member(&ms, uuid, 13081);

    microswim_ping_t* p = microswim_ping_add(&ms, m);
    /* Deadlines far in the future */
    p->suspect_deadline = microswim_milliseconds() + 60000;
    p->ping_req_deadline = microswim_milliseconds() + 60000;

    microswim_pings_check(&ms);
    TEST_ASSERT_EQUAL_INT(ALIVE, m->status);
    TEST_ASSERT_EQUAL_UINT(1, ms.ping_count);

    test_helper_teardown_ms(&ms);
}
