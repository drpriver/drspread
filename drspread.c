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
//    sum
//    min
//    max
//    avg
//    mod (3.5 style ability mode)
//    count - number of non-null entries
//    counttrue - number of true entries

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
//   & (logical and)
//   | (logical or)

// Terminals:
//   function call
//   number
//   range
//   (group)

// Tokens:
//   identifiers
//   numbers
//   lparen
//   rparen
//   plus
//   minus
//   slash
//   star
//   lsquare
//   rsquare
//   colon

// [a, 0]            - scalar (0d range)
// [a, :], [a, ]     - 1d range (column)
// [0, :], [a, ]     - 1d range (column)
// [a], [0]          - 1d range (column)
// [a:a, :], [a:a, ] - 2d range

// sum([a, 0])
// sum([a,:]) //
// sum([a,])
// sum([a:a,])
// sum([a:a,:])
// max(a[:])
// max(a)
// sum(a[:] * b[:])
// max(a[:]) + a[:]

int
drsp_evaluate_formulas(const SheetOps* ops){
    intptr_t row=-1, col=-1;
    int nerrs = 0;
    _Alignas(intptr_t) char buff[10000];
    intptr_t ncols = 1, nrows = 1;
    if(ops->dims){
        int err = ops->dims(ops->ctx, &ncols, &nrows);
        (void)err;
    }
    SpreadContext ctx = {
        .ops=*ops,
        .a={buff, buff, buff+sizeof buff},
        .cache={.ncols=ncols, .nrows=nrows},
        .null={EXPR_NULL},
        .error={EXPR_ERROR},
    };
    ctx.cache.vals = buff_alloc(&ctx.a, ncols*nrows*sizeof *ctx.cache.vals);
    if(!ctx.cache.vals) ctx.cache = (SpreadCache){0};
    for(intptr_t i = 0; i < ctx.cache.nrows*ctx.cache.ncols; i++)
        ctx.cache.vals[i].kind = CACHE_UNSET;
    char* chk = ctx.a.cursor;
    for(intptr_t i = 0; ctx.ops.next_cell(ctx.ops.ctx, i, &row, &col) == 0; i++){
        Expression* e = evaluate(&ctx, row, col);
        ctx.a.cursor = chk;
        if(e){
            switch(e->kind){
                case EXPR_NUMBER:
                    ctx.ops.set_display_number(ctx.ops.ctx, row, col, ((Number*)e)->value);
                    continue;
                case EXPR_STRING:
                    ctx.ops.set_display_string(ctx.ops.ctx, row, col, ((String*)e)->sv.text, ((String*)e)->sv.length);
                    continue;
                case EXPR_NULL:
                    ctx.ops.set_display_string(ctx.ops.ctx, row, col, "", 0);
                    continue;
                default: break;
            }
        }
        nerrs++;
        ctx.ops.set_display_error(ctx.ops.ctx, row, col, "error", 5);
    }
    return nerrs;
}

int
drsp_evaluate_string(const SheetOps* ops, const char* txt, size_t len, double* outval){
    _Alignas(intptr_t) char buff[4000];
    intptr_t ncols = 1, nrows = 1;
    if(ops->dims){
        int err = ops->dims(ops->ctx, &ncols, &nrows);
        (void)err;
    }
    SpreadContext ctx = {
        .ops=*ops,
        .a={buff, buff, buff+sizeof buff},
        .cache={.ncols=ncols, .nrows=nrows},
        .null={EXPR_NULL},
        .error={EXPR_ERROR},
    };
    ctx.cache.vals = buff_alloc(&ctx.a, ncols*nrows*sizeof *ctx.cache.vals);
    if(!ctx.cache.vals) ctx.cache = (SpreadCache){0};
    for(intptr_t i = 0; i < ctx.cache.nrows*ctx.cache.ncols; i++)
        ctx.cache.vals[i].kind = CACHE_UNSET;
    // printf("%zd\n", ctx.a.cursor - buff);
    Expression* e = evaluate_string(&ctx, txt, len);
    // printf("%zd\n", ctx.a.cursor - buff);
    if(e && e->kind == EXPR_NUMBER){
        *outval = ((Number*)e)->value;
        return 0;
    }
    if(!e){
        // puts("oom");
    }
    return 1;
}


#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#include "drspread_parse.c"
#include "drspread_formula_funcs.c"
#include "drspread_evaluate.c"
#endif
