#ifndef MICROWSIM_MEMBER_H
#define MICROWSIM_MEMBER_H

#include "microswim.h"

microswim_member_t* microswim_member_add(microswim_t* ms, microswim_member_t member);
microswim_member_t* microswim_member_find(microswim_t* ms, microswim_member_t* member);
microswim_member_t* microswim_member_update(microswim_t* ms, microswim_member_t* member);
microswim_member_t* microswim_member_remove(microswim_t* ms, microswim_member_t* member);

void microswim_member_mark_alive(microswim_t* ms, microswim_member_t* member);
void microswim_member_mark_suspect(microswim_t* ms, microswim_member_t* member);
void microswim_member_mark_confirmed(microswim_t* ms, microswim_member_t* member);

microswim_member_t* microswim_members_shift(microswim_t* ms, size_t position);

#endif
