#define _DEFAULT_SOURCE 1
#include "fdb/fdb.h"
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>

#if defined(__unix__) || defined(__APPLE__)
#include <sys/mman.h>
#include <unistd.h>
#endif

static fdb db;

static const struct {
    unsigned char bytes[4];
    size_t size;
} keys[] = {
    {{'a'}, 1},
    {{'a', 0}, 2},
    {{'a', 0, 'b'}, 3},
    {{'a', 0, 'c'}, 3},
    {{0xff, 0x80}, 2},
};

static int setup(void **state)
{
    (void)state;
    db = fdb_init("binary-keys");
    return db ? 0 : -1;
}

static int teardown(void **state)
{
    (void)state;
    fdb_deinit(db);
    return 0;
}

static void seed_records(void)
{
    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i) {
        unsigned char value = (unsigned char)i;
        assert_true(fdb_insert(db, keys[i].bytes, keys[i].size, &value, 1));
    }
}

static void check_value(fdb_node_t node, const void* key, size_t size,
                        unsigned char value)
{
    assert_true(fdb_node_valid(node));
    assert_int_equal(node.key_size, size);
    assert_int_equal(node.data_size, 1);
    assert_memory_equal(node.key, key, size);
    assert_memory_equal(node.data, &value, 1);
}

static void binary_reads_and_iteration(void **state)
{
    (void)state;
    seed_records();
    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i) {
        assert_true(fdb_exists(db, keys[i].bytes, keys[i].size));
        check_value(fdb_find(db, keys[i].bytes, keys[i].size),
                    keys[i].bytes, keys[i].size, (unsigned char)i);
        unsigned char replacement = 99;
        assert_false(fdb_insert(db, keys[i].bytes, keys[i].size, &replacement, 1));
    }
    fiter it = fdb_iterate(db);
    assert_non_null(it);
    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i) {
        assert_true(fiter_hasnext(it));
        check_value(fiter_next(it), keys[i].bytes, keys[i].size, (unsigned char)i);
    }
    assert_false(fiter_hasnext(it));
    fiter_free(it);
}

static void binary_updates(void **state)
{
    (void)state;
    seed_records();
    unsigned char replacement = 99;
    assert_true(fdb_update(db, keys[2].bytes, keys[2].size, &replacement, 1));
    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i)
        check_value(fdb_find(db, keys[i].bytes, keys[i].size),
                    keys[i].bytes, keys[i].size, i == 2 ? replacement : (unsigned char)i);
}

static void binary_removes(void **state)
{
    (void)state;
    seed_records();
    assert_true(fdb_remove(db, keys[2].bytes, keys[2].size));
    assert_false(fdb_exists(db, keys[2].bytes, keys[2].size));
    assert_false(fdb_remove(db, keys[2].bytes, keys[2].size));
    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i) {
        if (i != 2)
            check_value(fdb_find(db, keys[i].bytes, keys[i].size),
                        keys[i].bytes, keys[i].size, (unsigned char)i);
    }
}

static void explicit_lengths(void **state)
{
    (void)state;
    const char query[] = "ab-extra";
    unsigned char first = 1, second = 2, updated = 3;
    assert_true(fdb_insert(db, "ab", 2, &first, 1));
    assert_true(fdb_insert(db, "ab", 3, &second, 1));
    check_value(fdb_find(db, query, 2), "ab", 2, first);
    check_value(fdb_find(db, "ab", 3), "ab", 3, second);
    assert_false(fdb_exists(db, query, 3));
    assert_false(fdb_update(db, query, 3, &updated, 1));
    assert_false(fdb_remove(db, query, 3));
    assert_true(fdb_update(db, query, 2, &updated, 1));
    check_value(fdb_find(db, query, 2), "ab", 2, updated);
    check_value(fdb_find(db, "ab", 3), "ab", 3, second);
    assert_true(fdb_remove(db, query, 2));
    assert_false(fdb_exists(db, query, 2));
    check_value(fdb_find(db, "ab", 3), "ab", 3, second);
}

static void supported_key_boundary(void **state)
{
    (void)state;
    unsigned char key[63]; /* Existing backend limit, not a new API guarantee. */
    memset(key, 0xff, sizeof(key));
    unsigned char value = 7;
    assert_true(fdb_insert(db, key, sizeof(key), &value, 1));
    check_value(fdb_find(db, key, sizeof(key)), key, sizeof(key), value);
    assert_true(fdb_remove(db, key, sizeof(key)));
}

#if defined(__unix__) || defined(__APPLE__)
static unsigned char* mapping;
static size_t page_size;

static int guard_setup(void **state)
{
    if (setup(state) != 0) return -1;
    long size = sysconf(_SC_PAGESIZE);
    if (size <= 0) {
        fdb_deinit(db);
        return -1;
    }
    page_size = (size_t)size;
    mapping = mmap(NULL, page_size * 3, PROT_NONE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mapping == MAP_FAILED) {
        fdb_deinit(db);
        return -1;
    }
    if (mprotect(mapping + page_size, page_size, PROT_READ | PROT_WRITE) != 0) {
        munmap(mapping, page_size * 3);
        fdb_deinit(db);
        return -1;
    }
    return 0;
}

static int guard_teardown(void **state)
{
    int result = munmap(mapping, page_size * 3);
    teardown(state);
    return result;
}

static void exercise_guarded_key(unsigned char* key)
{
    const unsigned char bytes[] = {0x81, 0xfe, 0xff};
    unsigned char value = 1;
    memcpy(key, bytes, sizeof(bytes));
    assert_true(fdb_insert(db, key, sizeof(bytes), &value, 1));
    check_value(fdb_find(db, key, sizeof(bytes)), bytes, sizeof(bytes), value);
    ++value;
    assert_true(fdb_update(db, key, sizeof(bytes), &value, 1));
    check_value(fdb_find(db, key, sizeof(bytes)), bytes, sizeof(bytes), value);
    assert_true(fdb_remove(db, key, sizeof(bytes)));
    assert_false(fdb_exists(db, key, sizeof(bytes)));
}

static void no_read_before_key(void **state)
{
    (void)state;
    exercise_guarded_key(mapping + page_size);
}

static void no_read_after_key(void **state)
{
    (void)state;
    exercise_guarded_key(mapping + page_size * 2 - 3);
}
#endif

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(binary_reads_and_iteration, setup, teardown),
        cmocka_unit_test_setup_teardown(binary_updates, setup, teardown),
        cmocka_unit_test_setup_teardown(binary_removes, setup, teardown),
        cmocka_unit_test_setup_teardown(explicit_lengths, setup, teardown),
        cmocka_unit_test_setup_teardown(supported_key_boundary, setup, teardown),
#if defined(__unix__) || defined(__APPLE__)
        cmocka_unit_test_setup_teardown(no_read_before_key, guard_setup, guard_teardown),
        cmocka_unit_test_setup_teardown(no_read_after_key, guard_setup, guard_teardown),
#endif
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
