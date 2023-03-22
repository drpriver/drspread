#ifndef DRSPREAD_TYPES_C
#define DRSPREAD_TYPES_C
#include "drspread_types.h"
#include "hash_func.h"
#ifdef __clang__
#pragma clang assume_nonnull begin
#endif
#ifdef DRSPREAD_DIRECT_OPS
#define ARGS void
#else
#define ARGS const SheetOps* ops
#endif
DRSP_EXPORT
DrSpreadCtx* _Nullable
drsp_create_ctx(ARGS){
#undef ARGS
    enum {N=60000};
    size_t sz = (N+__builtin_offsetof(DrSpreadCtx, buff));
    DrSpreadCtx* ctx = malloc(sz);
    // fprintf(stderr, "%zu\n", sz);
    __builtin_memset(ctx, 0, sizeof *ctx);
    if(!ctx) return NULL;
    #ifndef DRSPREAD_DIRECT_OPS
        __builtin_memcpy((void*)&ctx->_ops, ops, sizeof *ops);
    #endif
    BuffAllocator a = {ctx->buff, ctx->buff, ctx->buff+N};
    __builtin_memcpy((void*)&ctx->_a, &a, sizeof a);
    ctx->a = &ctx->_a;
    ctx->null.kind = EXPR_NULL;
    ctx->error.kind = EXPR_ERROR;
    return ctx;
}

DRSP_EXPORT
int
drsp_destroy_ctx(DrSpreadCtx*_Nullable ctx){
    if(!ctx) return 0;
    free_string_arenas(ctx->temp_string_arena);
    free_sheet_datas(ctx);
    destroy_string_heap(&ctx->sheap);
    memset(ctx, 0xfe, __builtin_offsetof(DrSpreadCtx, buff));
    free(ctx);
    return 0;
}

// This has to be called before any other usage of that sheet.
DRSP_EXPORT
int
drsp_set_sheet_name(DrSpreadCtx*restrict ctx, SheetHandle sheet, const char*restrict name, size_t length){
    SheetData* sd = sheet_get_or_create_by_handle(ctx, sheet);
    if(!sd) return 1;
    DrspStr* str = drsp_create_str(ctx, name, length);
    if(!str) return 1;
    sd->name = str;
    return 0;
}

DRSP_EXPORT
int
drsp_set_sheet_alias(DrSpreadCtx*restrict ctx, SheetHandle sheet, const char*restrict name, size_t length){
    SheetData* sd = sheet_lookup_by_handle(ctx, sheet);
    if(!sd) return 1;
    DrspStr* str = drsp_create_str(ctx, name, length);
    if(!str) return 1;
    sd->alias = str;
    return 0;
}

DRSP_EXPORT
int
drsp_set_cell_str(DrSpreadCtx*restrict ctx, SheetHandle sheet, intptr_t row, intptr_t col, const char*restrict text, size_t length){
    SheetData* sd = sheet_lookup_by_handle(ctx, sheet);
    if(!sd) return 1;
    if(row+1 > sd->height)
        sd->height = row+1;
    if(col+1 > sd->width)
        sd->width = col+1;
    DrspStr* str = drsp_create_str(ctx, text, length);
    if(!str) return 1;
    return set_cached_string(&sd->str_cache, row, col, str->data, str->length);
}

DRSP_EXPORT
int
drsp_set_col_name(DrSpreadCtx*restrict ctx, SheetHandle sheet, intptr_t idx, const char*restrict text, size_t length){
    SheetData* sd = sheet_lookup_by_handle(ctx, sheet);
    if(!sd) return 1;
    DrspStr* str = drsp_create_str(ctx, text, length);
    if(!str) return 1;
    return set_cached_col_name(&sd->col_cache, str->data, length, idx);
}

// Delete all data associated with a sheet.
DRSP_EXPORT
int
drsp_del_sheet(DrSpreadCtx*restrict ctx, SheetHandle sheet){
    for(size_t i = 0; i < ctx->map.n; i++){
        SheetData* d = &ctx->map.data[i];
        if(d->handle != sheet) continue;
        free(d->str_cache.data);
        free(d->col_cache.data);
        free(d->result_cache.data);
        // unordered remove
        if(i != ctx->map.n-1)
            ctx->map.data[i] = ctx->map.data[--ctx->map.n];
        else
            --ctx->map.n;
        return 0;
    }
    return 1;
}

// preload empty string and length 1 strings
static
_Alignas(DrspStr)
const uint16_t short_strings[] = {
    0,   0,
    1,   0, 1,   1, 1,   2, 1,   3,
    1,   4, 1,   5, 1,   6, 1,   7,
    1,   8, 1,   9, 1,  10, 1,  11,
    1,  12, 1,  13, 1,  14, 1,  15,
    1,  16, 1,  17, 1,  18, 1,  19,
    1,  20, 1,  21, 1,  22, 1,  23,
    1,  24, 1,  25, 1,  26, 1,  27,
    1,  28, 1,  29, 1,  30, 1,  31,
    1,  32, 1,  33, 1,  34, 1,  35,
    1,  36, 1,  37, 1,  38, 1,  39,
    1,  40, 1,  41, 1,  42, 1,  43,
    1,  44, 1,  45, 1,  46, 1,  47,
    1,  48, 1,  49, 1,  50, 1,  51,
    1,  52, 1,  53, 1,  54, 1,  55,
    1,  56, 1,  57, 1,  58, 1,  59,
    1,  60, 1,  61, 1,  62, 1,  63,
    1,  64, 1,  65, 1,  66, 1,  67,
    1,  68, 1,  69, 1,  70, 1,  71,
    1,  72, 1,  73, 1,  74, 1,  75,
    1,  76, 1,  77, 1,  78, 1,  79,
    1,  80, 1,  81, 1,  82, 1,  83,
    1,  84, 1,  85, 1,  86, 1,  87,
    1,  88, 1,  89, 1,  90, 1,  91,
    1,  92, 1,  93, 1,  94, 1,  95,
    1,  96, 1,  97, 1,  98, 1,  99,
    1, 100, 1, 101, 1, 102, 1, 103,
    1, 104, 1, 105, 1, 106, 1, 107,
    1, 108, 1, 109, 1, 110, 1, 111,
    1, 112, 1, 113, 1, 114, 1, 115,
    1, 116, 1, 117, 1, 118, 1, 119,
    1, 120, 1, 121, 1, 122, 1, 123,
    1, 124, 1, 125, 1, 126, 1, 127,
};


DRSP_INTERNAL
DrspStr*_Nullable
drsp_create_str(DrSpreadCtx* ctx, const char* txt, size_t length){
    if(length > UINT16_MAX) return NULL;
    if(!length) return (DrspStr*)&short_strings[0];
    if(length == 1 && (uint8_t)*txt <= 127){
        uint8_t c = (uint8_t)*txt;
        c++;
        c*= 2;
        return (DrspStr*)&short_strings[c];
    }
    (void)ctx;
    StringHeap* heap = &ctx->sheap;
    size_t sz = __builtin_offsetof(DrspStr, data)+length;
    size_t cap = heap->cap;
    // XXX overflow checking
    if(unlikely(heap->n*2 >= cap)){
        size_t old_cap = cap;
        size_t new_cap = old_cap?old_cap*2:1024;
        size_t new_size = new_cap * (sizeof(DrspStr*)+sizeof(uint32_t));
        #ifdef __wasm__
            size_t old_size = old_cap * (sizeof(DrspStr*)+sizeof(uint32_t));
            unsigned char* new_data = sane_realloc(heap->data, old_size, new_size);
        #else
            unsigned char* new_data = realloc(heap->data, new_size);
        #endif
        if(!new_data) return NULL;
        heap->data = new_data;
        heap->cap = new_cap;
        cap = new_cap;
        uint32_t* indexes = (uint32_t*)(new_data + sizeof(DrspStr*)*new_cap);
        __builtin_memset(indexes, 0xff, sizeof(*indexes)*new_cap);
        DrspStr** items = (DrspStr**)new_data;
        for(size_t i = 0; i < heap->n; i++){
            DrspStr* item = items[i];
            uint32_t hash = hash_align1(item->data, item->length);
            uint32_t idx = fast_reduce32(hash, (uint32_t)new_cap);
            while(indexes[idx] != UINT32_MAX){
                idx++;
                if(unlikely(idx >= new_cap)) idx = 0;
            }
            indexes[idx] = i;
        }
    }
    uint32_t hash = hash_align1(txt, length);
    uint32_t* indexes = (uint32_t*)(heap->data + sizeof(DrspStr*)*cap);
    DrspStr** items = (DrspStr**)heap->data;
    uint32_t idx = fast_reduce32(hash, (uint32_t)cap);
    for(;;){
        uint32_t i = indexes[idx];
        if(i == UINT32_MAX){ // empty slot
            DrspStr* str = str_arena_alloc(&heap->arena, sz);
            // DrspStr* str = malloc(sz);
            if(!str) return NULL;
            str->length = length;
            __builtin_memcpy(str->data, txt, length);
            indexes[idx]= heap->n;
            items[heap->n++] = str;
            return str;
        }
        DrspStr* item = items[i];
        if(sv_equals2(drsp_to_sv(item), txt, length)){
            return item;
        }
        idx++;
        if(unlikely(idx >= cap)) idx = 0;
    }
}

DRSP_INTERNAL
void
destroy_string_heap(StringHeap* heap){
    free_string_arenas(heap->arena);
    free(heap->data);
    __builtin_memset(heap, 0, sizeof *heap);
}

DRSP_INTERNAL
void
free_string_arenas(StringArena*_Nullable arena){
    while(arena){
        StringArena* to_free = arena;
        arena = arena->next;
        free(to_free);
    }
}

DRSP_INTERNAL
void
free_sheet_datas(DrSpreadCtx* ctx){
    for(size_t i = 0; i < ctx->map.n; i++){
        SheetData* d = &ctx->map.data[i];
        free(d->str_cache.data);
        free(d->col_cache.data);
        free(d->result_cache.data);
    }
    free(ctx->map.data);
}

static inline
StringView*_Nullable
get_cached_string(StringCache* cache, intptr_t row, intptr_t col){
    size_t cap = cache->cap;
    if(!cap) return NULL;
    RowCol key = {row, col};
    uint32_t hash = hash_alignany(&key, sizeof key);
    RowColSv *items = (RowColSv*)cache->data;
    uint32_t* indexes = (uint32_t*)(cache->data + sizeof(RowColSv)*cap);
    uint32_t idx = fast_reduce32(hash, (uint32_t)cap);
    for(;;){
        uint32_t i = indexes[idx];
        if(i == UINT32_MAX){ // empty slot
            return NULL;
        }
        if(items[i].rc.row == row && items[i].rc.col == col){
            return &items[i].sv;
        }
        idx++;
        if(unlikely(idx >= cap)) idx = 0;
    }
}


static inline
int
set_cached_string(StringCache* cache, intptr_t row, intptr_t col, const char*restrict txt, size_t len){
    if(unlikely(cache->n*2 >= cache->cap)){
        size_t old_cap = cache->cap;
        size_t new_cap = old_cap?old_cap*2:128;
        size_t new_size = new_cap*(sizeof(RowColSv)+sizeof(uint32_t));
        #ifdef __wasm__
            size_t old_size = old_cap*(sizeof(RowColSv)+sizeof(uint32_t));
            unsigned char* new_data = sane_realloc(cache->data, old_size, new_size);
        #else
            // fprintf(stderr, "%zu\n", new_size);
            unsigned char* new_data = realloc(cache->data, new_size);
        #endif
        if(!new_data) return 1;
        cache->data = new_data;
        cache->cap = new_cap;
        uint32_t* indexes = (uint32_t*)(new_data + sizeof(RowColSv)*new_cap);
        __builtin_memset(indexes, 0xff, sizeof(*indexes)*new_cap);
        RowColSv *items = (RowColSv*)new_data;
        for(size_t i = 0; i < cache->n; i++){
            RowCol k = items[i].rc;
            uint32_t hash = hash_alignany(&k, sizeof k);
            uint32_t idx = fast_reduce32(hash, (uint32_t)new_cap);
            while(indexes[idx] != UINT32_MAX){
                idx++;
                if(unlikely(idx >= new_cap)) idx = 0;
            }
            indexes[idx] = i;
        }
    }
    size_t cap = cache->cap;
    RowCol key = {row, col};
    uint32_t hash = hash_alignany(&key, sizeof key);
    RowColSv *items = (RowColSv*)cache->data;
    uint32_t* indexes = (uint32_t*)(cache->data + sizeof(RowColSv)*cap);
    uint32_t idx = fast_reduce32(hash, (uint32_t)cap);
    for(;;){
        uint32_t i = indexes[idx];
        if(i == UINT32_MAX){ // empty slot
            indexes[idx] = cache->n;
            items[cache->n] = (RowColSv){key, {len, txt}};
            cache->n++;
            return 0;
        }
        if(items[i].rc.row == row && items[i].rc.col == col){
            items[i].sv = (StringView){len, txt};
            return 0;
        }
        idx++;
        if(unlikely(idx >= cap)) idx = 0;
    }
}

static inline
int
set_cached_col_name(ColCache* cache, const char* name, size_t len, intptr_t value){
    if(cache->n*2 >= cache->cap){
        size_t old_cap = cache->cap;
        size_t new_cap = old_cap?old_cap*2:16;
        size_t new_size = new_cap*(sizeof(ColName)+sizeof(uint32_t));
        #ifdef __wasm__
            size_t old_size = old_cap*(sizeof(ColName)+sizeof(uint32_t));
            unsigned char* new_data = sane_realloc(cache->data, old_size, new_size);
        #else
            // fprintf(stderr, "%zu\n", new_size);
            unsigned char* new_data = realloc(cache->data, new_size);
        #endif
        if(!new_data)
            return 1;
        cache->data = new_data;
        cache->cap = new_cap;
        uint32_t* indexes = (uint32_t*)(new_data + sizeof(ColName)*new_cap);
        __builtin_memset(indexes, 0xff, sizeof(*indexes)*new_cap);
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
}

static inline
intptr_t*_Nullable
get_cached_col_name(ColCache* cache, const char* name, size_t len){
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
}

DRSP_INTERNAL
CachedResult*_Nullable
get_cached_result(ResultCache* cache, intptr_t row, intptr_t col){
    if(unlikely(cache->n*2 >= cache->cap)){
        size_t old_cap = cache->cap;
        size_t new_cap = old_cap?old_cap*2:128;
        size_t new_size = new_cap*(sizeof(CachedResult)+sizeof(uint32_t));
        #ifdef __wasm__
            size_t old_size = old_cap*(sizeof(CachedResult)+sizeof(uint32_t));
            unsigned char* new_data = sane_realloc(cache->data, old_size, new_size);
        #else
            // fprintf(stderr, "%zu\n", new_size);
            unsigned char* new_data = realloc(cache->data, new_size);
        #endif
        if(!new_data) return NULL;
        cache->data = new_data;
        cache->cap = new_cap;
        uint32_t* indexes = (uint32_t*)(new_data + sizeof(CachedResult)*new_cap);
        __builtin_memset(indexes, 0xff, sizeof(*indexes)*new_cap);
        CachedResult *items = (CachedResult*)new_data;
        for(size_t i = 0; i < cache->n; i++){
            RowCol k = items[i].loc;
            uint32_t hash = hash_alignany(&k, sizeof k);
            uint32_t idx = fast_reduce32(hash, (uint32_t)new_cap);
            while(indexes[idx] != UINT32_MAX){
                idx++;
                if(unlikely(idx >= new_cap)) idx = 0;
            }
            indexes[idx] = i;
        }
    }
    size_t cap = cache->cap;
    RowCol key = {row, col};
    uint32_t hash = hash_alignany(&key, sizeof key);
    CachedResult *items = (CachedResult*)cache->data;
    uint32_t* indexes = (uint32_t*)(cache->data + sizeof(CachedResult)*cap);
    uint32_t idx = fast_reduce32(hash, (uint32_t)cap);
    for(;;){
        uint32_t i = indexes[idx];
        if(i == UINT32_MAX){ // empty slot
            indexes[idx] = cache->n;
            items[cache->n] = (CachedResult){
                .loc = key,
                .kind = CACHED_RESULT_NULL,
            };
            return &items[cache->n++];
        }
        if(items[i].loc.row == row && items[i].loc.col == col){
            return &items[i];
        }
        idx++;
        if(unlikely(idx >= cap)) idx = 0;
    }
}

DRSP_INTERNAL
SheetData*_Nullable
sheet_lookup_by_handle(const DrSpreadCtx* ctx, SheetHandle handle){
    for(size_t i = 0; i < ctx->map.n; i++){
        SheetData* data = &ctx->map.data[i];
        if(data->handle == handle)
            return data;
    }
    return NULL;
}

DRSP_INTERNAL
SheetData*_Nullable
sheet_get_or_create_by_handle(DrSpreadCtx* ctx, SheetHandle handle){
    SheetData* sd = sheet_lookup_by_handle(ctx, handle);
    if(sd) return sd;
    if(ctx->map.n >= ctx->map.cap){
        size_t cap = ctx->map.cap;
        size_t newcap = 2*cap;
        if(!newcap) newcap = 8;
        size_t new_size = newcap * sizeof(SheetData);
        #ifdef __wasm__
            size_t old_size = cap*sizeof(SheetData);
            SheetData* p = sane_realloc(ctx->map.data, old_size, new_size);
        #else
            SheetData* p = realloc(ctx->map.data, new_size);
            // fprintf(stderr, "%zu\n", new_size);
        #endif
        if(!p) return NULL;
        ctx->map.data = p;
        ctx->map.cap = newcap;
    }
    sd = &ctx->map.data[ctx->map.n++];
    memset(sd, 0, sizeof *sd);
    sd->handle = handle;
    return sd;
}

DRSP_INTERNAL
SheetData*_Nullable
sheet_lookup_by_name(DrSpreadCtx* ctx, const char* name, size_t len){
    for(size_t i = 0; i < ctx->map.n; i++){
        SheetData* data = &ctx->map.data[i];
        if(sv_iequals2(drsp_to_sv(data->name), name, len))
            return data;
        if(!data->alias) continue;
        if(sv_iequals2(drsp_to_sv(data->alias), name, len))
            return data;
    }
    return NULL;
}

DRSP_INTERNAL
const char*_Nullable
sp_cell_text(DrSpreadCtx* ctx, SheetHandle sheet, intptr_t row, intptr_t col, size_t* len){
    SheetData* sd = sheet_lookup_by_handle(ctx, sheet);
    if(row < 0 || col < 0 || row >= sd->height || col >= sd->width){
        *len = 0;
        return "";
    }
    if(!sd) return NULL;
    StringCache* cache = &sd->str_cache;
    StringView* cached = get_cached_string(cache, row, col);
    if(!cached) {
        *len = 0;
        return "";
    }
    *len = cached->length;
    return cached->text?cached->text:"";
}

#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#endif
