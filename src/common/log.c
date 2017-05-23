#define _POSIX_C_SOURCE 200809L

#include "fdb_log.h"
#include <stdbool.h>

static bool debug_enabled = false;

bool
fdb_is_debug_enabled()
{
    return debug_enabled;
}
