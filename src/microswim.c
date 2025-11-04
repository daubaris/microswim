#include "microswim.h"
#include "log.h"
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

/**
 * @brief Initializes the `microswim_t` structure.
 */
void microswim_initialize(microswim_t* ms) {
    *ms = (microswim_t){ .socket = 0,
                         .self = { { 0 } },
                         .members = { { { 0 } } },
                         .confirmed = { { { 0 } } },
                         .updates = { { 0 } },
                         .pings = { { 0 } },
                         .ping_reqs = { { 0 } },
                         .events = { { 0 } },
                         .indices = { 0 },
                         .member_count = 0,
                         .confirmed_count = 0,
                         .update_count = 0,
                         .ping_count = 0,
                         .ping_req_count = 0,
                         .event_count = 0,
                         .round_robin_index = 0 };
}

/**
 * @brief Sets up the socket.
 *
 * Assigns the supplied IP address and port to the socket and sets it to be non-blocking.
 */
void microswim_socket_setup(microswim_t* ms, char* addr, int port) {
    ms->socket = socket(AF_INET, SOCK_DGRAM, 0);
    ms->self.addr.sin_family = AF_INET;
    ms->self.addr.sin_port = htons(port);
    ms->self.addr.sin_addr.s_addr = inet_addr(addr);

    int flags = fcntl(ms->socket, F_GETFL, 0);
    if (fcntl(ms->socket, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("Failed to set non-blocking");
        close(ms->socket);
    }

    if (bind(ms->socket, (struct sockaddr*)&ms->self.addr, sizeof(ms->self.addr)) != 0) {
        int err = errno;
        LOG_ERROR("`bind` exited with an error code: %d\n", err);
        close(ms->socket);
    }
}

void microswim_index_remove(microswim_t* ms) {
    if ((ms->member_count + 1) == 0)
        return;

    // Find the index of the highest value
    int index = 0;
    for (int i = 1; i < ms->member_count + 1; i++) {
        if (ms->indices[i] > ms->indices[index]) {
            index = i;
        }
    }

    // Shift elements to remove the highest value
    for (int i = index; i < ms->member_count; i++) {
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
    for (int i = ms->member_count - 1; i > index; i--) {
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

int microswim_compare_by_count(const void* a, const void* b) {
    microswim_update_t* u_a = (microswim_update_t*)a;
    microswim_update_t* u_b = (microswim_update_t*)b;

    return u_a->count - u_b->count;
}
