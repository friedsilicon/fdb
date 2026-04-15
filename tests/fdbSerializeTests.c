/*
 * Copyright © 2016 Shivanand Velmurugan. All Rights Reserved.
 */
#include "fdb_private.h"
#include "fdb/fdb.h"
#include <criterion/criterion.h>
#include <stdio.h>
#include <string.h>

static fdb db;

static void
fdb_serialize_setup(void)
{
    db = fdb_init("foo");
}

static void
fdb_serialize_teardown(void)
{
    remove("./fdb-serialize.bin");
    fdb_deinit(db);
}

static void
check_node(const fnode n,
           const char* k, size_t kl,
           const char* v, size_t vl)
{
    cr_assert_not_null(n);
    cr_assert_str_eq(k, (char*) fnode_get_key(n));
    cr_assert_eq(kl, fnode_get_keysize(n));
    cr_assert_str_eq(v, (char*) fnode_get_data(n));
    cr_assert_eq(vl, fnode_get_datasize(n));
}

static void
check_record(fdb db_inst, const char* k, const char* v)
{
    size_t kl, vl;

    kl = strlen(k);
    vl = strlen(v);

    fnode n = fdb_find(db_inst, k, kl+1);
    check_node(n, k, kl+1, v, vl+1);
}

TestSuite(fdb_serialize, .init = fdb_serialize_setup, .fini = fdb_serialize_teardown);

Test(fdb_serialize, save)
{
    cr_assert(fdb_insert(db, "key1", 5, "data1", 6));
    check_record(db, "key1", "data1");
    cr_assert(fdb_insert(db, "key2", 5, "data2", 6));
    check_record(db, "key2", "data2");
    cr_assert(fdb_insert(db, "key3", 5, "data3", 6));
    check_record(db, "key3", "data3");

    cr_assert(fdb_save(db, "./fdb-serialize.bin"));
}

Test(fdb_serialize, save_load)
{
    cr_assert(fdb_insert(db, "key1", 5, "data1", 6));
    cr_assert(fdb_insert(db, "key2", 5, "data11", 7));
    cr_assert(fdb_insert(db, "key3", 5, "data111", 8));

    check_record(db, "key1", "data1");
    check_record(db, "key2", "data11");
    check_record(db, "key3", "data111");

    cr_assert(fdb_save(db, "./fdb-serialize.bin"));
    
    fdb_deinit(db);
    db = NULL;

    db = fdb_init("foo");
    cr_assert(fdb_load(db, "./fdb-serialize.bin"));

    check_record(db, "key1", "data1");
    check_record(db, "key2", "data11");
    check_record(db, "key3", "data111");
}

