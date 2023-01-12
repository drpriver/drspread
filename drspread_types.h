//
// Copyright © 2023, David Priver
//
#ifndef DRSPREAD_TYPES_H
#define DRSPREAD_TYPES_H
#include "drspread.h"
#include "buff_allocator.h"
#include "stringview.h"

#ifdef __wasm__
#include "drspread_wasm.h"
#endif

#ifndef force_inline
#define force_inline static inline __attribute__((always_inline))
#endif

#ifdef __clang__
#pragma clang assume_nonnull begin
#else
#ifndef _Nullable
#define _Nullable
#endif
#endif


typedef struct CacheVal CacheVal;

typedef struct SpreadCache SpreadCache;
struct SpreadCache {
    intptr_t ncols, nrows;
    CacheVal* vals;
};

enum ExpressionKind: intptr_t {
    EXPR_ERROR = 0,
    EXPR_NUMBER,
    EXPR_FUNCTION_CALL,
    EXPR_RANGE0D,
    EXPR_RANGE1D_COLUMN,
    EXPR_GROUP,
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_STRING,
    EXPR_NULL,
};
enum : intptr_t{
    CACHE_UNSET=EXPR_NULL+1,
    CACHE_IN_PROGRESS,
};
typedef enum ExpressionKind ExpressionKind;

typedef struct Expression Expression;
struct Expression {
    _Alignas(intptr_t) ExpressionKind kind;
};

typedef struct SpreadContext SpreadContext;
struct SpreadContext {
    const SheetOps _ops; // don't call these directly
    BuffAllocator a;
    SpreadCache cache;
    Expression null;
    Expression error;
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

enum {IDX_DOLLAR=-2147483647-1}; // INT32_MIN

typedef struct Range0D Range0D;
struct Range0D {
    Expression e;
    intptr_t col;
    intptr_t row;
};

typedef struct Range1DColumn Range1DColumn;
struct Range1DColumn {
    Expression e;
    intptr_t col;
    intptr_t row_start, row_end; // inclusive
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

static inline
void*_Nullable
expr_alloc(SpreadContext* ctx, ExpressionKind kind){
    size_t sz;
    switch(kind){
        case EXPR_ERROR:
            return &ctx->error;
            break;
        case EXPR_NUMBER:         sz = sizeof(Number); break;
        case EXPR_FUNCTION_CALL:  sz = sizeof(FunctionCall); break;
        case EXPR_RANGE0D:        sz = sizeof(Range0D); break;
        case EXPR_RANGE1D_COLUMN: sz = sizeof(Range1DColumn); break;
        case EXPR_GROUP:          sz = sizeof(Group); break;
        case EXPR_BINARY:         sz = sizeof(Binary); break;
        case EXPR_UNARY:          sz = sizeof(Unary); break;
        case EXPR_STRING:         sz = sizeof(String); break;
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

struct CacheVal {
    union {
        intptr_t kind;
        Expression e;
        Number n;
        String s;
    };
};
static inline
CacheVal*_Nullable
get_cached_val(const SpreadCache* cache, SheetHandle hnd, intptr_t row, intptr_t col){
    (void)hnd;
    return NULL;
    if(row < 0 || col < 0 || col >= cache->ncols || row >= cache->nrows) return NULL;
    intptr_t idx = col*cache->nrows + row;
    return &cache->vals[idx];
}

#ifdef __wasm__
#define SP_ARGS SheetHandle sheet
#define SP_CALL(func, ...) sheet_##func(sheet, __VA_ARGS__)
#else
#define SP_ARGS const SpreadContext* ctx, SheetHandle sheet
#define SP_CALL(func, ...) ctx->_ops.func(ctx->_ops.ctx, sheet, __VA_ARGS__)
#endif // use these inline functions instead of using _ops directly

force_inline
CellKind
sp_query_cell_kind(SP_ARGS, intptr_t row, intptr_t col){
    #ifndef __wasm__
        if(!ctx->_ops.query_cell_kind) return CELL_UNKNOWN;
        return SP_CALL(query_cell_kind, row, col);
    #else
        return (CellKind)SP_CALL(query_cell_kind, row, col);
    #endif
}

force_inline
double
sp_cell_number(SP_ARGS, intptr_t row, intptr_t col){
    return SP_CALL(cell_number, row, col);
}

force_inline
const char*_Nullable
sp_cell_text(SP_ARGS, intptr_t row, intptr_t col, size_t* len){
    #ifdef __wasm__
        PString* p = sheet_cell_text(sheet, row, col)
        *len = p->length;
        return (char*)p->text;
    #else
        return SP_CALL(cell_txt, row, col, len);
    #endif
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
sp_name_to_col_idx(SP_ARGS, const char* name, size_t len){
    return SP_CALL(name_to_col_idx, name, len);
}

#ifdef __wasm
force_inline
void*_Nullable
sp_name_to_sheet_(const char* name, size_t len){
    return (void*)sheet_name_to_sheet(name, len);
}
#else
force_inline
void*_Nullable
sp_name_to_sheet(const SpreadContext* ctx, const char* name, size_t len){
    return ctx->_ops.name_to_sheet(ctx->_ops.ctx, name, len);
}
#endif

#ifdef __wasm__
// alias these so the ctx is unused.
#define sp_query_cell_kind(ctx, ...) ((void)ctx, sp_query_cell_kind(__VA_ARGS__))
#define sp_query_cell_number(ctx, ...) ((void)ctx, sp_query_cell_number(__VA_ARGS__))
#define sp_cell_number(ctx, ...) ((void)ctx, sp_cell_number(__VA_ARGS__))
#define sp_cell_text(ctx, ...) ((void)ctx, sp_cell_text(__VA_ARGS__))
#define sp_col_height(ctx, ...) ((void)ctx, sp_col_height(__VA_ARGS__))
#define sp_row_width(ctx, ...) ((void)ctx, sp_row_width(__VA_ARGS__))
#define sp_dims(ctx, ...) ((void)ctx, sp_dims(__VA_ARGS__))
#define sp_set_display_number(ctx, ...) ((void)ctx, sp_set_display_number(__VA_ARGS__))
#define sp_set_display_error(ctx, ...) ((void)ctx, sp_set_display_error(__VA_ARGS__))
#define sp_set_display_string(ctx, ...) ((void)ctx, sp_set_display_string(__VA_ARGS__))
#define sp_next_cell(ctx, ...) ((void)ctx, sp_next_cell(__VA_ARGS__))
#define sp_name_to_col_idx(ctx, ...) ((void)ctx, sp_name_to_col_idx(__VA_ARGS__))
#define sp_name_to_sheet(ctx, name, len) ((void)ctx, sp_name_to_sheet_(name, len))
#endif




#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#endif
