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
check_record(fdb db, const char* k, const char* v)
{
    size_t kl = strlen(k);
    size_t vl = strlen(v);
    fdb_node_t n = fdb_find(db, k, kl + 1);
    check_node(n, k, kl + 1, v, vl + 1);
}

struct traversal_expect {
    size_t call_count;
    bool results[4];
};

static bool
test_traversal_callback(fdb_node_t node, void* user_data)
{
    (void)node;
    struct traversal_expect* expect = user_data;
    expect->call_count++;
    return expect->results[expect->call_count - 1];
}

static void fdb_ops_insert(void **state)
{
    (void) state;
    fdb db = fdb_init("foo");
    assert_true(fdb_insert(db, "key", 4, "data", 5));
    check_record(db, "key", "data");

    assert_false(fdb_insert(db, "key", 4, "data", 5));

    assert_true(fdb_insert(db, "key1", 5, "data", 5));
    check_record(db, "key1", "data");
    fdb_deinit(db);
}

static void fdb_ops_exists(void **state)
{
    (void) state;
    fdb db = fdb_init("foo");
    assert_true(fdb_insert(db, "key", 4, "data", 5));
    assert_true(fdb_exists(db, "key", 4));
    assert_false(fdb_exists(db, "key1", 5));
    fdb_deinit(db);
}

static void fdb_ops_prefix_keys_distinct(void **state)
{
    (void) state;
    fdb db = fdb_init("foo");
    assert_true(fdb_insert(db, "key", 4, "data", 5));
    assert_true(fdb_insert(db, "keylong", 8, "value", 6));

    fdb_node_t n1 = fdb_find(db, "key", 4);
    fdb_node_t n2 = fdb_find(db, "keylong", 8);

    assert_true(fdb_node_valid(n1));
    assert_true(fdb_node_valid(n2));
    assert_memory_equal("key",     n1.key, 4);
    assert_memory_equal("keylong", n2.key, 8);
    fdb_deinit(db);
}

static void fdb_ops_update(void **state)
{
    (void) state;
    fdb db = fdb_init("foo");
    assert_true(fdb_insert(db, "key", 4, "data", 5));
    check_record(db, "key", "data");

    assert_true(fdb_update(db, "key", 4, "data1", 6));
    check_record(db, "key", "data1");
    fdb_deinit(db);
}

static void fdb_ops_update_larger_data(void **state)
{
    (void) state;
    fdb db = fdb_init("foo");
    assert_true(fdb_insert(db, "key", 4, "hi", 3));
    check_record(db, "key", "hi");

    /* grow: new data is larger than original allocation */
    assert_true(fdb_update(db, "key", 4, "much_longer_value", 18));
    check_record(db, "key", "much_longer_value");

    /* key still works for exists/find after realloc */
    assert_true(fdb_exists(db, "key", 4));
    fdb_deinit(db);
}

static void fdb_ops_update_smaller_data(void **state)
{
    (void) state;
    fdb db = fdb_init("foo");
    assert_true(fdb_insert(db, "key", 4, "much_longer_value", 18));
    check_record(db, "key", "much_longer_value");

    /* shrink: new data is smaller than original allocation */
    assert_true(fdb_update(db, "key", 4, "hi", 3));
    check_record(db, "key", "hi");
    assert_int_equal(3, fdb_find(db, "key", 4).data_size);
    fdb_deinit(db);
}

static void fdb_ops_update_nonexistent(void **state)
{
    (void) state;
    fdb db = fdb_init("foo");
    assert_false(fdb_update(db, "key", 4, "data", 5));
    fdb_deinit(db);
}

static void fdb_ops_remove(void **state)
{
    (void) state;
    fdb db = fdb_init("foo");
    assert_true(fdb_insert(db, "key", 4, "data", 5));
    check_record(db, "key", "data");

    assert_true(fdb_remove(db, "key", 4));
    assert_false(fdb_exists(db, "key", 4));
    fdb_deinit(db);
}

static void fdb_ops_remove_then_reinsert(void **state)
{
    (void) state;
    fdb db = fdb_init("foo");
    assert_true(fdb_insert(db, "key", 4, "original", 9));
    check_record(db, "key", "original");

    assert_true(fdb_remove(db, "key", 4));
    assert_false(fdb_exists(db, "key", 4));

    /* reinsert with different data — must succeed and return new value */
    assert_true(fdb_insert(db, "key", 4, "newvalue", 9));
    check_record(db, "key", "newvalue");
    fdb_deinit(db);
}

static void fdb_ops_remove_nonexistent(void **state)
{
    (void) state;
    fdb db = fdb_init("foo");
    assert_false(fdb_remove(db, "key", 4));
    fdb_deinit(db);
}

static void fdb_ops_iterate(void **state)
{
    (void) state;
    fdb db = fdb_init("foo");

    assert_true(fdb_insert(db, "1", 2, "data1", 6));
    assert_true(fdb_insert(db, "2", 2, "data2", 6));
    assert_true(fdb_insert(db, "3", 2, "data3", 6));

    fiter iterator = fdb_iterate(db);
    assert_non_null(iterator);

    assert_true(fiter_hasnext(iterator));
    check_node(fiter_next(iterator), "1", 2, "data1", 6);

    assert_true(fiter_hasnext(iterator));
    check_node(fiter_next(iterator), "2", 2, "data2", 6);

    assert_true(fiter_hasnext(iterator));
    check_node(fiter_next(iterator), "3", 2, "data3", 6);

    assert_false(fiter_hasnext(iterator));
    assert_false(fdb_node_valid(fiter_next(iterator)));
    fiter_free(iterator);
    fdb_deinit(db);
}

static void fdb_ops_iterate_empty_db(void **state)
{
    (void) state;
    fdb db = fdb_init("foo");
    /* iterating an empty db must not crash and must return NULL */
    assert_null(fdb_iterate(db));
    fdb_deinit(db);
}

static void fdb_ops_fiter_free_null(void **state)
{
    (void) state;
    fiter_free(NULL); /* must not crash */
}

static void fdb_ops_traverse(void **state)
{
    (void) state;
    fdb db = fdb_init("foo");
    struct traversal_expect expect = { .call_count = 0, .results = { true, true, true } };

    assert_true(fdb_insert(db, "key1", 5, "data1", 6));
    assert_true(fdb_insert(db, "key2", 5, "data2", 6));
    assert_true(fdb_insert(db, "key3", 5, "data3", 6));

    assert_int_equal(3, fdb_traverse(db, test_traversal_callback, &expect));
    fdb_deinit(db);
}

static void fdb_ops_traverse_failure(void **state)
{
    (void) state;
    fdb db = fdb_init("foo");
    struct traversal_expect expect = { .call_count = 0, .results = { true, false } };

    assert_true(fdb_insert(db, "key1", 5, "data1", 6));
    assert_true(fdb_insert(db, "key2", 5, "data2", 6));
    assert_true(fdb_insert(db, "key3", 5, "data3", 6));

    assert_int_equal(2, fdb_traverse(db, test_traversal_callback, &expect));
    fdb_deinit(db);
}

static void fdb_ops_invalid(void **state)
{
    (void) state;
    assert_false(fdb_insert(NULL, NULL, 0, NULL, 0));
    assert_false(fdb_insert(NULL, NULL, 0, "data", 4));
    assert_false(fdb_insert(NULL, "key", 3, NULL, 0));
    assert_false(fdb_insert(NULL, "key", 3, "data", 4));

    assert_false(fdb_update(NULL, NULL, 0, NULL, 0));
    assert_false(fdb_update(NULL, NULL, 0, "data", 4));
    assert_false(fdb_update(NULL, "key", 3, NULL, 0));
    assert_false(fdb_update(NULL, "key", 3, "data1", 5));

    assert_false(fdb_remove(NULL, NULL, 0));
    assert_false(fdb_remove(NULL, "key", 3));
    assert_false(fdb_remove(NULL, "key", 0));
    assert_false(fdb_remove(NULL, "key", 3));

    assert_false(fdb_exists(NULL, "key", 3));
    assert_false(fdb_exists(NULL, NULL, 0));
    assert_false(fdb_exists(NULL, "key", 0));

    assert_false(fdb_node_valid(fdb_find(NULL, "key", 3)));
    assert_false(fdb_node_valid(fdb_find(NULL, NULL, 0)));
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(fdb_ops_insert),
        cmocka_unit_test(fdb_ops_exists),
        cmocka_unit_test(fdb_ops_prefix_keys_distinct),
        cmocka_unit_test(fdb_ops_update),
        cmocka_unit_test(fdb_ops_update_larger_data),
        cmocka_unit_test(fdb_ops_update_smaller_data),
        cmocka_unit_test(fdb_ops_update_nonexistent),
        cmocka_unit_test(fdb_ops_remove),
        cmocka_unit_test(fdb_ops_remove_then_reinsert),
        cmocka_unit_test(fdb_ops_remove_nonexistent),
        cmocka_unit_test(fdb_ops_iterate),
        cmocka_unit_test(fdb_ops_iterate_empty_db),
        cmocka_unit_test(fdb_ops_fiter_free_null),
        cmocka_unit_test(fdb_ops_traverse),
        cmocka_unit_test(fdb_ops_traverse_failure),
        cmocka_unit_test(fdb_ops_invalid),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
