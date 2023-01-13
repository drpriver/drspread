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
drsp_evaluate_formulas(ARGS){
#undef ARGS
    intptr_t row=-1, col=-1;
    int nerrs = 0;
    _Alignas(intptr_t) char evalbuff [10000];
    SpreadContext ctx = {
        #ifndef __wasm__
            ._ops=*ops,
        #endif
        .a={evalbuff, evalbuff, evalbuff+sizeof evalbuff},
        .null={EXPR_NULL},
        .error={EXPR_ERROR},
    };
    BuffCheckpoint bc = buff_checkpoint(&ctx.a);
    for(intptr_t i = 0; sp_next_cell(&ctx, sheethandle, i, &row, &col) == 0; i++){
        buff_set(&ctx.a, bc);
        Expression* e = evaluate(&ctx, sheethandle, row, col);
        #if !defined(__wasm__) && !defined(TESTING_H)
        if(0)
        #else
        if(0)
        #endif
        for(int i = 0; i < 10000; i++){
            buff_set(&ctx.a, bc);
            e = evaluate(&ctx, sheethandle, row, col);
        }
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
    free_caches(&ctx);
    return nerrs;
}
// Again, this sucks.
// Don't need the SheetOps* arg and as this is an export the
// number of parameters matters.
#ifdef __wasm__
#define ARGS SheetHandle sheethandle, const char* txt, size_t len, DrSpreadCellValue* outval
#else
#define ARGS SheetHandle sheethandle, const SheetOps* ops, const char* txt, size_t len, DrSpreadCellValue* outval
#endif

DRSP_EXPORT
int
drsp_evaluate_string(ARGS){
#undef ARGS
    _Alignas(intptr_t) char evalbuff [10000];
    SpreadContext ctx = {
#ifndef __wasm__
        ._ops=*ops,
#endif
        .a={evalbuff, evalbuff, evalbuff+sizeof evalbuff},
        .null={EXPR_NULL},
        .error={EXPR_ERROR},
    };
    Expression* e = evaluate_string(&ctx, sheethandle, txt, len, -1, -1);
    free_caches(&ctx);
    if(!e) return 1;
    switch(e->kind){
        case EXPR_NULL:
            outval->kind = CELL_EMPTY;
            return 0;
        case EXPR_NUMBER:
            outval->kind = CELL_NUMBER;
            outval->d = ((Number*)e)->value;
            return 0;
        case EXPR_STRING:
            outval->kind = CELL_OTHER;
            outval->s.length = ((String*)e)->sv.length;
            outval->s.text = ((String*)e)->sv.text;
            return 0;
        default:
            return 1;
    }
}


#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#include "drspread_parse.c"
#include "drspread_formula_funcs.c"
#include "drspread_evaluate.c"
#endif
