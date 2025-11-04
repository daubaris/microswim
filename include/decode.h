#ifndef MICROSWIM_DECODE_H
#define MICROSWIM_DECODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "microswim.h"

void microswim_decode_message(microswim_message_t* message, const char* buffer, ssize_t len);
microswim_message_type_t microswim_decode_message_type(unsigned char* buffer, ssize_t len);

#ifdef __cplusplus
}
#endif

#endif // MICROSWIM_DECODE_H
