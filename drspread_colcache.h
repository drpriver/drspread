#ifndef DRSPREAD_COLCACHE_H
#define DRSPREAD_COLCACHE_H
#include <stdint.h>
#include "stringview.h"
#ifdef __clang__
#pragma clang assume_nonnull begin
#endif
// This is a hash table
typedef struct ColCache ColCache;
struct ColCache {
    size_t n;
    size_t cap;
    unsigned char* data;
};


static inline
intptr_t*_Nullable
get_cached_col_name(ColCache* cache, const char* name, size_t len);


// empty string means delete
static inline
int
set_cached_col_name(ColCache* cache, const char* name, size_t len, intptr_t value);

static inline
void
cleanup_col_cache(ColCache* cache);


#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#endif
