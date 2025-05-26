#include "ping.h"
#include "constants.h"
#include "log.h"
#include "member.h"
#include "microswim.h"
#include "utils.h"
#include <stdlib.h>

microswim_ping_t* microswim_ping_add(microswim_t* ms, microswim_member_t* member) {
    microswim_ping_t* ping = microswim_ping_find(ms, member);
    if (ping != NULL) {
        microswim_ping_remove(ms, ping);
    }

    if (member->uuid[0] != '\0') {
        if (ms->ping_count < MAXIMUM_PINGS) {
            ms->pings[ms->ping_count].ping_req_deadline =
                (microswim_milliseconds() + (uint64_t)(PING_REQ_PERIOD * 1000));
            ms->pings[ms->ping_count].suspect_deadline =
                (microswim_milliseconds() + (uint64_t)(PROTOCOL_PERIOD * 1000));
            ms->pings[ms->ping_count].member = member;
            ms->pings[ms->ping_count].ping_req = false;

            return &ms->pings[ms->ping_count++];
        } else {
            LOG_WARN(
                "Unable to add a new ping: the maximum limit (%d) has been "
                "reached. Consider increasing MAXIMUM_PINGS to allow "
                "additional members.",
                MAXIMUM_PINGS);
            return NULL;
        }
    }

    return NULL;
}

microswim_ping_t* microswim_ping_find(microswim_t* ms, microswim_member_t* member) {
    for (int i = 0; i < ms->ping_count; i++) {
        if (strncmp(ms->pings[i].member->uuid, member->uuid, UUID_SIZE) == 0) {
            return &ms->pings[i];
        }
    }

    return NULL;
}

void microswim_ping_remove(microswim_t* ms, microswim_ping_t* ping) {
    int index;
    for (index = 0; index < ms->ping_count; index++) {
        if (&(ms->pings[index]) == ping) {
            break;
        }
    }

    for (int i = index; i < ms->ping_count - 1; i++) {
        ms->pings[i] = ms->pings[i + 1];
    }

    ms->ping_count--;
}

void microswim_pings_check(microswim_t* ms) {
    for (size_t i = 0; i < ms->ping_count; i++) {
        if (ms->pings[i].suspect_deadline < microswim_milliseconds()) {
            microswim_member_mark_suspect(ms, ms->pings[i].member);
            microswim_ping_t* ping = microswim_ping_find(ms, ms->pings[i].member);
            if (ping != NULL) {
                microswim_ping_remove(ms, ping);
            }
        }
        // else if (ms->pings[i].ping_req_deadline < microswim_milliseconds()) {
        //     if (ms->pings[i].ping_req == false && ms->member_count - 1 > FAILURE_DETECTION_GROUP) {
        //         for (int j = 0; j < FAILURE_DETECTION_GROUP; j++) {
        //             // WARN: this should be reimplemented as now,
        //             // the node might not be picked because there's a change
        //             // the code will not pass the if statement, which in that
        //             // case, it should continue looping until a suitable member
        //             // is found.
        //             // microswim_member_t* member = &ms->members[rand() % (ms->member_count - 1) + 1];
        //             // NOTE: we don't want to ask the same member to make the request.
        //             // We also want the member to be ALIVE.
        //             // if (strncmp(member->uuid, ms->pings[i].member->uuid, UUID_SIZE) != 0 &&
        //             //     (member->status == ALIVE)) {
        //             //     microswim_ping_req_message_send(ms, member, ms->pings[i].member);
        //             //     ms->pings[i].ping_req = true;
        //             // WARN: the below is unneccessary?
        //             // microswim_ping_req_t* ping_req =
        //             //     microswim_ping_req_check_existing(ms, ms->pings[i].member->uuid);
        //             // if (ping_req != NULL) {
        //             //     microswim_ping_req_remove(ms, ping_req);
        //             // }
        //         }
        //     }
        // }
        else {
            continue;
        }
    }
}
