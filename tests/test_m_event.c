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

static int handler_calls;
static void* handler_last_data;
static size_t handler_last_length;

static void recording_handler(void* ms, void* buffer, size_t length) {
    (void)ms;
    handler_calls++;
    handler_last_data = buffer;
    handler_last_length = length;
}

/* Register event, verify stored and counted */
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
    TEST_ASSERT_EQUAL_UINT(1, ms.event_count);

    /* Re-registering the same type updates in place without double-counting */
    event.size = 7;
    microswim_event_register(&ms, event);
    TEST_ASSERT_EQUAL_UINT(7, ms.events[0].size);
    TEST_ASSERT_EQUAL_UINT(1, ms.event_count);
}

/* Registering a type outside [0, MAXIMUM_EVENTS) is rejected, no OOB write */
void test_event_register_type_out_of_range(void) {
    microswim_t ms = { 0 };

    microswim_event_t event = {
        .type = MAXIMUM_EVENTS,
        .size = 1,
        .encoder = dummy_encoder,
        .decoder = dummy_decoder,
        .handler = dummy_handler,
    };

    /* Should not crash; the event is simply not registered */
    microswim_event_register(&ms, event);
    TEST_ASSERT_EQUAL_UINT(0, ms.event_count);
}

/* Dispatch invokes the bound handler; unbound/out-of-range types are safe no-ops */
void test_event_dispatch_invokes_handler(void) {
    microswim_t ms = { 0 };

    microswim_event_t event = {
        .type = 2,
        .size = 0,
        .encoder = dummy_encoder,
        .decoder = dummy_decoder,
        .handler = recording_handler,
    };
    microswim_event_register(&ms, event);

    handler_calls = 0;
    handler_last_data = NULL;
    handler_last_length = 0;

    int payload = 123;
    microswim_event_dispatch(&ms, 2, &payload, sizeof(payload));
    TEST_ASSERT_EQUAL_INT(1, handler_calls);
    TEST_ASSERT_EQUAL_PTR(&payload, handler_last_data);
    TEST_ASSERT_EQUAL_UINT(sizeof(payload), handler_last_length);

    /* Unbound type: no handler registered, no call, no crash */
    microswim_event_dispatch(&ms, 5, NULL, 0);
    TEST_ASSERT_EQUAL_INT(1, handler_calls);

    /* Out-of-range type: safe no-op */
    microswim_event_dispatch(&ms, MAXIMUM_EVENTS, NULL, 0);
    TEST_ASSERT_EQUAL_INT(1, handler_calls);
}
