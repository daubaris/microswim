#include "message.h"
#include "constants.h"
#include "decode.h"
#include "encode.h"
#include "member.h"
#include "microswim.h"
#include "microswim_log.h"
#ifdef CUSTOM_CONFIGURATION
#include "configuration.h"
#else
#include "microswim_configuration.h"
#endif
#include "ping.h"
#include "ping_req.h"
#include "update.h"
#include <errno.h>
#include <string.h>

/*
 * @brief Constructs a status message.
 */
void microswim_status_message_construct(
    microswim_t* ms, microswim_message_t* message, microswim_message_type_t type, microswim_member_t* member) {

    strncpy((char*)message->uuid, (char*)ms->self.uuid, UUID_SIZE);
    message->type = type;
    message->addr = ms->self.addr;
    message->status = ms->self.status;
    message->incarnation = ms->self.incarnation;
    message->mu[0] = *member;
    message->update_count = 1;
}

/*
 * @brief Constructs a gossip message.
 */
void microswim_message_construct(
    microswim_t* ms, microswim_message_t* message, microswim_message_type_t type,
    microswim_update_t* updates[MAXIMUM_MEMBERS_IN_AN_UPDATE], size_t update_count) {

    strncpy((char*)message->uuid, (char*)ms->self.uuid, UUID_SIZE);
    message->type = type;
    message->addr = ms->self.addr;
    message->status = ms->self.status;
    message->incarnation = ms->self.incarnation;

    for (size_t i = 0; i < update_count; i++) {
        message->mu[i] = *updates[i]->member;
    }

    message->update_count = update_count;
}

void microswim_message_send(microswim_t* ms, microswim_member_t* member, const char* buffer, size_t length) {
    ssize_t result =
        sendto(ms->socket, buffer, length, 0, (struct sockaddr*)(&member->addr), sizeof(member->addr));
    if (result < 0) {
        MICROSWIM_LOG_ERROR("(microswim_message_send) sendto failed: (%d) %d %s", result, errno, strerror(errno));
    }

    // TODO: return result.
}

void microswim_message_print(microswim_message_t* message) {
    MICROSWIM_LOG_DEBUG(
        "MESSAGE: %s, FROM: %s, STATUS: %d, INCARNATION: %zu, URI: %d",
        (message->type == ALIVE_MESSAGE ?
             "ALIVE MESSAGE" :
             (message->type == SUSPECT_MESSAGE ?
                  "SUSPECT MESSAGE" :
                  (message->type == CONFIRM_MESSAGE ?
                       "CONFIRM MESSAGE" :
                       (message->type == PING_MESSAGE ?
                            "PING MESSAGE" :
                            (message->type == PING_REQ_MESSAGE ? "PING_REQ_MESSAGE" : "ACK MESSAGE"))))),
        message->uuid, message->status, message->incarnation, ntohs(message->addr.sin_port));

    MICROSWIM_LOG_DEBUG("UPDATES:");
    for (size_t i = 0; i < message->update_count; i++) {
        MICROSWIM_LOG_DEBUG(
            "\t%s: STATUS: %d, INCARNATION: %zu", message->mu[i].uuid, message->mu[i].status,
            message->mu[i].incarnation);
    }
}

/*
 * @brief Extracts information from the message.
 */
void microswim_message_extract_members(microswim_t* ms, microswim_message_t* message) {
    microswim_member_t self;
    strncpy((char*)self.uuid, (char*)message->uuid, UUID_SIZE);
    self.addr = message->addr;
    self.status = message->status;
    self.incarnation = message->incarnation;

    microswim_members_check(ms, &self);

    for (size_t i = 0; i < message->update_count; i++) {
        microswim_member_t* message_member = &message->mu[i];
        microswim_members_check(ms, message_member);
    }
}

/*
 * @brief Sends an ACK message. TODO: refactor to use `microswim_message_send function.
 */
void microswim_ack_message_send(microswim_t* ms, struct sockaddr_in addr) {
    microswim_update_t* updates[MAXIMUM_MEMBERS_IN_AN_UPDATE] = { 0 };
    microswim_message_t message = { 0 };

    unsigned char buffer[BUFFER_SIZE] = { 0 };
    int update_count = microswim_updates_retrieve(ms, updates);
    microswim_message_construct(ms, &message, ACK_MESSAGE, updates, update_count);
    size_t len = microswim_encode_message(&message, buffer, BUFFER_SIZE);

    ssize_t result = sendto(ms->socket, buffer, len, 0, (struct sockaddr*)(&addr), sizeof(addr));
    if (result < 0) {
        MICROSWIM_LOG_ERROR("(microswim_ack_message_send) sendto failed.");
    }
}

/*
 * @brief Handles PING message.
 */
static void microswim_ping_message_handle(microswim_t* ms, microswim_message_t* message) {
    // NOTE: if a member receives a ping, it should send an ack.
    // An ack will piggyback known member information.
    microswim_ack_message_send(ms, message->addr);
    // A bit of a hack. Could be done cleaner.
    microswim_member_t temp = { 0 };
    strncpy((char*)temp.uuid, (char*)message->uuid, UUID_SIZE);
    microswim_ping_t* ping = microswim_ping_find(ms, &temp);
    if (ping != NULL) {
        microswim_ping_remove(ms, ping);
    }

    microswim_member_t* member = microswim_member_find(ms, &temp);
    if (member) {
        microswim_member_mark_alive(ms, member);
    }
}

/*
 * @brief Handles ACK message.
 */
void microswim_ack_message_handle(microswim_t* ms, microswim_message_t* message) {
    // TODO: decide what to do when a PING is NULL.
    // It should never happen here, though. But it must be handled.
    microswim_member_t member = { 0 };
    strncpy((char*)member.uuid, (char*)message->uuid, UUID_SIZE);
    microswim_ping_t* ping = microswim_ping_find(ms, &member);

    if (ping != NULL && ping->member->uuid[0] != '\0') {
        microswim_member_mark_alive(ms, ping->member);

        microswim_ping_req_t* ping_req = NULL;
        for (size_t i = 0; i < ms->ping_req_count; i++) {
            if (strncmp((char*)message->uuid, (char*)ms->ping_reqs[i].target->uuid, UUID_SIZE) == 0) {
                ping_req = &ms->ping_reqs[i];
                microswim_update_t* updates[MAXIMUM_MEMBERS_IN_AN_UPDATE] = { 0 };
                microswim_message_t message = { 0 };
                unsigned char buffer[BUFFER_SIZE] = { 0 };
                int update_count = microswim_updates_retrieve(ms, updates);
                microswim_message_construct(ms, &message, ACK_MESSAGE, updates, update_count);
                message.status = ping_req->target->status;
                message.incarnation = ping_req->target->incarnation;
                message.addr = ping_req->target->addr;
                strncpy((char*)message.uuid, (char*)ping_req->target->uuid, UUID_SIZE);
                size_t length = microswim_encode_message(&message, buffer, BUFFER_SIZE);

                microswim_message_send(ms, ping_req->source, (const char*)buffer, length);
                microswim_ping_req_remove(ms, ping_req);
            }
        }

        microswim_ping_remove(ms, ping);
    }
}

/*
 * @brief Handles the incoming message.
 */
void microswim_message_handle(
    microswim_t* ms, unsigned char* buffer, ssize_t len,
    void (*event_handler)(microswim_t*, unsigned char*, ssize_t)) {

    microswim_message_t message = { 0 };
    microswim_message_type_t type = microswim_decode_message_type(buffer, len);

    switch (type) {
        case PING_MESSAGE:
            microswim_decode_message(&message, (const char*)buffer, len);
            microswim_message_print(&message);
            microswim_message_extract_members(ms, &message);
            microswim_ping_message_handle(ms, &message);
            break;
        case PING_REQ_MESSAGE:
            microswim_decode_message(&message, (const char*)buffer, len);
            microswim_message_print(&message);
            microswim_message_extract_members(ms, &message);
            microswim_ping_req_message_handle(ms, &message);
            break;
        case ACK_MESSAGE:
            microswim_decode_message(&message, (const char*)buffer, len);
            microswim_message_print(&message);
            microswim_message_extract_members(ms, &message);
            microswim_ack_message_handle(ms, &message);
            break;
        case ALIVE_MESSAGE:
        case SUSPECT_MESSAGE:
        case CONFIRM_MESSAGE:
            break;
        case EVENT_MESSAGE:
            event_handler(ms, buffer, len);
            break;
        default:
            break;
    }
}
