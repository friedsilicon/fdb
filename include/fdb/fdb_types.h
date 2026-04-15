/**
 * @file fdb_types.h
 * @brief Public types for FDB.
 *
 * Copyright © 2016 Shivanand Velmurugan. All Rights Reserved.
 */
#ifndef FDB_TYPES_H_
#define FDB_TYPES_H_

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------- */
/* Node — a borrowed view of one key-value entry in storage.              */
/*                                                                        */
/* Pointers inside fdb_node_t are valid only until the next mutating      */
/* operation (insert / update / remove / deinit) on the same fdb handle.  */
/* Copy the data out if you need it to outlive that window.               */
/* ---------------------------------------------------------------------- */
typedef struct fdb_node_ {
    const void* key;
    size_t      key_size;
    const void* data;
    size_t      data_size;
} fdb_node_t;

#define FDB_NODE_INVALID ((fdb_node_t){ NULL, 0, NULL, 0 })
#define fdb_node_valid(n) ((n).key != NULL)

/* ---------------------------------------------------------------------- */
/* Opaque handles                                                          */
/* ---------------------------------------------------------------------- */
typedef struct fdb_iter_ * fiter;

typedef struct fdb_file_s  fdb_file_t;
typedef fdb_file_t*        fdb_file;

typedef struct fdb_        fdb_t;
typedef fdb_t*             fdb;

typedef struct ftx_        ftx_t;
typedef ftx_t*             ftx;

typedef enum { FDB_SINGLE_INDEX_DB, FDB_MULTI_INDEX_DB } fdb_type_t;

#ifdef __cplusplus
}
#endif

#endif /* FDB_TYPES_H_ */
