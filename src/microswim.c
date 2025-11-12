#include "microswim.h"
#ifdef RIOT_OS
#include "lwip/netif.h"
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
    ms->socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (ms->socket < 0) {
        MICROSWIM_LOG_ERROR("socket() failed: %d (%s)\n", errno, strerror(errno));
        return;
    }

    memset(&ms->self.addr, 0, sizeof(ms->self.addr));
    ms->self.addr.sin_family = AF_INET;
    ms->self.addr.sin_port = htons(port);

    if (addr == NULL || *addr == '\0') {
#ifdef RIOT_OS
        struct netif* n = netif_default;
        if (!n) {
            printf("No default netif available.\r\n");
            close(ms->socket);
            return;
        }

        const ip4_addr_t* ip = netif_ip4_addr(n);
        if (ip4_addr_isany_val(*ip)) {
            printf("Interface has no IPv4 address yet (0.0.0.0). Cannot bind.\r\n");
            close(ms->socket);
            return;
        }

        ms->self.addr.sin_addr.s_addr = ip->addr;

        char ipbuf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &ms->self.addr.sin_addr, ipbuf, sizeof(ipbuf));
        printf("Using interface IP: %s\r\n", ipbuf);
#else
        ms->self.addr.sin_addr.s_addr = htonl(INADDR_ANY);
#endif
    } else {
        if (inet_pton(AF_INET, addr, &ms->self.addr.sin_addr.s_addr) != 1) {
            printf("Invalid IPv4 address: %s (errno %d: %s)\r\n", addr, errno, strerror(errno));
            close(ms->socket);
            return;
        }
    }

    if (bind(ms->socket, (struct sockaddr*)&ms->self.addr, sizeof(ms->self.addr)) != 0) {
        int err = errno;
        MICROSWIM_LOG_ERROR("`bind` exited with an error code: %d (%s)\n", err, strerror(errno));
        close(ms->socket);
    }
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
