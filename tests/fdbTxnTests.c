/*
 * Copyright © 2016 Shivanand Velmurugan. All Rights Reserved.
 */
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "fdb/fdb.h"
#include "fdb_private.h"
#include <stdio.h>
#include <stdlib.h>
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
    snprintf(db->name, FDB_MAX_DB_NAME_SIZE, "%s%d", "db", id);
    setupCallback(db, fn);
}

static void fdb_txn_txn_ops_invalid(void **state)
{
    (void) state;
    assert_null(fdb_txn_start(NULL));
    assert_false(fdb_txn_join(NULL, NULL));
    assert_false(fdb_txn_commit(NULL));
    assert_false(fdb_txn_abort(NULL));
    fdb_txn_finish(NULL);
}

static void fdb_txn_txn_start_and_join_invalid(void **state)
{
    (void) state;
    reset_callback_state();
    ftx tx = NULL;
    fdb db1 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db2 = (fdb) calloc(1, sizeof(fdb_t));

    db1->id = 10;
    db1->type = FDB_SINGLE_INDEX_DB;
    setupCallback(db1, &test_callback);

    db2->id = 11;
    db2->type = FDB_SINGLE_INDEX_DB;
    setupCallback(db2, &test_2_callback);

    assert_false(fdb_txn_join(NULL, db2));
    assert_false(fdb_txn_join(tx, NULL));

    callback1_expected = false;
    assert_null(fdb_txn_start(db1));
    assert_int_equal(callback1_count, 1);

    callback1_expected = true;
    tx = fdb_txn_start(db1);
    assert_non_null(tx);
    assert_int_equal(callback1_count, 2);

    callback2_expected = false;
    assert_false(fdb_txn_join(tx, db2));
    assert_int_equal(callback2_count, 1);

    callback2_expected = true;
    assert_true(fdb_txn_join(tx, db2));
    assert_int_equal(callback2_count, 2);

    fdb_txn_finish(tx);
    free(db1);
    free(db2);
}

static void fdb_txn_txn_start_and_join(void **state)
{
    (void) state;
    reset_callback_state();
    fdb db1 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db2 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db3 = (fdb) calloc(1, sizeof(fdb_t));

    setupDB(db1, 10, &test_callback);
    setupDB(db2, 11, &test_2_callback);
    setupDB(db3, 12, &test_3_callback);

    ftx tx = fdb_txn_start(db1);
    assert_non_null(tx);

    assert_true(fdb_txn_join(tx, db3));
    assert_true(fdb_txn_join(tx, db2));

    fdb_txn_finish(tx);
    free(db1);
    free(db2);
    free(db3);
}

static void fdb_txn_txn_commit(void **state)
{
    (void) state;
    reset_callback_state();
    fdb db1 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db2 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db3 = (fdb) calloc(1, sizeof(fdb_t));

    setupDB(db1, 10, &test_callback);
    setupDB(db2, 11, &test_2_callback);
    setupDB(db3, 12, &test_3_callback);

    ftx tx = fdb_txn_start(db1);
    assert_non_null(tx);

    assert_true(fdb_txn_join(tx, db2));
    assert_true(fdb_txn_join(tx, db3));

    assert_true(fdb_txn_commit(tx));

    fdb_txn_finish(tx);
    free(db1);
    free(db2);
    free(db3);
}

static void fdb_txn_txn_commit_failed_during_prepare(void **state)
{
    (void) state;
    reset_callback_state();
    fdb db1 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db2 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db3 = (fdb) calloc(1, sizeof(fdb_t));

    setupDB(db1, 10, &test_callback);
    setupDB(db2, 11, &test_2_callback);
    setupDB(db3, 12, &test_3_callback);

    ftx tx = fdb_txn_start(db1);
    assert_non_null(tx);

    assert_true(fdb_txn_join(tx, db2));
    assert_true(fdb_txn_join(tx, db3));

    callback3_expected = false;
    assert_false(fdb_txn_commit(tx));

    fdb_txn_finish(tx);
    free(db1);
    free(db2);
    free(db3);
}

static void fdb_txn_txn_commit_failed_during_commit(void **state)
{
    (void) state;
    reset_callback_state();
    fail_on_commit = true;
    fdb db1 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db2 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db3 = (fdb) calloc(1, sizeof(fdb_t));

    setupDB(db1, 10, &test_callback);
    setupDB(db2, 11, &test_2_callback);
    setupDB(db3, 12, &test_3_callback);

    ftx tx = fdb_txn_start(db1);
    assert_non_null(tx);

    assert_true(fdb_txn_join(tx, db2));
    assert_true(fdb_txn_join(tx, db3));

    assert_false(fdb_txn_commit(tx));

    fdb_txn_finish(tx);
    free(db1);
    free(db2);
    free(db3);
}

static void fdb_txn_txn_abort(void **state)
{
    (void) state;
    reset_callback_state();
    fdb db1 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db2 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db3 = (fdb) calloc(1, sizeof(fdb_t));

    setupDB(db1, 10, &test_callback);
    setupDB(db2, 11, &test_2_callback);
    setupDB(db3, 12, &test_3_callback);

    ftx tx = fdb_txn_start(db1);
    assert_non_null(tx);

    assert_true(fdb_txn_join(tx, db2));
    assert_true(fdb_txn_join(tx, db3));

    assert_true(fdb_txn_abort(tx));

    fdb_txn_finish(tx);
    free(db1);
    free(db2);
    free(db3);
}

static void fdb_txn_txn_abort_fails(void **state)
{
    (void) state;
    reset_callback_state();
    fdb db1 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db2 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db3 = (fdb) calloc(1, sizeof(fdb_t));

    setupDB(db1, 10, &test_callback);
    setupDB(db2, 11, &test_2_callback);
    setupDB(db3, 12, &test_3_callback);

    ftx tx = fdb_txn_start(db1);
    assert_non_null(tx);

    assert_true(fdb_txn_join(tx, db2));
    assert_true(fdb_txn_join(tx, db3));

    callback3_expected = false;
    assert_false(fdb_txn_abort(tx));

    fdb_txn_finish(tx);
    free(db1);
    free(db2);
    free(db3);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(fdb_txn_txn_ops_invalid),
        cmocka_unit_test(fdb_txn_txn_start_and_join_invalid),
        cmocka_unit_test(fdb_txn_txn_start_and_join),
        cmocka_unit_test(fdb_txn_txn_commit),
        cmocka_unit_test(fdb_txn_txn_commit_failed_during_prepare),
        cmocka_unit_test(fdb_txn_txn_commit_failed_during_commit),
        cmocka_unit_test(fdb_txn_txn_abort),
        cmocka_unit_test(fdb_txn_txn_abort_fails),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
