#include "microswim.h"
#include "test_helpers.h"
#include "unity.h"
#include <string.h>

/* Single member index insertion */
void test_index_add_single(void) {
    microswim_t ms = { 0 };
    ms.member_count = 1;
    microswim_index_add(&ms);
    TEST_ASSERT_EQUAL_UINT(0, ms.indices[0]);
}

/* Multiple members, all indices present */
void test_index_add_multiple(void) {
    microswim_t ms = { 0 };
    for (size_t i = 0; i < 4; i++) {
        ms.member_count = i + 1;
        microswim_index_add(&ms);
    }

    /* Every value 0..3 must appear exactly once */
    int seen[4] = { 0 };
    for (size_t i = 0; i < 4; i++) {
        TEST_ASSERT_TRUE(ms.indices[i] < 4);
        seen[ms.indices[i]]++;
    }
    for (size_t i = 0; i < 4; i++) {
        TEST_ASSERT_EQUAL_INT(1, seen[i]);
    }
}

/* Highest-value index removed */
void test_index_remove_removes_highest(void) {
    microswim_t ms = { 0 };
    ms.member_count = 3;
    ms.indices[0] = 1;
    ms.indices[1] = 2;
    ms.indices[2] = 0;

    /* member_count is used as "count+1" in index_remove; set it to size-1 */
    ms.member_count = 2; /* index_remove reads member_count+1 entries */
    microswim_index_remove(&ms);

    /* Index with value 2 (the highest) should be gone */
    for (size_t i = 0; i < 2; i++) {
        TEST_ASSERT_TRUE(ms.indices[i] <= 1);
    }
}

/* Right-shift inserts at position */
void test_indices_shift(void) {
    microswim_t ms = { 0 };
    ms.member_count = 4;
    ms.indices[0] = 10;
    ms.indices[1] = 20;
    ms.indices[2] = 30;
    ms.indices[3] = 0; /* will be overwritten */

    size_t* pos = microswim_indices_shift(&ms, 1);
    *pos = 99;

    TEST_ASSERT_EQUAL_UINT(10, ms.indices[0]);
    TEST_ASSERT_EQUAL_UINT(99, ms.indices[1]);
    TEST_ASSERT_EQUAL_UINT(20, ms.indices[2]);
    TEST_ASSERT_EQUAL_UINT(30, ms.indices[3]);
}

/* Fisher-Yates preserves element set */
void test_indices_shuffle_preserves_set(void) {
    microswim_t ms = { 0 };
    ms.member_count = 5;
    for (size_t i = 0; i < 5; i++)
        ms.indices[i] = i;

    microswim_indices_shuffle(&ms);

    int seen[5] = { 0 };
    for (size_t i = 0; i < 5; i++) {
        TEST_ASSERT_TRUE(ms.indices[i] < 5);
        seen[ms.indices[i]]++;
    }
    for (size_t i = 0; i < 5; i++) {
        TEST_ASSERT_EQUAL_INT(1, seen[i]);
    }
}

/* No crash on empty */
void test_index_remove_empty(void) {
    microswim_t ms = { 0 };
    ms.member_count = 0;
    /* (member_count + 1) == 1, so the guard `(ms->member_count + 1) == 0`
       does not trigger; but the loop body has 0 iterations, so nothing breaks. */
    microswim_index_remove(&ms);
    TEST_ASSERT_EQUAL_UINT(0, ms.member_count);
}
