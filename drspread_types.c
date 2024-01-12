#ifndef DRSPREAD_TYPES_C
#define DRSPREAD_TYPES_C
#include <stddef.h>
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
enum {CTX_EXTRA=60000};
DRSP_EXPORT
DrSpreadCtx* _Nullable
drsp_create_ctx(ARGS){
#undef ARGS
    size_t sz = (CTX_EXTRA+sizeof(DrSpreadCtx));
    DrSpreadCtx* ctx = drsp_alloc(0, NULL, sz, _Alignof *ctx);
    // fprintf(stderr, "%zu\n", sz);
    __builtin_memset(ctx, 0, sizeof *ctx);
    if(!ctx) return NULL;
    #ifndef DRSPREAD_DIRECT_OPS
        __builtin_memcpy((void*)&ctx->_ops, ops, sizeof *ops);
    #endif
    BuffAllocator a = {(char*)(ctx+1), (char*)(ctx+1), CTX_EXTRA+(char*)(ctx+1)};
    __builtin_memcpy((void*)&ctx->_a, &a, sizeof a);
    ctx->a = &ctx->_a;
    ctx->null.kind = EXPR_BLANK;
    ctx->error.kind = EXPR_ERROR;
    return ctx;
}

static
void
drsp_destroy_ctx_(DrSpreadCtx* ctx){
    free_linked_arenas(ctx->temp_string_arena);
    free_sheet_datas(ctx);
    destroy_string_heap(&ctx->sheap);
    destroy_parse_heap(&ctx->pheap);
    memset(ctx, 0xfe, sizeof(DrSpreadCtx));
}

DRSP_EXPORT
int
drsp_destroy_ctx(DrSpreadCtx*_Nullable ctx){
    if(!ctx) return 0;
    drsp_destroy_ctx_(ctx);
    drsp_alloc(sizeof*ctx+CTX_EXTRA, ctx, 0, _Alignof *ctx);
    return 0;
}

// This has to be called before any other usage of that sheet.
DRSP_EXPORT
int
drsp_set_sheet_name(DrSpreadCtx*restrict ctx, SheetHandle sheet, const char*restrict name, size_t length){
    SheetData* sd = sheet_get_or_create_by_handle(ctx, sheet);
    if(!sd) return 1;
    DrspAtom str = drsp_intern_str_lower(ctx, name, length);
    if(!str) return 1;
    sd->name = str;
    return 0;
}

DRSP_EXPORT
int
drsp_set_sheet_alias(DrSpreadCtx*restrict ctx, SheetHandle sheet, const char*restrict name, size_t length){
    SheetData* sd = sheet_lookup_by_handle(ctx, sheet);
    if(!sd) return 1;
    DrspAtom str = drsp_intern_str_lower(ctx, name, length);
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
    StringView sv = stripped2(text, length);
    text = sv.text;
    length = sv.length;
    DrspAtom str = drsp_intern_str(ctx, text, length);
    if(!str) return 1;
    return set_cached_cell(&sd->cell_cache, row, col, str);
}

DRSP_EXPORT
int
drsp_set_cell_atom(DrSpreadCtx*restrict ctx, SheetHandle sheet, intptr_t row, intptr_t col, DrspAtom str){
    if(!str) return 1;
    SheetData* sd = sheet_lookup_by_handle(ctx, sheet);
    if(!sd) return 1;
    if(row+1 > sd->height)
        sd->height = row+1;
    if(col+1 > sd->width)
        sd->width = col+1;
    return set_cached_cell(&sd->cell_cache, row, col, str);
}

DRSP_EXPORT
int
drsp_set_extra_dimensional_str(DrSpreadCtx*restrict ctx, SheetHandle sheet, intptr_t id, const char*restrict text, size_t length){
    SheetData* sd = sheet_lookup_by_handle(ctx, sheet);
    if(!sd) return 1;
    for(unsigned i = 0; i < sd->extra_dimensional.count; i++){
        if(sd->extra_dimensional.cells[i].id == id){
            goto foundit;
        }
    }
    if(sd->extra_dimensional.count >= arrlen(sd->extra_dimensional.cells))
        return 1;
    sd->extra_dimensional.cells[sd->extra_dimensional.count++].id = id;
    foundit:;
    DrspAtom str = drsp_intern_str(ctx, text, length);
    if(!str) return 1;
    return set_cached_cell(&sd->cell_cache, IDX_EXTRA_DIMENSIONAL, id, str);
}

DRSP_EXPORT
int
drsp_set_col_name(DrSpreadCtx*restrict ctx, SheetHandle sheet, intptr_t idx, const char*restrict text, size_t length){
    SheetData* sd = sheet_lookup_by_handle(ctx, sheet);
    if(!sd) return 1;
    DrspAtom str = drsp_intern_str_lower(ctx, text, length);
    if(!str) return 1;
    return set_cached_col_name(&sd->col_cache, str, idx);
}

// Delete all data associated with a sheet.
DRSP_EXPORT
int
drsp_del_sheet(DrSpreadCtx*restrict ctx, SheetHandle sheet){
    for(size_t i = 0; i < ctx->map.n; i++){
        SheetData* d = &ctx->map.data[i];
        if(d->handle != sheet) continue;
        cleanup_sheet_data(d);
        // unordered remove
        if(i != ctx->map.n-1)
            ctx->map.data[i] = ctx->map.data[--ctx->map.n];
        else
            --ctx->map.n;
        return 0;
    }
    return 1;
}

DRSP_INTERNAL
void
cleanup_sheet_data(SheetData* d){
    drsp_alloc(d->cell_cache.cap*(sizeof(RowColSv)+2*sizeof(uint32_t)), d->cell_cache.data, 0, _Alignof(RowColSv));
    cleanup_col_cache(&d->col_cache);
    drsp_alloc(d->output_result_cache.cap*(sizeof(CachedResult)+2*sizeof(uint32_t)), d->output_result_cache.data, 0, _Alignof(CachedResult));
    cleanup_named_cells(&d->named_cells);
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

static
DrspAtom _Nullable
drsp_create_str_(DrSpreadCtx*, const char* txt, size_t len);

DRSP_INTERNAL
DrspAtom _Nullable
drsp_intern_sv_lower(DrSpreadCtx* ctx, StringView sv){
    return drsp_intern_str_lower(ctx, sv.text, sv.length);
}

force_inline
DrspAtom
drsp_dollar_atom(void){
    return (DrspAtom)&short_strings[('$'+1)*2];
}

force_inline
DrspAtom
drsp_nil_atom(void){
    return (DrspAtom)&short_strings[0];
}

static DrspAtom _Nullable
drsp_intern_str(DrSpreadCtx* ctx, const char*_Null_unspecified txt, size_t length){
    if(length > UINT16_MAX) return NULL;
    if(!length) return drsp_nil_atom();
    if(length == 1 && (uint8_t)*txt <= 127){
        uint8_t c = (uint8_t)*txt;
        c++;
        c*= 2;
        return (DrspAtom)&short_strings[c];
    }

    DrspAtom result = drsp_create_str_(ctx, txt, length);

    return result;
}

static DrspAtom _Nullable
drsp_intern_str_lower(DrSpreadCtx* ctx, const char*_Null_unspecified txt, size_t length){
    if(length > UINT16_MAX) return NULL;
    if(!length) return drsp_nil_atom();
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    char* tmp = buff_alloc(ctx->a, length);
    if(!tmp) return NULL;
    for(size_t i = 0; i < length; i++){
        tmp[i] = txt[i] |0x20;
    }
    txt = tmp;
    DrspAtom result;
    if(length == 1 && (uint8_t)*txt <= 127){
        uint8_t c = (uint8_t)*txt;
        c++;
        c*= 2;
        result = (DrspAtom)&short_strings[c];
    }
    else
        result = drsp_create_str_(ctx, txt, length);
    buff_set(ctx->a, bc);
    return result;
}

static
DrspAtom _Nullable
drsp_create_str_(DrSpreadCtx* ctx, const char* txt, size_t length){
    StringHeap* heap = &ctx->sheap;
    size_t sz = offsetof(DrspStr, data)+length;
    size_t cap = heap->cap;
    // XXX overflow checking
    if(unlikely(heap->n >= cap)){
        size_t old_cap = cap;
        size_t new_cap = old_cap?old_cap*2:1024;
        size_t new_size = new_cap * (sizeof(DrspAtom)+2*sizeof(uint32_t));
        size_t old_size = old_cap * (sizeof(DrspAtom)+2*sizeof(uint32_t));
        unsigned char* new_data = drsp_alloc(old_size, heap->data, new_size, _Alignof(DrspStr));
        if(!new_data) return NULL;
        heap->data = new_data;
        heap->cap = new_cap;
        cap = new_cap;
        uint32_t* indexes = (uint32_t*)(new_data + sizeof(DrspAtom)*new_cap);
        __builtin_memset(indexes, 0xff, 2*sizeof(*indexes)*new_cap);
        DrspAtom* items = (DrspAtom*)new_data;
        for(size_t i = 0; i < heap->n; i++){
            DrspAtom item = items[i];
            uint32_t hash = hash_align1(item->data, item->length);
            uint32_t idx = fast_reduce32(hash, (uint32_t)2*new_cap);
            while(indexes[idx] != UINT32_MAX){
                idx++;
                if(unlikely(idx >= 2*new_cap)) idx = 0;
            }
            indexes[idx] = i;
        }
    }
    uint32_t hash = hash_align1(txt, length);
    uint32_t* indexes = (uint32_t*)(heap->data + sizeof(DrspAtom)*cap);
    DrspAtom* items = (DrspAtom*)heap->data;
    uint32_t idx = fast_reduce32(hash, (uint32_t)2*cap);
    for(;;){
        uint32_t i = indexes[idx];
        if(i == UINT32_MAX){ // empty slot
            DrspStr* str = linked_arena_alloc(&heap->arena, sz);
            // DrspAtom str = malloc(sz);
            if(!str) return NULL;
            str->length = length;
            __builtin_memcpy(str->data, txt, length);
            indexes[idx]= heap->n;
            items[heap->n++] = str;
            return str;
        }
        DrspAtom item = items[i];
        if(sv_equals2((StringView){item->length, item->data}, txt, length)){
            return item;
        }
        idx++;
        if(unlikely(idx >= 2*cap)) idx = 0;
    }
}

DRSP_INTERNAL
void
destroy_string_heap(StringHeap* heap){
    free_linked_arenas(heap->arena);
    drsp_alloc(heap->cap*(sizeof(DrspAtom)+2*sizeof(uint32_t)), heap->data, 0, _Alignof(DrspStr));
    __builtin_memset(heap, 0, sizeof *heap);
}

typedef struct ParsePair ParsePair;
struct ParsePair {
    DrspAtom key;
    Expression* value;
};

DRSP_INTERNAL
void
destroy_parse_heap(ParseHeap* heap){
    free_linked_arenas(heap->arena);
    drsp_alloc(heap->cap*(sizeof(ParsePair)+2*sizeof(uint32_t)), heap->data, 0, _Alignof(ParsePair));
    __builtin_memset(heap, 0, sizeof *heap);
}


DRSP_INTERNAL
Expression*_Null_unspecified*_Nullable
getsert_cached_parse(DrSpreadCtx* ctx, DrspAtom a){
    ParseHeap* heap = &ctx->pheap;
    size_t cap = heap->cap;
    // XXX overflow checking
    if(unlikely(heap->n >= cap)){
        size_t old_cap = cap;
        size_t new_cap = old_cap?old_cap*2:64;
        size_t new_size = new_cap * (sizeof(ParsePair)+2*sizeof(uint32_t));
        size_t old_size = old_cap * (sizeof(ParsePair)+2*sizeof(uint32_t));
        unsigned char* new_data = drsp_alloc(old_size, heap->data, new_size, _Alignof(ParsePair));
        if(!new_data) return NULL;
        heap->data = new_data;
        heap->cap = new_cap;
        cap = new_cap;
        uint32_t* indexes = (uint32_t*)(new_data + sizeof(ParsePair)*new_cap);
        __builtin_memset(indexes, 0xff, 2*sizeof(*indexes)*new_cap);
        ParsePair* items = (ParsePair*)new_data;
        for(size_t i = 0; i < heap->n; i++){
            ParsePair* item = &items[i];
            uint32_t hash = hash_alignany(&item->key, sizeof item->key);
            // fprintf(stderr, "%p -> %u\n", item->key, hash);
            uint32_t idx = fast_reduce32(hash, (uint32_t)2*new_cap);
            while(indexes[idx] != UINT32_MAX){
                idx++;
                if(unlikely(idx >= 2*new_cap)) idx = 0;
            }
            indexes[idx] = i;
        }
    }
    uint32_t hash = hash_alignany(&a, sizeof a);
    uint32_t* indexes = (uint32_t*)(heap->data + sizeof(ParsePair)*cap);
    ParsePair* items = (ParsePair*)heap->data;
    uint32_t idx = fast_reduce32(hash, (uint32_t)2*cap);
    for(;;){
        uint32_t i = indexes[idx];
        if(i == UINT32_MAX){ // empty slot
            indexes[idx] = heap->n;
            items[heap->n].key = a;
            return &items[heap->n++].value;
        }
        ParsePair* item = &items[i];
        if(item->key ==  a)
            return &item->value;
        idx++;
        if(unlikely(idx >= 2*cap)) idx = 0;
    }
}

DRSP_INTERNAL
void
cache_parse(DrSpreadCtx* ctx, DrspAtom a, Expression*_Nonnull e){
    Expression** cached = getsert_cached_parse(ctx, a);
    if(cached) *cached = e;
}

DRSP_INTERNAL
Expression*_Nullable
has_cached_parse(DrSpreadCtx* ctx, DrspAtom a){
    ParseHeap* heap = &ctx->pheap;
    size_t cap = heap->cap;
    if(!cap) return NULL;
    uint32_t hash = hash_alignany(&a, sizeof a);
    uint32_t* indexes = (uint32_t*)(heap->data + sizeof(ParsePair)*cap);
    ParsePair* items = (ParsePair*)heap->data;
    uint32_t idx = fast_reduce32(hash, (uint32_t)2*cap);
    for(;;){
        uint32_t i = indexes[idx];
        if(i == UINT32_MAX) // empty slot
            return NULL;
        ParsePair* item = &items[i];
        if(item->key ==  a){
            assert(item->value > (Expression*)1024);
            return item->value;
        }
        idx++;
        if(unlikely(idx >= 2*cap)) idx = 0;
    }
}

DRSP_INTERNAL
void
free_linked_arenas(LinkedArena*_Nullable arena){
    while(arena){
        LinkedArena* to_free = arena;
        arena = arena->next;
        drsp_alloc(sizeof *to_free, to_free, 0, _Alignof *to_free);
    }
}

DRSP_INTERNAL
void
free_sheet_datas(DrSpreadCtx* ctx){
    for(size_t i = 0; i < ctx->map.n; i++){
        SheetData* d = &ctx->map.data[i];
        cleanup_sheet_data(d);
    }
    drsp_alloc(ctx->map.cap*sizeof *ctx->map.data, ctx->map.data, 0, _Alignof *ctx->map.data);
}

static inline
DrspAtom _Nullable
get_cached_cell(CellCache* cache, intptr_t row, intptr_t col){
    size_t cap = cache->cap;
    if(!cap) return NULL;
    RowCol key = {row, col};
    uint32_t hash = hash_alignany(&key, sizeof key);
    RowColSv *items = (RowColSv*)cache->data;
    uint32_t* indexes = (uint32_t*)(cache->data + sizeof(RowColSv)*cap);
    uint32_t idx = fast_reduce32(hash, (uint32_t)2*cap);
    for(;;){
        uint32_t i = indexes[idx];
        if(i == UINT32_MAX){ // empty slot
            return NULL;
        }
        if(items[i].rc.row == row && items[i].rc.col == col){
            return items[i].sv;
        }
        idx++;
        if(unlikely(idx >= 2*cap)) idx = 0;
    }
}


static inline
int
set_cached_cell(CellCache* cache, intptr_t row, intptr_t col, DrspAtom str){
    if(unlikely(cache->n >= cache->cap)){
        size_t old_cap = cache->cap;
        size_t new_cap = old_cap?old_cap*2:128;
        size_t new_size = new_cap*(sizeof(RowColSv)+2*sizeof(uint32_t));
        size_t old_size = old_cap*(sizeof(RowColSv)+2*sizeof(uint32_t));
        unsigned char* new_data = drsp_alloc(old_size, cache->data, new_size, _Alignof(RowColSv));
        if(!new_data) return 1;
        cache->data = new_data;
        cache->cap = new_cap;
        uint32_t* indexes = (uint32_t*)(new_data + sizeof(RowColSv)*new_cap);
        __builtin_memset(indexes, 0xff, 2*sizeof(*indexes)*new_cap);
        RowColSv *items = (RowColSv*)new_data;
        for(size_t i = 0; i < cache->n; i++){
            RowCol k = items[i].rc;
            uint32_t hash = hash_alignany(&k, sizeof k);
            uint32_t idx = fast_reduce32(hash, (uint32_t)2*new_cap);
            while(indexes[idx] != UINT32_MAX){
                idx++;
                if(unlikely(idx >= 2*new_cap)) idx = 0;
            }
            indexes[idx] = i;
        }
    }
    size_t cap = cache->cap;
    RowCol key = {row, col};
    uint32_t hash = hash_alignany(&key, sizeof key);
    RowColSv *items = (RowColSv*)cache->data;
    uint32_t* indexes = (uint32_t*)(cache->data + sizeof(RowColSv)*cap);
    uint32_t idx = fast_reduce32(hash, (uint32_t)2*cap);
    for(;;){
        uint32_t i = indexes[idx];
        if(i == UINT32_MAX){ // empty slot
            indexes[idx] = cache->n;
            items[cache->n] = (RowColSv){key, str};
            cache->n++;
            return 0;
        }
        if(items[i].rc.row == row && items[i].rc.col == col){
            items[i].sv = str;
            return 0;
        }
        idx++;
        if(unlikely(idx >= 2*cap)) idx = 0;
    }
}

DRSP_INTERNAL
CachedResult*_Nullable
get_cached_output_result(OutputResultCache* cache, intptr_t row, intptr_t col){
    // printf("%zd,%zd\n", row, col);
    if(unlikely(cache->n >= cache->cap)){
        size_t old_cap = cache->cap;
        size_t new_cap = old_cap?old_cap*2:128;
        size_t new_size = new_cap*(sizeof(CachedResult)+2*sizeof(uint32_t));
        size_t old_size = old_cap*(sizeof(CachedResult)+2*sizeof(uint32_t));
        // fprintf(stderr, "cache->n: %zu\n", cache->n);
        // fprintf(stderr, "old_cap: %zu\n", old_cap);
        // fprintf(stderr, "new_cap: %zu\n", old_cap);
        // fprintf(stderr, "new_size: %zu\n", new_size);
        unsigned char* new_data = drsp_alloc(old_size, cache->data, new_size, _Alignof(CachedResult));
        if(!new_data) return NULL;
        cache->data = new_data;
        cache->cap = new_cap;
        uint32_t* indexes = (uint32_t*)(new_data + sizeof(CachedResult)*new_cap);
        __builtin_memset(indexes, 0xff, 2*sizeof(*indexes)*new_cap);
        CachedResult *items = (CachedResult*)new_data;
        for(size_t i = 0; i < cache->n; i++){
            RowCol k = items[i].loc;
            uint32_t hash = hash_alignany(&k, sizeof k);
            uint32_t idx = fast_reduce32(hash, (uint32_t)new_cap*2);
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
    uint32_t idx = fast_reduce32(hash, (uint32_t)cap*2);
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
        if(unlikely(idx >= cap*2)) idx = 0;
    }
}

DRSP_INTERNAL
CachedResult*_Nullable
has_cached_output_result(const OutputResultCache* cache, intptr_t row, intptr_t col){
    size_t cap = cache->cap;
    if(!cap) return NULL;
    RowCol key = {row, col};
    uint32_t hash = hash_alignany(&key, sizeof key);
    CachedResult *items = (CachedResult*)cache->data;
    uint32_t* indexes = (uint32_t*)(cache->data + sizeof(CachedResult)*cap);
    uint32_t idx = fast_reduce32(hash, (uint32_t)cap*2);
    for(;;){
        uint32_t i = indexes[idx];
        if(i == UINT32_MAX){ // empty slot
            return NULL;
        }
        if(items[i].loc.row == row && items[i].loc.col == col){
            return &items[i];
        }
        idx++;
        if(unlikely(idx >= cap*2)) idx = 0;
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
        size_t old_size = cap*sizeof(SheetData);
        SheetData* p = drsp_alloc(old_size, ctx->map.data, new_size, _Alignof(SheetData));
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
sheet_lookup_by_name(DrSpreadCtx* ctx, DrspAtom name){
    for(size_t i = 0; i < ctx->map.n; i++){
        SheetData* data = &ctx->map.data[i];
        if(data->name == name)
            return data;
        if(!data->alias) continue;
        if(data->alias == name)
            return data;
    }
    return NULL;
}

DRSP_INTERNAL
DrspAtom
sp_cell_atom(SheetData* sd, intptr_t row, intptr_t col){
    if(row != IDX_EXTRA_DIMENSIONAL)
        if(row < 0 || col < 0 || row >= sd->height || col >= sd->width)
            return drsp_nil_atom();
    CellCache* cache = &sd->cell_cache;
    DrspAtom cached = get_cached_cell(cache, row, col);
    if(!cached)
        return drsp_nil_atom();
    return cached;
}

DRSP_INTERNAL
SheetData*_Nullable
udf_lookup_by_handle(const DrSpreadCtx* ctx, SheetHandle handle){
    SheetData* udf = sheet_lookup_by_handle(ctx, handle);
    if(udf && !(udf->flags & DRSP_SHEET_FLAGS_IS_FUNCTION))
        return NULL;
    return udf;
}

DRSP_INTERNAL
SheetData*_Nullable
udf_lookup_by_name(DrSpreadCtx* ctx, DrspAtom name){
    SheetData* udf = sheet_lookup_by_name(ctx, name);
    if(udf && !(udf->flags & DRSP_SHEET_FLAGS_IS_FUNCTION))
        return NULL;
    return udf;
}

DRSP_EXPORT
int
drsp_set_function_params(DrSpreadCtx*restrict ctx, SheetHandle function, size_t n_params, const intptr_t* rows, const intptr_t* cols){
    SheetData* udf = udf_lookup_by_handle(ctx, function);
    if(!udf) return 1;
    if(n_params >= arrlen(udf->params)) return 1;
    udf->paramc = 0;
    for(size_t i = 0; i < n_params; i++){
        UserDefinedFunctionParameter* p = &udf->params[i];
        p->col = cols[i];
        p->row = rows[i];
        if(p->row+1 > udf->height)
            udf->height = p->row+1;
        if(p->col+1 > udf->width)
            udf->width = p->col+1;
    }
    udf->paramc = n_params;
    return 0;
}

DRSP_EXPORT
int
drsp_clear_function_params(DrSpreadCtx* restrict ctx, SheetHandle function){
    SheetData* udf = udf_lookup_by_handle(ctx, function);
    if(!udf) return 1;
    udf->paramc = 0;
    return 0;
}

DRSP_EXPORT
int
drsp_set_function_output(DrSpreadCtx* restrict ctx, SheetHandle function, intptr_t row, intptr_t col){
    SheetData* udf = udf_lookup_by_handle(ctx, function);
    if(!udf) return 1;
    udf->out_col = col;
    udf->out_row = row;
    if(row+1 > udf->height)
        udf->height = row+1;
    if(col+1 > udf->width)
        udf->width = col+1;
    return 0;
}

DRSP_EXPORT
int
drsp_set_sheet_flags(DrSpreadCtx*restrict ctx, SheetHandle sheet, unsigned flags){
    SheetData* sd = sheet_lookup_by_handle(ctx, sheet);
    if(!sd) return 1;
    // Should we mask here?
    sd->flags = flags;
    return 0;
}

DRSP_EXPORT
int
drsp_set_sheet_flag(DrSpreadCtx*restrict ctx, SheetHandle sheet, unsigned flag, _Bool on){
    SheetData* sd = sheet_lookup_by_handle(ctx, sheet);
    if(!sd) return 1;
    if(on) sd->flags |= flag;
    else   sd->flags &= ~flag;
    return 0;
}

DRSP_EXPORT
unsigned
drsp_get_sheet_flags(DrSpreadCtx*restrict ctx, SheetHandle sheet){
    SheetData* sd = sheet_lookup_by_handle(ctx, sheet);
    if(!sd) return 0;
    return sd->flags;
}

DRSP_EXPORT
DrspAtom _Nullable
drsp_atomize(DrSpreadCtx* restrict ctx, const char* txt, size_t length){
    StringView sv = stripped2(txt, length);
    txt = sv.text;
    length = sv.length;
    DrspAtom atom = drsp_intern_str(ctx, txt, length);
    return atom;
}

DRSP_EXPORT
const char*
drsp_atom_get_str(DrSpreadCtx* restrict ctx, DrspAtom restrict atom, size_t* restrict length){
    (void)ctx;
    *length = atom->length;
    return atom->data;
}

static inline
void*_Nullable
linked_arena_alloc(LinkedArena*_Nullable*_Nonnull parena, size_t len){
    if(len & 1) len++;
    if(len > LINKED_ARENA_SIZE) return NULL;
    LinkedArena* arena = *parena;
    if(!arena || arena->used+len > LINKED_ARENA_SIZE){
        // This seems inefficient?
        // Do we really need to pack the arenas?
        if(1)while(arena){
            if(arena->used+len <= LINKED_ARENA_SIZE){
                goto alloced;
            }
            arena = arena->next;
        }
        arena = drsp_alloc(0, NULL, sizeof *arena, _Alignof *arena);
        if(!arena) return NULL;
        arena->next = *parena;
        arena->used = 0;
        *parena = arena;
    }
    alloced:;
    char* p = arena->data + arena->used;
    arena->used += len;
    return p;
}

DRSP_INTERNAL
const NamedCell*_Nullable
get_named_cell(const NamedCells* cells, DrspAtom name){
    for(size_t i = 0; i < cells->count; i++){
        if(name == cells->data[i].name)
            return &cells->data[i];
    }
    return NULL;
}

DRSP_INTERNAL
int
set_named_cell(NamedCells* cells, DrspAtom name, intptr_t row, intptr_t col){
    for(size_t i = 0; i < cells->count; i++){
        if(name == cells->data[i].name){
            cells->data[i].col = col;
            cells->data[i].row = row;
            return 0;
        }
    }
    if(cells->count == cells->capacity){
        size_t new_cap = cells->capacity?cells->capacity*2:2;
        void* p = drsp_alloc(cells->capacity * sizeof *cells->data, cells->data, new_cap * sizeof *cells->data, _Alignof(*cells->data));
        if(!p) return 1;
        cells->data = p;
        cells->capacity = new_cap;
    }
    cells->data[cells->count++] = (NamedCell){
        .name = name,
        .row = row,
        .col = col,
    };
    return 0;
}

DRSP_INTERNAL
void
cleanup_named_cells(NamedCells* cells){
    if(cells->data)
        drsp_alloc(cells->capacity * sizeof *cells->data, cells->data, 0, _Alignof(*cells->data));
}

DRSP_INTERNAL
void
clear_named_cell(NamedCells* cells, DrspAtom name){
    for(size_t i = 0; i < cells->count; i++){
        if(name != cells->data[i].name)
            continue;
        if(i == cells->count -1){
            cells->count--;
            return;
        }
        __builtin_memmove(cells->data+i, cells->data+i+1, (cells->count-i-1) * sizeof *cells->data);
        cells->count--;
        return;
    }
}

DRSP_EXPORT
int
drsp_set_named_cell(DrSpreadCtx* restrict ctx, SheetHandle sheet, const char* restrict name, size_t name_len, intptr_t row, intptr_t col){
    if(!name || !name_len) return 1;
    if(row < 0) return 1;
    if(col < 0) return 1;
    SheetData* sd = sheet_lookup_by_handle(ctx, sheet);
    if(!sd) return 1;
    StringView sv = stripped2(name, name_len);
    if(!sv.length) return 1;
    // don't allow overwriting implied column names
    if(sv.length < 2) return 1;
    DrspAtom a = drsp_intern_sv_lower(ctx, sv);
    if(!a) return 1;
    int err = set_named_cell(&sd->named_cells, a, row, col);
    return err;
}

DRSP_EXPORT
int
drsp_clear_named_cell(DrSpreadCtx* restrict ctx, SheetHandle sheet, const char* restrict name, size_t name_len){
    if(!name || !name_len) return 1;
    SheetData* sd = sheet_lookup_by_handle(ctx, sheet);
    if(!sd) return 1;
    StringView sv = stripped2(name, name_len);
    if(!sv.length) return 1;
    DrspAtom a = drsp_intern_sv_lower(ctx, sv);
    if(!a) return 1;
    clear_named_cell(&sd->named_cells, a);
    return 0;
}


#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#endif
