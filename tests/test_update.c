#include "microswim.h"
#include "test_helpers.h"
#include "unity.h"
#include "update.h"
#include <string.h>

/* 1. Add an update and verify pointer/count */
void test_update_add(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-UUID-0001", 9000);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 1);
    microswim_member_t* m = test_helper_add_member(&ms, uuid, 9001);

    microswim_update_t* u = microswim_update_find(&ms, m);
    TEST_ASSERT_NOT_NULL(u);
    TEST_ASSERT_EQUAL_PTR(m, u->member);
    TEST_ASSERT_EQUAL_UINT(0, u->count);

    test_helper_teardown_ms(&ms);
}

/* 2. Find returns NULL for unknown member */
void test_update_find_unknown(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-UUID-0002", 9010);

    microswim_member_t needle = test_helper_make_member("UNKNOWN-UUID", 9999);
    TEST_ASSERT_NULL(microswim_update_find(&ms, &needle));

    test_helper_teardown_ms(&ms);
}

/* 3. Overflow returns NULL */
void test_update_add_overflow(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-UUID-0003", 9020);

    /* Fill updates to capacity */
    microswim_member_t dummies[MAXIMUM_UPDATES + 1];
    for (size_t i = 0; i < MAXIMUM_UPDATES; i++) {
        char uuid[UUID_SIZE];
        test_helper_seed_uuid(uuid, (int)(100 + i));
        dummies[i] = test_helper_make_member(uuid, 9100 + (int)i);
    }
    /* ms already has some updates from init; keep adding until overflow */
    while (ms.update_count <= MAXIMUM_UPDATES) {
        char uuid[UUID_SIZE];
        test_helper_seed_uuid(uuid, (int)(200 + ms.update_count));
        dummies[0] = test_helper_make_member(uuid, 9200 + (int)ms.update_count);
        microswim_update_add(&ms, &dummies[0]);
    }

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 999);
    microswim_member_t overflow = test_helper_make_member(uuid, 9999);
    TEST_ASSERT_NULL(microswim_update_add(&ms, &overflow));

    test_helper_teardown_ms(&ms);
}

/* 4. Retrieve selects least-used first */
void test_updates_retrieve_least_used(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-UUID-0004", 9030);

    char uuid_a[UUID_SIZE], uuid_b[UUID_SIZE];
    test_helper_seed_uuid(uuid_a, 10);
    test_helper_seed_uuid(uuid_b, 11);
    test_helper_add_member(&ms, uuid_a, 9031);
    test_helper_add_member(&ms, uuid_b, 9032);

    /* Bump count of the self update so it sorts later */
    ms.updates[0].count = 100;

    microswim_update_t* out[MAXIMUM_MEMBERS_IN_AN_UPDATE] = { 0 };
    size_t n = microswim_updates_retrieve(&ms, out);
    TEST_ASSERT_TRUE(n >= 2);
    /* The two members with count=0 should appear before the bumped one */
    TEST_ASSERT_EQUAL_UINT(1, out[0]->count); /* incremented by retrieve */
    TEST_ASSERT_EQUAL_UINT(1, out[1]->count);

    test_helper_teardown_ms(&ms);
}

/* 5. Retrieve increments count */
void test_updates_retrieve_increments_count(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-UUID-0005", 9040);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 20);
    microswim_member_t* m = test_helper_add_member(&ms, uuid, 9041);
    microswim_update_t* u = microswim_update_find(&ms, m);
    TEST_ASSERT_EQUAL_UINT(0, u->count);

    microswim_update_t* out[MAXIMUM_MEMBERS_IN_AN_UPDATE] = { 0 };
    microswim_updates_retrieve(&ms, out);
    /* After retrieval the count is 1 */
    TEST_ASSERT_EQUAL_UINT(1, u->count);

    test_helper_teardown_ms(&ms);
}

/* 6. Retrieve skips members with empty UUID */
void test_updates_retrieve_skips_empty_uuid(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-UUID-0006", 9050);

    /* Add a member then blank its UUID to simulate a stale entry */
    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 30);
    microswim_member_t* m = test_helper_add_member(&ms, uuid, 9051);
    m->uuid[0] = '\0';

    /* Blank self UUID too */
    ms.members[0].uuid[0] = '\0';

    microswim_update_t* out[MAXIMUM_MEMBERS_IN_AN_UPDATE] = { 0 };
    size_t n = microswim_updates_retrieve(&ms, out);
    TEST_ASSERT_EQUAL_UINT(0, n);

    test_helper_teardown_ms(&ms);
}

/* 7. Retrieve respects MAXIMUM_MEMBERS_IN_AN_UPDATE limit */
void test_updates_retrieve_respects_max(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-UUID-0007", 9060);

    for (int i = 0; i < MAXIMUM_MEMBERS + 1; i++) {
        char uuid[UUID_SIZE];
        test_helper_seed_uuid(uuid, 40 + i);
        test_helper_add_member(&ms, uuid, 9061 + i);
    }

    microswim_update_t* out[MAXIMUM_MEMBERS_IN_AN_UPDATE] = { 0 };
    size_t n = microswim_updates_retrieve(&ms, out);
    TEST_ASSERT_TRUE(n <= MAXIMUM_MEMBERS_IN_AN_UPDATE);

    test_helper_teardown_ms(&ms);
}
