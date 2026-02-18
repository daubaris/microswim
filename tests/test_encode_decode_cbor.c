#ifdef MICROSWIM_CBOR

#include "decode.h"
#include "encode.h"
#include "microswim.h"
#include "test_helpers.h"
#include "unity.h"
#include <arpa/inet.h>
#include <string.h>

/* Encode/decode PING, verify fields survive round-trip */
void test_cbor_roundtrip_ping(void) {
    microswim_message_t orig = { 0 };
    orig.type = PING_MESSAGE;
    strncpy((char*)orig.uuid, "CBOR-TEST-UUID-01", UUID_SIZE);
    orig.addr.sin_family = AF_INET;
    orig.addr.sin_port = htons(7000);
    inet_pton(AF_INET, "127.0.0.1", &orig.addr.sin_addr);
    orig.status = ALIVE;
    orig.incarnation = 3;
    orig.update_count = 0;

    unsigned char buffer[BUFFER_SIZE] = { 0 };
    size_t len = microswim_encode_message(&orig, buffer, BUFFER_SIZE);
    TEST_ASSERT_TRUE(len > 0);

    microswim_message_type_t type = microswim_decode_message_type(buffer, (ssize_t)len);
    TEST_ASSERT_EQUAL_INT(PING_MESSAGE, type);

    microswim_message_t decoded = { 0 };
    microswim_decode_message(&decoded, (const char*)buffer, (ssize_t)len);

    TEST_ASSERT_EQUAL_INT(PING_MESSAGE, decoded.type);
    TEST_ASSERT_EQUAL_STRING("CBOR-TEST-UUID-01", (char*)decoded.uuid);
    TEST_ASSERT_EQUAL_INT(ALIVE, decoded.status);
    TEST_ASSERT_EQUAL_UINT(3, decoded.incarnation);
    TEST_ASSERT_EQUAL_UINT(0, decoded.update_count);
}

/* Encode/decode ACK with updates */
void test_cbor_roundtrip_ack_with_updates(void) {
    microswim_message_t orig = { 0 };
    orig.type = ACK_MESSAGE;
    strncpy((char*)orig.uuid, "CBOR-TEST-UUID-02", UUID_SIZE);
    orig.addr.sin_family = AF_INET;
    orig.addr.sin_port = htons(7001);
    inet_pton(AF_INET, "127.0.0.1", &orig.addr.sin_addr);
    orig.status = ALIVE;
    orig.incarnation = 0;

    /* Add two updates */
    strncpy((char*)orig.mu[0].uuid, "UPDATE-MEMBER-01", UUID_SIZE);
    orig.mu[0].addr.sin_family = AF_INET;
    orig.mu[0].addr.sin_port = htons(7010);
    inet_pton(AF_INET, "127.0.0.1", &orig.mu[0].addr.sin_addr);
    orig.mu[0].status = ALIVE;
    orig.mu[0].incarnation = 1;

    strncpy((char*)orig.mu[1].uuid, "UPDATE-MEMBER-02", UUID_SIZE);
    orig.mu[1].addr.sin_family = AF_INET;
    orig.mu[1].addr.sin_port = htons(7011);
    inet_pton(AF_INET, "127.0.0.1", &orig.mu[1].addr.sin_addr);
    orig.mu[1].status = SUSPECT;
    orig.mu[1].incarnation = 2;

    orig.update_count = 2;

    unsigned char buffer[BUFFER_SIZE] = { 0 };
    size_t len = microswim_encode_message(&orig, buffer, BUFFER_SIZE);
    TEST_ASSERT_TRUE(len > 0);

    microswim_message_t decoded = { 0 };
    microswim_decode_message(&decoded, (const char*)buffer, (ssize_t)len);

    TEST_ASSERT_EQUAL_INT(ACK_MESSAGE, decoded.type);
    TEST_ASSERT_EQUAL_UINT(2, decoded.update_count);

    TEST_ASSERT_EQUAL_STRING("UPDATE-MEMBER-01", (char*)decoded.mu[0].uuid);
    TEST_ASSERT_EQUAL_INT(ALIVE, decoded.mu[0].status);
    TEST_ASSERT_EQUAL_UINT(1, decoded.mu[0].incarnation);

    TEST_ASSERT_EQUAL_STRING("UPDATE-MEMBER-02", (char*)decoded.mu[1].uuid);
    TEST_ASSERT_EQUAL_INT(SUSPECT, decoded.mu[1].status);
    TEST_ASSERT_EQUAL_UINT(2, decoded.mu[1].incarnation);
}

/* Encode/decode SUSPECT message */
void test_cbor_roundtrip_suspect(void) {
    microswim_message_t orig = { 0 };
    orig.type = SUSPECT_MESSAGE;
    strncpy((char*)orig.uuid, "CBOR-TEST-UUID-03", UUID_SIZE);
    orig.addr.sin_family = AF_INET;
    orig.addr.sin_port = htons(7002);
    inet_pton(AF_INET, "127.0.0.1", &orig.addr.sin_addr);
    orig.status = SUSPECT;
    orig.incarnation = 5;

    strncpy((char*)orig.mu[0].uuid, "SUSPECT-TARGET-01", UUID_SIZE);
    orig.mu[0].addr.sin_family = AF_INET;
    orig.mu[0].addr.sin_port = htons(7020);
    inet_pton(AF_INET, "127.0.0.1", &orig.mu[0].addr.sin_addr);
    orig.mu[0].status = SUSPECT;
    orig.mu[0].incarnation = 4;
    orig.update_count = 1;

    unsigned char buffer[BUFFER_SIZE] = { 0 };
    size_t len = microswim_encode_message(&orig, buffer, BUFFER_SIZE);
    TEST_ASSERT_TRUE(len > 0);

    microswim_message_t decoded = { 0 };
    microswim_decode_message(&decoded, (const char*)buffer, (ssize_t)len);

    TEST_ASSERT_EQUAL_INT(SUSPECT_MESSAGE, decoded.type);
    TEST_ASSERT_EQUAL_INT(SUSPECT, decoded.status);
    TEST_ASSERT_EQUAL_UINT(5, decoded.incarnation);
    TEST_ASSERT_EQUAL_STRING("SUSPECT-TARGET-01", (char*)decoded.mu[0].uuid);
}

/* Malformed buffer returns MALFORMED_MESSAGE */
void test_cbor_malformed_buffer(void) {
    unsigned char garbage[] = { 0xFF, 0xFE, 0x00, 0x01 };
    microswim_message_type_t type = microswim_decode_message_type(garbage, sizeof(garbage));
    TEST_ASSERT_EQUAL_INT(MALFORMED_MESSAGE, type);
}

/* Empty updates array round-trip */
void test_cbor_roundtrip_empty_updates(void) {
    microswim_message_t orig = { 0 };
    orig.type = ACK_MESSAGE;
    strncpy((char*)orig.uuid, "CBOR-TEST-UUID-05", UUID_SIZE);
    orig.addr.sin_family = AF_INET;
    orig.addr.sin_port = htons(7005);
    inet_pton(AF_INET, "127.0.0.1", &orig.addr.sin_addr);
    orig.status = ALIVE;
    orig.incarnation = 0;
    orig.update_count = 0;

    unsigned char buffer[BUFFER_SIZE] = { 0 };
    size_t len = microswim_encode_message(&orig, buffer, BUFFER_SIZE);
    TEST_ASSERT_TRUE(len > 0);

    microswim_message_t decoded = { 0 };
    microswim_decode_message(&decoded, (const char*)buffer, (ssize_t)len);

    TEST_ASSERT_EQUAL_INT(ACK_MESSAGE, decoded.type);
    TEST_ASSERT_EQUAL_UINT(0, decoded.update_count);
}

/* CBOR round-trip with IPSO objects on origin and updates */
void test_cbor_roundtrip_with_ipso_objects(void) {
    microswim_message_t orig = { 0 };
    orig.type = PING_MESSAGE;
    strncpy((char*)orig.uuid, "CBOR-IPSO-01", UUID_SIZE);
    orig.addr.sin_family = AF_INET;
    orig.addr.sin_port = htons(7100);
    inet_pton(AF_INET, "127.0.0.1", &orig.addr.sin_addr);
    orig.status = ALIVE;
    orig.incarnation = 1;

    /* Origin IPSO objects */
    orig.ipso_object_count = 2;
    orig.ipso_objects[0] = (ipso_object_id_t){ .oid = 3303, .iid = 0 };
    orig.ipso_objects[1] = (ipso_object_id_t){ .oid = 3311, .iid = 1 };

    /* One update member with IPSO objects */
    strncpy((char*)orig.mu[0].uuid, "UPDATE-IPSO-01", UUID_SIZE);
    orig.mu[0].addr.sin_family = AF_INET;
    orig.mu[0].addr.sin_port = htons(7101);
    inet_pton(AF_INET, "127.0.0.1", &orig.mu[0].addr.sin_addr);
    orig.mu[0].status = ALIVE;
    orig.mu[0].incarnation = 0;
    orig.mu[0].ipso_object_count = 1;
    orig.mu[0].ipso_objects[0] = (ipso_object_id_t){ .oid = 3304, .iid = 0 };
    orig.update_count = 1;

    unsigned char buffer[BUFFER_SIZE] = { 0 };
    size_t len = microswim_encode_message(&orig, buffer, BUFFER_SIZE);
    TEST_ASSERT_TRUE(len > 0);

    microswim_message_t decoded = { 0 };
    microswim_decode_message(&decoded, (const char*)buffer, (ssize_t)len);

    /* Verify origin IPSO */
    TEST_ASSERT_EQUAL_UINT(2, decoded.ipso_object_count);
    TEST_ASSERT_EQUAL_UINT16(3303, decoded.ipso_objects[0].oid);
    TEST_ASSERT_EQUAL_UINT16(0, decoded.ipso_objects[0].iid);
    TEST_ASSERT_EQUAL_UINT16(3311, decoded.ipso_objects[1].oid);
    TEST_ASSERT_EQUAL_UINT16(1, decoded.ipso_objects[1].iid);

    /* Verify update member IPSO */
    TEST_ASSERT_EQUAL_UINT(1, decoded.update_count);
    TEST_ASSERT_EQUAL_UINT(1, decoded.mu[0].ipso_object_count);
    TEST_ASSERT_EQUAL_UINT16(3304, decoded.mu[0].ipso_objects[0].oid);
    TEST_ASSERT_EQUAL_UINT16(0, decoded.mu[0].ipso_objects[0].iid);
}

/* CBOR round-trip with empty IPSO objects */
void test_cbor_roundtrip_empty_ipso(void) {
    microswim_message_t orig = { 0 };
    orig.type = ACK_MESSAGE;
    strncpy((char*)orig.uuid, "CBOR-IPSO-02", UUID_SIZE);
    orig.addr.sin_family = AF_INET;
    orig.addr.sin_port = htons(7110);
    inet_pton(AF_INET, "127.0.0.1", &orig.addr.sin_addr);
    orig.status = ALIVE;
    orig.incarnation = 0;
    orig.ipso_object_count = 0;
    orig.update_count = 0;

    unsigned char buffer[BUFFER_SIZE] = { 0 };
    size_t len = microswim_encode_message(&orig, buffer, BUFFER_SIZE);
    TEST_ASSERT_TRUE(len > 0);

    microswim_message_t decoded = { 0 };
    microswim_decode_message(&decoded, (const char*)buffer, (ssize_t)len);

    TEST_ASSERT_EQUAL_UINT(0, decoded.ipso_object_count);
}

#endif /* MICROSWIM_CBOR */
