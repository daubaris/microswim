#include "unity.h"

void setUp(void) {
}
void tearDown(void) {
}

/* test_microswim.c */
void test_index_add_single(void);
void test_index_add_multiple(void);
void test_index_remove_removes_highest(void);
void test_indices_shift(void);
void test_indices_shuffle_preserves_set(void);
void test_index_remove_empty(void);

/* test_member.c */
void test_member_add_and_find(void);
void test_member_add_overflow(void);
void test_member_find_unknown(void);
void test_member_find_updates_uuid_from_address(void);
void test_member_address_compare_full(void);
void test_member_address_compare_partial(void);
void test_member_confirmed_add_and_find(void);
void test_member_move_to_confirmed(void);
void test_member_move_updates_pointer(void);
void test_members_shift(void);
void test_member_retrieve_skips_self(void);
void test_member_retrieve_empty(void);
void test_member_retrieve_only_self(void);
void test_member_retrieve_shuffles_at_cycle_end(void);
void test_member_update_alive_overrides_suspect_higher_inc(void);
void test_member_update_alive_no_override_suspect_lower_inc(void);
void test_member_update_alive_overrides_alive_higher_inc(void);
void test_member_update_alive_no_override_alive_same_inc(void);
void test_member_update_suspect_overrides_alive_equal_inc(void);
void test_member_update_suspect_overrides_alive_higher_inc(void);
void test_member_update_suspect_no_override_alive_lower_inc(void);
void test_member_update_suspect_overrides_suspect_higher_inc(void);
void test_member_update_suspect_no_override_suspect_same_inc(void);
void test_member_update_confirmed_overrides_alive(void);
void test_member_update_confirmed_overrides_suspect(void);
void test_member_update_self_suspect_refutation(void);
void test_member_mark_alive(void);
void test_member_mark_suspect(void);
void test_member_mark_suspect_no_op_if_already_suspect(void);
void test_member_mark_confirmed(void);
void test_members_check_suspects_timeout(void);
void test_members_check_suspects_no_timeout(void);
void test_members_check_adds_new_member(void);
void test_members_check_adds_confirmed_member(void);
void test_members_check_updates_existing(void);
void test_get_ping_req_candidates(void);
void test_get_ping_req_candidates_unique(void);
void test_member_add_copies_ipso(void);
void test_member_update_copies_ipso_on_state_change(void);
void test_member_update_no_ipso_overwrite_when_already_set(void);
void test_member_update_copies_ipso_same_incarnation(void);
void test_members_check_adds_new_member_with_ipso(void);

/* test_message.c */
void test_message_construct_ping(void);
void test_message_construct_ack(void);
void test_message_construct_status(void);
void test_message_extract_adds_new(void);
void test_message_extract_updates_existing(void);
void test_message_send_invalid_socket(void);
void test_message_construct_populates_ipso_from_registry(void);
void test_message_construct_no_ipso_when_no_registry(void);

/* test_ping.c */
void test_ping_add(void);
void test_ping_find(void);
void test_ping_remove(void);
void test_ping_add_duplicate_replaces(void);
void test_ping_add_empty_uuid_rejected(void);
void test_ping_add_overflow(void);
void test_pings_check_marks_suspect(void);
void test_pings_check_no_op_before_deadline(void);

/* test_ping_req.c */
void test_ping_req_add(void);
void test_ping_req_find(void);
void test_ping_req_remove(void);
void test_ping_req_add_duplicate_replaces(void);
void test_ping_req_add_empty_uuid_rejected(void);
void test_ping_reqs_check_removes_expired(void);
void test_ping_reqs_check_keeps_unexpired(void);

/* test_update.c */
void test_update_add(void);
void test_update_find_unknown(void);
void test_update_add_overflow(void);
void test_updates_retrieve_least_used(void);
void test_updates_retrieve_increments_count(void);
void test_updates_retrieve_skips_empty_uuid(void);
void test_updates_retrieve_respects_max(void);

/* test_m_event.c */
void test_event_register(void);
void test_event_register_at_capacity(void);
void test_event_dispatch_noop(void);

/* test_ipso.c */
void test_ipso_find_instance_success(void);
void test_ipso_find_instance_not_found(void);
void test_ipso_find_instance_null_reg(void);
void test_ipso_find_resource_success(void);
void test_ipso_find_resource_not_found(void);
void test_ipso_find_resource_null_inst(void);
void test_ipso_read_i64(void);
void test_ipso_read_f64(void);
void test_ipso_read_bool(void);
void test_ipso_read_str(void);
void test_ipso_read_not_found(void);
void test_ipso_write_i64(void);
void test_ipso_write_str(void);
void test_ipso_write_str_too_long(void);
void test_ipso_write_custom_set_handler(void);
void test_ipso_write_default_write(void);

/* test_encode_decode_cbor.c (CBOR only) */
#ifdef MICROSWIM_CBOR
void test_cbor_roundtrip_ping(void);
void test_cbor_roundtrip_ack_with_updates(void);
void test_cbor_roundtrip_suspect(void);
void test_cbor_malformed_buffer(void);
void test_cbor_roundtrip_empty_updates(void);
void test_cbor_roundtrip_with_ipso_objects(void);
void test_cbor_roundtrip_empty_ipso(void);
#endif

int main(void) {
    UNITY_BEGIN();

    /* Index management */
    RUN_TEST(test_index_add_single);
    RUN_TEST(test_index_add_multiple);
    RUN_TEST(test_index_remove_removes_highest);
    RUN_TEST(test_indices_shift);
    RUN_TEST(test_indices_shuffle_preserves_set);
    RUN_TEST(test_index_remove_empty);

    /* Member lifecycle */
    RUN_TEST(test_member_add_and_find);
    RUN_TEST(test_member_add_overflow);
    RUN_TEST(test_member_find_unknown);
    RUN_TEST(test_member_find_updates_uuid_from_address);
    RUN_TEST(test_member_address_compare_full);
    RUN_TEST(test_member_address_compare_partial);
    RUN_TEST(test_member_confirmed_add_and_find);
    RUN_TEST(test_member_move_to_confirmed);
    RUN_TEST(test_member_move_updates_pointer);
    RUN_TEST(test_members_shift);

    /* Round-robin */
    RUN_TEST(test_member_retrieve_skips_self);
    RUN_TEST(test_member_retrieve_empty);
    RUN_TEST(test_member_retrieve_only_self);
    RUN_TEST(test_member_retrieve_shuffles_at_cycle_end);

    /* SWIM state transition rules */
    RUN_TEST(test_member_update_alive_overrides_suspect_higher_inc);
    RUN_TEST(test_member_update_alive_no_override_suspect_lower_inc);
    RUN_TEST(test_member_update_alive_overrides_alive_higher_inc);
    RUN_TEST(test_member_update_alive_no_override_alive_same_inc);
    RUN_TEST(test_member_update_suspect_overrides_alive_equal_inc);
    RUN_TEST(test_member_update_suspect_overrides_alive_higher_inc);
    RUN_TEST(test_member_update_suspect_no_override_alive_lower_inc);
    RUN_TEST(test_member_update_suspect_overrides_suspect_higher_inc);
    RUN_TEST(test_member_update_suspect_no_override_suspect_same_inc);
    RUN_TEST(test_member_update_confirmed_overrides_alive);
    RUN_TEST(test_member_update_confirmed_overrides_suspect);
    RUN_TEST(test_member_update_self_suspect_refutation);

    /* Mark functions */
    RUN_TEST(test_member_mark_alive);
    RUN_TEST(test_member_mark_suspect);
    RUN_TEST(test_member_mark_suspect_no_op_if_already_suspect);
    RUN_TEST(test_member_mark_confirmed);

    /* Suspect timeout + gossip integration */
    RUN_TEST(test_members_check_suspects_timeout);
    RUN_TEST(test_members_check_suspects_no_timeout);
    RUN_TEST(test_members_check_adds_new_member);
    RUN_TEST(test_members_check_adds_confirmed_member);
    RUN_TEST(test_members_check_updates_existing);
    RUN_TEST(test_get_ping_req_candidates);
    RUN_TEST(test_get_ping_req_candidates_unique);

    /* Member IPSO */
    RUN_TEST(test_member_add_copies_ipso);
    RUN_TEST(test_member_update_copies_ipso_on_state_change);
    RUN_TEST(test_member_update_no_ipso_overwrite_when_already_set);
    RUN_TEST(test_member_update_copies_ipso_same_incarnation);
    RUN_TEST(test_members_check_adds_new_member_with_ipso);

    /* Message */
    RUN_TEST(test_message_construct_ping);
    RUN_TEST(test_message_construct_ack);
    RUN_TEST(test_message_construct_status);
    RUN_TEST(test_message_extract_adds_new);
    RUN_TEST(test_message_extract_updates_existing);
    RUN_TEST(test_message_send_invalid_socket);
    RUN_TEST(test_message_construct_populates_ipso_from_registry);
    RUN_TEST(test_message_construct_no_ipso_when_no_registry);

    /* Ping */
    RUN_TEST(test_ping_add);
    RUN_TEST(test_ping_find);
    RUN_TEST(test_ping_remove);
    RUN_TEST(test_ping_add_duplicate_replaces);
    RUN_TEST(test_ping_add_empty_uuid_rejected);
    RUN_TEST(test_ping_add_overflow);
    RUN_TEST(test_pings_check_marks_suspect);
    RUN_TEST(test_pings_check_no_op_before_deadline);

    /* Ping req */
    RUN_TEST(test_ping_req_add);
    RUN_TEST(test_ping_req_find);
    RUN_TEST(test_ping_req_remove);
    RUN_TEST(test_ping_req_add_duplicate_replaces);
    RUN_TEST(test_ping_req_add_empty_uuid_rejected);
    RUN_TEST(test_ping_reqs_check_removes_expired);
    RUN_TEST(test_ping_reqs_check_keeps_unexpired);

    /* Update */
    RUN_TEST(test_update_add);
    RUN_TEST(test_update_find_unknown);
    RUN_TEST(test_update_add_overflow);
    RUN_TEST(test_updates_retrieve_least_used);
    RUN_TEST(test_updates_retrieve_increments_count);
    RUN_TEST(test_updates_retrieve_skips_empty_uuid);
    RUN_TEST(test_updates_retrieve_respects_max);

    /* Events */
    RUN_TEST(test_event_register);
    RUN_TEST(test_event_register_at_capacity);
    RUN_TEST(test_event_dispatch_noop);

    /* IPSO */
    RUN_TEST(test_ipso_find_instance_success);
    RUN_TEST(test_ipso_find_instance_not_found);
    RUN_TEST(test_ipso_find_instance_null_reg);
    RUN_TEST(test_ipso_find_resource_success);
    RUN_TEST(test_ipso_find_resource_not_found);
    RUN_TEST(test_ipso_find_resource_null_inst);
    RUN_TEST(test_ipso_read_i64);
    RUN_TEST(test_ipso_read_f64);
    RUN_TEST(test_ipso_read_bool);
    RUN_TEST(test_ipso_read_str);
    RUN_TEST(test_ipso_read_not_found);
    RUN_TEST(test_ipso_write_i64);
    RUN_TEST(test_ipso_write_str);
    RUN_TEST(test_ipso_write_str_too_long);
    RUN_TEST(test_ipso_write_custom_set_handler);
    RUN_TEST(test_ipso_write_default_write);

    /* CBOR encode/decode */
#ifdef MICROSWIM_CBOR
    RUN_TEST(test_cbor_roundtrip_ping);
    RUN_TEST(test_cbor_roundtrip_ack_with_updates);
    RUN_TEST(test_cbor_roundtrip_suspect);
    RUN_TEST(test_cbor_malformed_buffer);
    RUN_TEST(test_cbor_roundtrip_empty_updates);
    RUN_TEST(test_cbor_roundtrip_with_ipso_objects);
    RUN_TEST(test_cbor_roundtrip_empty_ipso);
#endif

    return UNITY_END();
}
