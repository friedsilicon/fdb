/**
 * @file fdb_log.h
 * @brief Logging for FDB
 * @author Shivanand Velmurugan
 * @version 1.0
 * @date 2016-11-17
 *
 * Copyright © 2016 Shivanand Velmurugan. All Rights Reserved.
 */
#ifndef FDB_LOG_H_
#define FDB_LOG_H_

#include <stdio.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool fdb_is_debug_enabled();

#define FDB_DEBUG(fmt, ...)                                                    \
    do {                                                                       \
        if (fdb_is_debug_enabled()) {                                          \
            fprintf(stdout, "%s: %d: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
            fflush(stdout);                                                    \
        }                                                                      \
    } while (0)

#define FDB_ERROR(fmt, ...)                                                    \
    do {                                                                       \
        fprintf(stderr, "\n" fmt, ##__VA_ARGS__);                              \
        fflush(stderr);                                                        \
    } while (0)

#define FDB_INFO(fmt, ...)                                                     \
    do {                                                                       \
        fprintf(stdout, "%s: %d: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);\
        fflush(stdout);                                                        \
    } while (0)


#ifdef __cplusplus
}
#endif

#endif /* FDB_LOG_H_ */
