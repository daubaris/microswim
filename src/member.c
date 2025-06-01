#include "member.h"
#include "constants.h"
#include "log.h"
#include "message.h"
#include "microswim.h"
#include "ping.h"
#include "update.h"
#include "utils.h"
#include <stdlib.h>

microswim_member_t* microswim_member_retrieve(microswim_t* ms) {
    if (ms->member_count == 0) {
        return NULL;
    }

    int original_index = ms->round_robin_index;

    while (1) {
        // NOTE: we do not care whether the member is ALIVE or SUSPECT.
        // As long as it is not CONFIRMED, we can send it a ping message.
        size_t index = ms->indices[ms->round_robin_index];

        microswim_member_t* member = &ms->members[index];
        ms->round_robin_index = (ms->round_robin_index + 1) % ms->member_count;

        if (ms->round_robin_index == 0) {
            microswim_indices_shuffle(ms);
        }

        if (strncmp(ms->self.uuid, member->uuid, UUID_SIZE) != 0) {
            return member;
        }

        if (ms->round_robin_index == original_index) {
            // NOTE: No valid member found after iterating through
            // the entire list of members.
            return NULL;
        }
    }
}

microswim_member_t* microswim_member_add(microswim_t* ms, microswim_member_t member) {
    if (ms->member_count + 1 >= MAXIMUM_MEMBERS) {
        LOG_ERROR("Cannot add more than %d members\n", MAXIMUM_MEMBERS);
        return NULL;
    }

    microswim_member_t* slot = &ms->members[ms->member_count++];
    strncpy(slot->uuid, member.uuid, UUID_SIZE);
    slot->addr = member.addr;
    slot->incarnation = member.incarnation;
    slot->status = member.incarnation;
    slot->timeout = (microswim_milliseconds() + (uint64_t)(SUSPECT_TIMEOUT * 1000));

    return slot;
}

microswim_member_t* microswim_member_find(microswim_t* ms, microswim_member_t* member) {
    for (size_t i = 0; i < ms->member_count; i++) {
        if (ms->members[i].uuid[0] == '\0') {
            int c = microswim_member_address_compare(&ms->members[i], member);
            if (c == (SIN_FAMILY | SIN_PORT | SIN_ADDR)) {
                LOG_DEBUG("Updated member's UUID");
                strncpy(ms->members[i].uuid, member->uuid, UUID_SIZE);
            }
        }

        if (strncmp(member->uuid, ms->members[i].uuid, UUID_SIZE) == 0) {
            return &ms->members[i];
        }
    }

    return NULL;
}

microswim_member_t* microswim_member_confirmed_find(microswim_t* ms, microswim_member_t* member) {
    for (size_t i = 0; i < ms->confirmed_count; i++) {
        if (strncmp(member->uuid, ms->confirmed[i].uuid, UUID_SIZE) == 0) {
            return &ms->confirmed[i];
        }
    }

    return NULL;
}

void microswim_members_shift(microswim_t* ms, size_t index) {
    for (size_t i = index; i < ms->member_count - 1; i++) {
        microswim_update_t* update = microswim_update_find(ms, &ms->members[i + 1]);
        ms->members[i] = ms->members[i + 1];
        if (update != NULL) {
            update->member = &ms->members[i];
        }
    }

    ms->member_count--;
}

void microswim_member_update(microswim_t* ms, microswim_member_t* ex, microswim_member_t* nw) {
    // NOTE: this should probably move somewhere else.
    if (strncmp(ms->self.uuid, nw->uuid, UUID_SIZE) == 0 && (nw->status == SUSPECT)) {
        // TODO: check all the ms->self references.
        ms->self.incarnation = nw->incarnation + 1;
        ms->self.status = ALIVE;
        ex->incarnation = nw->incarnation + 1;
        ex->status = ALIVE;

        microswim_message_t message = { 0 };
        microswim_status_message_construct(ms, &message, ALIVE_MESSAGE, ex);
        microswim_member_t* recipient = microswim_member_retrieve(ms);
        microswim_status_message_send(ms, recipient, &message);

        return;
    }

    if (nw->status == ALIVE) {
        // {Alive M1, inc = i} overrides:
        // - {Suspect M1, inc = j} if i > j
        // - {Alive M1, inc = j} if i > j
        if ((ex->status == SUSPECT && nw->incarnation > ex->incarnation) ||
            (ex->status == ALIVE && nw->incarnation > ex->incarnation)) {
            ex->status = nw->status;
            ex->incarnation = nw->incarnation;
            ex->timeout = (microswim_milliseconds() + (uint64_t)(SUSPECT_TIMEOUT * 1000));

            microswim_member_t member = { 0 };
            strncpy(member.uuid, ex->uuid, UUID_SIZE);
            microswim_ping_t* ping = microswim_ping_find(ms, &member);
            if (ping != NULL) {
                microswim_ping_remove(ms, ping);
            }
        }
    }

    if (nw->status == SUSPECT) {
        // {Suspect M1, inc = i} overrides:
        // - {Suspect M1, inc = j} if i > j
        // - {Alive M1, inc = j} if i >= j
        if ((ex->status == SUSPECT && nw->incarnation > ex->incarnation) ||
            (ex->status == ALIVE && nw->incarnation >= ex->incarnation)) {
            ex->status = nw->status;
            ex->incarnation = nw->incarnation;
            ex->timeout = (microswim_milliseconds() + (uint64_t)(SUSPECT_TIMEOUT * 1000));

            microswim_member_t member = { 0 };
            strncpy(member.uuid, ex->uuid, UUID_SIZE);
            microswim_ping_t* ping = microswim_ping_find(ms, &member);
            if (ping != NULL) {
                microswim_ping_remove(ms, ping);
            }
        }
    }

    if (nw->status == CONFIRMED) {
        // {Confirm M1, inc = i} overrides:
        // - {Alive M1, inc = j} for any j
        // - {Suspect M1, inc = j} for any j
        if (ex->status == ALIVE || ex->status == SUSPECT) {
            ex->status = nw->status;
            ex->incarnation = nw->incarnation;

            microswim_member_t member = { 0 };
            strncpy(member.uuid, ex->uuid, UUID_SIZE);
            microswim_ping_t* ping = microswim_ping_find(ms, &member);
            if (ping != NULL) {
                microswim_ping_remove(ms, ping);
            }

            microswim_member_mark_confirmed(ms, ex);
            microswim_index_remove(ms);
        }
    }
}

microswim_member_t* microswim_member_move(microswim_t* ms, microswim_member_t* member) {
    int index = -1;

    for (int i = 0; i < ms->member_count; i++) {
        if (&ms->members[i] == member) {
            index = i;
            break;
        }
    }

    if (index >= 0) {
        microswim_ping_t* ping = microswim_ping_find(ms, member);
        if (ping != NULL) {
            microswim_ping_remove(ms, ping);
        }

        ms->confirmed[ms->confirmed_count] = ms->members[index];

        microswim_update_t* update = microswim_update_find(ms, &ms->members[index]);
        if (update != NULL) {
            update->member = &ms->confirmed[ms->confirmed_count];
        }

        microswim_members_shift(ms, index);

        return &ms->confirmed[ms->confirmed_count++];
    }

    return NULL;
}

void microswim_member_mark_alive(microswim_t* ms, microswim_member_t* member) {
    (void)ms;

    microswim_member_status_t status = member->status;

    member->status = ALIVE;
    member->timeout = (microswim_milliseconds() + (uint64_t)(SUSPECT_TIMEOUT * 1000));

    LOG_INFO("Member: %s was marked alive", member->uuid);

    if (status == SUSPECT) {
        microswim_message_t message = { 0 };
        microswim_status_message_construct(ms, &message, ALIVE_MESSAGE, member);
        microswim_member_t* recipient = microswim_member_retrieve(ms);
        microswim_status_message_send(ms, recipient, &message);
    }
}

void microswim_member_mark_suspect(microswim_t* ms, microswim_member_t* member) {
    if (member->status == ALIVE) {
        LOG_INFO("Member: %s was marked suspect", member->uuid);
        member->status = SUSPECT;
        member->timeout = (microswim_milliseconds() + (uint64_t)(SUSPECT_TIMEOUT * 1000));
        microswim_message_t message = { 0 };
        microswim_status_message_construct(ms, &message, SUSPECT_MESSAGE, member);
        microswim_member_t* recipient = microswim_member_retrieve(ms);
        microswim_status_message_send(ms, recipient, &message);
    }
}

void microswim_member_mark_confirmed(microswim_t* ms, microswim_member_t* member) {
    LOG_INFO("Member: %s was marked confirmed", member->uuid);
    member->status = CONFIRMED;

    microswim_ping_t* ping = microswim_ping_find(ms, member);
    if (ping != NULL) {
        microswim_ping_remove(ms, ping);
    }

    microswim_member_move(ms, member);

    microswim_message_t message = { 0 };
    microswim_status_message_construct(ms, &message, CONFIRM_MESSAGE, member);
    microswim_member_t* recipient = microswim_member_retrieve(ms);
    microswim_status_message_send(ms, recipient, &message);
}

void microswim_members_shuffle(microswim_t* ms) {
    for (int i = ms->member_count - 1; i > 0; i--) {
        int j = rand() % (i + 1);

        microswim_member_t temp = ms->members[i];
        ms->members[i] = ms->members[j];
        ms->members[j] = temp;
    }
}

size_t microswim_member_address_compare(microswim_member_t* a, microswim_member_t* b) {
    size_t r = 0;

    if (a->addr.sin_family == b->addr.sin_family)
        r |= SIN_FAMILY;
    if (ntohs(a->addr.sin_port) == ntohs(b->addr.sin_port))
        r |= SIN_PORT;
    if (a->addr.sin_addr.s_addr == b->addr.sin_addr.s_addr)
        r |= SIN_ADDR;

    return r;
}

microswim_member_t* microswim_member_confirmed_add(microswim_t* ms, microswim_member_t member) {
    if (ms->confirmed_count + 1 >= MAXIMUM_MEMBERS) {
        LOG_ERROR("Cannot add more than %d members\n", MAXIMUM_MEMBERS);
        return NULL;
    }

    microswim_member_t* slot = &ms->confirmed[ms->confirmed_count++];
    strncpy(slot->uuid, member.uuid, UUID_SIZE);
    slot->addr = member.addr;
    slot->incarnation = member.incarnation;
    slot->status = member.status;
    slot->timeout = 0;

    return slot;
}

void microswim_member_confirmed_remove(microswim_t* ms, microswim_member_t* member) {
    size_t index;
    for (index = 0; index < ms->confirmed_count; index++) {
        if (&(ms->confirmed[index]) == member) {
            break;
        }
    }

    for (int i = index; i < ms->confirmed_count - 1; i++) {
        ms->confirmed[i] = ms->confirmed[i + 1];
    }

    ms->confirmed_count--;
}

size_t microswim_get_ping_req_candidates(microswim_t* ms, size_t members[FAILURE_DETECTION_GROUP]) {
    size_t member_count = 0;
    for (size_t i = 0; (i < ms->member_count - 1 && i < FAILURE_DETECTION_GROUP); i++) {
        bool exists = false;
        while (!exists) {
            size_t index = rand() % (ms->member_count - 1) + 1;

            for (size_t j = 0; j <= member_count; j++) {
                if (index == members[j]) {
                    exists = true;
                    break;
                }
            }

            if (!exists) {
                members[member_count++] = index;
                break;
            }

            exists = false;
        }
    }

    return member_count;
}

void microswim_members_check(microswim_t* ms, microswim_member_t* member) {
    microswim_member_t* existing_member = microswim_member_find(ms, member);
    microswim_member_t* confirmed_member = microswim_member_confirmed_find(ms, member);

    if (existing_member == NULL && confirmed_member == NULL) {
        // Member is not found in either list, add it to the appropriate list
        if (member->status == CONFIRMED) {
            LOG_DEBUG("Added member: %s to confirmed list.", member->uuid);
            microswim_member_t* new_member = microswim_member_confirmed_add(ms, *member);
            if (new_member != NULL) {
                microswim_update_add(ms, new_member);
            }
        } else {
            microswim_member_t* new_member = microswim_member_add(ms, *member);
            if (new_member != NULL) {
                microswim_index_add(ms);
                microswim_update_add(ms, new_member);
            }
        }
    } else if (existing_member != NULL) {
        // Member exists in the regular list
        microswim_member_update(ms, existing_member, member);
        if (member->status != CONFIRMED) {
            microswim_update_t* update = microswim_update_find(ms, existing_member);
            if (existing_member->uuid[0] != '\0' && update == NULL) {
                microswim_update_add(ms, existing_member);
            }
        }
    }
}

void microswim_members_check_suspects(microswim_t* ms) {
    for (size_t i = 0; i < ms->member_count; i++) {
        if (ms->members[i].status == SUSPECT) {
            if (ms->members[i].timeout < microswim_milliseconds()) {
                microswim_member_t* confirmed = microswim_member_confirmed_find(ms, &ms->members[i]);
                // BUG: Potentially... what should happen if confirmed is found?
                if (!confirmed) {
                    microswim_member_mark_confirmed(ms, &ms->members[i]);
                    microswim_index_remove(ms);
                }
            }
        }
    }
}
