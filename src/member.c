#include "member.h"
#include "log.h"
#include "microswim.h"
#include "utils.h"

microswim_member_t* microswim_member_add(microswim_t* ms, microswim_member_t member) {
    if (ms->member_count + 1 >= MAXIMUM_MEMBERS) {
        LOG_ERROR("Cannot add more than %d members\n", MAXIMUM_MEMBERS);
        return NULL;
    }

    size_t position = microswim_random(ms->member_count);
    microswim_member_t* slot = microswim_members_shift(ms, position);
    strncpy(slot->uuid, member.uuid, UUID_SIZE);
    slot->addr = member.addr;
    slot->incarnation = member.incarnation;
    slot->status = member.incarnation;
    slot->timeout = (microswim_milliseconds() + (uint64_t)(SUSPECT_TIMEOUT * 1000));

    return slot;
}

microswim_member_t* microswim_member_find(microswim_t* ms, microswim_member_t* member) {
    for (size_t i = 0; i < ms->member_count; i++) {
        // TODO: if UUID is null, function needs to compare by addr.
        if (strncmp(member->uuid, ms->members[i].uuid, UUID_SIZE) == 0) {
            return &ms->members[i];
        }
    }

    return NULL;
}

microswim_member_t* microswim_member_update(microswim_t* ms, microswim_member_t* member) {
}

microswim_member_t* microswim_member_move(microswim_t* ms, microswim_member_t* member) {
}

void microswim_member_mark_alive(microswim_t* ms, microswim_member_t* member) {
}

void microswim_member_mark_suspect(microswim_t* ms, microswim_member_t* member) {
}

void microswim_member_mark_confirmed(microswim_t* ms, microswim_member_t* member) {
}

microswim_member_t* microswim_members_shift(microswim_t* ms, size_t position) {
    for (int i = ms->member_count; i > position; i--) {
        ms->members[i] = ms->members[i - 1];
    }

    microswim_member_t* new = &ms->members[position];
    memset(new, 0, sizeof(microswim_member_t));

    return new;
}
