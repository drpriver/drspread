#ifndef DRSPREAD_C
#define DRSPREAD_C

#include "drspread_evaluate.h"
#include "drspread_parse.h"
#include "spreadsheet.h"
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
    SpreadContext ctx = {
        *ops, {buff, buff, buff+sizeof buff},
    };
    while(ctx.ops.next_cell(ctx.ops.ctx, &row, &col) == 0){
        double val;
        EvaluateResult er = evaluate(&ctx, row, col, &val);
        ctx.a.cursor = ctx.a.data;
        if(er == EV_ERROR){
            nerrs++;
            ctx.ops.set_display_error(ctx.ops.ctx, row, col, "error", 5);
        }
        else if(er == EV_NUMBER)
            ctx.ops.set_display_number(ctx.ops.ctx, row, col, val);
    }
    return nerrs;
}


#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#include "drspread_parse.c"
#include "drspread_formula_funcs.c"
#include "drspread_evaluate.c"
#endif
