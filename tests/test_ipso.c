#include "ipso.h"
#include "unity.h"
#include <string.h>

/* ---------- helpers local to this file ---------- */

enum { IPSO_OK = 0, IPSO_ENOENT = -1, IPSO_ERO = -2, IPSO_EINVAL = -3 };

static int64_t g_i64;
static double g_f64;
static uint8_t g_bool;
static char g_str[32];

static ipso_resource_t g_resources[] = {
    { .rid = 5700, .type = IPSO_T_I64, .storage = &g_i64, .storage_len = 0, .set = NULL },
    { .rid = 5701, .type = IPSO_T_F64, .storage = &g_f64, .storage_len = 0, .set = NULL },
    { .rid = 5850, .type = IPSO_T_BOOL, .storage = &g_bool, .storage_len = 0, .set = NULL },
    { .rid = 5750, .type = IPSO_T_STR, .storage = g_str, .storage_len = sizeof(g_str), .set = NULL },
};

static ipso_instance_t g_instances[] = {
    { .oid = 3303, .iid = 0, .resources = g_resources, .resource_count = 4 },
};

static ipso_registry_t g_reg = {
    .instances = g_instances,
    .instance_count = 1,
};

static void reset_globals(void) {
    g_i64 = 0;
    g_f64 = 0.0;
    g_bool = 0;
    memset(g_str, 0, sizeof(g_str));
}

void test_ipso_find_instance_success(void) {
    ipso_instance_t* inst = ipso_find_instance(&g_reg, 3303, 0);
    TEST_ASSERT_NOT_NULL(inst);
    TEST_ASSERT_EQUAL_UINT16(3303, inst->oid);
}

void test_ipso_find_instance_not_found(void) {
    TEST_ASSERT_NULL(ipso_find_instance(&g_reg, 9999, 0));
}

void test_ipso_find_instance_null_reg(void) {
    TEST_ASSERT_NULL(ipso_find_instance(NULL, 3303, 0));
}

void test_ipso_find_resource_success(void) {
    ipso_instance_t* inst = ipso_find_instance(&g_reg, 3303, 0);
    ipso_resource_t* res = ipso_find_resource(inst, 5700);
    TEST_ASSERT_NOT_NULL(res);
    TEST_ASSERT_EQUAL_UINT16(5700, res->rid);
}

void test_ipso_find_resource_not_found(void) {
    ipso_instance_t* inst = ipso_find_instance(&g_reg, 3303, 0);
    TEST_ASSERT_NULL(ipso_find_resource(inst, 1234));
}

void test_ipso_find_resource_null_inst(void) {
    TEST_ASSERT_NULL(ipso_find_resource(NULL, 5700));
}

/* ---------- read ---------- */

/* 7 */
void test_ipso_read_i64(void) {
    reset_globals();
    g_i64 = 42;
    ipso_value_t v;
    int rc = ipso_read(&g_reg, 3303, 0, 5700, &v);
    TEST_ASSERT_EQUAL_INT(IPSO_OK, rc);
    TEST_ASSERT_EQUAL_INT64(42, v.i64);
}

void test_ipso_read_f64(void) {
    reset_globals();
    g_f64 = 3.14;
    ipso_value_t v;
    int rc = ipso_read(&g_reg, 3303, 0, 5701, &v);
    TEST_ASSERT_EQUAL_INT(IPSO_OK, rc);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 3.14, v.f64);
}

void test_ipso_read_bool(void) {
    reset_globals();
    g_bool = 1;
    ipso_value_t v;
    int rc = ipso_read(&g_reg, 3303, 0, 5850, &v);
    TEST_ASSERT_EQUAL_INT(IPSO_OK, rc);
    TEST_ASSERT_EQUAL_UINT8(1, v.b);
}

void test_ipso_read_str(void) {
    reset_globals();
    strcpy(g_str, "hello");
    ipso_value_t v;
    int rc = ipso_read(&g_reg, 3303, 0, 5750, &v);
    TEST_ASSERT_EQUAL_INT(IPSO_OK, rc);
    TEST_ASSERT_EQUAL_PTR(g_str, v.bytes.ptr);
    TEST_ASSERT_EQUAL_UINT(sizeof(g_str), v.bytes.len);
}

void test_ipso_read_not_found(void) {
    ipso_value_t v;
    int rc = ipso_read(&g_reg, 3303, 0, 1234, &v);
    TEST_ASSERT_EQUAL_INT(IPSO_ENOENT, rc);
}

void test_ipso_write_i64(void) {
    reset_globals();
    ipso_value_t v = { .i64 = 99 };
    int rc = ipso_write(&g_reg, 3303, 0, 5700, &v);
    TEST_ASSERT_EQUAL_INT(IPSO_OK, rc);
    TEST_ASSERT_EQUAL_INT64(99, g_i64);
}

void test_ipso_write_str(void) {
    reset_globals();
    const char* text = "world";
    ipso_value_t v = { .bytes = { .ptr = text, .len = 5 } };
    int rc = ipso_write(&g_reg, 3303, 0, 5750, &v);
    TEST_ASSERT_EQUAL_INT(IPSO_OK, rc);
    TEST_ASSERT_EQUAL_STRING_LEN("world", g_str, 5);
}

void test_ipso_write_str_too_long(void) {
    reset_globals();
    char big[64];
    memset(big, 'X', sizeof(big));
    ipso_value_t v = { .bytes = { .ptr = big, .len = sizeof(big) } };
    int rc = ipso_write(&g_reg, 3303, 0, 5750, &v);
    TEST_ASSERT_EQUAL_INT(IPSO_EINVAL, rc);
}

static int custom_set_called;
static int custom_set_handler(ipso_resource_t* res, const ipso_value_t* v) {
    (void)res;
    (void)v;
    custom_set_called = 1;
    return 0;
}

void test_ipso_write_custom_set_handler(void) {
    reset_globals();
    custom_set_called = 0;

    /* Temporarily install a custom set handler */
    ipso_set_fn_t old = g_resources[0].set;
    g_resources[0].set = custom_set_handler;

    ipso_value_t v = { .i64 = 7 };
    int rc = ipso_write(&g_reg, 3303, 0, 5700, &v);
    TEST_ASSERT_EQUAL_INT(IPSO_OK, rc);
    TEST_ASSERT_EQUAL_INT(1, custom_set_called);

    g_resources[0].set = old;
}

void test_ipso_write_default_write(void) {
    reset_globals();
    g_resources[2].set = NULL; /* ensure no set handler on bool */

    ipso_value_t v = { .b = 1 };
    int rc = ipso_write(&g_reg, 3303, 0, 5850, &v);
    TEST_ASSERT_EQUAL_INT(IPSO_OK, rc);
    TEST_ASSERT_EQUAL_UINT8(1, g_bool);
}
