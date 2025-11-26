#include "ping.h"
#include "constants.h"
#include "encode.h"
#include "member.h"
#include "message.h"
#include "microswim.h"
#include "microswim_log.h"
#include "utils.h"

/**
 * @brief
 */
microswim_ping_t* microswim_ping_add(microswim_t* ms, microswim_member_t* member) {
    microswim_ping_t* ping = microswim_ping_find(ms, member);
    if (ping != NULL) {
        microswim_ping_remove(ms, ping);
    }

    if (member->uuid[0] != '\0') {
        if (ms->ping_count > MAXIMUM_PINGS) {
            MICROSWIM_LOG_ERROR(
                "Unable to add a new ping: the maximum limit (%d) has been "
                "reached. Consider increasing MAXIMUM_PINGS to allow "
                "additional members.",
                MAXIMUM_PINGS);
            return NULL;
        }

        ms->pings[ms->ping_count].ping_req_deadline =
            (microswim_milliseconds() + (uint64_t)(PING_REQ_PERIOD * 1000));
        ms->pings[ms->ping_count].suspect_deadline =
            (microswim_milliseconds() + (uint64_t)(PROTOCOL_PERIOD * 1000));
        ms->pings[ms->ping_count].member = member;
        ms->pings[ms->ping_count].ping_req = false;

        return &ms->pings[ms->ping_count++];
    }

    return NULL;
}

microswim_ping_t* microswim_ping_find(microswim_t* ms, microswim_member_t* member) {
    for (size_t i = 0; i < ms->ping_count; i++) {
        if (strncmp((char*)ms->pings[i].member->uuid, (char*)member->uuid, UUID_SIZE) == 0) {
            return &ms->pings[i];
        }
    }

    return NULL;
}

void microswim_ping_remove(microswim_t* ms, microswim_ping_t* ping) {
    size_t index;
    for (index = 0; index < ms->ping_count; index++) {
        if (&(ms->pings[index]) == ping) {
            break;
        }
    }

    for (size_t i = index; i < ms->ping_count - 1; i++) {
        ms->pings[i] = ms->pings[i + 1];
    }

    ms->ping_count--;
}

void microswim_pings_check(microswim_t* ms) {
    uint64_t now = microswim_milliseconds();

    for (size_t i = 0; i < ms->ping_count;) {
        microswim_ping_t* p = &ms->pings[i];

        if (p->suspect_deadline < now) {
            microswim_member_mark_suspect(ms, p->member);
            size_t last = ms->ping_count - 1;
            if (i != last) {
                ms->pings[i] = ms->pings[last];
            }
            ms->ping_count--;
            continue;
        } else if (p->ping_req_deadline < now && !p->ping_req) {
            size_t members[FAILURE_DETECTION_GROUP];
            size_t count = microswim_get_ping_req_candidates(ms, members);

            if (count > FAILURE_DETECTION_GROUP) {
                count = FAILURE_DETECTION_GROUP;
            }

            for (size_t j = 0; j < count; j++) {
                unsigned char buffer[BUFFER_SIZE] = { 0 };
                microswim_message_t message = { 0 };
                microswim_member_t* member = &ms->members[members[j]];
                microswim_status_message_construct(ms, &message, PING_REQ_MESSAGE, p->member);
                size_t length = microswim_encode_message(&message, buffer, BUFFER_SIZE);
                microswim_message_send(ms, member, (const char*)buffer, length);
                p->ping_req = true;
            }

            i++;
        } else {
            i++;
        }
    }
}
