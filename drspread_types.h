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

#ifdef __wasm__
#include "drspread_wasm.h"
#endif

#ifndef force_inline
#if defined(__GNUC__) || defined(__clang__)
#define force_inline static inline __attribute__((always_inline))
#else
#define force_inline static inline
#endif
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
#define DRSP_INTERNAL static
#endif

#ifdef __clang__
#pragma clang assume_nonnull begin
#else
#ifndef _Nullable
#define _Nullable
#endif
#endif

#ifndef TYPED_ENUM
#if defined(__clang__) || __STDC_VERSION__ >= 202300L
#define TYPED_ENUM(name, type) enum name: type; typedef enum name name; enum name : type
#else
#define TYPED_ENUM(name, type) typedef type name; enum name
#endif
#endif

TYPED_ENUM(ValueKind, uintptr_t){
    VALUE_NUMBER,
    VALUE_STRING,
    VALUE_1D_ARRAY,
    VALUE_NULL,
    VALUE_ERROR,
};

typedef struct Value Value;
struct Value{
#ifdef __wasm__
    ValueKind kind;
    unsigned _pad;
    union {
        double number;
        StringView _string;
        struct {
            intptr_t length;
            Value* data;
        };
    };
#else
    ValueKind kind: 3;
    uintptr_t length: 61;
    union {
        const char* _s;
        double number;
        Value* data;
    };
#endif
};
_Static_assert(sizeof(Value)==16, "");

static inline
StringView
sv_of(const Value* v){
#ifdef __wasm__
    return v->_string;
#else
    return (StringView){v->length, v->_s};
#endif
}

static inline
void
sv_set(Value* v, StringView sv){
#ifdef __wasm__
    v->_string = sv;
#else
    v->_s = sv.text;
    v->length = sv.length;
#endif
}



typedef DrSpreadCtx SpreadContext;

TYPED_ENUM(ExpressionKind, uintptr_t){
    EXPR_ERROR = 0,
    EXPR_STRING,
    EXPR_NULL,
    EXPR_NUMBER,
    EXPR_FUNCTION_CALL,
    EXPR_RANGE0D,
    EXPR_RANGE0D_FOREIGN,
    EXPR_RANGE1D_COLUMN,
    EXPR_RANGE1D_COLUMN_FOREIGN,
    EXPR_RANGE1D_ROW,
    EXPR_RANGE1D_ROW_FOREIGN,
    EXPR_GROUP,
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_COMPUTED_ARRAY,
    // EXPR_TYPED_COLUMN,
};

typedef struct Expression Expression;
struct Expression {
    _Alignas(uintptr_t) ExpressionKind kind;
};



TYPED_ENUM(BinaryKind, uintptr_t){
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

TYPED_ENUM(UnaryKind, uintptr_t){
    UN_PLUS,
    UN_NEG,
    UN_NOT,
};

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

enum {IDX_DOLLAR = -2147483647}; // INT32_MIN+1
enum {IDX_UNSET  = -2147483647-1}; // INT32_MIN
enum {IDX_BLANK  = -2147483646}; // INT32_MIN+2


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

typedef struct Range1DRow Range1DRow;
struct Range1DRow {
    Expression e;
    intptr_t row_idx;
    StringView col_start, col_end; // inclusive
};

typedef struct ForeignRange1DColumn ForeignRange1DColumn;
struct ForeignRange1DColumn {
    union {
        Range1DColumn r;
        Expression e;
    };
    StringView sheet_name;
};

typedef struct ForeignRange1DRow ForeignRange1DRow;
struct ForeignRange1DRow {
    union {
        Range1DRow r;
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

typedef struct ComputedArray ComputedArray;
struct ComputedArray {
    Expression e;
    intptr_t length;
    _Alignas(uintptr_t) Expression*_Nonnull data[];
};

typedef struct SheetCache SheetCache;
struct SheetCache {
    struct {
        StringView s;
        SheetHandle sheet;
    } items[8];
};

// This is a hash table
typedef struct StringCache StringCache;
struct StringCache {
    SheetHandle handle;
    size_t n;
    size_t cap;
    unsigned char* data;
};

// This is a hash table
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
static inline
StringView*_Nullable
get_cached_string(StringCache* cache, intptr_t row, intptr_t col);

static inline
int
set_cached_string(StringCache* cache, intptr_t row, intptr_t col, const char*restrict txt, size_t len);


static inline
SheetHandle _Nullable*_Nullable
get_cached_sheet(SheetCache* cache, const char* name, size_t len);

typedef struct ColName ColName;
struct ColName {
    StringView name;
    intptr_t idx;
};

static inline
intptr_t*_Nullable
get_cached_col_name(ColCache* cache, const char* name, size_t len);

static inline
int
set_cached_col_name(ColCache* cache, const char* name, size_t len, intptr_t value);

enum {STRING_ARENA_SIZE=16*1024 - sizeof(void*) - sizeof(size_t)};
typedef struct StringArena StringArena;
struct StringArena {
    StringArena*_Nullable next;
    size_t used;
    char data[STRING_ARENA_SIZE];
};
static inline
void
free_string_arenas(StringArena*_Nullable arena);

struct DrspStr {
    uint16_t length;
    char data[];
};

DRSP_INTERNAL
DrspStr*_Nullable
drsp_create_str(DrSpreadCtx*, const char* txt, size_t len);

static inline
StringView
drsp_to_sv(const DrspStr* s){
    return (StringView){s->length, s->data};
}

typedef struct StringHeap StringHeap;
struct StringHeap {
    StringArena*_Nullable arena;
    size_t n, cap;
    unsigned char* data;
};

static inline
void
destroy_string_heap(StringHeap* heap){
    free_string_arenas(heap->arena);
    free(heap->data);
    __builtin_memset(heap, 0, sizeof *heap);
}

typedef struct ResultCache ResultCache;
struct ResultCache {
    size_t n, cap;
    unsigned char* data;
};
enum CachedResultKind {
    CACHED_RESULT_NULL,
    CACHED_RESULT_STRING,
    CACHED_RESULT_NUMBER,
    CACHED_RESULT_ERROR,
};
typedef struct CachedResult CachedResult;
struct CachedResult{
    RowCol loc;
    enum CachedResultKind kind;
    union {
        double number;
        DrspStr* string;
    };
};
static inline
_Bool
cached_result_eq_ignoring_loc(const CachedResult* a, const CachedResult* b){
    if(a->kind != b->kind) return 0;
    switch(a->kind){
        case CACHED_RESULT_NULL:
            return 1;
        case CACHED_RESULT_STRING:
            return sv_equals(drsp_to_sv(a->string), drsp_to_sv(b->string));
        case CACHED_RESULT_NUMBER:
            // eq or both nan
            return a->number == b->number || (a->number != a->number && b->number != b->number);
        case CACHED_RESULT_ERROR:
            return 1;
        default:
            return 0;
    }
}
static inline
int
expr_to_cached_result(DrSpreadCtx* ctx, Expression* e, CachedResult* out){
    switch(e->kind){
        case EXPR_NUMBER:
            out->kind = CACHED_RESULT_NUMBER;
            out->number = ((Number*)e)->value;
            return 0;
        case EXPR_STRING:{
            String* s = (String*)e;
            if(!s->sv.length){
                out->kind = CACHED_RESULT_NULL;
                return 0;
            }
            DrspStr* str = drsp_create_str(ctx, s->sv.text, s->sv.length);
            if(!str) return 1;
            out->kind = CACHED_RESULT_STRING;
            out->string = str;
            return 0;
        }break;
        case EXPR_NULL:
            out->kind = CACHED_RESULT_NULL;
            return 0;
        default:
        case EXPR_ERROR:
            out->kind = CACHED_RESULT_ERROR;
            return 0;
    }
}

force_inline
void*_Nullable
expr_alloc(SpreadContext* ctx, ExpressionKind kind);

force_inline
ComputedArray*_Nullable
computed_array_alloc(SpreadContext* ctx, size_t nitems);

static inline
Expression*_Nullable
cached_result_to_expr(DrSpreadCtx* ctx, const CachedResult* cr){
    switch(cr->kind){
        case CACHED_RESULT_NULL:
            return expr_alloc(ctx, EXPR_NULL);
        case CACHED_RESULT_NUMBER:{
            Number* n = expr_alloc(ctx, EXPR_NUMBER);
            if(!n) return NULL;
            n->value = cr->number;
            return &n->e;
        }
        case CACHED_RESULT_STRING:{
            String* s = expr_alloc(ctx, EXPR_STRING);
            if(!s) return NULL;
            s->sv = drsp_to_sv(cr->string);
            return &s->e;
        }
        default:
        case CACHED_RESULT_ERROR:
            return expr_alloc(ctx, EXPR_ERROR);
    }
}
DRSP_INTERNAL
CachedResult*_Nullable
get_cached_result(ResultCache* cache, intptr_t row, intptr_t col);


typedef struct SheetData SheetData;
struct SheetData {
    DrspStr* name;
    DrspStr*_Nullable alias;
    SheetHandle handle;
    StringCache str_cache;
    ColCache col_cache;
    intptr_t width, height;
    ResultCache result_cache;
};



typedef struct SheetMap SheetMap;
struct SheetMap {
    size_t cap;
    size_t n;
    SheetData* data;
};

struct DrSpreadCtx {
#ifndef DRSPREAD_DIRECT_OPS
    const SheetOps _ops; // don't call these directly
#endif
    StringArena* temp_string_arena;
    StringHeap sheap;
    SheetMap map;
    BuffAllocator* a;
    BuffAllocator _a;
    Expression null;
    Expression error;
#ifndef __wasm__
    uintptr_t limit; // this sucks but is to avoid stack overflow;
#endif
    _Alignas(double) char buff[];
};

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

static inline
void
free_string_arenas(StringArena*_Nullable arena){
    while(arena){
        StringArena* to_free = arena;
        arena = arena->next;
        free(to_free);
    }
}

static inline
void
free_sheet_datas(SpreadContext* ctx){
    for(size_t i = 0; i < ctx->map.n; i++){
        SheetData* d = &ctx->map.data[i];
        free(d->str_cache.data);
        free(d->col_cache.data);
        free(d->result_cache.data);
    }
    free(ctx->map.data);
}

// static inline
force_inline
void*_Nullable
expr_alloc(SpreadContext* ctx, ExpressionKind kind){
    size_t sz;
    switch(kind){
        case EXPR_ERROR:
            return &ctx->error;
            break;
        case EXPR_NUMBER:                 sz = sizeof(Number); break;
        case EXPR_FUNCTION_CALL:          sz = sizeof(FunctionCall); break;
        case EXPR_RANGE0D_FOREIGN:        sz = sizeof(ForeignRange0D); break;
        case EXPR_RANGE0D:                sz = sizeof(Range0D); break;
        case EXPR_RANGE1D_COLUMN_FOREIGN: sz = sizeof(ForeignRange1DColumn); break;
        case EXPR_RANGE1D_COLUMN:         sz = sizeof(Range1DColumn); break;
        case EXPR_RANGE1D_ROW:            sz = sizeof(Range1DRow); break;
        case EXPR_RANGE1D_ROW_FOREIGN:    sz = sizeof(ForeignRange1DRow); break;
        case EXPR_GROUP:                  sz = sizeof(Group); break;
        case EXPR_BINARY:                 sz = sizeof(Binary); break;
        case EXPR_UNARY:                  sz = sizeof(Unary); break;
        case EXPR_STRING:                 sz = sizeof(String); break;
        case EXPR_NULL:
            return &ctx->null;
            break;
        case EXPR_COMPUTED_ARRAY:
            __builtin_trap();
        default: __builtin_trap();
    }
    void* result = buff_alloc(ctx->a, sz);
    if(!result) return NULL;
    ((Expression*)result)->kind = kind;
    return result;
}

force_inline
ComputedArray*_Nullable
computed_array_alloc(SpreadContext* ctx, size_t nitems){
    ComputedArray* cc = buff_alloc(ctx->a, __builtin_offsetof(ComputedArray, data)+sizeof(Expression*)*nitems);
    if(!cc) return NULL;
    cc->e.kind = EXPR_COMPUTED_ARRAY;
    cc->length = nitems;
    return cc;
}

static inline
void*_Nullable
str_arena_alloc(StringArena*_Nullable*_Nonnull parena, size_t len){
    // return buff_alloc(&ctx->a, len*8);
    // return malloc(len);
    if(len & 1) len++;
    if(len > STRING_ARENA_SIZE) return NULL;
    StringArena* arena = *parena;
    if(!arena || arena->used+len > STRING_ARENA_SIZE){
        while(arena){
            if(arena->used+len <= STRING_ARENA_SIZE){
                goto alloced;
            }
            arena = arena->next;
        }
        // fprintf(stderr, "%zu\n", sizeof *arena);
        arena = malloc(sizeof *arena);
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


static inline
int
sv_cat(SpreadContext* ctx, size_t n, const StringView* strs, StringView* out){
    size_t len = 0;
    for(size_t i = 0; i < n; i++)
        len += strs[i].length;
    if(!len) {
        out->length = 0;
        out->text = "";
        return 0;
    }
    // char* data = malloc(len);
    char* data = str_arena_alloc(&ctx->temp_string_arena, len);
    if(!data) return 1;
    char* p = data;
    for(size_t i = 0; i < n; i++){
        __builtin_memcpy(p, strs[i].text, strs[i].length);
        p += strs[i].length;
    }
    out->text = data;
    out->length = len;
    return 0;
}

static inline
size_t
expr_size(ExpressionKind kind){
    size_t sz;
    switch(kind){
        case EXPR_ERROR:                  sz = sizeof(Expression); break;
        case EXPR_NUMBER:                 sz = sizeof(Number); break;
        case EXPR_FUNCTION_CALL:          sz = sizeof(FunctionCall); break;
        case EXPR_RANGE0D_FOREIGN:        sz = sizeof(ForeignRange0D); break;
        case EXPR_RANGE0D:                sz = sizeof(Range0D); break;
        case EXPR_RANGE1D_COLUMN_FOREIGN: sz = sizeof(ForeignRange1DColumn); break;
        case EXPR_RANGE1D_COLUMN:         sz = sizeof(Range1DColumn); break;
        case EXPR_RANGE1D_ROW:            sz = sizeof(Range1DRow); break;
        case EXPR_RANGE1D_ROW_FOREIGN:    sz = sizeof(ForeignRange1DRow); break;
        case EXPR_GROUP:                  sz = sizeof(Group); break;
        case EXPR_BINARY:                 sz = sizeof(Binary); break;
        case EXPR_UNARY:                  sz = sizeof(Unary); break;
        case EXPR_STRING:                 sz = sizeof(String); break;
        case EXPR_NULL:                   sz = sizeof(Expression); break;
        case EXPR_COMPUTED_ARRAY: __builtin_trap();
        default: __builtin_trap();
    }
    return sz;
}

union ExprU{
    Expression e;
    Number n;
    FunctionCall fc;
    ForeignRange0D fr0;
    Range0D r0;
    ForeignRange1DColumn fr1c;
    Range1DColumn r1c;
    ForeignRange1DRow fr1r;
    Range1DRow r1r;
    Group g;
    Binary b;
    Unary u;
    String s;
};

#if 0
#ifdef __wasm__
extern
void
logline(const char*, int);
#else
static inline
void
logline(const char* f, int l){
    fprintf(stderr, "%s: %d\n", f, l);
}
#endif

#pragma clang assume_nonnull end
#include "debugging.h"
#pragma clang assume_nonnull begin
// #define Error(ctx, mess) (bt(), logline(__func__, __LINE__), fprintf(stderr, "%s:%d:(%s) %s\n", __FILE__, __LINE__, __func__, mess), expr_alloc(ctx, EXPR_ERROR))
#define Error(ctx, mess) (bt(), fprintf(stderr, "%d: %s\n", __LINE__, mess), __builtin_debugtrap(), expr_alloc(ctx, EXPR_ERROR))
#else
#define Error(ctx, mess) expr_alloc(ctx, EXPR_ERROR)
#endif

typedef struct FuncInfo FuncInfo;
struct FuncInfo {
    StringView name;
    FormulaFunc* func;
};


force_inline
const char*_Nullable
sp_cell_text(SpreadContext* ctx, SheetHandle sheet, intptr_t row, intptr_t col, size_t* len){
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

force_inline
intptr_t
sp_col_height(const SpreadContext* ctx, SheetHandle sheet, intptr_t col){
    (void)col;
    SheetData* sd = sheet_lookup_by_handle(ctx, sheet);
    if(!sd) return 0;
    return sd->height;
}

force_inline
intptr_t
sp_row_width(const SpreadContext* ctx, SheetHandle sheet, intptr_t row){
    (void)row;
    SheetData* sd = sheet_lookup_by_handle(ctx, sheet);
    if(!sd) return 0;
    return sd->width;
}

#ifdef DRSPREAD_DIRECT_OPS
#define SP_ARGS SheetHandle sheet
#define SP_CALL(func, ...) sheet_##func(sheet, __VA_ARGS__)
#else
#define SP_ARGS const SpreadContext* ctx, SheetHandle sheet
#define SP_CALL(func, ...) ctx->_ops.func(ctx->_ops.ctx, sheet, __VA_ARGS__)
#endif // use these inline functions instead of using _ops directly


force_inline
int
sp_set_display_number(SP_ARGS, intptr_t row, intptr_t col, double value){
    #ifdef DRSPREAD_DIRECT_OPS
        SP_CALL(set_display_number, row, col, value);
        return 0;
    #else
        return SP_CALL(set_display_number, row, col, value);
    #endif
}

force_inline
int
sp_set_display_error(SP_ARGS, intptr_t row, intptr_t col, const char* errmess, size_t errmess_len){
    #ifdef DRSPREAD_DIRECT_OPS
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
    #ifdef DRSPREAD_DIRECT_OPS
        SP_CALL(set_display_string, row, col, txt, len);
        return 0;
    #else
        return SP_CALL(set_display_string, row, col, txt, len);
    #endif
}

force_inline
intptr_t
sp_name_to_col_idx(SpreadContext* ctx, SheetHandle sheet, const char* name, size_t len){
    if(len < 3){
        intptr_t x = 0;
        for(size_t i = 0; i < len; i++){
            x *= 26;
            uint8_t c = name[i];
            c |= 0x20u;
            if(c < 'a') goto lookup;
            if(c > 'z') goto lookup;
            if(c >= 'a' && c <= 'z'){
                x += c - 'a' + 1;
            }
        }
        return x - 1;
    }
    lookup:;
    SheetData* sd = sheet_lookup_by_handle(ctx, sheet);
    intptr_t* pidx = get_cached_col_name(&sd->col_cache, name, len);
    if(pidx) return *pidx;
    return -1;
}

force_inline
SheetHandle _Nullable
sp_name_to_sheet(SpreadContext* ctx, const char* name, size_t len){
    SheetData* sd = sheet_lookup_by_name(ctx, name, len);
    if(!sd) return NULL;
    return sd->handle;
}

#ifdef DRSPREAD_DIRECT_OPS
// alias these so the ctx is unused.
#define sp_set_display_number(ctx, ...) ((void)ctx, sp_set_display_number(__VA_ARGS__))
#define sp_set_display_error(ctx, ...) ((void)ctx, sp_set_display_error(__VA_ARGS__))
#define sp_set_display_string(ctx, ...) ((void)ctx, sp_set_display_string(__VA_ARGS__))
#endif

#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#endif
