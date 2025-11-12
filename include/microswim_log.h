#ifndef MICROSWIM_LOG_H
#define MICROSWIM_LOG_H

#include <stdio.h>

#define NONE 0
#define ERROR 1
#define WARN 2
#define INFO 3
#define DEBUG 4

#define MICROSWIM_LOG_LEVEL DEBUG

#define MICROSWIM_LOG(level, format, ...)                         \
    do {                                                          \
        if (level <= MICROSWIM_LOG_LEVEL) {                       \
            printf("[%s] " format "\r\n", #level, ##__VA_ARGS__); \
        }                                                         \
    } while (0)

#define MICROSWIM_LOG_ERROR(format, ...) MICROSWIM_LOG(ERROR, format, ##__VA_ARGS__)
#define MICROSWIM_LOG_WARN(format, ...) MICROSWIM_LOG(WARN, format, ##__VA_ARGS__)
#define MICROSWIM_LOG_INFO(format, ...) MICROSWIM_LOG(INFO, format, ##__VA_ARGS__)
#define MICROSWIM_LOG_DEBUG(format, ...) MICROSWIM_LOG(DEBUG, format, ##__VA_ARGS__)

#endif // LOG_H
