#ifndef MICROSWIM_ENCODE
#define MICROSWIM_ENCODE

#ifdef __cplusplus
extern "C" {
#endif

#include "microswim.h"

size_t microswim_encode_message(microswim_message_t* message, unsigned char* buffer, size_t size);

#ifdef __cplusplus
}
#endif

#endif // MICROSWIM_ENCODE
