#include "constants.h"
#include "member.h"
#include "microswim.h"
#include "ping.h"
#include "test_helpers.h"
#include "unity.h"
#include "update.h"
#include "utils.h"
#include <string.h>
#include <unistd.h>

void test_member_add_and_find(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-M-0001", 10000);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 1);
    microswim_member_t m = test_helper_make_member(uuid, 10001);
    microswim_member_t* added = microswim_member_add(&ms, m);
    TEST_ASSERT_NOT_NULL(added);

    microswim_member_t* found = microswim_member_find(&ms, &m);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_STRING(uuid, (char*)found->uuid);

    test_helper_teardown_ms(&ms);
}

void test_member_add_overflow(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-M-0002", 10010);

    for (int i = 0; i < MAXIMUM_MEMBERS; i++) {
        char uuid[UUID_SIZE];
        test_helper_seed_uuid(uuid, 100 + i);
        microswim_member_t m = test_helper_make_member(uuid, 10100 + i);
        microswim_member_add(&ms, m);
    }

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 999);
    microswim_member_t overflow = test_helper_make_member(uuid, 19999);
    TEST_ASSERT_NULL(microswim_member_add(&ms, overflow));

    test_helper_teardown_ms(&ms);
}

void test_member_find_unknown(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-M-0003", 10020);

    microswim_member_t needle = test_helper_make_member("UNKNOWN-0000", 19998);
    TEST_ASSERT_NULL(microswim_member_find(&ms, &needle));

    test_helper_teardown_ms(&ms);
}

void test_member_find_updates_uuid_from_address(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-M-0004", 10030);

    /* Add member with empty UUID but valid address */
    microswim_member_t m = test_helper_make_member("", 10031);
    microswim_member_add(&ms, m);

    /* Now search with the same address but with a UUID */
    microswim_member_t search = test_helper_make_member("NEW-UUID-0001", 10031);
    microswim_member_t* found = microswim_member_find(&ms, &search);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_STRING("NEW-UUID-0001", (char*)found->uuid);

    test_helper_teardown_ms(&ms);
}

void test_member_address_compare_full(void) {
    microswim_member_t a = test_helper_make_member("A", 5000);
    microswim_member_t b = test_helper_make_member("B", 5000);
    size_t r = microswim_member_address_compare(&a, &b);
    TEST_ASSERT_EQUAL_UINT(SIN_FAMILY | SIN_PORT | SIN_ADDR, r);
}

void test_member_address_compare_partial(void) {
    microswim_member_t a = test_helper_make_member("A", 5000);
    microswim_member_t b = test_helper_make_member("B", 6000);
    size_t r = microswim_member_address_compare(&a, &b);
    TEST_ASSERT_EQUAL_UINT(SIN_FAMILY | SIN_ADDR, r);
}

void test_member_confirmed_add_and_find(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-M-0007", 10040);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 7);
    microswim_member_t m = test_helper_make_member(uuid, 10041);
    m.status = CONFIRMED;
    microswim_member_confirmed_add(&ms, m);

    TEST_ASSERT_NOT_NULL(microswim_member_confirmed_find(&ms, &m));

    test_helper_teardown_ms(&ms);
}

void test_member_move_to_confirmed(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-M-0008", 10050);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 8);
    microswim_member_t* added = test_helper_add_member(&ms, uuid, 10051);
    TEST_ASSERT_NOT_NULL(added);

    size_t before = ms.member_count;
    microswim_member_t* moved = microswim_member_move(&ms, added);
    TEST_ASSERT_NOT_NULL(moved);
    TEST_ASSERT_EQUAL_UINT(before - 1, ms.member_count);
    TEST_ASSERT_EQUAL_UINT(1, ms.confirmed_count);

    test_helper_teardown_ms(&ms);
}

void test_member_move_updates_pointer(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-M-0009", 10060);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 9);
    microswim_member_t* added = test_helper_add_member(&ms, uuid, 10061);

    microswim_member_t* moved = microswim_member_move(&ms, added);
    TEST_ASSERT_NOT_NULL(moved);

    /* The update should now point to the confirmed copy */
    microswim_update_t* u = microswim_update_find(&ms, moved);
    TEST_ASSERT_NOT_NULL(u);
    TEST_ASSERT_EQUAL_PTR(moved, u->member);

    test_helper_teardown_ms(&ms);
}

void test_members_shift(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-M-0010", 10070);

    char uuid_a[UUID_SIZE], uuid_b[UUID_SIZE];
    test_helper_seed_uuid(uuid_a, 10);
    test_helper_seed_uuid(uuid_b, 11);
    test_helper_add_member(&ms, uuid_a, 10071);
    test_helper_add_member(&ms, uuid_b, 10072);

    size_t before = ms.member_count;
    /* Remove the member at index 1 (uuid_a) */
    microswim_members_shift(&ms, 1);
    TEST_ASSERT_EQUAL_UINT(before - 1, ms.member_count);

    test_helper_teardown_ms(&ms);
}

void test_member_retrieve_skips_self(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-M-RR01", 10080);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 20);
    test_helper_add_member(&ms, uuid, 10081);

    microswim_member_t* m = microswim_member_retrieve(&ms);
    TEST_ASSERT_NOT_NULL(m);
    TEST_ASSERT_NOT_EQUAL(0, strncmp((char*)ms.self.uuid, (char*)m->uuid, UUID_SIZE));

    test_helper_teardown_ms(&ms);
}

void test_member_retrieve_empty(void) {
    microswim_t ms = { 0 };
    TEST_ASSERT_NULL(microswim_member_retrieve(&ms));
}

void test_member_retrieve_only_self(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-M-RR03", 10090);

    /* Only self in members list */
    microswim_member_t* m = microswim_member_retrieve(&ms);
    TEST_ASSERT_NULL(m);

    test_helper_teardown_ms(&ms);
}

void test_member_retrieve_shuffles_at_cycle_end(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-M-RR04", 10100);

    for (int i = 0; i < 3; i++) {
        char uuid[UUID_SIZE];
        test_helper_seed_uuid(uuid, 30 + i);
        test_helper_add_member(&ms, uuid, 10101 + i);
    }

    /* Exhaust one full round-robin cycle */
    for (size_t i = 0; i < ms.member_count; i++) {
        microswim_member_retrieve(&ms);
    }

    /* After a full cycle, round_robin_index wraps and triggers shuffle.
       Just verify we still get valid members. */
    microswim_member_t* m = microswim_member_retrieve(&ms);
    TEST_ASSERT_NOT_NULL(m);

    test_helper_teardown_ms(&ms);
}

void test_member_update_alive_overrides_suspect_higher_inc(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-ST-01", 11000);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 50);
    microswim_member_t* ex = test_helper_add_member(&ms, uuid, 11001);
    ex->status = SUSPECT;
    ex->incarnation = 1;

    microswim_member_t nw = test_helper_make_member(uuid, 11001);
    nw.status = ALIVE;
    nw.incarnation = 2;

    microswim_member_update(&ms, ex, &nw);
    TEST_ASSERT_EQUAL_INT(ALIVE, ex->status);
    TEST_ASSERT_EQUAL_UINT(2, ex->incarnation);

    test_helper_teardown_ms(&ms);
}

void test_member_update_alive_no_override_suspect_lower_inc(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-ST-02", 11010);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 51);
    microswim_member_t* ex = test_helper_add_member(&ms, uuid, 11011);
    ex->status = SUSPECT;
    ex->incarnation = 3;

    microswim_member_t nw = test_helper_make_member(uuid, 11011);
    nw.status = ALIVE;
    nw.incarnation = 2;

    microswim_member_update(&ms, ex, &nw);
    TEST_ASSERT_EQUAL_INT(SUSPECT, ex->status);
    TEST_ASSERT_EQUAL_UINT(3, ex->incarnation);

    test_helper_teardown_ms(&ms);
}

void test_member_update_alive_overrides_alive_higher_inc(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-ST-03", 11020);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 52);
    microswim_member_t* ex = test_helper_add_member(&ms, uuid, 11021);
    ex->status = ALIVE;
    ex->incarnation = 1;

    microswim_member_t nw = test_helper_make_member(uuid, 11021);
    nw.status = ALIVE;
    nw.incarnation = 5;

    microswim_member_update(&ms, ex, &nw);
    TEST_ASSERT_EQUAL_INT(ALIVE, ex->status);
    TEST_ASSERT_EQUAL_UINT(5, ex->incarnation);

    test_helper_teardown_ms(&ms);
}

void test_member_update_alive_no_override_alive_same_inc(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-ST-04", 11030);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 53);
    microswim_member_t* ex = test_helper_add_member(&ms, uuid, 11031);
    ex->status = ALIVE;
    ex->incarnation = 3;

    microswim_member_t nw = test_helper_make_member(uuid, 11031);
    nw.status = ALIVE;
    nw.incarnation = 3;

    microswim_member_update(&ms, ex, &nw);
    TEST_ASSERT_EQUAL_INT(ALIVE, ex->status);
    TEST_ASSERT_EQUAL_UINT(3, ex->incarnation);

    test_helper_teardown_ms(&ms);
}

void test_member_update_suspect_overrides_alive_equal_inc(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-ST-05", 11040);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 54);
    microswim_member_t* ex = test_helper_add_member(&ms, uuid, 11041);
    ex->status = ALIVE;
    ex->incarnation = 2;

    microswim_member_t nw = test_helper_make_member(uuid, 11041);
    nw.status = SUSPECT;
    nw.incarnation = 2;

    microswim_member_update(&ms, ex, &nw);
    TEST_ASSERT_EQUAL_INT(SUSPECT, ex->status);
    TEST_ASSERT_EQUAL_UINT(2, ex->incarnation);

    test_helper_teardown_ms(&ms);
}

void test_member_update_suspect_overrides_alive_higher_inc(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-ST-06", 11050);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 55);
    microswim_member_t* ex = test_helper_add_member(&ms, uuid, 11051);
    ex->status = ALIVE;
    ex->incarnation = 1;

    microswim_member_t nw = test_helper_make_member(uuid, 11051);
    nw.status = SUSPECT;
    nw.incarnation = 3;

    microswim_member_update(&ms, ex, &nw);
    TEST_ASSERT_EQUAL_INT(SUSPECT, ex->status);
    TEST_ASSERT_EQUAL_UINT(3, ex->incarnation);

    test_helper_teardown_ms(&ms);
}

void test_member_update_suspect_no_override_alive_lower_inc(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-ST-07", 11060);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 56);
    microswim_member_t* ex = test_helper_add_member(&ms, uuid, 11061);
    ex->status = ALIVE;
    ex->incarnation = 5;

    microswim_member_t nw = test_helper_make_member(uuid, 11061);
    nw.status = SUSPECT;
    nw.incarnation = 3;

    microswim_member_update(&ms, ex, &nw);
    TEST_ASSERT_EQUAL_INT(ALIVE, ex->status);
    TEST_ASSERT_EQUAL_UINT(5, ex->incarnation);

    test_helper_teardown_ms(&ms);
}

void test_member_update_suspect_overrides_suspect_higher_inc(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-ST-08", 11070);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 57);
    microswim_member_t* ex = test_helper_add_member(&ms, uuid, 11071);
    ex->status = SUSPECT;
    ex->incarnation = 1;

    microswim_member_t nw = test_helper_make_member(uuid, 11071);
    nw.status = SUSPECT;
    nw.incarnation = 4;

    microswim_member_update(&ms, ex, &nw);
    TEST_ASSERT_EQUAL_INT(SUSPECT, ex->status);
    TEST_ASSERT_EQUAL_UINT(4, ex->incarnation);

    test_helper_teardown_ms(&ms);
}

void test_member_update_suspect_no_override_suspect_same_inc(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-ST-09", 11080);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 58);
    microswim_member_t* ex = test_helper_add_member(&ms, uuid, 11081);
    ex->status = SUSPECT;
    ex->incarnation = 2;

    microswim_member_t nw = test_helper_make_member(uuid, 11081);
    nw.status = SUSPECT;
    nw.incarnation = 2;

    microswim_member_update(&ms, ex, &nw);
    TEST_ASSERT_EQUAL_INT(SUSPECT, ex->status);
    TEST_ASSERT_EQUAL_UINT(2, ex->incarnation);

    test_helper_teardown_ms(&ms);
}

void test_member_update_confirmed_overrides_alive(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-ST-10", 11090);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 59);
    microswim_member_t* ex = test_helper_add_member(&ms, uuid, 11091);
    ex->status = ALIVE;

    microswim_member_t nw = test_helper_make_member(uuid, 11091);
    nw.status = CONFIRMED;

    size_t before_confirmed = ms.confirmed_count;
    microswim_member_update(&ms, ex, &nw);
    TEST_ASSERT_GREATER_THAN(before_confirmed, ms.confirmed_count);

    test_helper_teardown_ms(&ms);
}

void test_member_update_confirmed_overrides_suspect(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-ST-11", 11100);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 60);
    microswim_member_t* ex = test_helper_add_member(&ms, uuid, 11101);
    ex->status = SUSPECT;

    microswim_member_t nw = test_helper_make_member(uuid, 11101);
    nw.status = CONFIRMED;

    size_t before_confirmed = ms.confirmed_count;
    microswim_member_update(&ms, ex, &nw);
    TEST_ASSERT_GREATER_THAN(before_confirmed, ms.confirmed_count);

    test_helper_teardown_ms(&ms);
}

void test_member_update_self_suspect_refutation(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-ST-12", 11110);

    /* Add another member so retrieve has someone to send to */
    char other[UUID_SIZE];
    test_helper_seed_uuid(other, 61);
    test_helper_add_member(&ms, other, 11111);

    /* ex is the self entry in the members array */
    microswim_member_t* ex = &ms.members[0];
    TEST_ASSERT_EQUAL_STRING("SELF-ST-12", (char*)ex->uuid);

    microswim_member_t nw = test_helper_make_member("SELF-ST-12", 11110);
    nw.status = SUSPECT;
    nw.incarnation = 0;

    microswim_member_update(&ms, ex, &nw);
    TEST_ASSERT_EQUAL_INT(ALIVE, ex->status);
    TEST_ASSERT_EQUAL_UINT(1, ex->incarnation);
    TEST_ASSERT_EQUAL_INT(ALIVE, ms.self.status);

    test_helper_teardown_ms(&ms);
}

void test_member_mark_alive(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-MK-01", 12000);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 70);
    microswim_member_t* m = test_helper_add_member(&ms, uuid, 12001);
    m->status = SUSPECT;

    microswim_member_mark_alive(&ms, m);
    TEST_ASSERT_EQUAL_INT(ALIVE, m->status);
    TEST_ASSERT_TRUE(m->timeout > 0);

    test_helper_teardown_ms(&ms);
}

void test_member_mark_suspect(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-MK-02", 12010);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 71);
    microswim_member_t* m = test_helper_add_member(&ms, uuid, 12011);
    m->status = ALIVE;

    microswim_member_mark_suspect(&ms, m);
    TEST_ASSERT_EQUAL_INT(SUSPECT, m->status);
    TEST_ASSERT_TRUE(m->timeout > 0);

    test_helper_teardown_ms(&ms);
}

void test_member_mark_suspect_no_op_if_already_suspect(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-MK-03", 12020);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 72);
    microswim_member_t* m = test_helper_add_member(&ms, uuid, 12021);
    m->status = SUSPECT;
    uint64_t old_timeout = m->timeout;

    microswim_member_mark_suspect(&ms, m);
    TEST_ASSERT_EQUAL_INT(SUSPECT, m->status);
    TEST_ASSERT_EQUAL_UINT64(old_timeout, m->timeout);

    test_helper_teardown_ms(&ms);
}

void test_member_mark_confirmed(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-MK-04", 12030);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 73);
    microswim_member_t* m = test_helper_add_member(&ms, uuid, 12031);

    size_t before = ms.confirmed_count;
    microswim_member_mark_confirmed(&ms, m);
    TEST_ASSERT_GREATER_THAN(before, ms.confirmed_count);

    test_helper_teardown_ms(&ms);
}

void test_members_check_suspects_timeout(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-SU-01", 12040);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 80);
    microswim_member_t* m = test_helper_add_member(&ms, uuid, 12041);
    m->status = SUSPECT;
    m->timeout = 1; /* already expired */

    size_t before = ms.confirmed_count;
    microswim_members_check_suspects(&ms);
    TEST_ASSERT_GREATER_THAN(before, ms.confirmed_count);

    test_helper_teardown_ms(&ms);
}

void test_members_check_suspects_no_timeout(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-SU-02", 12050);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 81);
    microswim_member_t* m = test_helper_add_member(&ms, uuid, 12051);
    m->status = SUSPECT;
    m->timeout = microswim_milliseconds() + 60000;

    size_t members_before = ms.member_count;
    microswim_members_check_suspects(&ms);
    TEST_ASSERT_EQUAL_UINT(members_before, ms.member_count);

    test_helper_teardown_ms(&ms);
}

void test_members_check_adds_new_member(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-MC-01", 12060);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 90);
    microswim_member_t m = test_helper_make_member(uuid, 12061);

    size_t before = ms.member_count;
    microswim_members_check(&ms, &m);
    TEST_ASSERT_EQUAL_UINT(before + 1, ms.member_count);

    test_helper_teardown_ms(&ms);
}

void test_members_check_adds_confirmed_member(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-MC-02", 12070);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 91);
    microswim_member_t m = test_helper_make_member(uuid, 12071);
    m.status = CONFIRMED;

    size_t before = ms.confirmed_count;
    microswim_members_check(&ms, &m);
    TEST_ASSERT_EQUAL_UINT(before + 1, ms.confirmed_count);

    test_helper_teardown_ms(&ms);
}

void test_members_check_updates_existing(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-MC-03", 12080);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 92);
    microswim_member_t* ex = test_helper_add_member(&ms, uuid, 12081);
    ex->status = ALIVE;
    ex->incarnation = 1;

    microswim_member_t nw = test_helper_make_member(uuid, 12081);
    nw.status = ALIVE;
    nw.incarnation = 5;

    microswim_members_check(&ms, &nw);
    TEST_ASSERT_EQUAL_UINT(5, ex->incarnation);

    test_helper_teardown_ms(&ms);
}

void test_get_ping_req_candidates(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-PRC-01", 12090);

    for (int i = 0; i < 4; i++) {
        char uuid[UUID_SIZE];
        test_helper_seed_uuid(uuid, 200 + i);
        test_helper_add_member(&ms, uuid, 12091 + i);
    }

    size_t candidates[FAILURE_DETECTION_GROUP] = { 0 };
    size_t n = microswim_get_ping_req_candidates(&ms, candidates);
    TEST_ASSERT_TRUE(n > 0);
    TEST_ASSERT_TRUE(n <= FAILURE_DETECTION_GROUP);

    test_helper_teardown_ms(&ms);
}

void test_get_ping_req_candidates_unique(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-PRC-02", 12100);

    for (int i = 0; i < 5; i++) {
        char uuid[UUID_SIZE];
        test_helper_seed_uuid(uuid, 210 + i);
        test_helper_add_member(&ms, uuid, 12101 + i);
    }

    size_t candidates[FAILURE_DETECTION_GROUP] = { 0 };
    size_t n = microswim_get_ping_req_candidates(&ms, candidates);

    /* All returned indices should be unique */
    for (size_t i = 0; i < n; i++) {
        for (size_t j = i + 1; j < n; j++) {
            TEST_ASSERT_NOT_EQUAL(candidates[i], candidates[j]);
        }
    }

    test_helper_teardown_ms(&ms);
}
