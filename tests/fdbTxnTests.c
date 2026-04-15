/*
 * Copyright © 2016 Shivanand Velmurugan. All Rights Reserved.
 */
#include <criterion/criterion.h>
#include "fdb/fdb.h"
#include "fdb_private.h"
#include <stdio.h>
#include <string.h>

static bool callback1_expected;
static bool callback2_expected;
static bool callback3_expected;

static int callback1_count;
static int callback2_count;
static int callback3_count;

static bool fail_on_commit;

static void
reset_callback_state(void)
{
    callback1_expected = true;
    callback2_expected = true;
    callback3_expected = true;
    callback1_count = 0;
    callback2_count = 0;
    callback3_count = 0;
    fail_on_commit = false;
}

static bool
test_callback(ftx tx, fdb db)
{
    (void) tx;
    (void) db;
    callback1_count++;
    return callback1_expected;
}

static bool
test_2_callback(ftx tx, fdb db)
{
    (void) tx;
    (void) db;
    callback2_count++;
    return callback2_expected;
}

static bool
test_3_callback(ftx tx, fdb db)
{
    (void) tx;
    (void) db;
    callback3_count++;
    if (fail_on_commit && callback3_count == 3) return false;
    return callback3_expected;
}

static void
setupCallback(fdb db, callback_fn fn)
{
    db->join    = fn;
    db->prepare = fn;
    db->commit  = fn;
    db->abort   = fn;
    db->finish  = fn;
}

static void
setupDB(fdb db, int id, callback_fn fn)
{
    memset(db, 0, sizeof(fdb_t));
    db->id = id;
    db->type = FDB_SINGLE_INDEX_DB;
    db->idx_type = DB_SINGLE_INDEX;
    snprintf(db->name, FDB_MAX_DB_NAME_SIZE, "%s%d", "db", id);
    setupCallback(db, fn);
}

Test(fdb_txn, txn_ops_invalid)
{
    cr_assert_null(fdb_txn_start(NULL));
    cr_assert_not(fdb_txn_join(NULL, NULL));
    cr_assert_not(fdb_txn_commit(NULL));
    cr_assert_not(fdb_txn_abort(NULL));
    fdb_txn_finish(NULL);
}

Test(fdb_txn, txn_start_and_join_invalid)
{
    reset_callback_state();
    ftx tx = NULL;
    fdb db1 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db2 = (fdb) calloc(1, sizeof(fdb_t));

    db1->id = 10;
    db1->type = FDB_SINGLE_INDEX_DB;
    db1->idx_type = DB_SINGLE_INDEX;
    setupCallback(db1, &test_callback);

    db2->id = 11;
    db2->type = FDB_SINGLE_INDEX_DB;
    db2->idx_type = DB_SINGLE_INDEX;
    setupCallback(db2, &test_2_callback);

    cr_assert_not(fdb_txn_join(NULL, db2));
    cr_assert_not(fdb_txn_join(tx, NULL));

    callback1_expected = false;
    cr_assert_null(fdb_txn_start(db1));
    cr_assert_eq(callback1_count, 1);

    callback1_expected = true;
    tx = fdb_txn_start(db1);
    cr_assert_not_null(tx);
    cr_assert_eq(callback1_count, 2);

    callback2_expected = false;
    cr_assert_not(fdb_txn_join(tx, db2));
    cr_assert_eq(callback2_count, 1);

    callback2_expected = true;
    cr_assert(fdb_txn_join(tx, db2));
    cr_assert_eq(callback2_count, 2);

    callback1_expected = true;
    callback2_expected = true;
    fdb_txn_finish(tx);
    free(db1);
    free(db2);
}

Test(fdb_txn, txn_start_and_join)
{
    reset_callback_state();
    ftx tx = NULL;
    fdb db1 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db2 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db3 = (fdb) calloc(1, sizeof(fdb_t));

    setupDB(db1, 10, &test_callback);
    setupDB(db2, 11, &test_2_callback);
    setupDB(db3, 12, &test_3_callback);

    tx = fdb_txn_start(db1);
    cr_assert_not_null(tx);

    cr_assert(fdb_txn_join(tx, db3));
    cr_assert(fdb_txn_join(tx, db2));

    callback1_expected = true;
    callback2_expected = true;
    callback3_expected = true;
    fdb_txn_finish(tx);
    free(db1);
    free(db2);
    free(db3);
}

Test(fdb_txn, txn_commit)
{
    reset_callback_state();
    ftx tx = NULL;
    fdb db1 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db2 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db3 = (fdb) calloc(1, sizeof(fdb_t));

    setupDB(db1, 10, &test_callback);
    setupDB(db2, 11, &test_2_callback);
    setupDB(db3, 12, &test_3_callback);

    tx = fdb_txn_start(db1);
    cr_assert_not_null(tx);

    cr_assert(fdb_txn_join(tx, db2));
    cr_assert(fdb_txn_join(tx, db3));

    callback1_expected = true;
    callback2_expected = true;
    callback3_expected = true;
    cr_assert(fdb_txn_commit(tx));

    callback1_expected = true;
    callback2_expected = true;
    callback3_expected = true;
    fdb_txn_finish(tx);
    free(db1);
    free(db2);
    free(db3);
}

Test(fdb_txn, txn_commit_failed_during_prepare)
{
    reset_callback_state();
    ftx tx = NULL;
    fdb db1 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db2 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db3 = (fdb) calloc(1, sizeof(fdb_t));

    setupDB(db1, 10, &test_callback);
    setupDB(db2, 11, &test_2_callback);
    setupDB(db3, 12, &test_3_callback);

    tx = fdb_txn_start(db1);
    cr_assert_not_null(tx);

    cr_assert(fdb_txn_join(tx, db2));
    cr_assert(fdb_txn_join(tx, db3));

    callback1_expected = true;
    callback2_expected = true;
    callback3_expected = true;
    cr_assert_not(fdb_txn_commit(tx));

    callback1_expected = true;
    callback2_expected = true;
    callback3_expected = true;
    fdb_txn_finish(tx);
    free(db1);
    free(db2);
    free(db3);
}

Test(fdb_txn, txn_commit_failed_during_commit)
{
    reset_callback_state();
    fail_on_commit = true;
    ftx tx = NULL;
    fdb db1 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db2 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db3 = (fdb) calloc(1, sizeof(fdb_t));

    setupDB(db1, 10, &test_callback);
    setupDB(db2, 11, &test_2_callback);
    setupDB(db3, 12, &test_3_callback);

    tx = fdb_txn_start(db1);
    cr_assert_not_null(tx);

    cr_assert(fdb_txn_join(tx, db2));
    cr_assert(fdb_txn_join(tx, db3));

    callback1_expected = true;
    callback2_expected = true;
    callback3_expected = true;
    cr_assert_not(fdb_txn_commit(tx));

    callback1_expected = true;
    callback2_expected = true;
    callback3_expected = true;
    fdb_txn_finish(tx);
    free(db1);
    free(db2);
    free(db3);
}

Test(fdb_txn, txn_abort)
{
    reset_callback_state();
    ftx tx = NULL;
    fdb db1 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db2 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db3 = (fdb) calloc(1, sizeof(fdb_t));

    setupDB(db1, 10, &test_callback);
    setupDB(db2, 11, &test_2_callback);
    setupDB(db3, 12, &test_3_callback);

    tx = fdb_txn_start(db1);
    cr_assert_not_null(tx);

    cr_assert(fdb_txn_join(tx, db2));
    cr_assert(fdb_txn_join(tx, db3));

    callback1_expected = true;
    callback2_expected = true;
    callback3_expected = true;
    cr_assert(fdb_txn_abort(tx));

    callback1_expected = true;
    callback2_expected = true;
    callback3_expected = true;
    fdb_txn_finish(tx);
    free(db1);
    free(db2);
    free(db3);
}

Test(fdb_txn, txn_abort_fails)
{
    reset_callback_state();
    ftx tx = NULL;
    fdb db1 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db2 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db3 = (fdb) calloc(1, sizeof(fdb_t));

    setupDB(db1, 10, &test_callback);
    setupDB(db2, 11, &test_2_callback);
    setupDB(db3, 12, &test_3_callback);

    tx = fdb_txn_start(db1);
    cr_assert_not_null(tx);

    cr_assert(fdb_txn_join(tx, db2));
    cr_assert(fdb_txn_join(tx, db3));

    callback1_expected = true;
    callback2_expected = true;
    callback3_expected = false;
    cr_assert_not(fdb_txn_abort(tx));

    callback1_expected = true;
    callback2_expected = true;
    callback3_expected = true;
    fdb_txn_finish(tx);
    free(db1);
    free(db2);
    free(db3);
}
