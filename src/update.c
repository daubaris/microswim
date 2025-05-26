#include "update.h"
#include "log.h"
#include "microswim.h"
#include <stdlib.h>

microswim_update_t* microswim_update_add(microswim_t* ms, microswim_member_t* member) {
    if (ms->update_count + 1 >= MAXIMUM_UPDATES) {
        LOG_ERROR("Cannot add more than %d members\n", MAXIMUM_UPDATES);
        return NULL;
    }

    ms->updates[ms->update_count].member = member;
    ms->updates[ms->update_count].count = 0;

    return &ms->updates[ms->update_count++];
}

microswim_update_t* microswim_update_find(microswim_t* ms, microswim_member_t* member) {
    for (int i = 0; i < ms->update_count; i++) {
        if (strncmp(ms->updates[i].member->uuid, member->uuid, UUID_SIZE) == 0) {
            return &ms->updates[i];
        }
    }

    return NULL;
}

size_t microswim_updates_retrieve(microswim_t* ms, microswim_update_t* updates[MAXIMUM_MEMBERS_IN_AN_UPDATE]) {
    qsort(ms->updates, ms->update_count, sizeof(microswim_update_t), microswim_compare_by_count);
    size_t count = 0;

    for (size_t j = 0; (j < ms->update_count && j < MAXIMUM_MEMBERS_IN_AN_UPDATE); j++) {
        if (ms->updates[j].member->uuid[0] != '\0') {
            updates[count] = &ms->updates[j];
            ms->updates[j].count++;
            count++;
        }
    }

    return count;
}
