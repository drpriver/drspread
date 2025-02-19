//
// Copyright © 2023-2025, David Priver <david@davidpriver.com>
//
#ifndef DRSPREAD_TYPES_H
#define DRSPREAD_TYPES_H
#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include "drspread.h"
#include "drspread_colcache.h"
#include "buff_allocator.h"
#include "stringview.h"
#include "drspread_allocators.h"

#if defined(__IMPORTC__)
__import ldc.intrinsics;
#define FrameAddress() (uintptr_t)llvm_frameaddress(0)
#elif defined(__GNUC__) || defined(__clang__)
#define FrameAddress() (uintptr_t)__builtin_frame_address(0)
#elif defined(_MSC_VER)
#include <intrin.h>
#define FrameAddress() (uintptr_t)_AddressOfReturnAddress()
#endif


#ifdef __wasm__
#include "drspread_wasm.h"
#endif

#if (defined(_MSC_VER) && !defined(__clang__)) || defined(__IMPORTC__)
#define __builtin_memset memset
#define __builtin_memcpy memcpy
#define __builtin_trap abort
#define __builtin_unreachable abort
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
#pragma clang diagnostic ignored "-Wmicrosoft-fixed-enum"
#else
#ifndef _Nullable
#define _Nullable
#endif
#endif

#ifndef TYPED_ENUM
// Does MSVC also support this?
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
#if UINTPTR_MAX == UINT32_MAX
#define DRSPREAD_32BIT
#endif

typedef struct DrspStr DrspStr;
typedef const struct DrspStr* DrspAtom;

typedef struct Value Value;
struct Value{
#ifdef DRSPREAD_32BIT
    ValueKind kind;
    uintptr_t _pad;
    union {
        double number;
        DrspAtom atom;
        struct {
            intptr_t length;
            Value* data;
        };
    };
#else
    ValueKind kind: 3;
    uintptr_t length: 61;
    union {
        DrspAtom atom;
        double number;
        Value* data;
    };
#endif
};
_Static_assert(sizeof(Value)==16, "");


// Destroys the contents, but does not free ctx.
static void drsp_destroy_ctx_(DrSpreadCtx* ctx);

TYPED_ENUM(ExpressionKind, uintptr_t){
    EXPR_ERROR                  =  0,
    EXPR_STRING                 =  1,
    EXPR_BLANK                  =  2,
    EXPR_NUMBER                 =  3,
    EXPR_FUNCTION_CALL          =  4,
    EXPR_RANGE0D                =  5,
    EXPR_RANGE0D_FOREIGN        =  6,
    EXPR_RANGE1D_COLUMN         =  7,
    EXPR_RANGE1D_COLUMN_FOREIGN =  8,
    EXPR_RANGE1D_ROW            =  9,
    EXPR_RANGE1D_ROW_FOREIGN    = 10,
    EXPR_GROUP                  = 11,
    EXPR_BINARY                 = 12,
    EXPR_UNARY                  = 13,
    EXPR_COMPUTED_ARRAY         = 14,
    EXPR_USER_DEFINED_FUNC_CALL = 15,
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
typedef struct SheetData SheetData;
#define FORMULAFUNC(name) Expression*_Nullable (name)(DrSpreadCtx* ctx, SheetData* sd,  intptr_t caller_row, intptr_t caller_col, int argc, Expression*_Nonnull*_Nonnull argv)
typedef FORMULAFUNC(FormulaFunc);
struct FunctionCall {
    Expression e;
    FormulaFunc* func;
    int argc;
    Expression*_Nonnull*_Nonnull argv;
};

typedef struct UserFunctionCall UserFunctionCall;
struct UserFunctionCall {
    Expression e;
    DrspAtom name;
    int argc;
    Expression*_Nonnull*_Nonnull argv;
};

enum {IDX_UNSET             = -2147483647-1}; // INT32_MIN
enum {IDX_DOLLAR            = -2147483647  }; // INT32_MIN+1
enum {IDX_BLANK             = -2147483646  }; // INT32_MIN+2
enum {IDX_EXTRA_DIMENSIONAL = DRSP_IDX_EXTRA_DIMENSIONAL}; // INT32_MIN+3

typedef struct Range0D Range0D;
struct Range0D {
    Expression e;
    DrspAtom col_name;
    intptr_t row;
};

typedef struct ForeignRange0D ForeignRange0D;
struct ForeignRange0D {
    union {
        Expression e;
        Range0D r;
    };
    DrspAtom sheet_name;
};

typedef struct Range1DColumn Range1DColumn;
struct Range1DColumn {
    Expression e;
    DrspAtom col_name;
    intptr_t row_start, row_end; // inclusive
};

typedef struct Range1DRow Range1DRow;
struct Range1DRow {
    Expression e;
    intptr_t row_idx;
    DrspAtom col_start, col_end; // inclusive
};

typedef struct ForeignRange1DColumn ForeignRange1DColumn;
struct ForeignRange1DColumn {
    union {
        Range1DColumn r;
        Expression e;
    };
    DrspAtom sheet_name;
};

typedef struct ForeignRange1DRow ForeignRange1DRow;
struct ForeignRange1DRow {
    union {
        Range1DRow r;
        Expression e;
    };
    DrspAtom sheet_name;
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
    DrspAtom str;
};

typedef struct ComputedArray ComputedArray;
struct ComputedArray {
    Expression e;
    intptr_t length;
    _Alignas(uintptr_t) Expression*_Nonnull data[];
};

// This is a hash table
typedef struct CellCache CellCache;
struct CellCache {
    size_t n;
    size_t cap;
    unsigned char* data;
};


typedef struct RowCol RowCol;
struct RowCol {
    int32_t row, col;
};

typedef struct RowColSv RowColSv;
struct RowColSv {
    RowCol rc;
    DrspAtom sv;
};
static inline
DrspAtom _Nullable
get_cached_cell(CellCache* cache, intptr_t row, intptr_t col);

static inline
int
set_cached_cell(CellCache* cache, intptr_t row, intptr_t col, DrspAtom txt);



enum {LINKED_ARENA_SIZE=16*1024 - sizeof(void*) - sizeof(size_t)};
typedef struct LinkedArena LinkedArena;
struct LinkedArena {
    LinkedArena*_Nullable next;
    size_t used;
    char data[LINKED_ARENA_SIZE];
};

DRSP_INTERNAL
void
free_linked_arenas(LinkedArena*_Nullable arena);

struct DrspStr {
    uint16_t length;
    char data[];
};


DRSP_INTERNAL
DrspAtom _Nullable
drsp_intern_str(DrSpreadCtx*, const char*_Null_unspecified txt, size_t len);

DRSP_INTERNAL
DrspAtom _Nullable
drsp_intern_str_lower(DrSpreadCtx*, const char*_Null_unspecified txt, size_t len);

DRSP_INTERNAL
DrspAtom _Nullable
drsp_atom_lower(DrSpreadCtx*, DrspAtom a);

DRSP_INTERNAL
DrspAtom _Nullable
drsp_intern_sv_lower(DrSpreadCtx* ctx, StringView sv);

force_inline
DrspAtom
drsp_dollar_atom(void);

force_inline
DrspAtom
drsp_nil_atom(void);

typedef struct StringHeap StringHeap;
struct StringHeap {
    LinkedArena*_Nullable arena;
    size_t n, cap;
    unsigned char* data;
};

DRSP_INTERNAL
void
destroy_string_heap(StringHeap* heap);

typedef struct ParseHeap ParseHeap;
struct ParseHeap {
    LinkedArena*_Nullable arena;
    size_t n, cap;
    unsigned char* data;
};

DRSP_INTERNAL
void
destroy_parse_heap(ParseHeap* heap);

DRSP_INTERNAL
Expression*_Nullable
has_cached_parse(DrSpreadCtx*, DrspAtom a);

DRSP_INTERNAL
Expression*_Null_unspecified*_Nullable
getsert_cached_parse(DrSpreadCtx*, DrspAtom a);

DRSP_INTERNAL
void
cache_parse(DrSpreadCtx* ctx, DrspAtom a, Expression*_Nonnull e);

typedef struct OutputResultCache OutputResultCache;
struct OutputResultCache {
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
        DrspAtom string;
    };
};
static inline
_Bool
cached_result_eq_ignoring_loc(const CachedResult* a, const CachedResult* b);
static inline
int
expr_to_cached_result(DrSpreadCtx* ctx, Expression* e, CachedResult* out);
static inline
int
expr_to_cached_result_no_array(DrSpreadCtx* ctx, Expression* e, CachedResult* out);

static inline
Expression*_Nullable
cached_result_to_expr(DrSpreadCtx* ctx, const CachedResult* cr);

force_inline
void*_Nullable
expr_alloc(DrSpreadCtx* ctx, ExpressionKind kind);

force_inline
void*_Nullable
parser_expr_alloc(DrSpreadCtx* ctx, ExpressionKind kind);

force_inline
ComputedArray*_Nullable
computed_array_alloc(DrSpreadCtx* ctx, size_t nitems);
DRSP_INTERNAL
CachedResult*_Nullable
get_cached_output_result(OutputResultCache* cache, intptr_t row, intptr_t col);

DRSP_INTERNAL
CachedResult*_Nullable
has_cached_output_result(const OutputResultCache* cache, intptr_t row, intptr_t col);

DRSP_INTERNAL
void
clear_cached_output_result(OutputResultCache* cache);


typedef struct UserDefinedFunctionParameter UserDefinedFunctionParameter;
struct UserDefinedFunctionParameter {
    intptr_t row, col;
};


typedef struct ExtraDimensionalCell ExtraDimensionalCell;
struct ExtraDimensionalCell {
    intptr_t id;
};

typedef struct ExtraDimensionalCellCache ExtraDimensionalCellCache;
struct ExtraDimensionalCellCache {
    unsigned count;
    ExtraDimensionalCell cells[8];
};
typedef struct NamedCell NamedCell;
struct NamedCell {
    DrspAtom name;
    intptr_t row, col;
};

typedef struct NamedCells NamedCells;
struct NamedCells {
    NamedCell* data;
    size_t count, capacity;
};

DRSP_INTERNAL
const NamedCell*_Nullable
get_named_cell(const NamedCells* cells, DrspAtom name);

DRSP_INTERNAL
int
set_named_cell(NamedCells* cells, DrspAtom name, intptr_t row, intptr_t col);

DRSP_INTERNAL
void
clear_named_cell(NamedCells* cells, DrspAtom name);

DRSP_INTERNAL
void
cleanup_named_cells(NamedCells* cells);

typedef struct UniqueSheets UniqueSheets;
// Dynamic array, but
struct UniqueSheets {
    SheetHandle _Null_unspecified*_Null_unspecified data;
    size_t count, capacity;
};

DRSP_INTERNAL
void
unique_cleanup(UniqueSheets* u);

DRSP_INTERNAL
int
unique_add(UniqueSheets* u, SheetHandle h);

typedef struct SheetData SheetData;
struct SheetData {
    DrspAtom name;
    DrspAtom _Nullable alias;
    SheetHandle handle;
    CellCache cell_cache;
    ColCache col_cache;
    ExtraDimensionalCellCache extra_dimensional;
    intptr_t width, height;
    OutputResultCache output_result_cache;
    NamedCells named_cells;
    OutputResultCache result_cache;
    unsigned flags;
    int paramc;
    UserDefinedFunctionParameter params[4];
    intptr_t out_row, out_col;
    // HACK
    struct HackyFuncArg {
        intptr_t row;
        intptr_t col;
        Expression* e;
    }hacky_func_args[4];
    UniqueSheets dependants;
    _Bool dirty : 1;
};

DRSP_INTERNAL
void
cleanup_sheet_data(SheetData*);

DRSP_INTERNAL
int
sheet_add_dependant(DrSpreadCtx* ctx, SheetData* sd, SheetHandle h);

DRSP_INTERNAL
void
sheet_mark_dirty(DrSpreadCtx* ctx, SheetData* h);

// Note: this is just a dynamic array, it is not a hash table.
// It could be turned into a hash table though, idk.
typedef struct SheetMap SheetMap;
struct SheetMap {
    size_t cap;
    size_t n;
    SheetData* data;
};
typedef struct ErrorExpression ErrorExpression;
struct ErrorExpression {
    Expression e;
    DrspAtom message;
};

struct DrSpreadCtx {
#ifndef DRSPREAD_DIRECT_OPS
    const SheetOps _ops; // don't call these directly
#endif
    LinkedArena* temp_string_arena;
    StringHeap sheap;
    ParseHeap pheap;
    SheetMap map;
    BuffAllocator* a;
    BuffAllocator _a;
    Expression null;
    ErrorExpression error;
#ifndef __wasm__
    uintptr_t limit; // this sucks but is to avoid stack overflow;
#endif
    // _Alignas(double) char buff[];
};

DRSP_INTERNAL
SheetData*_Nullable
sheet_lookup_by_handle(const DrSpreadCtx* ctx, SheetHandle handle);

DRSP_INTERNAL
SheetData*_Nullable
sheet_get_or_create_by_handle(DrSpreadCtx* ctx, SheetHandle handle);

DRSP_INTERNAL
SheetData*_Nullable
sheet_lookup_by_name(DrSpreadCtx* ctx, DrspAtom name);

DRSP_INTERNAL
SheetData*_Nullable
udf_lookup_by_handle(const DrSpreadCtx* ctx, SheetHandle handle);

DRSP_INTERNAL
SheetData*_Nullable
udf_lookup_by_name(DrSpreadCtx* ctx, DrspAtom name);

DRSP_INTERNAL
void
free_linked_arenas(LinkedArena*_Nullable arena);

DRSP_INTERNAL
void
free_sheet_datas(DrSpreadCtx* ctx);

static inline
void*_Nullable
linked_arena_alloc(LinkedArena*_Nullable*_Nonnull parena, size_t len);


// GCOV_EXCL_START
force_inline
void*_Nullable
expr_alloc(DrSpreadCtx* ctx, ExpressionKind kind){
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
        case EXPR_BLANK:
            return &ctx->null;
            break;
        case EXPR_COMPUTED_ARRAY:
            __builtin_trap();
        case EXPR_USER_DEFINED_FUNC_CALL:      sz = sizeof(UserFunctionCall); break;
        default: __builtin_trap();
    }
    void* result = buff_alloc(ctx->a, sz);
    if(!result) return NULL;
    ((Expression*)result)->kind = kind;
    return result;
}


force_inline
void*_Nullable
parser_expr_alloc(DrSpreadCtx* ctx, ExpressionKind kind){
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
        case EXPR_BLANK:
            return &ctx->null;
            break;
        case EXPR_COMPUTED_ARRAY:
            __builtin_trap();
        case EXPR_USER_DEFINED_FUNC_CALL:      sz = sizeof(UserFunctionCall); break;
        default: __builtin_trap();
    }
    void* result = linked_arena_alloc(&ctx->pheap.arena, sz);
    if(!result) return NULL;
    ((Expression*)result)->kind = kind;
    return result;
}

force_inline
ComputedArray*_Nullable
computed_array_alloc(DrSpreadCtx* ctx, size_t nitems){
    ComputedArray* cc = buff_alloc(ctx->a, offsetof(ComputedArray, data)+sizeof(Expression*)*nitems);
    if(!cc) return NULL;
    cc->e.kind = EXPR_COMPUTED_ARRAY;
    cc->length = nitems;
    return cc;
}
// GCOV_EXCL_STOP


static inline
int
sv_cat(DrSpreadCtx* ctx, size_t n, const DrspAtom _Nonnull* const _Nonnull strs, DrspAtom _Nullable*_Nonnull out){
    size_t len = 0;
    for(size_t i = 0; i < n; i++)
        len += strs[i]->length;
    if(!len) {
        *out = drsp_nil_atom();
        return 0;
    }
    char* data = linked_arena_alloc(&ctx->temp_string_arena, len);
    if(!data) return 1;
    char* p = data;
    for(size_t i = 0; i < n; i++){
        __builtin_memcpy(p, strs[i]->data, strs[i]->length);
        p += strs[i]->length;
    }
    // FIXME: free data here
    DrspAtom s = drsp_intern_str(ctx, data, len);
    if(!s) return 1;
    *out = s;
    return 0;
}

// GCOV_EXCL_START
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
        case EXPR_BLANK:                  sz = sizeof(Expression); break;
        case EXPR_COMPUTED_ARRAY: __builtin_trap();
        case EXPR_USER_DEFINED_FUNC_CALL: sz = sizeof(UserFunctionCall); break;
        default: __builtin_trap();
    }
    return sz;
}
// GCOV_EXCL_STOP

static
void*_Nullable
expr_clone(DrSpreadCtx* ctx, Expression* e){
    if(e->kind == EXPR_COMPUTED_ARRAY){
        abort();
    }
    else {
        void* result = expr_alloc(ctx, e->kind);
        if(!result) return NULL;
        __builtin_memcpy(result, e, expr_size(e->kind));
        switch(e->kind){
            case EXPR_GROUP:{
                Group* g = result;
                g->expr = expr_clone(ctx, g->expr);
                if(!g->expr) return NULL;
            }break;
            case EXPR_UNARY:{
                Unary* u = result;
                u->expr = expr_clone(ctx, u->expr);
                if(!u->expr) return NULL;
            }break;
            case EXPR_BINARY:{
                Binary* b = result;
                b->lhs = expr_clone(ctx, b->lhs);
                if(!b->lhs) return NULL;
                b->rhs = expr_clone(ctx, b->rhs);
                if(!b->rhs) return NULL;
            }break;
            case EXPR_FUNCTION_CALL:{
                FunctionCall* fc = result;
                void* p = buff_alloc(ctx->a, fc->argc * sizeof *fc->argv);
                if(!p) return NULL;
                __builtin_memcpy(p, fc->argv, fc->argc * sizeof *fc->argv);
                fc->argv = p;
                for(int i = 0; i < fc->argc; i++){
                    fc->argv[i] = expr_clone(ctx, fc->argv[i]);
                    if(!fc->argv[i]) return NULL;
                }
            }break;
            case EXPR_USER_DEFINED_FUNC_CALL:{
                UserFunctionCall* fc = result;
                void* p = buff_alloc(ctx->a, fc->argc * sizeof *fc->argv);
                if(!p) return NULL;
                __builtin_memcpy(p, fc->argv, fc->argc * sizeof *fc->argv);
                fc->argv = p;
                for(int i = 0; i < fc->argc; i++){
                    fc->argv[i] = expr_clone(ctx, fc->argv[i]);
                    if(!fc->argv[i]) return NULL;
                }
            }break;
            default:
                break;

        }
        return result;
    }
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
    UserFunctionCall ufc;
};

static inline
Expression*
Error_(DrSpreadCtx* ctx, const char* mess, size_t len);

#define Error(ctx, mess) Error_(ctx, "" mess, -1 + sizeof mess)

typedef struct FuncInfo FuncInfo;
struct FuncInfo {
    StringView name;
    FormulaFunc* func;
};

DRSP_INTERNAL
DrspAtom
sp_cell_atom(SheetData* sd, intptr_t row, intptr_t col);

// We pretend that we support ragged sheets, even though we don't.
force_inline
intptr_t
sp_col_height(const SheetData* sd, intptr_t col){
    (void)col;
    return sd->height;
}

force_inline
intptr_t
sp_row_width(const SheetData* sd, intptr_t row){
    (void)row;
    return sd->width;
}

force_inline
intptr_t
sp_name_to_col_idx(SheetData* sd, DrspAtom name){
    intptr_t* pidx = get_cached_col_name(&sd->col_cache, name);
    if(pidx) return *pidx;
    const char* txt = name->data;
    size_t len = name->length;
    if(len < 3){
        intptr_t x = 0;
        for(size_t i = 0; i < len; i++){
            x *= 26;
            uint8_t c = txt[i];
            c |= 0x20u;
            if(c < 'a') goto notfound;
            if(c > 'z') goto notfound;
            if(c >= 'a' && c <= 'z'){
                x += c - 'a' + 1;
            }
        }
        return x - 1;
    }
    notfound:;
    return -1;
}

#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#endif
