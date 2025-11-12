#ifdef MICROSWIM_CBOR

#include "cbor.h"
#include "microswim.h"
#include "microswim_log.h"
#include "utils.h"

size_t microswim_encode_message(microswim_message_t* message, unsigned char* buffer, size_t size) {
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
            .value = cbor_move(message->uuid[0] != '\0' ? cbor_build_string((char*)message->uuid) : cbor_new_null()) });
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
        MICROSWIM_LOG_ERROR("Preallocated storage for map is full (origin_map)");
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
                                .value = cbor_move(cbor_build_string((char*)message->mu[i].uuid)) });
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
            MICROSWIM_LOG_ERROR("Preallocated storage for map is full (update_map)");
            return 0;
        }
    }

    success &= cbor_map_add(
        origin_map,
        (struct cbor_pair){ .key = cbor_move(cbor_build_string("updates")), .value = cbor_move(update_array) });
    if (!success) {
        MICROSWIM_LOG_ERROR("Preallocated storage for map is full (origin_map, update_array)");
        return 0;
    }

    size_t len = cbor_serialize(origin_map, buffer, size);
    if (!len) {
        MICROSWIM_LOG_ERROR("Message serialization has failed");
        return 0;
    }

    cbor_decref(&origin_map);

    return len;
}

#endif
