/**
 * @file fdb_private.h
 * @brief
 * @author Shivanand Velmurugan
 * @version 1.0
 * @date 2016-11-01
 *
 * Copyright © 2016 Shivanand Velmurugan. All Rights Reserved.
 */
#ifndef FDB_PRIVATE_H_
#define FDB_PRIVATE_H_

#include "fdb/fdb.h"
#include "dict.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum db_index_type_e {
    DB_SINGLE_INDEX,
    DB_MULTI_INDEX
} db_index_type_t;

typedef bool (*callback_fn)(ftx tx, fdb db);

/* TODO: segmentation of data ? */
struct fdb_
{
    /* properties */
    int             id;
    fdb_type_t      type;
    db_index_type_t idx_type;
    char*           name;
    dict*           dstore;
    fdb_file        file_on_disk;

    /* txn methods */
    callback_fn join;
    callback_fn prepare;
    callback_fn commit;
    callback_fn abort;
    callback_fn finish;

    /* state */
};

struct felem
{
    size_t size;
    void*  content;
};

struct node_
{
    int          type;
    struct felem key;
    struct felem data;
};

struct iter_
{
    dict_itor* it;
    fnode      next;
};

typedef struct {
    traverse_cb callback;
} fdb_traversal_functor_state;

typedef enum tstate_e {
    TXN_ST_NEW,            /*! New and clean! */
    TXN_ST_LIVE,           /*! Live and in operation - not validated */
    TXN_ST_PREPARED,       /*! Ready for commit */
    TXN_ST_ABORTED,        /*! Aborted */
    TXN_ST_COMMITTED,      /*! Committed */
    TXN_ST_PREPARE_FAILED, /*! Validation failed */
    TXN_ST_COMMIT_FAILED,  /*! Commit failed */
    TXN_ST_ABORT_FAILED    /*! Abort failed */
} tstate_t;

struct ftx_
{
    int      id;
    tstate_t state;
    dict*    fdb_list; /* list of databases in this txn */
};

bool is_fdb_handle_valid(fdb db);

#ifdef __cplusplus
}
#endif

#endif /* FDB_PRIVATE_H_ */
