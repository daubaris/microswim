#include "utils.h"
#include "microswim_log.h"
#include "net/ipv4/addr.h"
#include "net/sock/udp.h"
#include "random.h"
#include "uuid.h"
#include "ztimer.h"
#include <stdio.h>
#include <stdlib.h>

size_t microswim_random(void) {
    return random_uint32();
}

uint64_t microswim_milliseconds(void) {
    return ztimer_now(ZTIMER_MSEC);
}

void microswim_uuid_generate(char* uuid) {
    uuid_t uuid_binary;
    uuid_v4(&uuid_binary);
    uuid_to_string(&uuid_binary, uuid);
}

void microswim_sockaddr_to_uri(sock_udp_ep_t* addr, char* buffer, size_t buffer_size) {
    char ip_str[buffer_size];
    memset(&ip_str, 0, buffer_size);
    ipv4_addr_to_str(ip_str, (ipv4_addr_t*)&addr->addr.ipv4, sizeof(ip_str));
    snprintf(buffer, buffer_size, "%s:%d", ip_str, addr->port);
}
