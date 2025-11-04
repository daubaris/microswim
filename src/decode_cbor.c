#ifdef MICROSWIM_CBOR

#include "cbor.h"
#include "log.h"
#include "microswim.h"

microswim_message_type_t microswim_decode_message_type(unsigned char* buffer, ssize_t len) {
    struct cbor_load_result result;
    cbor_item_t* root = cbor_load(buffer, len, &result);

    if (result.error.code != CBOR_ERR_NONE) {
        LOG_ERROR(
            "There was an error while reading the input near byte %zu (read "
            "%zu bytes in total)",
            result.error.position, result.read);

        return MALFORMED_MESSAGE;
    }

    microswim_message_type_t message_type = MALFORMED_MESSAGE;

    switch (cbor_typeof(root)) {
        case CBOR_TYPE_MAP: {
            struct cbor_pair pair = cbor_map_handle(root)[0];
            size_t key_length = cbor_string_length(pair.key);
            char key[key_length];
            memcpy(key, cbor_string_handle(pair.key), key_length);
            if (strncmp(key, "message", key_length) == 0) {
                size_t value = cbor_get_uint8(pair.value);
                message_type = (microswim_message_type_t)value;
            }
            break;
        }
        default:
            LOG_ERROR("Wrong message type: %d, ignoring...", cbor_typeof(root));
            break;
    }

    cbor_decref(&root);
    return message_type;
}

static void microswim_decode_uri_to_sockaddr(struct sockaddr_in* addr, cbor_item_t* item) {
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

static void microswim_decode_updates(microswim_message_t* message, cbor_item_t* updates) {
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
                microswim_decode_uri_to_sockaddr(&message->mu[j].addr, array_pair.value);
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

static void microswim_decode_pair(microswim_message_t* message, const char* key, size_t key_length, struct cbor_pair pair) {
    if (strncmp(key, "message", key_length) == 0) {
        size_t value = cbor_get_uint8(pair.value);
        message->type = (microswim_message_type_t)value;
    } else if (strncmp(key, "uuid", key_length) == 0) {
        size_t length = cbor_string_length(pair.value);
        memcpy(message->uuid, cbor_string_handle(pair.value), length);
        message->uuid[length] = '\0';
    } else if (strncmp(key, "uri", key_length) == 0) {
        microswim_decode_uri_to_sockaddr(&message->addr, pair.value);
    } else if (strncmp(key, "status", key_length) == 0) {
        size_t value = cbor_get_uint8(pair.value);
        message->status = (microswim_member_status_t)value;
    } else if (strncmp(key, "incarnation", key_length) == 0) {
        size_t value = cbor_get_uint8(pair.value);
        message->incarnation = value;
    } else if (strncmp(key, "updates", key_length) == 0) {
        microswim_decode_updates(message, pair.value);
    }
}

void microswim_decode_message(microswim_message_t* message, const char* buffer, ssize_t len) {
    struct cbor_load_result result;
    cbor_item_t* root = cbor_load((unsigned char*)buffer, len, &result);

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
                microswim_decode_pair(message, key, key_length, pair);
            }
            break;
        default:
            LOG_ERROR("Wrong message type: %d, ignoring...", cbor_typeof(root));
            break;
    }

    cbor_decref(&root);
}

#endif // MICROSWIM_CBOR
