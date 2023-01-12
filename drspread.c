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

#ifdef __wasm__
DRSP_EXPORT
int
drsp_evaluate_formulas(SheetHandle sheethandle){
#else
DRSP_EXPORT
int
drsp_evaluate_formulas(SheetHandle sheethandle, const SheetOps* ops){
#endif
    intptr_t row=-1, col=-1;
    int nerrs = 0;
    _Alignas(intptr_t) char buff[10000];
    intptr_t ncols = 1, nrows = 1;
    SpreadContext ctx = {
#ifndef __wasm__
        ._ops=*ops,
#endif
        .a={buff, buff, buff+sizeof buff},
        .null={EXPR_NULL},
        .error={EXPR_ERROR},
    };
#ifdef __wasm__
    if(1){
#else
    if(ops->dims){
#endif
        int err = sp_dims(&ctx, sheethandle, &ncols, &nrows);
        (void)err;
        ctx.cache.ncols = ncols;
        ctx.cache.nrows = nrows;
    }
    ctx.cache.vals = buff_alloc(&ctx.a, ncols*nrows*sizeof *ctx.cache.vals);
    if(!ctx.cache.vals) ctx.cache = (SpreadCache){0};
    for(intptr_t i = 0; i < ctx.cache.nrows*ctx.cache.ncols; i++)
        ctx.cache.vals[i].kind = CACHE_UNSET;
    char* chk = ctx.a.cursor;
    for(intptr_t i = 0; sp_next_cell(&ctx, sheethandle, i, &row, &col) == 0; i++){
        Expression* e = evaluate(&ctx, sheethandle, row, col);
        ctx.a.cursor = chk;
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
    return nerrs;
}

#ifdef __wasm__
DRSP_EXPORT
int
drsp_evaluate_string(SheetHandle sheethandle, const char* txt, size_t len, DrSpreadCellValue* outval){
#else

DRSP_EXPORT
int
drsp_evaluate_string(SheetHandle sheethandle, const SheetOps* ops, const char* txt, size_t len, DrSpreadCellValue* outval){
#endif
    _Alignas(intptr_t) char buff[4000];
    intptr_t ncols = 1, nrows = 1;
    SpreadContext ctx = {
#ifndef __wasm__
        ._ops=*ops,
#endif
        .a={buff, buff, buff+sizeof buff},
        .cache={.ncols=ncols, .nrows=nrows},
        .null={EXPR_NULL},
        .error={EXPR_ERROR},
    };
#ifdef __wasm__
    if(1){
#else
    if(ops->dims){
#endif
        int err = sp_dims(&ctx, sheethandle, &ncols, &nrows);
        (void)err;
        ctx.cache.ncols = ncols;
        ctx.cache.nrows = nrows;
    }
    ctx.cache.vals = buff_alloc(&ctx.a, ncols*nrows*sizeof *ctx.cache.vals);
    if(!ctx.cache.vals) ctx.cache = (SpreadCache){0};
    for(intptr_t i = 0; i < ctx.cache.nrows*ctx.cache.ncols; i++)
        ctx.cache.vals[i].kind = CACHE_UNSET;
    // printf("%zd\n", ctx.a.cursor - buff);
    Expression* e = evaluate_string(&ctx, sheethandle, txt, len);
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
