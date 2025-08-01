// TODO:: custom functions depending on the platform.

#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <uuid/uuid.h>

size_t microswim_random(size_t size) {
    size_t random = rand() % (size + 1);
    return random;
}

uint64_t microswim_milliseconds() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)(ts.tv_sec) * 1000 + (ts.tv_nsec) / 1000000;
}

void microswim_uuid_generate(char* uuid) {
    uuid_t uuid_binary;
    uuid_generate_random(uuid_binary);
    uuid_unparse(uuid_binary, uuid);
}

void microswim_sockaddr_to_uri(struct sockaddr_in* addr, char* buffer, size_t buffer_size) {
    char ip_str[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr->sin_addr), ip_str, sizeof(ip_str));
    int port = ntohs(addr->sin_port);
    snprintf(buffer, buffer_size, "%s:%d", ip_str, port);
}
