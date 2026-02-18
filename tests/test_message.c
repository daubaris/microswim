#include "constants.h"
#include "member.h"
#include "message.h"
#include "microswim.h"
#include "test_helpers.h"
#include "unity.h"
#include "update.h"
#include <string.h>

/* Construct PING message */
void test_message_construct_ping(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-MSG-01", 15000);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 1);
    test_helper_add_member(&ms, uuid, 15001);

    microswim_update_t* ups[MAXIMUM_MEMBERS_IN_AN_UPDATE] = { 0 };
    size_t n = microswim_updates_retrieve(&ms, ups);

    microswim_message_t msg = { 0 };
    microswim_message_construct(&ms, &msg, PING_MESSAGE, ups, n);

    TEST_ASSERT_EQUAL_INT(PING_MESSAGE, msg.type);
    TEST_ASSERT_EQUAL_STRING("SELF-MSG-01", (char*)msg.uuid);
    TEST_ASSERT_EQUAL_UINT(n, msg.update_count);

    test_helper_teardown_ms(&ms);
}

/* Construct ACK message */
void test_message_construct_ack(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-MSG-02", 15010);

    microswim_update_t* ups[MAXIMUM_MEMBERS_IN_AN_UPDATE] = { 0 };
    size_t n = microswim_updates_retrieve(&ms, ups);

    microswim_message_t msg = { 0 };
    microswim_message_construct(&ms, &msg, ACK_MESSAGE, ups, n);

    TEST_ASSERT_EQUAL_INT(ACK_MESSAGE, msg.type);

    test_helper_teardown_ms(&ms);
}

/* Construct status message */
void test_message_construct_status(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-MSG-03", 15020);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 2);
    microswim_member_t* m = test_helper_add_member(&ms, uuid, 15021);

    microswim_message_t msg = { 0 };
    microswim_status_message_construct(&ms, &msg, SUSPECT_MESSAGE, m);

    TEST_ASSERT_EQUAL_INT(SUSPECT_MESSAGE, msg.type);
    TEST_ASSERT_EQUAL_STRING("SELF-MSG-03", (char*)msg.uuid);
    TEST_ASSERT_EQUAL_UINT(1, msg.update_count);
    TEST_ASSERT_EQUAL_STRING(uuid, (char*)msg.mu[0].uuid);

    test_helper_teardown_ms(&ms);
}

/* Extract members adds new member */
void test_message_extract_adds_new(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-MSG-04", 15030);

    microswim_message_t msg = { 0 };
    char uuid_sender[UUID_SIZE];
    test_helper_seed_uuid(uuid_sender, 3);
    strncpy((char*)msg.uuid, uuid_sender, UUID_SIZE);
    msg.addr.sin_family = AF_INET;
    msg.addr.sin_port = htons(15031);
    inet_pton(AF_INET, "127.0.0.1", &msg.addr.sin_addr);
    msg.status = ALIVE;
    msg.incarnation = 0;
    msg.update_count = 0;

    size_t before = ms.member_count;
    /* microswim_message_extract_members is static, use microswim_members_check instead */
    microswim_member_t sender = { 0 };
    strncpy((char*)sender.uuid, uuid_sender, UUID_SIZE);
    sender.addr = msg.addr;
    sender.status = msg.status;
    sender.incarnation = msg.incarnation;
    microswim_members_check(&ms, &sender);

    TEST_ASSERT_EQUAL_UINT(before + 1, ms.member_count);

    test_helper_teardown_ms(&ms);
}

/* Extract members updates existing */
void test_message_extract_updates_existing(void) {
    microswim_t ms = { 0 };
    test_helper_init_ms(&ms, "SELF-MSG-05", 15040);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 4);
    microswim_member_t* m = test_helper_add_member(&ms, uuid, 15041);
    m->incarnation = 1;

    microswim_member_t updated = test_helper_make_member(uuid, 15041);
    updated.incarnation = 5;

    microswim_members_check(&ms, &updated);
    TEST_ASSERT_EQUAL_UINT(5, m->incarnation);

    test_helper_teardown_ms(&ms);
}

/* Send on invalid socket doesn't crash */
void test_message_send_invalid_socket(void) {
    microswim_t ms = { 0 };
    ms.socket = -1;
    strncpy((char*)ms.self.uuid, "SELF-MSG-06", UUID_SIZE);

    char uuid[UUID_SIZE];
    test_helper_seed_uuid(uuid, 5);
    microswim_member_t m = test_helper_make_member(uuid, 15050);

    /* Should not crash -- sendto returns EBADF */
    microswim_message_send(&ms, &m, "test", 4);
}
