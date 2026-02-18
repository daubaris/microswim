#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include "microswim.h"

void test_helper_init_ms(microswim_t* ms, const char* uuid, int port);
microswim_member_t test_helper_make_member(const char* uuid, int port);
microswim_member_t* test_helper_add_member(microswim_t* ms, const char* uuid, int port);
void test_helper_seed_uuid(char* buf, int seed);
void test_helper_teardown_ms(microswim_t* ms);

#endif
