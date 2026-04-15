/**
 * @file fdb_storage.h
 * @brief Pluggable storage backend interface for fdb.
 *
 * A backend is described by an fdb_storage_ops_t filled with function
 * pointers plus two size fields:
 *
 *   ctx_size        — bytes the caller must allocate for the backend context.
 *   cursor_buf_size — bytes the caller must allocate for one cursor.
 *
 * Both sizes are used by fdb to perform allocations on behalf of the backend,
 * so backends need not call malloc themselves.  On embedded targets the
 * allocation can come from a static pool or the stack.
 *
 * Lifetime rules for fdb_node_t pointers returned by find / cursor_get:
 *   The key and data pointers are BORROWED from backend storage.
 *   They are valid until the next mutating call (insert / update / remove)
 *   on the same fdb handle, or until fdb_deinit.  Do not cache them.
 */

#ifndef FDB_STORAGE_H_
#define FDB_STORAGE_H_

#include "fdb/fdb_types.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Visit callback used by traverse. */
typedef bool (*fdb_storage_visit_fn)(fdb_node_t entry, void* userdata);

typedef struct fdb_storage_ops_ {
    /* ------------------------------------------------------------------ */
    /* Sizing — used by fdb to allocate ctx and cursor buffers.            */
    /* ------------------------------------------------------------------ */
    size_t ctx_size;         /* bytes needed for one backend context       */
    size_t cursor_buf_size;  /* bytes needed for one cursor                */

    /* ------------------------------------------------------------------ */
    /* Lifecycle                                                           */
    /* ------------------------------------------------------------------ */

    /* Initialise a pre-allocated context block. Called once after alloc. */
    bool (*open) (void* ctx);

    /* Tear down the context and free all stored entries.                 */
    void (*close)(void* ctx);

    /* ------------------------------------------------------------------ */
    /* Mutations                                                           */
    /* ------------------------------------------------------------------ */

    /* Insert a new key-value pair.  Fails (returns false) if the key     */
    /* already exists.                                                     */
    bool (*insert)(void* ctx,
                   const void* key, size_t ksz,
                   const void* val, size_t vsz);

    /* Replace the value for an existing key.  Fails if the key does not  */
    /* exist.  The new value may differ in size from the stored one.      */
    bool (*update)(void* ctx,
                   const void* key, size_t ksz,
                   const void* val, size_t vsz);

    /* Remove the entry for key.  Fails if the key does not exist.        */
    bool (*remove)(void* ctx, const void* key, size_t ksz);

    /* ------------------------------------------------------------------ */
    /* Reads                                                               */
    /* ------------------------------------------------------------------ */

    /* Locate an entry.  Returns a node with key == NULL when not found.  */
    fdb_node_t (*find)(void* ctx, const void* key, size_t ksz);

    /* ------------------------------------------------------------------ */
    /* Bulk scan                                                           */
    /* ------------------------------------------------------------------ */

    /* Call visit() for every entry in storage order.  Stops early if     */
    /* visit() returns false.  Returns the number of entries visited.     */
    size_t (*traverse)(void* ctx,
                       fdb_storage_visit_fn visit, void* userdata);

    /* ------------------------------------------------------------------ */
    /* Cursor (forward iteration)                                         */
    /* ------------------------------------------------------------------ */

    /* Initialise cursor_buf (caller-allocated, cursor_buf_size bytes).   */
    bool (*cursor_open) (void* ctx, void* cursor_buf);

    /* Move to the first entry.  Returns false if empty.                  */
    bool (*cursor_first)(void* cursor_buf);

    /* True while the cursor points to a valid entry.                     */
    bool (*cursor_valid)(void* cursor_buf);

    /* Advance to the next entry.  Returns false when exhausted.          */
    bool (*cursor_next) (void* cursor_buf);

    /* Return a view of the current entry (borrowed pointers).            */
    fdb_node_t (*cursor_get)(void* cursor_buf);

    /* Release any resources held inside cursor_buf (not the buf itself). */
    void (*cursor_close)(void* cursor_buf);

} fdb_storage_ops_t;

#ifdef __cplusplus
}
#endif

#endif /* FDB_STORAGE_H_ */
