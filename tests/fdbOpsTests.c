/*
 * Copyright © 2016 Shivanand Velmurugan. All Rights Reserved.
 */
#include "fdb_private.h"
#include "fdb/fdb.h"
#include <criterion/criterion.h>
#include <stdio.h>
#include <string.h>

static void
check_node(const fnode n,
           const char* k, size_t kl,
           const char* v, size_t vl)
{
    cr_assert_not_null(n);
    fdb_log_node(n);

    cr_assert_str_eq(k, (char*) fnode_get_key(n));
    cr_assert_eq(kl, fnode_get_keysize(n));
    cr_assert_str_eq(v, (char*) fnode_get_data(n));
    cr_assert_eq(vl, fnode_get_datasize(n));
}

static void
check_record(fdb db, const char* k, const char* v)
{
    size_t kl, vl;

    kl = strlen(k);
    vl = strlen(v);

    fnode n = fdb_find(db, k, kl+1);
    check_node(n, k, kl+1, v, vl+1);
}

struct traversal_expect {
    size_t call_count;
    bool results[4];
};

static bool
test_traversal_callback(fnode node, void* user_data)
{
    struct traversal_expect* expect = user_data;
    expect->call_count++;
    return expect->results[expect->call_count - 1];
}

Test(fdb_ops, insert)
{
    fdb db = fdb_init("foo");
    cr_assert(fdb_insert(db, "key", 4, "data", 5));
    check_record(db, "key", "data");

    cr_assert_not(fdb_insert(db, "key", 4, "data", 5));

    cr_assert(fdb_insert(db, "key1", 5, "data", 5));
    check_record(db, "key1", "data");
    fdb_deinit(db);
}

Test(fdb_ops, exists)
{
    fdb db = fdb_init("foo");
    cr_assert(fdb_insert(db, "key", 4, "data", 5));
    cr_assert(fdb_exists(db, "key", 4));
    cr_assert_not(fdb_exists(db, "key1", 5));
    fdb_deinit(db);
}

Test(fdb_ops, prefix_keys_distinct)
{
    fdb db = fdb_init("foo");
    cr_assert(fdb_insert(db, "key", 4, "data", 5));
    cr_assert(fdb_insert(db, "keylong", 8, "value", 6));

    fnode n1 = fdb_find(db, "key", 4);
    fnode n2 = fdb_find(db, "keylong", 8);

    cr_assert_not_null(n1);
    cr_assert_not_null(n2);
    cr_assert_str_eq("key", (char*) fnode_get_key(n1));
    cr_assert_str_eq("keylong", (char*) fnode_get_key(n2));
    fdb_deinit(db);
}

Test(fdb_ops, update)
{
    fdb db = fdb_init("foo");
    cr_assert(fdb_insert(db, "key", 4, "data", 5));
    check_record(db, "key", "data");

    cr_assert(fdb_update(db, "key", 4, "data1", 6));
    check_record(db, "key", "data1");
    fdb_deinit(db);
}

Test(fdb_ops, remove)
{
    fdb db = fdb_init("foo");
    cr_assert(fdb_insert(db, "key", 4, "data", 5));
    check_record(db, "key", "data");

    cr_assert(fdb_remove(db, "key", 4));
    cr_assert_not(fdb_exists(db, "key", 4));
    fdb_deinit(db);
}

Test(fdb_ops, iterate)
{
    fdb db = fdb_init("foo");
    fiter iterator = NULL;
    fnode node = NULL;

    cr_assert(fdb_insert(db, "1", 2, "data1", 6));
    cr_assert(fdb_insert(db, "2", 2, "data2", 6));
    cr_assert(fdb_insert(db, "3", 2, "data3", 6));

    iterator = fdb_iterate(db);
    cr_assert_not_null(iterator);

    cr_assert(fiter_hasnext(iterator));
    node = fiter_next(iterator);
    cr_assert_not_null(node);
    check_node(node, "1", 2, "data1", 6);

    cr_assert(fiter_hasnext(iterator));
    node = fiter_next(iterator);
    cr_assert_not_null(node);
    check_node(node, "2", 2, "data2", 6);

    cr_assert(fiter_hasnext(iterator));
    node = fiter_next(iterator);
    cr_assert_not_null(node);
    check_node(node, "3", 2, "data3", 6);

    cr_assert_not(fiter_hasnext(iterator));
    cr_assert_null(fiter_next(iterator));
    fdb_deinit(db);
}

Test(fdb_ops, traverse)
{
    fdb db = fdb_init("foo");
    struct traversal_expect expect = { .call_count = 0, .results = { true, true, true } };

    cr_assert(fdb_insert(db, "key1", 5, "data1", 6));
    cr_assert(fdb_insert(db, "key2", 5, "data2", 6));
    cr_assert(fdb_insert(db, "key3", 5, "data3", 6));

    cr_assert_eq(3, fdb_traverse(db, test_traversal_callback, &expect));
    fdb_deinit(db);
}

Test(fdb_ops, traverse_failure)
{
    fdb db = fdb_init("foo");
    struct traversal_expect expect = { .call_count = 0, .results = { true, false } };

    cr_assert(fdb_insert(db, "key1", 5, "data1", 6));
    cr_assert(fdb_insert(db, "key2", 5, "data2", 6));
    cr_assert(fdb_insert(db, "key3", 5, "data3", 6));

    cr_assert_eq(2, fdb_traverse(db, test_traversal_callback, &expect));
    fdb_deinit(db);
}

Test(fdb_ops, invalid)
{
    cr_assert_not(fdb_insert(NULL, NULL, 0, NULL, 0));
    cr_assert_not(fdb_insert(NULL, NULL, 0, "data", 4));
    cr_assert_not(fdb_insert(NULL, "key", 3, NULL, 0));
    cr_assert_not(fdb_insert(NULL, "key", 3, "data", 4));

    cr_assert_not(fdb_update(NULL, NULL, 0, NULL, 0));
    cr_assert_not(fdb_update(NULL, NULL, 0, "data", 4));
    cr_assert_not(fdb_update(NULL, "key", 3, NULL, 0));
    cr_assert_not(fdb_update(NULL, "key", 3, "data1", 5));

    cr_assert_not(fdb_remove(NULL, NULL, 0));
    cr_assert_not(fdb_remove(NULL, "key", 3));
    cr_assert_not(fdb_remove(NULL, "key", 0));
    cr_assert_not(fdb_remove(NULL, "key", 3));

    cr_assert_not(fdb_exists(NULL, "key", 3));
    cr_assert_not(fdb_exists(NULL, NULL, 0));
    cr_assert_not(fdb_exists(NULL, "key", 0));

    cr_assert_null(fnode_get_key(NULL));
    cr_assert_eq(0, fnode_get_keysize(NULL));

    cr_assert_null(fnode_get_data(NULL));
    cr_assert_eq(0, fnode_get_datasize(NULL));
}
