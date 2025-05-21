#ifndef MICROSWIM_LOG_H
#define MICROSWIM_LOG_H

#include <stdio.h>

#define NONE 0
#define ERROR 1
#define WARN 2
#define INFO 3
#define DEBUG 4

#define LOG_LEVEL DEBUG

#define LOG(level, format, ...)                                   \
    do {                                                          \
        if (level <= LOG_LEVEL) {                                 \
            printf("[%s] " format "\r\n", #level, ##__VA_ARGS__); \
        }                                                         \
    } while (0)

#define LOG_ERROR(format, ...) LOG(ERROR, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...) LOG(WARN, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...) LOG(INFO, format, ##__VA_ARGS__)
#define LOG_DEBUG(format, ...) LOG(DEBUG, format, ##__VA_ARGS__)

#endif // LOG_H
