/*
 * Copyright © 2016 Shivanand Velmurugan. All Rights Reserved.
 */
#include "fdb/fdb.h"
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdio.h>
#include <string.h>

static fdb db;

static int fdb_serialize_setup(void **state)
{
    (void) state;
    db = fdb_init("foo");
    return 0;
}

static int fdb_serialize_teardown(void **state)
{
    (void) state;
    remove("./fdb-serialize.bin");
    fdb_deinit(db);
    return 0;
}

static void
check_node(fdb_node_t n,
           const char* k, size_t kl,
           const char* v, size_t vl)
{
    assert_true(fdb_node_valid(n));
    assert_memory_equal(k, n.key, kl);
    assert_int_equal(kl, n.key_size);
    assert_memory_equal(v, n.data, vl);
    assert_int_equal(vl, n.data_size);
}

static void
check_record(fdb db_inst, const char* k, const char* v)
{
    size_t kl = strlen(k);
    size_t vl = strlen(v);
    fdb_node_t n = fdb_find(db_inst, k, kl + 1);
    check_node(n, k, kl + 1, v, vl + 1);
}

static void fdb_serialize_save(void **state)
{
    (void) state;
    assert_true(fdb_insert(db, "key1", 5, "data1", 6));
    check_record(db, "key1", "data1");
    assert_true(fdb_insert(db, "key2", 5, "data2", 6));
    check_record(db, "key2", "data2");
    assert_true(fdb_insert(db, "key3", 5, "data3", 6));
    check_record(db, "key3", "data3");

    assert_true(fdb_save(db, "./fdb-serialize.bin"));
}

static void fdb_serialize_save_load(void **state)
{
    (void) state;
    assert_true(fdb_insert(db, "key1", 5, "data1", 6));
    assert_true(fdb_insert(db, "key2", 5, "data11", 7));
    assert_true(fdb_insert(db, "key3", 5, "data111", 8));

    check_record(db, "key1", "data1");
    check_record(db, "key2", "data11");
    check_record(db, "key3", "data111");

    assert_true(fdb_save(db, "./fdb-serialize.bin"));

    fdb_deinit(db);
    db = fdb_init("foo");
    assert_true(fdb_load(db, "./fdb-serialize.bin"));

    check_record(db, "key1", "data1");
    check_record(db, "key2", "data11");
    check_record(db, "key3", "data111");
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(fdb_serialize_save,      fdb_serialize_setup, fdb_serialize_teardown),
        cmocka_unit_test_setup_teardown(fdb_serialize_save_load, fdb_serialize_setup, fdb_serialize_teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
