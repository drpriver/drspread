#ifndef DRSPREAD_COLCACHE_C
#define DRSPREAD_COLCACHE_C
#include "drspread_colcache.h"
#include "hash_func.h"
#ifdef __clang__
#pragma clang assume_nonnull begin
#endif

typedef struct ColName ColName;
struct ColName {
    StringView name;
    intptr_t idx;
};

static inline
int
set_cached_col_name(ColCache* cache, const char* name, size_t len, intptr_t value){
    StringView nm = {len, name};
    ColName* names = (ColName*)cache->data;
    _Bool found = 0;
    for(size_t i = 0; i < cache->n; i++){
        if(sv_iequals(nm, names[i].name)){
            names[i].idx = value;
            found = 1;
        }
        else if(names[i].idx == value){
            names[i].name = nm;
            found = 1;
        }
    }
    if(found) return 0;
    if(cache->n >= cache->cap){
        size_t new_cap = cache->cap?cache->cap*2:4;
        names = drsp_alloc(cache->cap * sizeof *names, names, new_cap* sizeof *names, _Alignof *names);
        if(!names) return 1;
        cache->data = (unsigned char*)names;
        cache->cap = new_cap;
    }
    names[cache->n++] = (ColName){nm, value};
    return 0;
#if 0
    if(cache->n*2 >= cache->cap){
        size_t old_cap = cache->cap;
        size_t new_cap = old_cap?old_cap*2:16;
        size_t new_size = new_cap*(sizeof(ColName)+sizeof(uint32_t)*1);
        size_t old_size = old_cap*(sizeof(ColName)+sizeof(uint32_t)*1);
        unsigned char* new_data = drsp_alloc(old_size, cache->data, new_size, _Alignof(ColName));
        if(!new_data)
            return 1;
        cache->data = new_data;
        cache->cap = new_cap;
        uint32_t* indexes = (uint32_t*)(new_data + sizeof(ColName)*new_cap);
        __builtin_memset(indexes, 0xff, sizeof(*indexes)*new_cap*1);
        ColName *items = (ColName*)new_data;
        for(size_t i = 0; i < cache->n; i++){
            StringView k = items[i].name;
            uint32_t hash = ascii_insensitive_hash_align1(k.text, k.length);
            uint32_t idx = fast_reduce32(hash, (uint32_t)new_cap);
            while(indexes[idx] != UINT32_MAX){
                idx++;
                if(unlikely(idx >= new_cap)) idx = 0;
            }
            indexes[idx] = i;
        }
    }
    size_t cap = cache->cap;
    StringView key = {len, name};
    uint32_t hash = ascii_insensitive_hash_align1(key.text, key.length);
    ColName *items = (ColName*)cache->data;
    uint32_t* indexes = (uint32_t*)(cache->data + sizeof(ColName)*cap);
    uint32_t idx = fast_reduce32(hash, (uint32_t)cap);
    for(;;){
        uint32_t i = indexes[idx];
        if(i == UINT32_MAX){ // empty slot
            indexes[idx] = cache->n;
            items[cache->n] = (ColName){key, value};
            cache->n++;
            return 0;
        }
        if(sv_iequals(items[i].name, key)){
            items[i].idx = value;
            return 0;
        }
        idx++;
        if(unlikely(idx >= cap)) idx = 0;
    }
#endif
}

static inline
intptr_t*_Nullable
get_cached_col_name(ColCache* cache, const char* name, size_t len){
    StringView nm = {len, name};
    ColName* names = (ColName*)cache->data;
    for(size_t i = 0; i < cache->n; i++){
        if(sv_iequals(nm, names[i].name)){
            return &names[i].idx;
        }
    }
    return NULL;
#if 0
    size_t cap = cache->cap;
    if(!cap) return NULL;
    StringView key = {len, name};
    uint32_t hash = ascii_insensitive_hash_align1(key.text, key.length);
    ColName *items = (ColName*)cache->data;
    uint32_t* indexes = (uint32_t*)(cache->data + sizeof(ColName)*cap);
    uint32_t idx = fast_reduce32(hash, (uint32_t)cap);
    for(;;){
        uint32_t i = indexes[idx];
        if(i == UINT32_MAX){ // empty slot
            return NULL;
        }
        if(sv_iequals(items[i].name, key)){
            return &items[i].idx;
        }
        idx++;
        if(unlikely(idx >= cap)) idx = 0;
    }
#endif
}

static inline
void
cleanup_col_cache(ColCache* cache){
    drsp_alloc(cache->cap*sizeof(ColName), cache->data, 0, _Alignof(ColName));
#if 0
    drsp_alloc(d->col_cache.cap*(sizeof(ColName)+sizeof(uint32_t)), d->col_cache.data, 0, _Alignof(ColName));
#endif
}
#ifdef __clang__
#pragma clang assume_nonnull end
#endif

#endif
