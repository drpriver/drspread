//
// Copyright © 2023, David Priver
//
#ifndef DRSPREAD_C
#define DRSPREAD_C

#include "drspread_evaluate.h"
#include "drspread_parse.h"
#include "stringview.h"
#include "drspread_types.h"
#include "drspread.h"


#ifdef __clang__
#pragma clang assume_nonnull begin
#endif

#ifndef arrlen
#define arrlen(x) (sizeof(x)/sizeof(x[0]))
#endif

// null and 0 are false
// everything else is true

// functions:
//    sum(array)
//    avg(array)
//    min(array)
//    max(array)
//    mod(score) (3.5 style ability mode)
//    abs(x)
//    floor(x)
//    ceil(x)
//    trunc(x)
//    round(x)
//    tlu(needle, haystack, values, ?default) - table look up (
//    find(needle, haystack)
//    num(num, ?default) - coerce to a number, use default if impossible
//    try(fallible, default) - if first arg errors, use default, otherwise use
//                             first arg
//    pow(base, power)
//    cell(?sheet, col, row)
//    count(array)

// unary:
//   -
//   +
//   !

// binary:
//   +
//   -
//   /
//   *
//   >
//   >=
//   <
//   <=
//   =, ==
//   !=

// Terminals:
//   function call
//   number
//   range
//   'string', "string"
//   (group)

// This sucks, but not many languages have any way of abstracting over arity at all.
#ifdef __wasm__
#define ARGS SheetHandle sheethandle
#else
#define ARGS SheetHandle sheethandle, const SheetOps* ops
#endif

DRSP_EXPORT
int
drsp_evaluate_formulas(ARGS, SheetHandle _Null_unspecified*_Nullable sheetdeps, size_t sheetdepslen){
#undef ARGS
    intptr_t row=-1, col=-1;
    int nerrs = 0;
    _Alignas(intptr_t) char evalbuff [30000];
    SpreadContext ctx = {
        #ifndef __wasm__
            ._ops=*ops,
        #endif
        .a={evalbuff, evalbuff, evalbuff+sizeof evalbuff},
        .null={EXPR_NULL},
        .error={EXPR_ERROR},
        #ifdef __wasm__
        .limit = (uintptr_t)__builtin_frame_address(0) - 30000,
        #else
        .limit = (uintptr_t)__builtin_frame_address(0) - 300000,
        #endif
    };
    BuffCheckpoint bc = buff_checkpoint(&ctx.a);
    for(intptr_t i = 0; sp_next_cell(&ctx, sheethandle, i, &row, &col) == 0; i++){
        buff_set(&ctx.a, bc);
        Expression* e = evaluate(&ctx, sheethandle, row, col);
        // benchmarking
        #ifdef BENCHMARKING
            for(int i = 0; i < 100000; i++){
                buff_set(&ctx.a, bc);
                e = evaluate(&ctx, sheethandle, row, col);
            }
        #endif
        if(e){
            switch(e->kind){
                case EXPR_NUMBER:
                    sp_set_display_number(&ctx, sheethandle, row, col, ((Number*)e)->value);
                    continue;
                case EXPR_STRING:
                    sp_set_display_string(&ctx, sheethandle, row, col, ((String*)e)->sv.text, ((String*)e)->sv.length);
                    continue;
                case EXPR_NULL:
                    sp_set_display_string(&ctx, sheethandle, row, col, "", 0);
                    continue;
                default: break;
            }
        }
        nerrs++;
        sp_set_display_error(&ctx, sheethandle, row, col, "error", 5);
    }
    if(sheetdeps)
        for(size_t i = 0; i < arrlen(ctx.sheetcache.items) && i < sheetdepslen; i++){
            if(!ctx.sheetcache.items[i].s.length)
                break;
            sheetdeps[i] = ctx.sheetcache.items[i].sheet;
        }
    free_string_arenas(ctx.sarena);
    free_caches(&ctx);
    return nerrs;
}
// Again, this sucks.
// Don't need the SheetOps* arg and as this is an export the
// number of parameters matters.
#ifdef __wasm__
#define ARGS SheetHandle sheethandle, const char* txt, size_t len, DrSpreadResult* outval, intptr_t row, intptr_t col
#else
#define ARGS SheetHandle sheethandle, const SheetOps* ops, const char* txt, size_t len, DrSpreadResult* outval, intptr_t row, intptr_t col
#endif

DRSP_EXPORT
int
drsp_evaluate_string(ARGS){
#undef ARGS
    _Alignas(intptr_t) char evalbuff [30000];
    SpreadContext ctx = {
#ifndef __wasm__
        ._ops=*ops,
#endif
        .a={evalbuff, evalbuff, evalbuff+sizeof evalbuff},
        .null={EXPR_NULL},
        .error={EXPR_ERROR},
        #ifdef __wasm__
        .limit = (uintptr_t)__builtin_frame_address(0) - 30000,
        #else
        .limit = (uintptr_t)__builtin_frame_address(0) - 300000,
        #endif
    };
    Expression* e = evaluate_string(&ctx, sheethandle, txt, len, row, col);
    int error = 0;
    if(!e){
        error = 1;
        goto finish;
    }
    switch(e->kind){
        case EXPR_NULL:
            outval->kind = DRSP_RESULT_NULL;
            break;
        case EXPR_NUMBER:
            outval->kind = DRSP_RESULT_NUMBER;
            outval->d = ((Number*)e)->value;
            break;
        case EXPR_STRING:{
            outval->kind = DRSP_RESULT_STRING;
            StringView sv = ((String*)e)->sv;
            char* t = malloc(sv.length);
            __builtin_memcpy(t, sv.text, sv.length);
            outval->s.length = sv.length;
            outval->s.text = t;
        }break;
        default:
            error = 1;
            break;
    }
    finish:
    free_string_arenas(ctx.sarena);
    free_caches(&ctx);
    return error;
}


#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#include "drspread_parse.c"
#include "drspread_formula_funcs.c"
#include "drspread_evaluate.c"
#endif
