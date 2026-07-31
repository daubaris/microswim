#ifdef MICROSWIM_CBOR

#include "cbor.h"
#include "decode.h"
#include "microswim.h"
#include "microswim_log.h"

microswim_message_type_t microswim_decode_message_type(unsigned char* buffer, ssize_t len) {
    struct cbor_load_result result;
    cbor_item_t* root = cbor_load(buffer, len, &result);

    if (result.error.code != CBOR_ERR_NONE) {
        MICROSWIM_LOG_ERROR(
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

            if (key_length > CBOR_MAX_KEY_SIZE) {
                MICROSWIM_LOG_DEBUG("Array key length (%zu) exceeds the maximum size (%d)", key_length, CBOR_MAX_KEY_SIZE);
                return MALFORMED_MESSAGE;
            }

            char key[key_length];
            memcpy(key, cbor_string_handle(pair.key), key_length);
            if (strncmp(key, "message", key_length) == 0) {
                size_t value = cbor_get_uint8(pair.value);
                message_type = (microswim_message_type_t)value;
            }
            break;
        }
        default:
            MICROSWIM_LOG_ERROR("Wrong message type: %d, ignoring...", cbor_typeof(root));
            break;
    }

    cbor_decref(&root);
    return message_type;
}

static microswim_decoder_status_t microswim_decode_uri_to_sockaddr(struct sockaddr_in* addr, cbor_item_t* item) {
    size_t length = cbor_string_length(item);

    if (length > MAXIMUM_URI_LENGTH) {
        return DECODING_ERROR_URI_LENGTH_EXCEEDED;
    }

    char buffer[length];
    memset(buffer, 0, length);
    memcpy(buffer, cbor_string_handle(item), length);

    const char* colon = memchr(buffer, ':', length);
    if (!colon) {
        return DECODING_ERROR_URI_INCOMPLETE;
    }

    size_t ip_len = colon - buffer;
    size_t port_len = length - ip_len - 1;

    char ip[64];
    char port_str[8];

    if (ip_len >= sizeof(ip) || port_len >= sizeof(port_str)) {
        return DECODING_ERROR_URI_INVALID;
    }

    memcpy(ip, buffer, ip_len);
    ip[ip_len] = '\0';

    memcpy(port_str, colon + 1, port_len);
    port_str[port_len] = '\0';

    char* end;
    long port = strtol(port_str, &end, 10);

    if (*end != '\0' || port <= 0 || port > 65535) {
        return DECODING_ERROR_URI_INVALID;
    }

    if (inet_pton(AF_INET, ip, &(addr->sin_addr)) != 1) {
        MICROSWIM_LOG_ERROR("Invalid IP address: %s\n", ip);
        return DECODING_ERROR_URI_INVALID;
    }

    return DECODING_SUCCESSFUL;
}

static void microswim_decode_ipso_objects(ipso_object_id_t* objects, size_t* count, cbor_item_t* array) {
    size_t n = cbor_array_size(array);
    if (n > MAXIMUM_IPSO_OBJECTS) {
        n = MAXIMUM_IPSO_OBJECTS;
    }
    for (size_t i = 0; i < n; i++) {
        cbor_item_t* pair = cbor_array_handle(array)[i];
        objects[i].oid = cbor_get_uint16(cbor_array_handle(pair)[0]);
        objects[i].iid = cbor_get_uint16(cbor_array_handle(pair)[1]);
    }
    *count = n;
}

static microswim_decoder_status_t microswim_decode_updates(microswim_message_t* message, cbor_item_t* updates) {
    size_t update_count = cbor_array_size(updates);
    if (update_count > MAXIMUM_UPDATES) {
        MICROSWIM_LOG_DEBUG("Update count (%zu) exceeds the maximum (%d), clamping", update_count, MAXIMUM_UPDATES);
        update_count = MAXIMUM_UPDATES;
    }
    message->update_count = (int)update_count;
    for (size_t j = 0; j < update_count; j++) {
        cbor_item_t* array_item = cbor_array_handle(updates)[j];
        for (size_t k = 0; k < cbor_map_size(array_item); k++) {
            struct cbor_pair array_pair = cbor_map_handle(array_item)[k];
            size_t array_key_length = cbor_string_length(array_pair.key);

            if (array_key_length > CBOR_MAX_KEY_SIZE) {
                MICROSWIM_LOG_DEBUG("Array key length (%zu) exceeds the maximum size (%d)", array_key_length, CBOR_MAX_KEY_SIZE);
                return DECODING_ERROR_KEY_LENGTH_EXCEEDED;
            }

            char array_key[array_key_length];
            memcpy(array_key, cbor_string_handle(array_pair.key), array_key_length);
            if (strncmp(array_key, "uuid", array_key_length) == 0) {
                size_t uuid_length = cbor_string_length(array_pair.value);
                if (uuid_length >= UUID_SIZE) {
                    uuid_length = UUID_SIZE - 1;
                }
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
            } else if (strncmp(array_key, "objects", array_key_length) == 0) {
                microswim_decode_ipso_objects(
                    message->mu[j].ipso_objects, &message->mu[j].ipso_object_count, array_pair.value);
            }
        }
    }

    return DECODING_SUCCESSFUL;
}

static microswim_decoder_status_t
    microswim_decode_pair(microswim_message_t* message, const char* key, size_t key_length, struct cbor_pair pair) {
    if (strncmp(key, "message", key_length) == 0) {
        size_t value = cbor_get_uint8(pair.value);
        message->type = (microswim_message_type_t)value;
    } else if (strncmp(key, "uuid", key_length) == 0) {
        size_t length = cbor_string_length(pair.value);
        if (length >= UUID_SIZE) {
            length = UUID_SIZE - 1;
        }
        memcpy(message->uuid, cbor_string_handle(pair.value), length);
        message->uuid[length] = '\0';
    } else if (strncmp(key, "uri", key_length) == 0) {
        return microswim_decode_uri_to_sockaddr(&message->addr, pair.value);
    } else if (strncmp(key, "status", key_length) == 0) {
        size_t value = cbor_get_uint8(pair.value);
        message->status = (microswim_member_status_t)value;
    } else if (strncmp(key, "incarnation", key_length) == 0) {
        size_t value = cbor_get_uint8(pair.value);
        message->incarnation = value;
    } else if (strncmp(key, "objects", key_length) == 0) {
        microswim_decode_ipso_objects(message->ipso_objects, &message->ipso_object_count, pair.value);
    } else if (strncmp(key, "updates", key_length) == 0) {
        return microswim_decode_updates(message, pair.value);
    }

    return DECODING_SUCCESSFUL;
}

microswim_decoder_status_t microswim_decode_message(microswim_message_t* message, const char* buffer, ssize_t len) {
    struct cbor_load_result result;
    cbor_item_t* root = cbor_load((unsigned char*)buffer, len, &result);

    if (result.error.code != CBOR_ERR_NONE) {
        MICROSWIM_LOG_ERROR(
            "There was an error while reading the input near byte %zu (read "
            "%zu bytes in total): \n",
            result.error.position, result.read);

        return DECODING_ERROR_MESSAGE_INVALID;
    }

    switch (cbor_typeof(root)) {
        case CBOR_TYPE_MAP:
            for (size_t i = 0; i < cbor_map_size(root); i++) {
                struct cbor_pair pair = cbor_map_handle(root)[i];
                size_t key_length = cbor_string_length(pair.key);

                if (key_length > CBOR_MAX_KEY_SIZE) {
                    MICROSWIM_LOG_DEBUG("Array key length (%zu) exceeds the maximum size (%d)", key_length, CBOR_MAX_KEY_SIZE);
                    return DECODING_ERROR_KEY_LENGTH_EXCEEDED;
                }

                char key[key_length];
                memcpy(key, cbor_string_handle(pair.key), key_length);
                microswim_decoder_status_t result = microswim_decode_pair(message, key, key_length, pair);
                if (result != DECODING_SUCCESSFUL) {
                    cbor_decref(&root);
                    return result;
                }
            }
            break;
        default:
            MICROSWIM_LOG_ERROR("Wrong message type: %d, ignoring...", cbor_typeof(root));
            return DECODING_ERROR_UNKNOWN_MESSAGE;
    }

    cbor_decref(&root);
    return DECODING_SUCCESSFUL;
}

#endif // MICROSWIM_CBOR
