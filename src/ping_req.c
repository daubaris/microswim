#include "ping_req.h"
#include "log.h"
#include "member.h"
#include "message.h"
#include "ping.h"
#include "utils.h"

void microswim_ping_req_message_handle(microswim_t* ms, microswim_message_t* message) {
    microswim_member_t temp = { 0 };
    strncpy(temp.uuid, message->uuid, UUID_SIZE);
    microswim_member_t* source = microswim_member_find(ms, &temp);
    microswim_member_t* target = microswim_member_find(ms, &message->mu[0]);

    if (!source) {
        LOG_ERROR("Could not find the source member for ping_req");
        return;
    }
    if (!target) {
        LOG_ERROR("Could not find the target member for ping_req");
        return;
    }

    microswim_ping_message_send(ms, target);
    microswim_ping_req_add(ms, source, target);
}

microswim_ping_req_t*
    microswim_ping_req_find(microswim_t* ms, microswim_member_t* source, microswim_member_t* target) {
    for (int i = 0; i < ms->ping_req_count; i++) {
        if (strncmp(ms->ping_reqs[i].source->uuid, source->uuid, UUID_SIZE) == 0 &&
            strncmp(ms->ping_reqs[i].target->uuid, target->uuid, UUID_SIZE) == 0) {
            return &ms->ping_reqs[i];
        }
    }

    return NULL;
}

void microswim_ping_req_remove(microswim_t* ms, microswim_ping_req_t* ping) {
    int index;
    for (index = 0; index < ms->ping_req_count; index++) {
        if (&(ms->ping_reqs[index]) == ping) {
            break;
        }
    }

    for (int i = index; i < ms->ping_req_count - 1; i++) {
        ms->ping_reqs[i] = ms->ping_reqs[i + 1];
    }

    ms->ping_req_count--;
}

// Safe guard to not have any left.
void microswim_ping_reqs_check(microswim_t* ms) {
    for (int i = 0; i < ms->ping_req_count; i++) {
        if (ms->ping_reqs[i].timeout < microswim_milliseconds()) {
            microswim_ping_req_remove(ms, &ms->ping_reqs[i]);
        }
    }
}

void microswim_ping_req_add(microswim_t* ms, microswim_member_t* source, microswim_member_t* target) {
    microswim_ping_req_t* ping_req = microswim_ping_req_find(ms, source, target);
    if (ping_req != NULL) {
        microswim_ping_req_remove(ms, ping_req);
    }

    if (source->uuid[0] != '\0' && target->uuid[0] != '\0') {
        if (ms->ping_req_count < MAXIMUM_PINGS) {
            ms->ping_reqs[ms->ping_req_count].source = source;
            ms->ping_reqs[ms->ping_req_count].target = target;
            ms->ping_reqs[ms->ping_req_count].timeout =
                (microswim_milliseconds() + (uint64_t)(PROTOCOL_PERIOD * 1000));
            ms->ping_req_count++;
        } else {
            LOG_WARN(
                "Unable to add a new ping: the maximum limit (%d) has been "
                "reached. Consider increasing MAXIMUM_PINGS to allow "
                "additional members.",
                MAXIMUM_PINGS);
        }
    }
}
