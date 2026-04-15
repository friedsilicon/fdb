/*
 * Copyright © 2016 Shivanand Velmurugan. All Rights Reserved.
 */
#include <stdio.h>
#include <string.h>
#include "fdb/fdb.h"

#define ERROR(fmt, ...) \
    do { \
        fprintf(stderr, fmt"\n", ##__VA_ARGS__); \
        fflush(stderr); \
    } while(0)

int main(int argc, char *argv[])
{
    fdb db;
    fdb_node_t n;
    const char* key = "key1";
    const char* data = "data1";
    size_t      ksize = strlen(key) + 1;
    size_t      dsize = strlen(data) + 1;

    (void)argc;
    (void)argv;

    db = fdb_init("foo");

    if (!fdb_insert(db, key, ksize, data, dsize)) {
        fdb_deinit(db);
        ERROR("cannot insert");
        return 1;
    }

    n = fdb_find(db, key, ksize);

    if (!fdb_node_valid(n)) {
        fdb_deinit(db);
        ERROR("cannot find %s", key);
        return 1;
    }

    printf("%.*s(%zu):%.*s(%zu)\n",
           (int)n.key_size, (const char*)n.key, n.key_size,
           (int)n.data_size, (const char*)n.data, n.data_size);

    fdb_deinit(db);
    return 0;
}
