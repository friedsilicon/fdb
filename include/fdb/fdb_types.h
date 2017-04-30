/**
 * @file fdb_types.h
 * @brief Types used by FDB
 * @author Shivanand Velmurugan
 * @version 1.0
 * @date 2016-11-01
 *
 * Copyright © 2016 Shivanand Velmurugan. All Rights Reserved.
 */

#ifndef FDB_TYPES_H_
#define FDB_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef const void* data;
typedef const void* key;

typedef struct node_* fnode;
typedef struct iter_* fiter;

typedef struct cache_ fcache_t;
typedef fcache_t*     fcache;

typedef struct fdb_file_s fdb_file_t;
typedef fdb_file_t*       fdb_file;

typedef struct fdb_ fdb_t;
typedef fdb_t*      fdb;

typedef struct ftx_ ftx_t;
typedef ftx_t*      ftx;

typedef enum { FDB_SINGLE_INDEX_DB, FDB_MULTI_INDEX_DB } fdb_type_t;

#ifdef __cplusplus
}
#endif

#endif /* FDB_TYPES_H_ */
