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
#include <stdio.h>
#include <inttypes.h>

#define FDB_MAX_DB_NAME_LEN  31
#define FDB_MAX_DB_NAME_SIZE FDB_MAX_DB_NAME_LEN + 1

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
    int32_t         id;
    fdb_type_t      type;
    db_index_type_t idx_type;
    dict*           dstore;
    fdb_file        file_on_disk;
    char            name[FDB_MAX_DB_NAME_SIZE];

    /* txn methods */
    callback_fn join;
    callback_fn prepare;
    callback_fn commit;
    callback_fn abort;
    callback_fn finish;

    /* state */
};

struct node_header_
{
    uint32_t node_size;
    uint32_t key_size;
    uint32_t data_size;
    uint16_t key_type;
    uint16_t data_type;
};

struct node_
{
    struct node_header_ header;
    uint8_t             content[0];
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

static void hexDump (char *desc, void *addr, int len) 
{
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;

    // Output description if given.
    if (desc != NULL)
        printf ("%s:\n", desc);

    if (len == 0) {
        printf("  ZERO LENGTH\n");
        return;
    }
    if (len < 0) {
        printf("  NEGATIVE LENGTH: %i\n",len);
        return;
    }

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                printf ("  %s\n", buff);

            // Output the offset.
            printf ("  %04x ", i);
        }

        // Now the hex code for the specific character.
        printf (" %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        printf ("   ");
        i++;
    }

    // And print the final ASCII bit.
    printf ("  %s\n", buff);
}

void fdb_log_db_state(const fdb db);
void fdb_log_node_header(struct node_header_* h);
void fdb_log_node(struct node_* n);
#ifdef __cplusplus
}
#endif

#endif /* FDB_PRIVATE_H_ */
