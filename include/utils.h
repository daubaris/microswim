#ifndef MICROSWIM_UTILS_H
#define MICROSWIM_UTILS_H

#include <arpa/inet.h>
#include <stddef.h>
#include <stdint.h>

size_t microswim_random(size_t size);
uint64_t microswim_milliseconds();
void microswim_uuid_generate(char* uuid);
void microswim_sockaddr_to_uri(struct sockaddr_in* addr, char* buffer, size_t buffer_size);

#endif
