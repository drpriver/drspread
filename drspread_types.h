//
// Copyright © 2023, David Priver
//
#ifndef DRSPREAD_TYPES_H
#define DRSPREAD_TYPES_H
#include "drspread.h"
#include "buff_allocator.h"
#include "stringview.h"

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
    const SheetOps ops;
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
#define FORMULAFUNC(name) Expression*_Nullable (name)(SpreadContext* ctx, int argc, Expression*_Nonnull*_Nonnull argv)
typedef FORMULAFUNC(FormulaFunc);
struct FunctionCall {
    Expression e;
    FormulaFunc* func;
    int argc;
    Expression*_Nonnull*_Nonnull argv;
};

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
        case EXPR_NUMBER:
            sz = sizeof(Number);
            break;
        case EXPR_FUNCTION_CALL:
            sz = sizeof(FunctionCall);
            break;
        case EXPR_RANGE0D:
            sz = sizeof(Range0D);
            break;
        case EXPR_RANGE1D_COLUMN:
            sz = sizeof(Range1DColumn);
            break;
        case EXPR_GROUP:
            sz = sizeof(Group);
            break;
        case EXPR_BINARY:
            sz = sizeof(Binary);
            break;
        case EXPR_UNARY:
            sz = sizeof(Unary);
            break;
        case EXPR_STRING:
            sz = sizeof(String);
            break;
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
get_cached_val(const SpreadCache* cache, intptr_t row, intptr_t col){
    if(row < 0 || col < 0 || col >= cache->ncols || row >= cache->nrows) return NULL;
    intptr_t idx = col*cache->nrows + row;
    return &cache->vals[idx];
}



#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#endif
