#include "cbor.h"
#include "event.h"
#include "log.h"
#include "member.h"
#include "message.h"
#include "microswim.h"
#include "ping.h"
#include "ping_req.h"
#include "update.h"
#include "utils.h"

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#define IMAGE_SIZE 32 * 32

typedef enum { EVENT_INFERENCE_REQUEST, EVENT_UNKNOWN, EVENT_MALFORMED } mock_event_type_t;

typedef struct {
    mock_event_type_t event_type;
    uint8_t data[IMAGE_SIZE];
} mock_inference_request_t;

static void cbor_process_pair(microswim_message_t* message, const char* key, size_t key_length, struct cbor_pair pair) {
    if (strncmp(key, "event", key_length) == 0) {
        size_t value = cbor_get_uint8(pair.value);
        message->type = (microswim_message_type_t)value;
    } else if (strncmp(key, "uuid", key_length) == 0) {
        size_t length = cbor_string_length(pair.value);
        memcpy(message->uuid, cbor_string_handle(pair.value), length);
        message->uuid[length] = '\0';
    }
}

mock_event_type_t cbor_event_type_get(unsigned char* buffer, ssize_t len) {
    struct cbor_load_result result;
    cbor_item_t* root = cbor_load(buffer, len, &result);

    if (result.error.code != CBOR_ERR_NONE) {
        LOG_ERROR(
            "There was an error while reading the input near byte %zu (read "
            "%zu bytes in total): \n",
            result.error.position, result.read);

        return EVENT_MALFORMED;
    }

    mock_event_type_t event_type = EVENT_UNKNOWN;

    switch (cbor_typeof(root)) {
        case CBOR_TYPE_MAP: {
            struct cbor_pair pair = cbor_map_handle(root)[2];
            size_t key_length = cbor_string_length(pair.key);
            char key[key_length];
            memcpy(key, cbor_string_handle(pair.key), key_length);
            // cbor_process_pair(message, key, key_length, pair);
            if (strncmp(key, "event_type", key_length) == 0) {
                size_t value = cbor_get_uint8(pair.value);
                event_type = (mock_event_type_t)value;
            }
        }
        default:
            LOG_ERROR("Wrong message type: %d, ignoring...", cbor_typeof(root));
            break;
    }

    cbor_decref(&root);

    return event_type;
}

void cbor_decode_mock_event_message(microswim_message_t* message, unsigned char* buffer, ssize_t len) {
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
                cbor_process_pair(message, key, key_length, pair);
            }
            break;
        default:
            LOG_ERROR("Wrong message type: %d, ignoring...", cbor_typeof(root));
            break;
    }

    cbor_decref(&root);
}

// void mock_event_handle(microswim_t* ms, unsigned char* buffer, ssize_t len) {
//     cbor_decode_mock_event_message();
// }

void* listener(void* params) {
    microswim_t* ms = (microswim_t*)params;

    for (;;) {
        pthread_mutex_lock(&ms->mutex);
        microswim_ping_reqs_check(ms);
        microswim_pings_check(ms);
        microswim_members_check_suspects(ms);

        unsigned char buffer[BUFFER_SIZE] = { 0 };
        struct sockaddr_in from;
        socklen_t len = sizeof(from);

        ssize_t bytes = recvfrom(ms->socket, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&from, &len);

        if (bytes > 0) {
            buffer[bytes] = '\0';
            microswim_message_type_t message_type = microswim_message_type_get(buffer, bytes);

            if (message_type == EVENT_MESSAGE) {
                // mock_event_handle(ms, buffer, bytes);
                LOG_DEBUG("Received an event!");

                mock_event_type_t event_type = cbor_event_type_get(buffer, bytes);

                printf("MOCK EVENT_TYPE: %d\n", event_type);
            } else {
                microswim_message_handle(ms, buffer, bytes, NULL);
            }
        }

        pthread_mutex_unlock(&ms->mutex);
        usleep(100 * 10);
    }
}

void* failure_detection(void* params) {
    microswim_t* ms = (microswim_t*)params;

    for (;;) {
        pthread_mutex_lock(&ms->mutex);

        for (int i = 0; i < GOSSIP_FANOUT; i++) {
            microswim_member_t* member = microswim_member_retrieve(ms);
            if (member != NULL) {
                microswim_ping_message_send(ms, member);
                microswim_ping_add(ms, member);
            }
        }

        LOG_DEBUG("ms->ping_count: %zu", ms->ping_count);
        LOG_DEBUG("ms->ping_req_count: %zu", ms->ping_req_count);
        LOG_DEBUG("ms->update_count: %zu", ms->update_count);
        LOG_DEBUG("ms->member_count: %zu", ms->member_count);
        LOG_DEBUG("ms->confirmed_count: %zu", ms->confirmed_count);
        printf("[DEBUG] ms->indices: [");
        for (int i = 0; i < ms->member_count; i++) {
            if (i < ms->member_count - 1) {
                printf("%zu ", ms->indices[i]);
            } else {
                printf("%zu]\n", ms->indices[i]);
            }
        }

        pthread_mutex_unlock(&ms->mutex);
        usleep(PROTOCOL_PERIOD * 1000000);
    }
}

void* inference(void* params) {
    microswim_t* ms = (microswim_t*)params;

    for (;;) {
        pthread_mutex_lock(&ms->mutex);

        int accuracy = microswim_random(80);
        LOG_DEBUG("Accuracy: %d", accuracy);
        if (accuracy < 80) {
            int member_index = microswim_random(ms->member_count);
            printf("CHOSEN MEMBER: %d\n", member_index);
            microswim_event_message_t message;
            mock_inference_request_t event;
            message.type = EVENT_MESSAGE;
            message.payload = (void*)&event;
            event.event_type = EVENT_INFERENCE_REQUEST;
            memset(event.data, 1, sizeof(event.data));

            unsigned char buffer[32 * 32 + 50];
            size_t len = ms->events[event.event_type].encoder(&message, buffer, 32 * 32 + 50);
            LOG_DEBUG("SENDING A EVENT TO: %s", ms->members[member_index].uuid);
            ssize_t result = sendto(
                ms->socket, (const char*)buffer, len, 0,
                (struct sockaddr*)(&ms->members[member_index].addr),
                sizeof(ms->members[member_index].addr));
            if (result < 0) {
                LOG_ERROR("sendto failed: %s\n", strerror(errno));
            }
        }

        pthread_mutex_unlock(&ms->mutex);
        usleep(10 * 1000000);
    }
}

size_t inference_request_encoder(void* raw, void* output, size_t size) {
    microswim_event_message_t* message = (microswim_event_message_t*)raw;
    mock_inference_request_t* payload = (mock_inference_request_t*)message->payload;

    cbor_item_t* origin_map = cbor_new_definite_map(2);
    int success = cbor_map_add(
        origin_map,
        (struct cbor_pair){ .key = cbor_move(cbor_build_string("message")),
                            .value = cbor_move(cbor_build_uint8((uint8_t)message->type)) });
    if (!success) {
        LOG_ERROR("Preallocated storage for map is full (origin_map)");
        return 0;
    }

    cbor_item_t* payload_map = cbor_new_definite_map(2);
    success = cbor_map_add(
        payload_map,
        (struct cbor_pair){ .key = cbor_move(cbor_build_string("event_type")),
                            .value = cbor_move(cbor_build_uint8((uint8_t)payload->event_type)) });

    success &= cbor_map_add(
        payload_map,
        (struct cbor_pair){ .key = cbor_move(cbor_build_string("data")),
                            .value = cbor_move(cbor_build_bytestring(payload->data, 32 * 32)) });
    if (!success) {
        LOG_ERROR("Preallocated storage for map is full (origin_map, update_array)");
        return 0;
    }

    success &= cbor_map_add(
        origin_map,
        (struct cbor_pair){ .key = cbor_move(cbor_build_string("payload")), .value = cbor_move(payload_map) });
    if (!success) {
        LOG_ERROR("Preallocated storage for map is full (origin_map, update_array)");
        return 0;
    }

    size_t len = cbor_serialize(origin_map, output, size);
    if (!len) {
        LOG_ERROR("Message serialization has failed");
        return 0;
    }

    cbor_decref(&origin_map);

    return len;
}

bool inference_request_decoder(void* raw, void* output) {
}

void inference_request_handler(void* data) {
}

int main(int argc, char** argv) {
    srand(time(NULL));

    microswim_t ms;
    memset(&ms, 0, sizeof(ms));
    microswim_socket_setup(&ms, argv[1], atoi(argv[2]));

    microswim_member_t member;

    member.uuid[0] = '\0';
    member.addr.sin_family = AF_INET;
    member.addr.sin_port = htons(atoi(argv[4]));
    member.status = ALIVE;
    member.incarnation = 0;

    if (inet_pton(AF_INET, argv[3], &(member.addr.sin_addr)) != 1) {
        LOG_ERROR("Invalid IP address: %s\n", argv[3]);
        return 1;
    }

    microswim_member_t* remote = microswim_member_add(&ms, member);
    if (remote) {
        microswim_update_add(&ms, remote);
        microswim_index_add(&ms);
    }
    // EVENT_INFERENCE_REQUEST,
    microswim_event_t inference_request = { EVENT_INFERENCE_REQUEST, sizeof(mock_inference_request_t),
                                            &inference_request_encoder, &inference_request_decoder,
                                            &inference_request_handler };
    microswim_event_register(&ms, inference_request);

    pthread_t fd_thread, pl_thread;
    pthread_create(&fd_thread, NULL, failure_detection, (void*)&ms);
    pthread_create(&fd_thread, NULL, inference, (void*)&ms);
    pthread_create(&pl_thread, NULL, listener, (void*)&ms);

    pthread_join(fd_thread, NULL);
    pthread_join(pl_thread, NULL);

    close(ms.socket);
    pthread_mutex_destroy(&ms.mutex);

    return 0;
}
