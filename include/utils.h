#ifndef MICROSWIM_UTILS_H
#define MICROSWIM_UTILS_H

#include <stddef.h>
#include <stdint.h>

size_t microswim_random(size_t size);
uint64_t microswim_milliseconds();
void microswim_uuid_generate(char* uuid);

#endif
