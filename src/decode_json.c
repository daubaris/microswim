#ifdef MICROSWIM_JSON

#include "jsmn.h"
#include "microswim.h"
#include "microswim_log.h"
#include <stdio.h>
#include <stdlib.h>

static int jsoneq(const char* json, jsmntok_t* tok, const char* s) {
    if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
        strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return 0;
    }
    return -1;
}

microswim_message_type_t microswim_decode_message_type(char* buffer, ssize_t len) {
    int r;
    jsmn_parser p;
    jsmntok_t t[len];

    jsmn_init(&p);
    r = jsmn_parse(&p, buffer, strlen(buffer), t, sizeof(t) / sizeof(t[0]));
    if (r < 0) {
        MICROSWIM_LOG_ERROR("Failed to parse JSON: %d", r);
        return MALFORMED_MESSAGE;
    }

    if (r < 1 || t[0].type != JSMN_OBJECT) {
        MICROSWIM_LOG_ERROR("Object expected\n");
        return UNKOWN_MESSAGE;
    }

    microswim_message_type_t message_type = MALFORMED_MESSAGE;

    for (int i = 0; i < r; i++) {
        if (jsoneq(buffer, &t[i], "message") == 0) {
            message_type = (microswim_message_type_t)strtol(buffer + t[i + 1].start, NULL, 10);
            break;
        }
    }

    return message_type;
}

#ifdef RIOT_OS
static void microswim_decode_uri_to_sockaddr(sock_udp_ep_t* addr, char* buffer, size_t length) {
    memset(addr, 0, sizeof(*addr));

    const char* colon = memchr(buffer, ':', length);
    if (!colon) {
        return;
    }

    size_t ip_len = colon - buffer;
    size_t port_len = length - ip_len - 1;

    char ip[64];
    char port_str[8];

    memcpy(ip, buffer, ip_len);
    ip[ip_len] = '\0';

    memcpy(port_str, colon + 1, port_len);
    port_str[port_len] = '\0';

    char* end;
    long port = strtol(port_str, &end, 10);

    if (*end != '\0' || port <= 0 || port > 65535) {
        return;
    }

    if (inet_pton(AF_INET, ip, &(addr->addr)) != 1) {
        MICROSWIM_LOG_ERROR("Invalid IP address: %s\n", ip);
        return;
    }

    addr->family = AF_INET;
    addr->port = port;
}
#else
static void microswim_decode_uri_to_sockaddr(struct sockaddr_in* addr, const char* buffer, size_t length) {
    memset(addr, 0, sizeof(*addr));

    const char* colon = memchr(buffer, ':', length);
    if (!colon) {
        return;
    }

    size_t ip_len = colon - buffer;
    size_t port_len = length - ip_len - 1;

    char ip[64];
    char port_str[8];

    memcpy(ip, buffer, ip_len);
    ip[ip_len] = '\0';

    memcpy(port_str, colon + 1, port_len);
    port_str[port_len] = '\0';

    char* end;
    long port = strtol(port_str, &end, 10);

    if (*end != '\0' || port <= 0 || port > 65535) {
        return;
    }

    if (inet_pton(AF_INET, ip, &addr->sin_addr) != 1) {
        MICROSWIM_LOG_ERROR("Invalid IP address: %s\n", ip);
        return;
    }

    addr->sin_family = AF_INET;
    addr->sin_port = htons((uint16_t)port);
}
#endif

void microswim_decode_message(microswim_message_t* message, const char* buffer, ssize_t len) {
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

    for (int i = 1; i < r; i++) {
        if (jsoneq(buffer, &t[i], "message") == 0) {
            message->type = strtol(buffer + t[i + 1].start, NULL, 10);
            i++;
        }
        if (jsoneq(buffer, &t[i], "uuid") == 0) {
            strncpy((char*)message->uuid, buffer + t[i + 1].start, t[i + 1].end - t[i + 1].start);
            i++;
        }
        if (jsoneq(buffer, &t[i], "uri") == 0) {
            char uri_buffer[t[i + 1].end - t[i + 1].start];
            memset(uri_buffer, 0, sizeof(uri_buffer));
            strncpy(uri_buffer, buffer + t[i + 1].start, t[i + 1].end - t[i + 1].start);
            microswim_decode_uri_to_sockaddr(&message->addr, uri_buffer, t[i + 1].end - t[i + 1].start);
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
                            (char*)message->mu[j].uuid, buffer + (inner + 1)->start,
                            (inner + 1)->end - (inner + 1)->start);
                        i++;
                    } else if (jsoneq(buffer, inner, "uri") == 0) {
                        char uri_buffer[(inner + 1)->end - (inner + 1)->start];
                        memset(uri_buffer, 0, sizeof(uri_buffer));
                        strncpy(
                            uri_buffer, buffer + (inner + 1)->start, (inner + 1)->end - (inner + 1)->start);
                        microswim_decode_uri_to_sockaddr(
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

#endif
