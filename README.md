# FDB

FDB is a small embeddable C key-value database. The default build uses an
in-memory backend, with optional save/load support for a simple flat binary
format.

[![coverage](https://img.shields.io/badge/coverage-report-blue)](https://friedsilicon.github.io/fdb/coverage/)

## Features

- C99 API
- In-memory storage backend
- Borrowed node-view API via `fdb_node_t`
- Forward iteration and traversal helpers
- Basic transaction coordinator scaffolding
- Save/load support for database contents

## Build

FDB uses CMake. A typical out-of-tree build looks like this:

```sh
git submodule update --init --recursive
cmake -S . -B build -DWITH_TESTS=ON
make -C build
```

The example program is built as `build/bin/simple`.

## Test

With `WITH_TESTS=ON`, the build produces these test binaries:

- `build/bin/test-fdb-init`
- `build/bin/test-fdb-ops`
- `build/bin/test-fdb-txn`
- `build/bin/test-fdb-serialize`

You can run them directly:

```sh
./build/bin/test-fdb-init
./build/bin/test-fdb-ops
./build/bin/test-fdb-txn
./build/bin/test-fdb-serialize
```

Or run the CTest targets from the build directory:

```sh
cd build
ctest --output-on-failure
```

Unit tests use CMocka via CMake.

## Example

String keys and values should be inserted with the intended byte length. For
NUL-terminated C strings, that usually means `strlen(value) + 1`.

```c
#include <stdio.h>
#include <string.h>
#include "fdb/fdb.h"

int main(void)
{
    fdb db = fdb_init("example");
    const char* key = "key1";
    const char* val = "data1";

    size_t key_size = strlen(key) + 1;
    size_t val_size = strlen(val) + 1;

    if (!fdb_insert(db, key, key_size, val, val_size)) {
        return 1;
    }

    fdb_node_t node = fdb_find(db, key, key_size);
    if (!fdb_node_valid(node)) {
        return 1;
    }

    printf("%s -> %s\n", (const char*)node.key, (const char*)node.data);
    fdb_deinit(db);
    return 0;
}
```

`fdb_node_t` is a borrowed view into backend storage. Its `key` and `data`
pointers remain valid only until the next mutating operation on the same
database handle, or until `fdb_deinit()`.

## Public API

Core entry points are declared in [include/fdb/fdb.h](include/fdb/fdb.h):

- `fdb_init()` / `fdb_deinit()`
- `fdb_insert()` / `fdb_update()` / `fdb_remove()` / `fdb_exists()`
- `fdb_find()`
- `fdb_iterate()` / `fiter_next()` / `fiter_free()`
- `fdb_traverse()`
- `fdb_txn_start()` / `fdb_txn_join()` / `fdb_txn_commit()` / `fdb_txn_abort()`
- `fdb_save()` / `fdb_load()`

## License

At the moment, all rights are reserved. This project may eventually move to a
dual-license model.
