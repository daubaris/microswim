#ifndef MICROSWIM_DECODE_H
#define MICROSWIM_DECODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "microswim.h"

typedef enum {
    DECODING_ERROR_URI_LENGTH_EXCEEDED,
    DECODING_ERROR_URI_INCOMPLETE,
    DECODING_ERROR_URI_INVALID,
    DECODING_ERROR_PORT_MISSING,
    DECODING_ERROR_MESSAGE_INVALID,
    DECODING_ERROR_KEY_LENGTH_EXCEEDED,
    DECODING_ERROR_UNKNOWN_MESSAGE,
    DECODING_SUCCESSFUL
} microswim_decoder_status_t;

microswim_decoder_status_t microswim_decode_message(microswim_message_t* message, const char* buffer, ssize_t len);
microswim_message_type_t microswim_decode_message_type(unsigned char* buffer, ssize_t len);

#ifdef __cplusplus
}
#endif

#endif // MICROSWIM_DECODE_H
