#ifndef MICROSWIM_UTILS_H
#define MICROSWIM_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <arpa/inet.h>
#include <stddef.h>
#include <stdint.h>

uint64_t microswim_milliseconds();
size_t microswim_random();
void microswim_uuid_generate(char* uuid);
void microswim_sockaddr_to_uri(struct sockaddr_in* addr, char* buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif // MICROSWIM_UTILS_H
