//
// Copyright © 2023, David Priver
//
#ifndef DRSPREAD_TYPES_H
#define DRSPREAD_TYPES_H
#include <assert.h>
#include <stdlib.h>
#include "drspread.h"
#include "buff_allocator.h"
#include "stringview.h"
#include "hash_func.h"

#ifdef __wasm__
#include "drspread_wasm.h"
#endif

#ifndef force_inline
#define force_inline static inline __attribute__((always_inline))
#endif

#if !defined(likely) && !defined(unlikely)
#if defined(__GNUC__) || defined(__clang__)
#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif
#endif

#ifndef arrlen
#define arrlen(arr) (sizeof(arr)/sizeof((arr)[0]))
#endif

#ifndef DRSP_INTERNAL
#define DRSP_INTERNAL static __attribute__((visibility("hidden")))
#endif

#ifdef __clang__
#pragma clang assume_nonnull begin
#else
#ifndef _Nullable
#define _Nullable
#endif
#endif


typedef struct SpreadContext SpreadContext;

enum ExpressionKind: intptr_t {
    EXPR_ERROR = 0,
    EXPR_NUMBER,
    EXPR_FUNCTION_CALL,
    EXPR_RANGE0D,
    EXPR_RANGE0D_FOREIGN,
    EXPR_RANGE1D_COLUMN,
    EXPR_RANGE1D_COLUMN_FOREIGN,
    EXPR_GROUP,
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_STRING,
    EXPR_NULL,
};
typedef enum ExpressionKind ExpressionKind;

typedef struct Expression Expression;
struct Expression {
    _Alignas(intptr_t) ExpressionKind kind;
};



enum BinaryKind: intptr_t {
    BIN_ADD,
    BIN_SUB,
    BIN_MUL,
    BIN_DIV,
    BIN_LT,
    BIN_LE,
    BIN_GT,
    BIN_GE,
    BIN_EQ,
    BIN_NE,
};
typedef enum BinaryKind BinaryKind;

enum UnaryKind: intptr_t {
    UN_PLUS,
    UN_NEG,
    UN_NOT,
};

typedef enum UnaryKind UnaryKind;

typedef struct Number Number;
struct Number {
    Expression e;
    double value;
};
typedef struct FunctionCall FunctionCall;
#define FORMULAFUNC(name) Expression*_Nullable (name)(SpreadContext* ctx, SheetHandle hnd,  intptr_t caller_row, intptr_t caller_col, int argc, Expression*_Nonnull*_Nonnull argv)
typedef FORMULAFUNC(FormulaFunc);
struct FunctionCall {
    Expression e;
    FormulaFunc* func;
    int argc;
    Expression*_Nonnull*_Nonnull argv;
};

enum {IDX_DOLLAR=-2147483647}; // INT32_MIN+1
enum {IDX_UNSET=-2147483647-1}; // INT32_MIN


typedef struct Range0D Range0D;
struct Range0D {
    Expression e;
    StringView col_name;
    intptr_t row;
};

typedef struct ForeignRange0D ForeignRange0D;
struct ForeignRange0D {
    union {
        Expression e;
        Range0D r;
    };
    StringView sheet_name;
};

typedef struct Range1DColumn Range1DColumn;
struct Range1DColumn {
    Expression e;
    StringView col_name;
    intptr_t row_start, row_end; // inclusive
};

typedef struct ForeignRange1DColumn ForeignRange1DColumn;
struct ForeignRange1DColumn {
    union {
        Range1DColumn r;
        Expression e;
    };
    StringView sheet_name;
};

typedef struct Binary Binary;
struct Binary {
    Expression e;
    BinaryKind op;
    Expression *lhs, *rhs;
};
typedef struct Group Group;
struct Group{
    Expression e;
    Expression* expr;
};

typedef struct Unary Unary;
struct Unary {
    Expression e;
    UnaryKind op;
    Expression* expr;
};

typedef struct String String;
struct String {
    Expression e;
    StringView sv;
};

typedef struct SheetCache SheetCache;
struct SheetCache {
    struct {
        StringView s;
        SheetHandle sheet;
    } items[8];
};

typedef struct StringCache StringCache;
struct StringCache {
    SheetHandle handle;
    size_t n;
    size_t cap;
    unsigned char* data;
};

typedef struct ColCache ColCache;
struct ColCache {
    SheetHandle handle;
    size_t n;
    size_t cap;
    unsigned char* data;
};

typedef struct RowCol RowCol;
struct RowCol {
    intptr_t row, col;
};

typedef struct RowColSv RowColSv;
struct RowColSv {
    RowCol rc;
    StringView sv;
};

__attribute__((no_sanitize("nullability")))
static inline
StringView*_Nullable
get_cached_string(StringCache* cache, intptr_t row, intptr_t col){
#if 1 && !defined(__wasm__)
    // In native code the string accessors actually get inlined so this is
    // unnecessary.
    //
    // Also we don't free in native code, but in wasm we wipe the memory
    // anyway.
    return NULL;
#endif
    if(cache->n*2 >= cache->cap){
        size_t old_cap = cache->cap;
        size_t new_cap = old_cap?old_cap*2:128;
        #ifdef __wasm__
            unsigned char* new_data = sane_realloc(cache->data, old_cap*(4*sizeof(intptr_t)+sizeof(uint32_t)), new_cap*(4*sizeof(intptr_t)+sizeof(uint32_t)));
        #else
            unsigned char* new_data = realloc(cache->data, new_cap*(4*sizeof(intptr_t)+sizeof(uint32_t)));
        #endif
        if(!new_data) return NULL;
        cache->data = new_data;
        cache->cap = new_cap;
        uint32_t* indexes = (uint32_t*)(new_data + 4*sizeof(intptr_t)*new_cap);
        memset(indexes, 0xff, sizeof(*indexes)*new_cap);
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
    uint32_t* indexes = (uint32_t*)(cache->data + 4*sizeof(intptr_t)*cap);
    uint32_t idx = fast_reduce32(hash, (uint32_t)cap);
    for(;;){
        uint32_t i = indexes[idx];
        if(i == UINT32_MAX){ // empty slot
            indexes[idx] = cache->n;
            items[cache->n] = (RowColSv){key, {0}};
            StringView* result = &items[cache->n].sv;
            cache->n++;
            return result;
        }
        if(items[i].rc.row == row && items[i].rc.col == col){
            return &items[i].sv;
        }
        idx++;
        if(unlikely(idx >= cap)) idx = 0;
    }
}


static inline
SheetHandle _Nullable*_Nullable
get_cached_sheet(SheetCache* cache, const char* name, size_t len){
#if 0 && !defined(__wasm__)
    return NULL;
#endif
    if(!len) return NULL;
    for(size_t i = 0; i < arrlen(cache->items); i++){
        if(!cache->items[i].s.length){
            cache->items[i].s.text = name;
            cache->items[i].s.length = len;
            return &cache->items[i].sheet;
        }
        if(cache->items[i].s.length != len) continue;
        if(sv_equals2(cache->items[i].s, name, len))
            return &cache->items[i].sheet;
    }
    return NULL;
}

typedef struct ColName ColName;
struct ColName {
    StringView name;
    intptr_t idx;
};

static inline
intptr_t*_Nullable
get_cached_col_name(ColCache* cache, const char* name, size_t len){
#if 0 && !defined(__wasm__)
    return NULL;
#endif
    if(cache->n*2 >= cache->cap){
        size_t old_cap = cache->cap;
        size_t new_cap = old_cap?old_cap*2:128;
        #ifdef __wasm__
            unsigned char* new_data = sane_realloc(cache->data, old_cap*(3*sizeof(intptr_t)+sizeof(uint32_t)), new_cap*(3*sizeof(intptr_t)+sizeof(uint32_t)));
        #else
            unsigned char* new_data = realloc(cache->data, new_cap*(3*sizeof(intptr_t)+sizeof(uint32_t)));
        #endif
        if(!new_data) return NULL;
        cache->data = new_data;
        cache->cap = new_cap;
        uint32_t* indexes = (uint32_t*)(new_data + 3*sizeof(intptr_t)*new_cap);
        memset(indexes, 0xff, sizeof(*indexes)*new_cap);
        ColName *items = (ColName*)new_data;
        for(size_t i = 0; i < cache->n; i++){
            StringView k = items[i].name;
            uint32_t hash = hash_align1(k.text, k.length);
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
    uint32_t hash = hash_alignany(key.text, key.length);
    ColName *items = (ColName*)cache->data;
    uint32_t* indexes = (uint32_t*)(cache->data + 3*sizeof(intptr_t)*cap);
    uint32_t idx = fast_reduce32(hash, (uint32_t)cap);
    for(;;){
        uint32_t i = indexes[idx];
        if(i == UINT32_MAX){ // empty slot
            indexes[idx] = cache->n;
            items[cache->n] = (ColName){key, -1};
            intptr_t* result = &items[cache->n].idx;
            cache->n++;
            return result;
        }
        if(sv_equals(items[i].name, key)){
            return &items[i].idx;
        }
        idx++;
        if(unlikely(idx >= cap)) idx = 0;
    }
}



struct SpreadContext {
#ifndef __wasm__
    const SheetOps _ops; // don't call these directly
#endif
    StringCache scache[4];
    ColCache colcache[4];
    SheetCache sheetcache;
    BuffAllocator a;
    Expression null;
    Expression error;
};

static inline
void
free_caches(SpreadContext* ctx){
    for(size_t i = 0; i < arrlen(ctx->scache); i++){
        StringCache* cache = &ctx->scache[i];
        free(cache->data);
    }
    for(size_t i = 0; i < arrlen(ctx->colcache); i++){
        ColCache* cache = &ctx->colcache[i];
        free(cache->data);
    }
}

static inline
void*_Nullable
expr_alloc(SpreadContext* ctx, ExpressionKind kind){
    size_t sz;
    switch(kind){
        case EXPR_ERROR:
            return &ctx->error;
            break;
        case EXPR_NUMBER:          sz = sizeof(Number); break;
        case EXPR_FUNCTION_CALL:   sz = sizeof(FunctionCall); break;
        case EXPR_RANGE0D_FOREIGN: sz = sizeof(ForeignRange0D); break;
        case EXPR_RANGE0D:         sz = sizeof(Range0D); break;
        case EXPR_RANGE1D_COLUMN_FOREIGN:
            sz = sizeof(ForeignRange1DColumn);
            break;
        case EXPR_RANGE1D_COLUMN:  sz = sizeof(Range1DColumn); break;
        case EXPR_GROUP:           sz = sizeof(Group); break;
        case EXPR_BINARY:          sz = sizeof(Binary); break;
        case EXPR_UNARY:           sz = sizeof(Unary); break;
        case EXPR_STRING:          sz = sizeof(String); break;
        case EXPR_NULL:
            return &ctx->null;
            break;
        default: __builtin_trap();
    }
    void* result = buff_alloc(&ctx->a, sz);
    if(!result) return NULL;
    ((Expression*)result)->kind = kind;
    return result;
}

#if 0
#pragma clang assume_nonnull end
#include "debugging.h"
#pragma clang assume_nonnull begin
#define Error(ctx, mess) bt(), fprintf(stderr, "%s:%d:(%s) %s\n", __FILE__, __LINE__, __func__, mess), expr_alloc(ctx, EXPR_ERROR)
// #define Error(ctx, mess) bt(), fprintf(stderr, "%d: %s\n", __LINE__, mess), __builtin_debugtrap(), expr_alloc(ctx, EXPR_ERROR)
#else
#define Error(ctx, mess) expr_alloc(ctx, EXPR_ERROR)
#endif

typedef struct FuncInfo FuncInfo;
struct FuncInfo {
    StringView name;
    FormulaFunc* func;
};


#ifdef __wasm__
#define SP_ARGS SheetHandle sheet
#define SP_CALL(func, ...) sheet_##func(sheet, __VA_ARGS__)
#else
#define SP_ARGS const SpreadContext* ctx, SheetHandle sheet
#define SP_CALL(func, ...) ctx->_ops.func(ctx->_ops.ctx, sheet, __VA_ARGS__)
#endif // use these inline functions instead of using _ops directly

force_inline
const char*_Nullable
sp_cell_text(SpreadContext* ctx, SheetHandle sheet, intptr_t row, intptr_t col, size_t* len){
    StringCache* cache = NULL;
    for(size_t i = 0; i < arrlen(ctx->scache); i++){
        if(ctx->scache[i].handle == sheet){
            cache = &ctx->scache[i];
            break;
        }
    }
    StringView* cached = cache?get_cached_string(cache, row, col):NULL;
    if(cached && cached->text){
        *len = cached->length;
        return cached->text;
    }
    size_t l;
    const char* s;
    #ifdef __wasm__
        PString* p = sheet_cell_text(sheet, row, col);
        l = p->length;
        s = (char*)p->text;
    #else
        s = SP_CALL(cell_txt, row, col, &l);
    #endif
    if(cached){
        cached->length = l;
        if(!s) cached->text = (char*)1;
        else cached->text = s;
    }
    *len = l;
    return s;
}

force_inline
intptr_t
sp_col_height(SP_ARGS, intptr_t col){
    return SP_CALL(col_height, col);
}

force_inline
intptr_t
sp_row_width(SP_ARGS, intptr_t row){
    return SP_CALL(row_width, row);
}

force_inline
int
sp_dims(SP_ARGS, intptr_t* ncols, intptr_t* nrows){
    return SP_CALL(dims, ncols, nrows);
}

force_inline
int
sp_set_display_number(SP_ARGS, intptr_t row, intptr_t col, double value){
    #ifdef __wasm__
        SP_CALL(set_display_number, row, col, value);
        return 0;
    #else
        return SP_CALL(set_display_number, row, col, value);
    #endif
}

force_inline
int
sp_set_display_error(SP_ARGS, intptr_t row, intptr_t col, const char* errmess, size_t errmess_len){
    #ifdef __wasm__
        (void)errmess;
        (void)errmess_len;
        SP_CALL(set_display_error, row, col);
    #else
        return SP_CALL(set_display_error, row, col, errmess, errmess_len);
    #endif
    return 0;
}

force_inline
int
sp_set_display_string(SP_ARGS, intptr_t row, intptr_t col, const char* txt, size_t len){
    #ifdef __wasm__
        SP_CALL(set_display_string, row, col, txt, len);
        return 0;
    #else
        return SP_CALL(set_display_string, row, col, txt, len);
    #endif
}

force_inline
int
sp_next_cell(SP_ARGS, intptr_t nth, intptr_t* row, intptr_t* col){
    return SP_CALL(next_cell, nth, row, col);
}
force_inline
intptr_t
sp_name_to_col_idx(SpreadContext* ctx, SheetHandle sheet, const char* name, size_t len){
    ColCache* cache = NULL;
    for(size_t i = 0; i < arrlen(ctx->colcache); i++){
        if(ctx->colcache[i].handle == sheet){
            cache = &ctx->colcache[i];
            break;
        }
    }
    intptr_t* pidx = cache?get_cached_col_name(cache, name, len):NULL;
    if(pidx && *pidx != -1) return *pidx;
    intptr_t idx = SP_CALL(name_to_col_idx, name, len);
    if(pidx) *pidx = idx;
    return idx;
}

force_inline
SheetHandle _Nullable
sp_name_to_sheet(SpreadContext* ctx, const char* name, size_t len){
    SheetCache* cache = &ctx->sheetcache;
    SheetHandle* h = get_cached_sheet(cache, name, len);
    if(h && *h) return *h;
    SheetHandle hnd;
    #ifdef __wasm__
        hnd = sheet_name_to_sheet(name, len);
    #else
        hnd = ctx->_ops.name_to_sheet(ctx->_ops.ctx, name, len);
    #endif
    if(h) *h = hnd;
    return hnd;
}

#ifdef __wasm__
// alias these so the ctx is unused.
#define sp_query_cell_number(ctx, ...) ((void)ctx, sp_query_cell_number(__VA_ARGS__))
#define sp_col_height(ctx, ...) ((void)ctx, sp_col_height(__VA_ARGS__))
#define sp_row_width(ctx, ...) ((void)ctx, sp_row_width(__VA_ARGS__))
#define sp_dims(ctx, ...) ((void)ctx, sp_dims(__VA_ARGS__))
#define sp_set_display_number(ctx, ...) ((void)ctx, sp_set_display_number(__VA_ARGS__))
#define sp_set_display_error(ctx, ...) ((void)ctx, sp_set_display_error(__VA_ARGS__))
#define sp_set_display_string(ctx, ...) ((void)ctx, sp_set_display_string(__VA_ARGS__))
#define sp_next_cell(ctx, ...) ((void)ctx, sp_next_cell(__VA_ARGS__))
#endif


#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#endif
