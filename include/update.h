#ifndef MICROSWIM_UPDATE_H
#define MICROSWIM_UPDATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "microswim.h"

microswim_update_t* microswim_update_add(microswim_t* ms, microswim_member_t* member);
microswim_update_t* microswim_update_find(microswim_t* ms, microswim_member_t* member);
microswim_update_t* microswim_update_update(microswim_t* ms, microswim_update_t* update);
microswim_update_t* microswim_update_remove(microswim_t* ms, microswim_update_t* update);

size_t microswim_updates_retrieve(microswim_t* ms, microswim_update_t* updates[MAXIMUM_MEMBERS_IN_AN_UPDATE]);

#ifdef __cplusplus
}
#endif

#endif // MICROSWIM_UPDATE_H
