#include "message.h"
#include "cbor.h"
#include "constants.h"
#include "jsmn.h"
#include "log.h"
#include "member.h"
#include "microswim.h"
#include "ping.h"
#include "ping_req.h"
#include "update.h"
#include <cbor/arrays.h>
#include <cbor/maps.h>
#include <cbor/serialization.h>
#include <cbor/strings.h>
#include <errno.h>
#include <string.h>

void microswim_status_message_construct(
    microswim_t* ms, microswim_message_t* message, microswim_message_type_t type, microswim_member_t* member) {
    strncpy(message->uuid, ms->self.uuid, UUID_SIZE);
    message->type = type;
    message->addr = ms->self.addr;
    message->status = ms->self.status;
    message->incarnation = ms->self.incarnation;
    message->mu[0] = *member;
    message->update_count = 1;
}

void microswim_message_construct(
    microswim_t* ms, microswim_message_t* message, microswim_message_type_t type,
    microswim_update_t* updates[MAXIMUM_MEMBERS_IN_AN_UPDATE], size_t update_count) {

    message->type = type;
    message->status = ms->self.status;
    message->incarnation = ms->self.incarnation;
    message->addr = ms->self.addr;

    strncpy(message->uuid, ms->self.uuid, UUID_SIZE);

    for (size_t i = 0; i < update_count; i++) {
        message->mu[i] = *updates[i]->member;
    }

    message->update_count = update_count;
}

void microswim_ping_message_send(microswim_t* ms, microswim_member_t* member) {
    microswim_update_t* updates[MAXIMUM_MEMBERS_IN_AN_UPDATE] = { 0 };
    microswim_message_t message = { 0 };
    unsigned char buffer[BUFFER_SIZE] = { 0 };
    size_t update_count = microswim_updates_retrieve(ms, updates);
    microswim_message_construct(ms, &message, PING_MESSAGE, updates, update_count);
    size_t len = microswim_encode_message(&message, buffer, BUFFER_SIZE);

    ssize_t result = sendto(
        ms->socket, (const char*)buffer, len, 0, (struct sockaddr*)(&member->addr), sizeof(member->addr));
    if (result < 0) {
        LOG_ERROR("sendto failed: %s\n", strerror(errno));
    }
}

void microswim_status_message_send(microswim_t* ms, microswim_member_t* member, microswim_message_t* message) {
    // A message is sent after suspected node is marked as alive.
    unsigned char buffer[BUFFER_SIZE] = { 0 };
    if (member != NULL) {
        size_t len = microswim_encode_message(message, buffer, BUFFER_SIZE);
        ssize_t result =
            sendto(ms->socket, buffer, len, 0, (struct sockaddr*)(&member->addr), sizeof(member->addr));
        if (result < 0) {
            LOG_ERROR("sendto failed: %s\n", strerror(errno));
        }
    }
}

void microswim_ping_req_message_send(microswim_t* ms, microswim_member_t* member, microswim_message_t* message) {
    unsigned char buffer[BUFFER_SIZE] = { 0 };
    size_t len = microswim_encode_message(message, buffer, BUFFER_SIZE);
    ssize_t result =
        sendto(ms->socket, buffer, len, 0, (struct sockaddr*)(&member->addr), sizeof(member->addr));
    if (result < 0) {
        LOG_ERROR("sendto failed: %s\n", strerror(errno));
    }
}

void microswim_message_print(microswim_message_t* message) {
    LOG_DEBUG(
        "MESSAGE: %s, FROM: %s, STATUS: %d, INCARNATION: %d, URI: %d",
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

    LOG_DEBUG("UPDATES:");
    for (int i = 0; i < message->update_count; i++) {
        LOG_DEBUG(
            "\t%s: STATUS: %d, INCARNATION: %zu", message->mu[i].uuid, message->mu[i].status,
            message->mu[i].incarnation);
    }
}

void microswim_message_extract_members(microswim_t* ms, microswim_message_t* message) {
    microswim_member_t self;
    strncpy(self.uuid, message->uuid, UUID_SIZE);
    self.addr = message->addr;
    self.status = message->status;
    self.incarnation = message->incarnation;

    microswim_members_check(ms, &self);

    for (int i = 0; i < message->update_count; i++) {
        microswim_member_t* message_member = &message->mu[i];
        microswim_members_check(ms, message_member);
    }
}

void microswim_ack_message_send(microswim_t* ms, struct sockaddr_in addr) {
    microswim_update_t* updates[MAXIMUM_MEMBERS_IN_AN_UPDATE] = { 0 };
    microswim_message_t message = { 0 };

    unsigned char buffer[BUFFER_SIZE] = { 0 };
    int update_count = microswim_updates_retrieve(ms, updates);
    microswim_message_construct(ms, &message, ACK_MESSAGE, updates, update_count);
    size_t len = microswim_encode_message(&message, buffer, BUFFER_SIZE);

    ssize_t result = sendto(ms->socket, buffer, len, 0, (struct sockaddr*)(&addr), sizeof(addr));
    if (result < 0) {
        LOG_ERROR("sendto failed: %s\n", strerror(errno));
    }
}

static void microswim_ping_message_handle(microswim_t* ms, microswim_message_t* message) {
    // NOTE: if a member receives a ping, it should send an ack.
    // An ack will piggyback known member information.
    microswim_ack_message_send(ms, message->addr);
    // A bit of a hack. Could be done cleaner.
    microswim_member_t temp = { 0 };
    strncpy(temp.uuid, message->uuid, UUID_SIZE);
    microswim_ping_t* ping = microswim_ping_find(ms, &temp);
    if (ping != NULL) {
        microswim_ping_remove(ms, ping);
    }

    microswim_member_t* member = microswim_member_find(ms, &temp);
    if (member) {
        microswim_member_mark_alive(ms, member);
    }
}

void microswim_ack_message_handle(microswim_t* ms, microswim_message_t* message) {
    // TODO: decide what to do when a PING is NULL.
    // It should never happen here, though. But it must be handled.
    microswim_member_t member = { 0 };
    strncpy(member.uuid, message->uuid, UUID_SIZE);
    microswim_ping_t* ping = microswim_ping_find(ms, &member);

    if (ping != NULL && ping->member->uuid[0] != '\0') {
        microswim_member_mark_alive(ms, ping->member);

        microswim_ping_req_t* ping_req = NULL;
        for (int i = 0; i < ms->ping_req_count; i++) {
            if (strncmp(ping->member->uuid, ms->ping_reqs[i].target->uuid, UUID_SIZE) == 0) {
                ping_req = &ms->ping_reqs[i];
                microswim_update_t* updates[MAXIMUM_MEMBERS_IN_AN_UPDATE] = { 0 };
                microswim_message_t message = { 0 };

                unsigned char buffer[BUFFER_SIZE] = { 0 };
                int update_count = microswim_updates_retrieve(ms, updates);
                microswim_message_construct(ms, &message, ACK_MESSAGE, updates, update_count);
                message.status = ping_req->target->status;
                message.incarnation = ping_req->target->incarnation;
                message.addr = ping_req->target->addr;

                strncpy(message.uuid, ping_req->target->uuid, UUID_SIZE);
                size_t len = microswim_encode_message(&message, buffer, BUFFER_SIZE);

                ssize_t result = sendto(
                    ms->socket, buffer, len, 0, (struct sockaddr*)(&ping_req->source->addr),
                    sizeof(ping_req->source->addr));
                if (result < 0) {
                    LOG_ERROR("sendto failed: %s\n", strerror(errno));
                }

                microswim_ping_req_remove(ms, ping_req);
            }
        }

        microswim_ping_remove(ms, ping);
    }
}

void microswim_message_handle(microswim_t* ms, unsigned char* buffer, ssize_t len) {
    microswim_message_t message = { 0 };
    microswim_decode_message(&message, buffer, len);
    microswim_message_print(&message);
    microswim_message_extract_members(ms, &message);

    switch (message.type) {
        case PING_MESSAGE:
            microswim_ping_message_handle(ms, &message);
            break;
        case PING_REQ_MESSAGE:
            microswim_ping_req_message_handle(ms, &message);
            break;
        case ACK_MESSAGE:
            microswim_ack_message_handle(ms, &message);
            break;
        case ALIVE_MESSAGE:
        case SUSPECT_MESSAGE:
        case CONFIRM_MESSAGE:
            break;
        default:
            break;
    }
}

static void microswim_sockaddr_to_uri(struct sockaddr_in* addr, char* buffer, size_t buffer_size) {
    char ip_str[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr->sin_addr), ip_str, sizeof(ip_str));
    int port = ntohs(addr->sin_port);
    snprintf(buffer, buffer_size, "%s:%d", ip_str, port);
}

#ifdef MICROSWIM_CBOR
static void microswim_cbor_uri_to_sockaddr(struct sockaddr_in* addr, cbor_item_t* item) {
    size_t length = cbor_string_length(item);
    char buffer[length];
    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer, cbor_string_handle(item), sizeof(buffer));
    char* colon = strchr(buffer, ':');
    if (colon) {
        int port_length = (buffer + length) - (colon);
        char p[port_length - 1];
        memset(p, 0, sizeof(p));
        memcpy(p, colon + 1, sizeof(p));
        buffer[length - port_length] = '\0';
        p[port_length] = '\0';
        int port = strtol(p, NULL, 10);
        if (port) {
            if (inet_pton(AF_INET, buffer, &(addr->sin_addr)) != 1) {
                LOG_ERROR("Invalid IP address: %s\n", buffer);
                exit(1);
            }
            addr->sin_port = htons(port);
            addr->sin_family = AF_INET;
        }
    }
}

size_t microswim_cbor_encode_message(microswim_message_t* message, unsigned char* buffer, size_t size) {
    cbor_item_t* origin_map = cbor_new_definite_map(6);
    char uri_buffer[INET6_ADDRSTRLEN];
    microswim_sockaddr_to_uri(&message->addr, uri_buffer, sizeof(uri_buffer));
    int success = cbor_map_add(
        origin_map,
        (struct cbor_pair){ .key = cbor_move(cbor_build_string("message")),
                            .value = cbor_move(cbor_build_uint8((uint8_t)message->type)) });
    success &= cbor_map_add(
        origin_map,
        (struct cbor_pair){
            .key = cbor_move(cbor_build_string("uuid")),
            .value = cbor_move(message->uuid[0] != '\0' ? cbor_build_string(message->uuid) : cbor_new_null()) });
    success &= cbor_map_add(
        origin_map,
        (struct cbor_pair){ .key = cbor_move(cbor_build_string("uri")),
                            .value = cbor_move(cbor_build_string(uri_buffer)) });
    success &= cbor_map_add(
        origin_map,
        (struct cbor_pair){ .key = cbor_move(cbor_build_string("status")),
                            .value = cbor_move(cbor_build_uint8((uint8_t)message->status)) });
    success &= cbor_map_add(
        origin_map,
        (struct cbor_pair){ .key = cbor_move(cbor_build_string("incarnation")),
                            .value = cbor_move(cbor_build_uint8((uint8_t)message->incarnation)) });
    if (!success) {
        LOG_ERROR("Preallocated storage for map is full (origin_map)");
        return 0;
    }

    cbor_item_t* update_array = cbor_new_definite_array(message->update_count);

    for (int i = 0; i < message->update_count; i++) {
        char uri_buffer[INET6_ADDRSTRLEN];
        microswim_sockaddr_to_uri(&message->mu[i].addr, uri_buffer, sizeof(uri_buffer));
        cbor_item_t* update_map = cbor_new_definite_map(4);
        int success = cbor_map_add(
            update_map,
            (struct cbor_pair){ .key = cbor_move(cbor_build_string("uuid")),
                                .value = cbor_move(cbor_build_string(message->mu[i].uuid)) });
        success &= cbor_map_add(
            update_map,
            (struct cbor_pair){ .key = cbor_move(cbor_build_string("uri")),
                                .value = cbor_move(cbor_build_string(uri_buffer)) });
        success &= cbor_map_add(
            update_map,
            (struct cbor_pair){ .key = cbor_move(cbor_build_string("status")),
                                .value = cbor_move(cbor_build_uint8((uint8_t)message->mu[i].status)) });
        success &= cbor_map_add(
            update_map,
            (struct cbor_pair){ .key = cbor_move(cbor_build_string("incarnation")),
                                .value = cbor_move(cbor_build_uint8((uint8_t)message->mu[i].incarnation)) });
        success &= cbor_array_push(update_array, cbor_move(update_map));

        if (!success) {
            LOG_ERROR("Preallocated storage for map is full (update_map)");
            return 0;
        }
    }

    success &= cbor_map_add(
        origin_map,
        (struct cbor_pair){ .key = cbor_move(cbor_build_string("updates")), .value = cbor_move(update_array) });
    if (!success) {
        LOG_ERROR("Preallocated storage for map is full (origin_map, update_array)");
        return 0;
    }

    size_t len = cbor_serialize(origin_map, buffer, size);
    if (!len) {
        LOG_ERROR("Message serialization has failed");
        return 0;
    }

    cbor_decref(&origin_map);

    return len;
}

// Function to handle processing updates (the array inside "updates")
static void microswim_cbor_process_updates(microswim_message_t* message, cbor_item_t* updates) {
    size_t update_count = cbor_array_size(updates);
    message->update_count = (int)update_count;
    for (size_t j = 0; j < update_count; j++) {
        cbor_item_t* array_item = cbor_array_handle(updates)[j];
        for (size_t k = 0; k < cbor_map_size(array_item); k++) {
            struct cbor_pair array_pair = cbor_map_handle(array_item)[k];
            size_t array_key_length = cbor_string_length(array_pair.key);
            char array_key[array_key_length];
            memcpy(array_key, cbor_string_handle(array_pair.key), array_key_length);
            if (strncmp(array_key, "uuid", array_key_length) == 0) {
                size_t uuid_length = cbor_string_length(array_pair.value);
                memcpy(message->mu[j].uuid, cbor_string_handle(array_pair.value), uuid_length);
                message->mu[j].uuid[uuid_length] = '\0';
            } else if (strncmp(array_key, "uri", array_key_length) == 0) {
                microswim_cbor_uri_to_sockaddr(&message->mu[j].addr, array_pair.value);
            } else if (strncmp(array_key, "status", array_key_length) == 0) {
                size_t status = cbor_get_uint8(array_pair.value);
                message->mu[j].status = (microswim_member_status_t)status;
            } else if (strncmp(array_key, "incarnation", array_key_length) == 0) {
                size_t incarnation = cbor_get_uint8(array_pair.value);
                message->mu[j].incarnation = (int)incarnation;
            }
        }
    }
}
// Function to handle processing each key-value pair
static void microswim_cbor_process_pair(microswim_message_t* message, const char* key, size_t key_length, struct cbor_pair pair) {
    if (strncmp(key, "message", key_length) == 0) {
        size_t value = cbor_get_uint8(pair.value);
        message->type = (microswim_message_type_t)value;
    } else if (strncmp(key, "uuid", key_length) == 0) {
        size_t length = cbor_string_length(pair.value);
        memcpy(message->uuid, cbor_string_handle(pair.value), length);
        message->uuid[length] = '\0';
    } else if (strncmp(key, "uri", key_length) == 0) {
        microswim_cbor_uri_to_sockaddr(&message->addr, pair.value);
    } else if (strncmp(key, "status", key_length) == 0) {
        size_t value = cbor_get_uint8(pair.value);
        message->status = (microswim_member_status_t)value;
    } else if (strncmp(key, "incarnation", key_length) == 0) {
        size_t value = cbor_get_uint8(pair.value);
        message->incarnation = value;
    } else if (strncmp(key, "updates", key_length) == 0) {
        microswim_cbor_process_updates(message, pair.value);
    }
}

void microswim_cbor_decode_message(microswim_message_t* message, unsigned char* buffer, ssize_t len) {
    struct cbor_load_result result;
    cbor_item_t* root = cbor_load(buffer, len, &result);

    if (result.error.code != CBOR_ERR_NONE) {
        LOG_ERROR(
            "There was an error while reading the input near byte %zu (read "
            "%zu bytes in total): \n",
            result.error.position, result.read);

        return;
    }

    switch (cbor_typeof(root)) {
        case CBOR_TYPE_MAP:
            for (size_t i = 0; i < cbor_map_size(root); i++) {
                struct cbor_pair pair = cbor_map_handle(root)[i];
                size_t key_length = cbor_string_length(pair.key);
                char key[key_length];
                memcpy(key, cbor_string_handle(pair.key), key_length);
                microswim_cbor_process_pair(message, key, key_length, pair);
            }
            break;
        default:
            LOG_ERROR("Wrong message type: %d, ignoring...", cbor_typeof(root));
            break;
    }

    cbor_decref(&root);
}
#endif

#ifdef MICROSWIM_JSON
static void microswim_json_uri_to_sockaddr(struct sockaddr_in* addr, char* buffer, size_t length) {
    char* colon = strchr(buffer, ':');
    if (colon) {
        int port_length = (buffer + length) - (colon);
        char p[port_length - 1];
        memset(p, 0, sizeof(p));
        memcpy(p, colon + 1, sizeof(p));
        buffer[length - port_length] = '\0';
        p[port_length] = '\0';
        int port = strtol(p, NULL, 10);
        if (port) {
            if (inet_pton(AF_INET, buffer, &(addr->sin_addr)) != 1) {
                LOG_ERROR("Invalid IP address: %s\n", buffer);
                exit(1);
            }
            addr->sin_port = htons(port);
            addr->sin_family = AF_INET;
        }
    }
}

static int jsoneq(const char* json, jsmntok_t* tok, const char* s) {
    if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
        strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return 0;
    }
    return -1;
}

// TODO: might need to change from void to int to represent successful parse.
void microswim_json_decode_message(microswim_message_t* message, const char* buffer, ssize_t len) {
    int r;
    jsmn_parser p;
    jsmntok_t t[len];

    jsmn_init(&p);
    r = jsmn_parse(&p, buffer, strlen(buffer), t, sizeof(t) / sizeof(t[0]));
    if (r < 0) {
        printf("Failed to parse JSON: %d\n", r);
        return;
    }

    if (r < 1 || t[0].type != JSMN_OBJECT) {
        printf("Object expected\n");
        return;
    }

    for (int i = 0; i < r; i++) {
        if (jsoneq(buffer, &t[i], "message") == 0) {
            message->type = strtol(buffer + t[i + 1].start, NULL, 10);
            i++;
        }
        if (jsoneq(buffer, &t[i], "uuid") == 0) {
            strncpy(message->uuid, buffer + t[i + 1].start, t[i + 1].end - t[i + 1].start);
            i++;
        }
        if (jsoneq(buffer, &t[i], "uri") == 0) {
            char uri_buffer[t[i + 1].end - t[i + 1].start];
            memset(uri_buffer, 0, sizeof(uri_buffer));
            strncpy(uri_buffer, buffer + t[i + 1].start, t[i + 1].end - t[i + 1].start);
            microswim_json_uri_to_sockaddr(&message->addr, uri_buffer, t[i + 1].end - t[i + 1].start);
            i++;
        }
        if (jsoneq(buffer, &t[i], "status") == 0) {
            message->status = strtol(buffer + t[i + 1].start, NULL, 10);
            i++;
        }
        if (jsoneq(buffer, &t[i], "incarnation") == 0) {
            message->incarnation = strtol(buffer + t[i + 1].start, NULL, 10);
            i++;
        }
        if (jsoneq(buffer, &t[i], "updates") == 0) {
            if (t[i + 1].type != JSMN_ARRAY) {
                printf("Expected an array.\n");
            }
            // + 1 means that we hit the '[', indicating an array.
            int array_size = t[i + 1].size;
            message->update_count = array_size;

            // + 2 means that we hit the '{', indicating an object.
            if (t[i + 2].type != JSMN_OBJECT) {
                printf("Expected an object.\n");
            }

            int object_size = t[i + 2].size;

            // Two loops: one for the array, and the other for the object.
            for (int j = 0; j < array_size; j++) {
                jsmntok_t* object = &t[i + j + 2];
                if (object->type != JSMN_OBJECT) {
                    printf("expected an object\n");
                }
                for (int k = 0; k < object_size; k++) {
                    // if +2 is the object, it means the content will start at +3
                    jsmntok_t* inner = &t[i + j + k + 3];
                    if (jsoneq(buffer, inner, "uuid") == 0) {
                        strncpy(
                            message->mu[j].uuid, buffer + (inner + 1)->start,
                            (inner + 1)->end - (inner + 1)->start);
                        i++;
                    } else if (jsoneq(buffer, inner, "uri") == 0) {
                        char uri_buffer[(inner + 1)->end - (inner + 1)->start];
                        memset(uri_buffer, 0, sizeof(uri_buffer));
                        strncpy(
                            uri_buffer, buffer + (inner + 1)->start, (inner + 1)->end - (inner + 1)->start);
                        microswim_json_uri_to_sockaddr(
                            &message->mu[j].addr, uri_buffer, (inner + 1)->end - (inner + 1)->start);
                        i++;
                    } else if (jsoneq(buffer, inner, "status") == 0) {
                        message->mu[j].status = strtol(buffer + (inner + 1)->start, NULL, 10);
                        i++;
                    } else if (jsoneq(buffer, inner, "incarnation") == 0) {
                        message->mu[j].incarnation = strtol(buffer + (inner + 1)->start, NULL, 10);
                        i++;
                    }
                }

                i += object_size;
            }

            break;
        }
    }
}

size_t microswim_json_encode_message(microswim_message_t* message, unsigned char* buffer, size_t size) {
    char uri_buffer[INET6_ADDRSTRLEN];
    microswim_sockaddr_to_uri(&message->addr, uri_buffer, sizeof(uri_buffer));

    int remainder = snprintf(
        (char*)buffer, BUFFER_SIZE,
        "{\"message\": %d, \"uuid\": \"%s\", \"uri\": \"%s\", \"status\": %d, \"incarnation\": %d, "
        "\"updates\": [",
        message->type, message->uuid, uri_buffer, message->status, message->incarnation);

    for (int i = 0; i < message->update_count; i++) {
        char uri_buffer[INET6_ADDRSTRLEN];
        microswim_sockaddr_to_uri(&message->mu[i].addr, uri_buffer, sizeof(uri_buffer));
        remainder += snprintf(
            (char*)buffer + remainder, BUFFER_SIZE - remainder,
            "{\"uuid\": \"%s\", \"uri\": \"%s\", \"status\": %d, \"incarnation\": %d}",
            message->mu[i].uuid, uri_buffer, message->mu[i].status, message->mu[i].incarnation);

        if (i < message->update_count - 1) {
            remainder += snprintf((char*)buffer + remainder, BUFFER_SIZE - remainder, ",");
        } else {
            remainder += snprintf((char*)buffer + remainder, BUFFER_SIZE - remainder, "]}");
        }
    }

    return remainder;
}
#endif

size_t microswim_encode_message(microswim_message_t* message, unsigned char* buffer, size_t size) {
#ifdef MICROSWIM_CBOR
    return microswim_cbor_encode_message(message, buffer, size);
#endif
#ifdef MICROSWIM_JSON
    return microswim_json_encode_message(message, buffer, size);
#endif
}

void microswim_decode_message(microswim_message_t* message, unsigned char* buffer, ssize_t len) {
#ifdef MICROSWIM_CBOR
    microswim_cbor_decode_message(message, buffer, len);
#endif
#ifdef MICROSWIM_JSON
    microswim_json_decode_message(message, (const char*)buffer, len);
#endif
}
