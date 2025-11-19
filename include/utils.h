#ifndef MICROSWIM_UTILS_H
#define MICROSWIM_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef RIOT_OS
#include "net/sock/udp.h"
#endif
#include <arpa/inet.h>
#include <stddef.h>
#include <stdint.h>

uint64_t microswim_milliseconds(void);
size_t microswim_random(void);
void microswim_uuid_generate(char* uuid);
#ifdef RIOT_OS
void microswim_sockaddr_to_uri(sock_udp_ep_t* addr, char* buffer, size_t buffer_size);
#else
void microswim_sockaddr_to_uri(struct sockaddr_in* addr, char* buffer, size_t buffer_size);
#endif

#ifdef __cplusplus
}
#endif

#endif // MICROSWIM_UTILS_H
