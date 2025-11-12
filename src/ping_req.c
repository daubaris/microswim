#include "ping_req.h"
#include "encode.h"
#include "member.h"
#include "message.h"
#include "microswim_log.h"
#include "update.h"
#include "utils.h"

void microswim_ping_req_message_handle(microswim_t* ms, microswim_message_t* message) {
    microswim_member_t temp = { 0 };
    strncpy((char*)temp.uuid, (char*)message->uuid, UUID_SIZE);
    microswim_member_t* source = microswim_member_find(ms, &temp);
    microswim_member_t* target = microswim_member_find(ms, &message->mu[0]);

    if (!source) {
        MICROSWIM_LOG_ERROR("Could not find the source member for ping_req");
        return;
    }
    if (!target) {
        MICROSWIM_LOG_ERROR("Could not find the target member for ping_req");
        return;
    }

    unsigned char buffer[BUFFER_SIZE] = { 0 };
    microswim_message_t ping_message = { 0 };
    microswim_update_t* updates[MAXIMUM_MEMBERS_IN_AN_UPDATE] = { 0 };
    size_t update_count = microswim_updates_retrieve(ms, updates);
    microswim_message_construct(ms, &ping_message, PING_MESSAGE, updates, update_count);
    size_t length = microswim_encode_message(&ping_message, buffer, BUFFER_SIZE);

    microswim_message_send(ms, target, (const char*)buffer, length);
    microswim_ping_req_add(ms, source, target);
}

microswim_ping_req_t*
    microswim_ping_req_find(microswim_t* ms, microswim_member_t* source, microswim_member_t* target) {
    for (size_t i = 0; i < ms->ping_req_count; i++) {
        if (strncmp((char*)ms->ping_reqs[i].source->uuid, (char*)source->uuid, UUID_SIZE) == 0 &&
            strncmp((char*)ms->ping_reqs[i].target->uuid, (char*)target->uuid, UUID_SIZE) == 0) {
            return &ms->ping_reqs[i];
        }
    }

    return NULL;
}

void microswim_ping_req_remove(microswim_t* ms, microswim_ping_req_t* ping) {
    size_t index;
    for (index = 0; index < ms->ping_req_count; index++) {
        if (&(ms->ping_reqs[index]) == ping) {
            break;
        }
    }

    for (size_t i = index; i < ms->ping_req_count - 1; i++) {
        ms->ping_reqs[i] = ms->ping_reqs[i + 1];
    }

    ms->ping_req_count--;
}

// Safe guard to not have any left.
void microswim_ping_reqs_check(microswim_t* ms) {
    for (size_t i = 0; i < ms->ping_req_count; i++) {
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
            MICROSWIM_LOG_WARN(
                "Unable to add a new ping: the maximum limit (%d) has been "
                "reached. Consider increasing MAXIMUM_PINGS to allow "
                "additional members.",
                MAXIMUM_PINGS);
        }
    }
}
