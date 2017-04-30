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
    fnode n;
    const char* key = "key1";
    const char* data = "data1";
    size_t      ksize = strlen(key);
    size_t      dsize = strlen(data);

    db = fdb_init("foo");

    if (!fdb_insert(db, key, ksize, data, dsize)) {
        fdb_deinit(&db);
        ERROR("cannot insert");
        return 1;
    }

    n = fdb_find(db, key, ksize);

    if (!n) {
        fdb_deinit(&db);
        ERROR("cannot find %s", key);
        return 1;
    }

    // found node, get data and print
    printf("%s(%lu):%s(%lu)\n", 
            (char *) fnode_get_key(n),
            fnode_get_keysize(n),
            (char *) fnode_get_data(n),
            fnode_get_datasize(n));

    fdb_deinit(&db);
    return 0;
}
