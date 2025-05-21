#ifndef MICROSWIM_UPDATE_H
#define MICROSWIM_UPDATE_H

#include "microswim.h"

microswim_update_t* microswim_update_add(microswim_t* ms, microswim_update_t* update);
microswim_update_t* microswim_update_find(microswim_t* ms, microswim_update_t* update);
microswim_update_t* microswim_update_update(microswim_t* ms, microswim_update_t* update);
microswim_update_t* microswim_update_remove(microswim_t* ms, microswim_update_t* update);

#endif
