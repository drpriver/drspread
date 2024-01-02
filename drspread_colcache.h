#ifndef DRSPREAD_COLCACHE_H
#define DRSPREAD_COLCACHE_H
#include <stdint.h>
#include "stringview.h"
#ifdef __clang__
#pragma clang assume_nonnull begin
#endif

typedef struct DrspStr DrspStr;
typedef const struct DrspStr* DrspAtom;

// This is a hash table
typedef struct ColCache ColCache;
struct ColCache {
    size_t n;
    size_t cap;
    unsigned char* data;
};


static inline
intptr_t*_Nullable
get_cached_col_name(ColCache* cache, DrspAtom name);


// empty string means delete
static inline
int
set_cached_col_name(ColCache* cache, DrspAtom name, intptr_t value);

static inline
void
cleanup_col_cache(ColCache* cache);


#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#endif
