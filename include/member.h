#ifndef MICROSWIM_MEMBER_H
#define MICROSWIM_MEMBER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "microswim.h"

size_t microswim_member_address_compare(microswim_member_t* a, microswim_member_t* b);

microswim_member_t* microswim_member_retrieve(microswim_t* ms);
microswim_member_t* microswim_member_add(microswim_t* ms, microswim_member_t member);
microswim_member_t* microswim_member_find(microswim_t* ms, microswim_member_t* member);
microswim_member_t* microswim_member_find_by_uuid(microswim_t* ms, const char* uuid);
microswim_member_t* microswim_member_remove(microswim_t* ms, microswim_member_t* member);
microswim_member_t* microswim_member_move(microswim_t* ms, microswim_member_t* member);

void microswim_member_update(microswim_t* ms, microswim_member_t* ex, microswim_member_t* nw);

void microswim_member_mark_alive(microswim_t* ms, microswim_member_t* member);
void microswim_member_mark_suspect(microswim_t* ms, microswim_member_t* member);
void microswim_member_mark_confirmed(microswim_t* ms, microswim_member_t* member);

void microswim_members_shift(microswim_t* ms, size_t index);
void microswim_members_check(microswim_t* ms, microswim_member_t* member);
void microswim_members_check_suspects(microswim_t* ms);

microswim_member_t* microswim_member_confirmed_find(microswim_t* ms, microswim_member_t* member);
microswim_member_t* microswim_member_confirmed_add(microswim_t* ms, microswim_member_t member);

size_t microswim_get_ping_req_candidates(microswim_t* ms, size_t members[FAILURE_DETECTION_GROUP]);

bool microswim_member_has_oid(const microswim_member_t* m, ipso_oid_t oid);

#ifdef __cplusplus
}
#endif

#endif // MICROSWIM_MEMBER_H
