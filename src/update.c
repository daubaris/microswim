#include "update.h"
#include "log.h"
#include "microswim.h"
#ifndef EMBEDDED
#include <stdlib.h>
#endif

/**
 * @brief Adds an update to the central update array, referencing the supplied member.
 */
microswim_update_t* microswim_update_add(microswim_t* ms, microswim_member_t* member) {
    if (ms->update_count > MAXIMUM_UPDATES) {
        LOG_ERROR("Cannot add more than %d updates\n", MAXIMUM_UPDATES);
        return NULL;
    }

    ms->updates[ms->update_count].member = member;
    ms->updates[ms->update_count].count = 0;

    return &ms->updates[ms->update_count++];
}

/**
 * @brief Finds update referencing the supplied member from the central update array.
 */
microswim_update_t* microswim_update_find(microswim_t* ms, microswim_member_t* member) {
    for (int i = 0; i < ms->update_count; i++) {
        if (strncmp((char*)ms->updates[i].member->uuid, (char*)member->uuid, UUID_SIZE) == 0) {
            return &ms->updates[i];
        }
    }

    return NULL;
}

static void microswim_sort_updates_by_count(microswim_update_t* updates, size_t count) {
    for (size_t i = 1; i < count; ++i) {
        microswim_update_t key = updates[i];
        size_t j = i;

        while (j > 0 && updates[j - 1].count > key.count) {
            updates[j] = updates[j - 1];
            j--;
        }

        updates[j] = key;
    }
}

/**
 * @brief Selects and retrieves the least used updates.
 */
size_t microswim_updates_retrieve(microswim_t* ms, microswim_update_t* updates[MAXIMUM_MEMBERS_IN_AN_UPDATE]) {
    microswim_sort_updates_by_count(ms->updates, ms->update_count);
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
