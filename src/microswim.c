#include "microswim.h"
#ifdef RIOT_OS
#include "lwip/netif.h"
#include "net/ipv4/addr.h"
#include "net/sock.h"
#include "net/utils.h"
#endif
#include "microswim_log.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/**
 * @brief Sets up the socket.
 *
 * Assigns the supplied IP address and port to the socket and sets it to be non-blocking.
 */
void microswim_socket_setup(microswim_t* ms, char* addr, int port) {
#ifdef RIOT_OS
    ms->self.addr.family = AF_INET;
    ms->self.addr.port = port;
#else
    ms->socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (ms->socket < 0) {
        MICROSWIM_LOG_ERROR("socket() failed: %d (%s)\n", errno, strerror(errno));
        return;
    }

    memset(&ms->self.addr, 0, sizeof(ms->self.addr));
    ms->self.addr.sin_family = AF_INET;
    ms->self.addr.sin_port = htons(port);
#endif

    if (addr == NULL || *addr == '\0') {
#ifdef RIOT_OS
        struct netif* n = netif_default;
        if (!n) {
            printf("No default netif available.\r\n");
            return;
        }

        const ip4_addr_t* ip = netif_ip4_addr(n);
        if (ip4_addr_isany_val(*ip)) {
            printf("Interface has no IPv4 address yet (0.0.0.0). Cannot bind.\r\n");
            return;
        }

        memcpy(&ms->self.addr.addr.ipv4, ip, sizeof(ms->self.addr.addr.ipv4));

        char buffer[IPV4_ADDR_MAX_STR_LEN] = { 0 };
        inet_ntop(AF_INET, &ms->self.addr.addr.ipv4, buffer, IPV4_ADDR_MAX_STR_LEN);
        printf("Using interface IP: %s\r\n", buffer);
#else
        ms->self.addr.sin_addr.s_addr = htonl(INADDR_ANY);
#endif
    } else {
#ifdef RIOT_OS
        MICROSWIM_LOG_WARN("Not implemented!");
#else
        if (inet_pton(AF_INET, addr, &ms->self.addr.sin_addr.s_addr) != 1) {
            printf("Invalid IPv4 address: %s (errno %d: %s)\r\n", addr, errno, strerror(errno));
            return;
        }
#endif
    }

#ifdef RIOT_OS
    if (sock_udp_create(&ms->socket, &ms->self.addr, NULL, 0) < 0) {
        MICROSWIM_LOG_ERROR("Error creating socket\n");
        return;
    }
#else
    if (bind(ms->socket, (struct sockaddr*)&ms->self.addr, sizeof(ms->self.addr)) != 0) {
        int err = errno;
        MICROSWIM_LOG_ERROR("`bind` exited with an error code: %d (%s)\n", err, strerror(errno));
        close(ms->socket);
    }
#endif
}

void microswim_index_remove(microswim_t* ms) {
    if ((ms->member_count + 1) == 0)
        return;

    // Find the index of the highest value
    int index = 0;
    for (size_t i = 1; i < ms->member_count + 1; i++) {
        if (ms->indices[i] > ms->indices[index]) {
            index = i;
        }
    }

    // Shift elements to remove the highest value
    for (size_t i = index; i < ms->member_count; i++) {
        ms->indices[i] = ms->indices[i + 1];
    }
}

void microswim_index_add(microswim_t* ms) {
    size_t index = 0;
    if (ms->member_count >= 1) {
        index = rand() % (ms->member_count);
    }

    size_t* position = microswim_indices_shift(ms, index);
    *position = ms->member_count - 1;
}

size_t* microswim_indices_shift(microswim_t* ms, size_t index) {
    for (size_t i = ms->member_count - 1; i > index; i--) {
        ms->indices[i] = ms->indices[i - 1];
    }

    size_t* new = &ms->indices[index];

    return new;
}

void microswim_indices_shuffle(microswim_t* ms) {
    for (size_t i = ms->member_count - 1; i > 0; i--) {
        int j = rand() % (i + 1);

        size_t temp = ms->indices[i];
        ms->indices[i] = ms->indices[j];
        ms->indices[j] = temp;
    }
}
