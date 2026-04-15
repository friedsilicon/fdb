/*
 * Copyright © 2016 Shivanand Velmurugan. All Rights Reserved.
 */
#include "fdb_private.h"
#include "fdb/fdb.h"
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdio.h>
#include <string.h>

static void fdb_init_already_deinitialized(void **state)
{
    (void) state;
    fdb db = fdb_init("foo");
    fdb_deinit(db);
}

static void fdb_init_init_pass(void **state)
{
    (void) state;
    fdb db1 = fdb_init("foo");
    assert_non_null(db1);
    assert_string_equal("foo", db1->name);
    assert_true(db1->id != 0);
    fdb_deinit(db1);

    fdb db2 = fdb_init("bar");
    assert_non_null(db2);
    assert_string_equal("bar", db2->name);
    assert_true(db2->id != 0);
    fdb_deinit(db2);
}

static void fdb_init_init_fail_with_null(void **state)
{
    (void) state;
    fdb db = fdb_init(NULL);
    assert_null(db);
}

static void fdb_init_duplicate_name(void **state)
{
    (void) state;
    fdb db = fdb_init("foo");
    assert_non_null(db);
    assert_true(db->id != 0);
    assert_string_equal("foo", db->name);

    fdb db1 = fdb_init("foo");
    assert_true(db1 != db);
    assert_true(db->id != db1->id);
    assert_string_equal("foo", db1->name);

    fdb_deinit(db);
    fdb_deinit(db1);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(fdb_init_already_deinitialized),
        cmocka_unit_test(fdb_init_init_pass),
        cmocka_unit_test(fdb_init_init_fail_with_null),
        cmocka_unit_test(fdb_init_duplicate_name),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
