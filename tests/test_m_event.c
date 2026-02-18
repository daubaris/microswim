#include "m_event.h"
#include "microswim.h"
#include "unity.h"
#include <string.h>

static size_t dummy_encoder(void* output, void* input, size_t size) {
    (void)output;
    (void)input;
    (void)size;
    return 0;
}

static void dummy_decoder(void* output, void* input, size_t size) {
    (void)output;
    (void)input;
    (void)size;
}

static void dummy_handler(void* ms, void* buffer, size_t length) {
    (void)ms;
    (void)buffer;
    (void)length;
}

/* Register event, verify stored */
void test_event_register(void) {
    microswim_t ms = { 0 };

    microswim_event_t event = {
        .type = 0,
        .size = 42,
        .encoder = dummy_encoder,
        .decoder = dummy_decoder,
        .handler = dummy_handler,
    };

    microswim_event_register(&ms, event);
    TEST_ASSERT_EQUAL_UINT(0, ms.events[0].type);
    TEST_ASSERT_EQUAL_UINT(42, ms.events[0].size);
    TEST_ASSERT_EQUAL_PTR(dummy_encoder, ms.events[0].encoder);
    TEST_ASSERT_EQUAL_PTR(dummy_decoder, ms.events[0].decoder);
    TEST_ASSERT_EQUAL_PTR(dummy_handler, ms.events[0].handler);
}

/* Register at capacity, verify rejection (no crash) */
void test_event_register_at_capacity(void) {
    microswim_t ms = { 0 };
    ms.event_count = MAXIMUM_EVENTS;

    microswim_event_t event = {
        .type = 0,
        .size = 1,
        .encoder = dummy_encoder,
        .decoder = dummy_decoder,
        .handler = dummy_handler,
    };

    /* Should not crash; the event is simply not registered */
    microswim_event_register(&ms, event);
    /* event_count unchanged since we reached capacity */
    TEST_ASSERT_EQUAL_UINT(MAXIMUM_EVENTS, ms.event_count);
}

/* 3. Dispatch is a no-op (stub) */
void test_event_dispatch_noop(void) {
    microswim_t ms = { 0 };
    /* Should not crash */
    microswim_event_dispatch(&ms, "test", NULL);
}
