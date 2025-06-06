//
// Copyright © 2023-2025, David Priver <david@davidpriver.com>
//
#ifndef DRSPREAD_FORMULA_FUNCS_C
#define DRSPREAD_FORMULA_FUNCS_C
#include "drspread_types.h"
#include "drspread_evaluate.h"
#include "drspread_utils.h"
#include "drspread_formula_funcs.h"
#include "parse_numbers.h"
#include <stdarg.h>
#ifdef __wasm__
#include <math.h>
#define __builtin_round round
#define __builtin_pow pow
#define __builtin_log log
#endif
#if (defined(_MSC_VER) && !defined(__clang__))
#include <math.h>
#define __builtin_floor floor
#define __builtin_ceil ceil
#define __builtin_trunc trunc
#define __builtin_fabs fabs
#define __builtin_sqrt sqrt
#define __builtin_round round
#define __builtin_pow pow
#define __builtin_log log
#endif


#ifndef arrlen
#define arrlen(x) (sizeof(x)/sizeof(x[0]))
#endif

#if defined(TESTING_H) && !defined(DRSP_INTRINS)
#define DRSP_INTRINS 1
#endif

#ifdef DRSP_INTRINS
#include <stdio.h>
#endif

#ifdef __clang__
#pragma clang assume_nonnull begin
#endif

DRSP_INTERNAL
FORMULAFUNC(drsp_sum){
    if(argc != 1) return Error(ctx, "sum() accepts 1 argument");
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    double sum = 0;
    if(arg->kind == EXPR_COMPUTED_ARRAY){
        ComputedArray* c = (ComputedArray*)arg;
        for(intptr_t i = 0; i < c->length; i++){
            Expression* e = c->data[i];
            if(evaled_is_not_scalar(e)) return Error(ctx, "Range to be summed contains range");
            if(e->kind != EXPR_NUMBER)
                continue;
            sum += ((Number*)e)->value;
        }
    }
    else if(arg->kind == EXPR_RANGE1D_COLUMN || arg->kind == EXPR_RANGE1D_COLUMN_FOREIGN){
        intptr_t col, start, end;
        SheetData* rsd = sd;
        if(get_range1dcol(ctx, sd, arg, &col, &start, &end, &rsd, caller_row, caller_col) != 0)
            return Error(ctx, "Invalid range");
        // NOTE: inclusive range
        for(intptr_t row = start; row <= end; row++){
            BuffCheckpoint bc = buff_checkpoint(ctx->a);
            Expression* e = evaluate(ctx, rsd, row, col);
            if(!e || e->kind == EXPR_ERROR) return e;
            if(evaled_is_not_scalar(e)) return Error(ctx, "Range to be summed contains range");
            if(e->kind != EXPR_NUMBER) continue;
            sum += ((Number*)e)->value;
            buff_set(ctx->a, bc);
        }
    }
    else {
        intptr_t row, start, end;
        SheetData* rsd = sd;
        if(get_range1drow(ctx, sd, arg, &row, &start, &end, &rsd, caller_row, caller_col) != 0)
            return Error(ctx, "Invalid range");
        // NOTE: inclusive range
        for(intptr_t col = start; col <= end; col++){
            BuffCheckpoint bc = buff_checkpoint(ctx->a);
            Expression* e = evaluate(ctx, rsd, row, col);
            if(!e || e->kind == EXPR_ERROR) return e;
            if(evaled_is_not_scalar(e)) return Error(ctx, "Range to be summed contains range");
            if(e->kind != EXPR_NUMBER) continue;
            sum += ((Number*)e)->value;
            buff_set(ctx->a, bc);
        }
    }
    buff_set(ctx->a, bc);
    Number* n = expr_alloc(ctx, EXPR_NUMBER);
    if(!n) return NULL;
    n->value = sum;
    return &n->e;
}
DRSP_INTERNAL
FORMULAFUNC(drsp_prod){
    if(argc != 1) return Error(ctx, "prod() accepts 1 argument");
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    double prod = 1;
    if(arg->kind == EXPR_COMPUTED_ARRAY){
        ComputedArray* c = (ComputedArray*)arg;
        for(intptr_t i = 0; i < c->length; i++){
            Expression* e = c->data[i];
            if(evaled_is_not_scalar(e)) return Error(ctx, "Range input to prod() contains range");
            if(e->kind != EXPR_NUMBER)
                continue;
            prod *= ((Number*)e)->value;
        }
    }
    else if(arg->kind == EXPR_RANGE1D_COLUMN || arg->kind == EXPR_RANGE1D_COLUMN_FOREIGN){
        intptr_t col, start, end;
        SheetData* rsd = sd;
        if(get_range1dcol(ctx, sd, arg, &col, &start, &end, &rsd, caller_row, caller_col) != 0)
            return Error(ctx, "Invalid range");
        // NOTE: inclusive range
        for(intptr_t row = start; row <= end; row++){
            BuffCheckpoint bc = buff_checkpoint(ctx->a);
            Expression* e = evaluate(ctx, rsd, row, col);
            if(!e || e->kind == EXPR_ERROR) return e;
            if(evaled_is_not_scalar(e)) return Error(ctx, "Range input to prod() contains range");
            if(e->kind != EXPR_NUMBER) continue;
            prod *= ((Number*)e)->value;
            buff_set(ctx->a, bc);
        }
    }
    else {
        intptr_t row, start, end;
        SheetData* rsd = sd;
        if(get_range1drow(ctx, sd, arg, &row, &start, &end, &rsd, caller_row, caller_col) != 0)
            return Error(ctx, "Invalid range");
        // NOTE: inclusive range
        for(intptr_t col = start; col <= end; col++){
            BuffCheckpoint bc = buff_checkpoint(ctx->a);
            Expression* e = evaluate(ctx, rsd, row, col);
            if(!e || e->kind == EXPR_ERROR) return e;
            if(evaled_is_not_scalar(e)) return Error(ctx, "Range input to prod() contains range");
            if(e->kind != EXPR_NUMBER) continue;
            prod *= ((Number*)e)->value;
            buff_set(ctx->a, bc);
        }
    }
    buff_set(ctx->a, bc);
    Number* n = expr_alloc(ctx, EXPR_NUMBER);
    if(!n) return NULL;
    n->value = prod;
    return &n->e;
}

DRSP_INTERNAL
FORMULAFUNC(drsp_avg){
    if(argc != 1) return Error(ctx, "avg() accepts 1 argument");
    Number* n = expr_alloc(ctx, EXPR_NUMBER);
    if(!n) return NULL;
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    double sum = 0.;
    double count = 0.;
    if(arg->kind == EXPR_COMPUTED_ARRAY){
        ComputedArray* c = (ComputedArray*)arg;
        for(intptr_t i = 0; i < c->length; i++){
            Expression* e = c->data[i];
            if(evaled_is_not_scalar(e)) return Error(ctx, "Range input to avg() contains range");
            if(e->kind != EXPR_NUMBER) continue;
            sum += ((Number*)e)->value;
            count += 1.0;
        }
    }
    else if(arg->kind == EXPR_RANGE1D_COLUMN || arg->kind == EXPR_RANGE1D_COLUMN_FOREIGN){
        intptr_t col, start, end;
        SheetData* rsd = sd;
        if(get_range1dcol(ctx, sd, arg, &col, &start, &end, &rsd, caller_row, caller_col) != 0)
            return Error(ctx, "Invalid range");
        // NOTE: inclusive range
        for(intptr_t row = start; row <= end; row++){
            BuffCheckpoint bc = buff_checkpoint(ctx->a);
            Expression* e = evaluate(ctx, rsd, row, col);
            if(!e || e->kind == EXPR_ERROR) return e;
            if(evaled_is_not_scalar(e)) return Error(ctx, "Range input to avg() contains range");
            if(e->kind != EXPR_NUMBER) continue;
            sum += ((Number*)e)->value;
            count += 1.0;
            buff_set(ctx->a, bc);
        }
    }
    else {
        intptr_t row, start, end;
        SheetData* rsd = sd;
        if(get_range1drow(ctx, sd, arg, &row, &start, &end, &rsd, caller_row, caller_col) != 0)
            return Error(ctx, "Invalid range");
        // NOTE: inclusive range
        for(intptr_t col = start; col <= end; col++){
            BuffCheckpoint bc = buff_checkpoint(ctx->a);
            Expression* e = evaluate(ctx, rsd, row, col);
            if(!e || e->kind == EXPR_ERROR) return e;
            if(evaled_is_not_scalar(e)) return Error(ctx, "Range input to avg() contains range");
            if(e->kind != EXPR_NUMBER) continue;
            sum += ((Number*)e)->value;
            count += 1.0;
            buff_set(ctx->a, bc);
        }
    }
    buff_set(ctx->a, bc);
    n->value = count!=0.?sum/count:0;
    return &n->e;
}

DRSP_INTERNAL
FORMULAFUNC(drsp_count){
    if(argc != 1) return Error(ctx, "count() accepts 1 argument");
    Number* n = expr_alloc(ctx, EXPR_NUMBER);
    if(!n) return NULL;
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    intptr_t count = 0;
    if(arg->kind == EXPR_COMPUTED_ARRAY){
        ComputedArray* c = (ComputedArray*)arg;
        for(intptr_t i = 0; i < c->length; i++){
            Expression* e= c->data[i];
            if(e->kind != EXPR_NUMBER && e->kind != EXPR_STRING)
                continue;
            count += 1;
        }
    }
    else if(arg->kind == EXPR_RANGE1D_COLUMN || arg->kind == EXPR_RANGE1D_COLUMN_FOREIGN){
        SheetData* rsd = sd;
        intptr_t col, start, end;
        if(get_range1dcol(ctx, sd, arg, &col, &start, &end, &rsd, caller_row, caller_col) != 0)
            return Error(ctx, "Invalid range");
        // NOTE: inclusive range
        for(intptr_t row = start; row <= end; row++){
            BuffCheckpoint bc = buff_checkpoint(ctx->a);
            Expression* e = evaluate(ctx, rsd, row, col);
            if(!e || e->kind == EXPR_ERROR) return e;
            if(e->kind != EXPR_NUMBER && e->kind != EXPR_STRING)
                continue;
            count += 1;
            buff_set(ctx->a, bc);
        }
    }
    else {
        SheetData* rsd = sd;
        intptr_t row, start, end;
        if(get_range1drow(ctx, sd, arg, &row, &start, &end, &rsd, caller_row, caller_col) != 0)
            return Error(ctx, "Invalid range");
        // NOTE: inclusive range
        for(intptr_t col = start; col <= end; col++){
            BuffCheckpoint bc = buff_checkpoint(ctx->a);
            Expression* e = evaluate(ctx, rsd, row, col);
            if(!e || e->kind == EXPR_ERROR) return e;
            if(e->kind != EXPR_NUMBER && e->kind != EXPR_STRING)
                continue;
            count += 1;
            buff_set(ctx->a, bc);
        }
    }
    buff_set(ctx->a, bc);
    n->value = (double)count;
    return &n->e;
}

DRSP_INTERNAL
FORMULAFUNC(drsp_min){
    if(!argc) return Error(ctx, "min() requires more than 0 arguments");
    if(argc > 1){
        BuffCheckpoint bc = buff_checkpoint(ctx->a);
        double v = 1e32;
        for(;argc;argc--, argv++, buff_set(ctx->a, bc)){
            Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
            if(!arg || arg->kind == EXPR_ERROR)
                return arg;
            if(arg->kind != EXPR_NUMBER)
                return Error(ctx, "arguments to min() must be numbers");
            double n = ((Number*)arg)->value;
            if(n < v)
                v = n;
        }
        Number* n = expr_alloc(ctx, EXPR_NUMBER);
        if(!n) return NULL;
        n->value = v == 1e32?0:v;
        return &n->e;
    }
    Number* n = expr_alloc(ctx, EXPR_NUMBER);
    if(!n) return NULL;
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    double v = 1e32;
    if(arg->kind == EXPR_COMPUTED_ARRAY){
        ComputedArray* c = (ComputedArray*)arg;
        for(intptr_t i = 0; i < c->length; i++){
            Expression* e = c->data[i];
            if(evaled_is_not_scalar(e)) return Error(ctx, "Range input to min() contains range");
            if(e->kind != EXPR_NUMBER) continue;
            if(((Number*)e)->value < v)
                v = ((Number*)e)->value;
        }
    }
    else if(arg->kind == EXPR_RANGE1D_COLUMN || arg->kind == EXPR_RANGE1D_COLUMN_FOREIGN){
        SheetData* rsd = sd;
        intptr_t col, start, end;
        if(get_range1dcol(ctx, sd, arg, &col, &start, &end, &rsd, caller_row, caller_col) != 0)
            return Error(ctx, "Invalid range");
        // NOTE: inclusive range
        for(intptr_t row = start; row <= end; row++){
            BuffCheckpoint bc = buff_checkpoint(ctx->a);
            Expression* e = evaluate(ctx, rsd, row, col);
            if(!e || e->kind == EXPR_ERROR) return e;
            if(evaled_is_not_scalar(e)) return Error(ctx, "Range input to min() contains range");
            if(e->kind != EXPR_NUMBER) continue;
            if(((Number*)e)->value < v)
                v = ((Number*)e)->value;
            buff_set(ctx->a, bc);
        }
    }
    else {
        SheetData* rsd = sd;
        intptr_t row, start, end;
        if(get_range1drow(ctx, sd, arg, &row, &start, &end, &rsd, caller_row, caller_col) != 0)
            return Error(ctx, "Invalid range");
        // NOTE: inclusive range
        for(intptr_t col = start; col <= end; col++){
            BuffCheckpoint bc = buff_checkpoint(ctx->a);
            Expression* e = evaluate(ctx, rsd, row, col);
            if(!e || e->kind == EXPR_ERROR) return e;
            if(evaled_is_not_scalar(e)) return Error(ctx, "Range input to min() contains range");
            if(e->kind != EXPR_NUMBER) continue;
            if(((Number*)e)->value < v)
                v = ((Number*)e)->value;
            buff_set(ctx->a, bc);
        }
    }
    if(v == 1e32) v = 0.;
    buff_set(ctx->a, bc);
    n->value = v;
    return &n->e;
}

DRSP_INTERNAL
FORMULAFUNC(drsp_max){
    if(!argc) return Error(ctx, "max() requires more than 0 arguments");
    if(argc > 1){
        BuffCheckpoint bc = buff_checkpoint(ctx->a);
        double v = -1e32;
        for(;argc;argc--, argv++, buff_set(ctx->a, bc)){
            Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
            if(!arg || arg->kind == EXPR_ERROR)
                return arg;
            if(arg->kind != EXPR_NUMBER)
                return Error(ctx, "arguments to max() must be numbers");
            double n = ((Number*)arg)->value;
            if(n > v)
                v = n;
        }
        Number* n = expr_alloc(ctx, EXPR_NUMBER);
        if(!n) return NULL;
        n->value = v == -1e32?0:v;
        return &n->e;
    }
    Number* n = expr_alloc(ctx, EXPR_NUMBER);
    if(!n) return NULL;
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    double v = -1e32;
    if(arg->kind == EXPR_COMPUTED_ARRAY){
        ComputedArray* c = (ComputedArray*)arg;
        for(intptr_t i = 0; i < c->length; i++){
            Expression* e = c->data[i];
            if(evaled_is_not_scalar(e)) return Error(ctx, "Range input to max() contains range");
            if(e->kind != EXPR_NUMBER) continue;
            if(((Number*)e)->value > v)
                v = ((Number*)e)->value;
        }
    }
    else if(arg->kind == EXPR_RANGE1D_COLUMN || arg->kind == EXPR_RANGE1D_COLUMN_FOREIGN){
        SheetData* rsd = sd;
        intptr_t col, start, end;
        if(get_range1dcol(ctx, sd, arg, &col, &start, &end, &rsd, caller_row, caller_col) != 0)
            return Error(ctx, "Invalid range");
        // NOTE: inclusive range
        for(intptr_t row = start; row <= end; row++){
            BuffCheckpoint bc = buff_checkpoint(ctx->a);
            Expression* e = evaluate(ctx, rsd, row, col);
            if(!e || e->kind == EXPR_ERROR) return e;
            if(evaled_is_not_scalar(e)) return Error(ctx, "Range input to max() contains range");
            if(e->kind != EXPR_NUMBER) continue;
            if(((Number*)e)->value > v)
                v = ((Number*)e)->value;
            buff_set(ctx->a, bc);
        }
    }
    else {
        SheetData* rsd = sd;
        intptr_t row, start, end;
        if(get_range1drow(ctx, sd, arg, &row, &start, &end, &rsd, caller_row, caller_col) != 0)
            return Error(ctx, "Invalid range");
        // NOTE: inclusive range
        for(intptr_t col = start; col <= end; col++){
            BuffCheckpoint bc = buff_checkpoint(ctx->a);
            Expression* e = evaluate(ctx, rsd, row, col);
            if(!e || e->kind == EXPR_ERROR) return e;
            if(evaled_is_not_scalar(e)) return Error(ctx, "Range input to max() contains range");
            if(e->kind != EXPR_NUMBER) continue;
            if(((Number*)e)->value > v)
                v = ((Number*)e)->value;
            buff_set(ctx->a, bc);
        }
    }
    if(v == -1e32) v = 0.;
    buff_set(ctx->a, bc);
    n->value = v;
    return &n->e;
}

DRSP_INTERNAL
FORMULAFUNC(drsp_mod){
    if(argc != 1) return Error(ctx, "mod() requires 1 argument");
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(expr_is_arraylike(arg)){
        arg = convert_to_computed_array(ctx, sd, arg, caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        ComputedArray* c = (ComputedArray*)arg;
        for(intptr_t i = 0; i < c->length; i++){
            Expression* e = c->data[i];
            if(e->kind == EXPR_BLANK)
                continue;
            if(e->kind != EXPR_NUMBER)
                return Error(ctx, "argument to mod() must be a number");
            Number* n = (Number*)e;
            n->value = __builtin_floor((n->value - 10)/2);
        }
        return arg;
    }
    else {
        if(arg->kind != EXPR_NUMBER)
            return Error(ctx, "argument to mod() must be a number");
        double score = ((Number*)arg)->value;
        double mod = __builtin_floor((score - 10)/2);
        buff_set(ctx->a, bc);
        Number* n = expr_alloc(ctx, EXPR_NUMBER);
        if(!n) return NULL;
        n->value = mod;
        return &n->e;
    }
}

DRSP_INTERNAL
FORMULAFUNC(drsp_floor){
    if(argc != 1) return Error(ctx, "floor() requires 1 argument");
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(expr_is_arraylike(arg)){
        arg = convert_to_computed_array(ctx, sd, arg, caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        ComputedArray* c = (ComputedArray*)arg;
        for(intptr_t i = 0; i < c->length; i++){
            Expression* e = c->data[i];
            if(e->kind == EXPR_BLANK)
                continue;
            if(e->kind != EXPR_NUMBER)
                return Error(ctx, "argument to floor() must be a number");
            Number* n = (Number*)e;
            n->value = __builtin_floor(n->value);
        }
        return arg;
    }
    else {
        if(arg->kind != EXPR_NUMBER)
            return Error(ctx, "argument to floor() must be a number");
        buff_set(ctx->a, bc);
        Number* n = expr_alloc(ctx, EXPR_NUMBER);
        if(!n) return NULL;
        n->value = __builtin_floor(((Number*)arg)->value);
        return &n->e;
    }
}

DRSP_INTERNAL
FORMULAFUNC(drsp_ceil){
    if(argc != 1) return Error(ctx, "ceil() requires 1 argument");
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(expr_is_arraylike(arg)){
        arg = convert_to_computed_array(ctx, sd, arg, caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        ComputedArray* c = (ComputedArray*)arg;
        for(intptr_t i = 0; i < c->length; i++){
            Expression* e = c->data[i];
            if(e->kind == EXPR_BLANK)
                continue;
            if(e->kind != EXPR_NUMBER)
                return Error(ctx, "argument to ceil() must be a number");
            Number* n = (Number*)e;
            n->value = __builtin_ceil(n->value);
        }
        return arg;
    }
    else {
        if(arg->kind != EXPR_NUMBER)
            return Error(ctx, "argument to ceil() must be a number");
        buff_set(ctx->a, bc);
        Number* n = expr_alloc(ctx, EXPR_NUMBER);
        if(!n) return NULL;
        n->value = __builtin_ceil(((Number*)arg)->value);
        return &n->e;
    }
}

DRSP_INTERNAL
FORMULAFUNC(drsp_trunc){
    if(argc != 1) return Error(ctx, "trunc() requires 1 argument");
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(expr_is_arraylike(arg)){
        arg = convert_to_computed_array(ctx, sd, arg, caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        ComputedArray* c = (ComputedArray*)arg;
        for(intptr_t i = 0; i < c->length; i++){
            Expression* e = c->data[i];
            if(e->kind == EXPR_BLANK)
                continue;
            if(e->kind != EXPR_NUMBER)
                return Error(ctx, "argument to trunc() must be a number");
            Number* n = (Number*)e;
            n->value = __builtin_trunc(n->value);
        }
        return arg;
    }
    else {
        if(arg->kind != EXPR_NUMBER)
            return Error(ctx, "argument to trunc() must be a number");
        buff_set(ctx->a, bc);
        Number* n = expr_alloc(ctx, EXPR_NUMBER);
        if(!n) return NULL;
        n->value = __builtin_trunc(((Number*)arg)->value);
        return &n->e;
    }
}

DRSP_INTERNAL
FORMULAFUNC(drsp_round){
    if(argc != 1) return Error(ctx, "round() requires 1 argument");
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(expr_is_arraylike(arg)){
        arg = convert_to_computed_array(ctx, sd, arg, caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        ComputedArray* c = (ComputedArray*)arg;
        for(intptr_t i = 0; i < c->length; i++){
            Expression* e = c->data[i];
            if(e->kind == EXPR_BLANK)
                continue;
            if(e->kind != EXPR_NUMBER)
                return Error(ctx, "argument to round() must be a number");
            Number* n = (Number*)e;
            n->value = __builtin_round(n->value);
        }
        return arg;
    }
    else {
        if(arg->kind != EXPR_NUMBER)
            return Error(ctx, "argument to round() must be a number");
        buff_set(ctx->a, bc);
        Number* n = expr_alloc(ctx, EXPR_NUMBER);
        if(!n) return NULL;
        n->value = __builtin_round(((Number*)arg)->value);
        return &n->e;
    }
}

DRSP_INTERNAL
FORMULAFUNC(drsp_abs){
    if(argc != 1) return Error(ctx, "abs() requires 1 argument");
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(expr_is_arraylike(arg)){
        arg = convert_to_computed_array(ctx, sd, arg, caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        ComputedArray* c = (ComputedArray*)arg;
        for(intptr_t i = 0; i < c->length; i++){
            Expression* e = c->data[i];
            if(e->kind == EXPR_BLANK)
                continue;
            if(e->kind != EXPR_NUMBER)
                return Error(ctx, "argument to abs() must be a number");
            Number* n = (Number*)e;
            n->value = __builtin_fabs(n->value);
        }
        return arg;
    }
    else {
        if(arg->kind != EXPR_NUMBER)
            return Error(ctx, "argument to abs() must be a number");
        buff_set(ctx->a, bc);
        Number* n = expr_alloc(ctx, EXPR_NUMBER);
        if(!n) return NULL;
        n->value = __builtin_fabs(((Number*)arg)->value);
        return &n->e;
    }
}

DRSP_INTERNAL
FORMULAFUNC(drsp_sqrt){
    if(argc != 1) return Error(ctx, "sqrt() requires 1 argument");
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(expr_is_arraylike(arg)){
        arg = convert_to_computed_array(ctx, sd, arg, caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        ComputedArray* c = (ComputedArray*)arg;
        for(intptr_t i = 0; i < c->length; i++){
            Expression* e = c->data[i];
            if(e->kind == EXPR_BLANK)
                continue;
            if(e->kind != EXPR_NUMBER)
                return Error(ctx, "argument to sqrt() must be a number");
            Number* n = (Number*)e;
            n->value = __builtin_sqrt(n->value);
        }
        return arg;
    }
    else {
        if(arg->kind != EXPR_NUMBER)
            return Error(ctx, "argument to sqrt() must be a number");
        buff_set(ctx->a, bc);
        Number* n = expr_alloc(ctx, EXPR_NUMBER);
        if(!n) return NULL;
        n->value = __builtin_sqrt(((Number*)arg)->value);
        return &n->e;
    }
}

DRSP_INTERNAL
FORMULAFUNC(drsp_log){
    if(argc != 1 && argc != 2) return Error(ctx, "log() requires 1 or 2 arguments");
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    double base = 0;
    if(argc > 1){
        Expression* arg2 = evaluate_expr(ctx, sd, argv[1], caller_row, caller_col);
        if(!arg2 || arg2->kind == EXPR_ERROR) return arg;
        if(arg2->kind != EXPR_NUMBER)
            return Error(ctx, "base must be a number");
        base = ((Number*)arg2)->value;
        if(base <= 1) return Error(ctx, "base must be > 1");
    }

    if(expr_is_arraylike(arg)){
        arg = convert_to_computed_array(ctx, sd, arg, caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        ComputedArray* c = (ComputedArray*)arg;
        for(intptr_t i = 0; i < c->length; i++){
            Expression* e = c->data[i];
            if(e->kind == EXPR_BLANK)
                continue;
            if(e->kind != EXPR_NUMBER)
                return Error(ctx, "argument to log() must be a number");
            Number* n = (Number*)e;
            n->value = __builtin_log(n->value);
            if(base > 0) n->value /= __builtin_log(base);
        }
        return arg;
    }
    else {
        if(arg->kind != EXPR_NUMBER)
            return Error(ctx, "argument to log() must be a number");
        buff_set(ctx->a, bc);
        Number* n = expr_alloc(ctx, EXPR_NUMBER);
        if(!n) return NULL;
        n->value = __builtin_log(((Number*)arg)->value);
        if(base > 0) n->value /= __builtin_log(base);
        return &n->e;
    }
}

#ifndef CASE_0_9
#define CASE_0_9 '0': case '1': case '2': case '3': case '4': case '5': \
    case '6': case '7': case '8': case '9'
#endif

static inline
int
parse_leading_double(const char* txt, size_t len, double* result){
    for(;len;len--, txt++){
        switch(*txt){
            case ' ': case '\t':
                continue;
            default:
                break;
        }
        break;
    }
    const char* first = txt;
    const char* end = txt+len;
    for(;txt != end; txt++){
        switch(*txt){
            case '+':
            case '-':
                continue;
            default:
                break;
        }
        break;
    }
    for(;txt != end; txt++){
        switch(*txt){
            case '.':
                txt++;
                break;
            case CASE_0_9:
                continue;
            default:
                break;
        }
        break;
    }
    for(;txt != end; txt++){
        switch(*txt){
            case CASE_0_9:
                continue;
            default:
                break;
        }
        break;
    }
    if(txt == first) return 1;
    DoubleResult dr = parse_double(first, txt-first);
    if(dr.errored) return 1;
    *result = dr.result;
    return 0;
}

DRSP_INTERNAL
FORMULAFUNC(drsp_num){
    if(argc != 1 && argc != 2) return Error(ctx, "num() requires 1 or 2 arguments");
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    double default_ = 0.;
    if(argc == 2){
        Expression* d = evaluate_expr(ctx, sd, argv[1], caller_row, caller_col);
        if(!d || d->kind == EXPR_ERROR)
            return d;
        if(d->kind != EXPR_NUMBER)
            return Error(ctx, "second argument to num() must be a number");
        default_ = ((Number*)d)->value;
        buff_set(ctx->a, bc);
    }
    Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(expr_is_arraylike(arg)){
        arg = convert_to_computed_array(ctx, sd, arg, caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR)
            return arg;
        ComputedArray* cc = (ComputedArray*)arg;
        for(intptr_t i = 0; i < cc->length; i++){
            Expression* e = cc->data[i];
            if(e->kind == EXPR_NUMBER)
                continue;
            Number* n = expr_alloc(ctx, EXPR_NUMBER);
            if(!n) return NULL;
            if(e->kind == EXPR_STRING){
                String* s = (String*)e;
                int err = parse_leading_double(s->str->data, s->str->length, &n->value);
                if(!err){
                    cc->data[i] = &n->e;
                    continue;
                }
            }
            n->value = default_;
            cc->data[i] = &n->e;
        }
        return arg;
    }
    else {
        double value;
        if(arg->kind == EXPR_NUMBER)
            value = ((Number*)arg)->value;
        else if(arg->kind == EXPR_STRING){
            String* s = (String*)arg;
            int err = parse_leading_double(s->str->data, s->str->length, &value);
            if(err) value = default_;
        }
        else {
            value = default_;
        }
        buff_set(ctx->a, bc);
        Number* n = expr_alloc(ctx, EXPR_NUMBER);
        if(!n) return NULL;
        n->value = value;
        return &n->e;
    }
}
DRSP_INTERNAL
FORMULAFUNC(drsp_try){
    if(argc != 2) return Error(ctx, "try() requires 2 arguments");
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    if(!arg) return NULL;
    if(arg->kind != EXPR_ERROR) return arg;
    buff_set(ctx->a, bc);
    return evaluate_expr(ctx, sd, argv[1], caller_row, caller_col);
}

DRSP_INTERNAL
FORMULAFUNC(drsp_cell){
    SheetData* fsd = sd;
    if(argc != 1 && argc != 2 && argc != 3) return Error(ctx, "cell() requires 1, 2, or 3 arguments");
    if(argc == 1){
        // cell('cell name')
        Expression* name = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
        if(!name || name->kind == EXPR_ERROR) return name;
        if(name->kind != EXPR_STRING) return Error(ctx, "argument to cell() with one argument must be a string");
        DrspAtom a = drsp_atom_lower(ctx, ((String*)name)->str);
        if(!a) return NULL;
        const NamedCell* cell = get_named_cell(&sd->named_cells, a);
        if(!cell) return Error(ctx, "named cell not found in call to cell()");
        return evaluate(ctx, sd, cell->row, cell->col);
    }
    if(argc == 2){
        // either
        // cell('col', row)
        // or
        // cell('sheet', 'cell name')
        Expression* arg1 = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
        if(!arg1 || arg1->kind == EXPR_ERROR) return arg1;
        if(arg1->kind != EXPR_STRING) return Error(ctx, "first argument to cell() with two arguments must be a string");
        DrspAtom a = drsp_atom_lower(ctx, ((String*)arg1)->str);
        if(!a) return NULL;

        Expression* arg2 = evaluate_expr(ctx, sd, argv[1], caller_row, caller_col);
        if(!arg2 || arg2->kind == EXPR_ERROR) return arg2;
        if(arg2->kind == EXPR_STRING){
            fsd = sheet_lookup_by_name(ctx, a);
            if(!fsd) return Error(ctx, "sheet not found in call to cell()");
            a = drsp_atom_lower(ctx, ((String*)arg2)->str);
            if(!a) return NULL;
            const NamedCell* cell = get_named_cell(&fsd->named_cells, a);
            if(!cell) return Error(ctx, "named cell not found in call to cell()");
            if(fsd != sd){
                int err = sheet_add_dependant(ctx, fsd, sd->handle);
                if(err) return Error(ctx, "oom");
            }
            return evaluate(ctx, fsd, cell->row, cell->col);
        }
        else {
            intptr_t col_idx = sp_name_to_col_idx(fsd, a);
            if(col_idx < 0) return Error(ctx, "column name not found in call to cell()");

            if(arg2->kind != EXPR_NUMBER) return Error(ctx, "second argument to cell() with two arguments must be a number");
            intptr_t row_idx = (intptr_t)((Number*)arg2)->value;
            row_idx--;
            if(row_idx < 0) return Error(ctx, "row index out of bounds in call to cell()");
            return evaluate(ctx, fsd, row_idx, col_idx);
        }
    }
    assert(argc == 3);
    // cell('sheet', 'col', row);
    {
        Expression* sheet = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
        if(!sheet || sheet->kind == EXPR_ERROR) return sheet;
        if(sheet->kind != EXPR_STRING) return Error(ctx, "first argument to cell() with three arguments must be a string");
        DrspAtom a = drsp_atom_lower(ctx, ((String*)sheet)->str);
        if(!a) return NULL;
        fsd = sheet_lookup_by_name(ctx, a);
        if(!fsd) return Error(ctx, "sheet not found in call to cell()");
    }

    intptr_t col_idx;
    {
        Expression* col = evaluate_expr(ctx, sd, argv[1], caller_row, caller_col);
        if(!col || col->kind == EXPR_ERROR) return col;
        if(col->kind != EXPR_STRING) return Error(ctx, "second argument to cell() with three arguments must be a string");
        DrspAtom a = drsp_atom_lower(ctx, ((String*)col)->str);
        if(!a) return NULL;
        col_idx = sp_name_to_col_idx(fsd, a);
        if(col_idx < 0) return Error(ctx, "column name not found in call to cell()");
    }

    intptr_t row_idx;
    {
        Expression* row = evaluate_expr(ctx, sd, argv[2], caller_row, caller_col);
        if(!row || row->kind == EXPR_ERROR) return row;
        if(row->kind != EXPR_NUMBER) return Error(ctx, "third argument to cell() with three arguments must be a number");
        row_idx = (intptr_t)((Number*)row)->value;
        row_idx--;
        if(row_idx < 0) return Error(ctx, "row index out of bounds in call to cell()");
    }
    if(fsd != sd){
        int err = sheet_add_dependant(ctx, fsd, sd->handle);
        if(err) return Error(ctx, "oom");
    }
    return evaluate(ctx, fsd, row_idx, col_idx);
}

DRSP_INTERNAL
FORMULAFUNC(drsp_col){
    // This one is kind of complicated, but it accepts up to 4 args
    // col('Sheetname', 'Colname', rowstart, rowend)
    // col('Sheetname', 'Colname', rowstart)
    // col('Sheetname', 'Colname')
    // col('Colname', rowstart, rowend)
    // col('Colname', rowstart)
    // col('Colname')
    //
    // It's basically the dynamic form of range literals.
    if(!argc || argc > 4) return Error(ctx, "col() requires 1-4 arguments");
    DrspAtom colname = NULL, sheetname = NULL;
    intptr_t rowstart = IDX_UNSET, rowend = IDX_UNSET;
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    {
        Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        if(arg->kind != EXPR_STRING && arg->kind != EXPR_BLANK) return Error(ctx, "first argument to col() must be a string");
        if(arg->kind == EXPR_BLANK)
            colname = drsp_nil_atom();
        else
            colname = drsp_intern_str_lower(ctx, ((String*)arg)->str->data, ((String*)arg)->str->length);
        if(!colname) return NULL;
        argc--, argv++;
    }
    if(argc){
        Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        if(arg->kind == EXPR_STRING){
            sheetname = colname;
            colname = drsp_intern_str_lower(ctx, ((String*)arg)->str->data, ((String*)arg)->str->length);
            if(!colname) return NULL;
        }
        else if(arg->kind == EXPR_NUMBER){
            rowstart = (intptr_t)((Number*)arg)->value;
        }
        else {
            return Error(ctx, "second argument to col() must be a string or a number");
        }
        argc--, argv++;
    }
    if(argc){
        Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        if(arg->kind != EXPR_NUMBER)
            return Error(ctx, "third argument to col() must be a number");
        if(rowstart == IDX_UNSET)
            rowstart = (intptr_t)((Number*)arg)->value;
        else
            rowend = (intptr_t)((Number*)arg)->value;
        argc--, argv++;
    }
    if(argc){
        if(rowend != IDX_UNSET)
            return Error(ctx, "second argument to col() with four arguments must be a string");
        Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        if(arg->kind != EXPR_NUMBER)
            return Error(ctx, "fourth argument to col() must be a number");
        rowend = (intptr_t)((Number*)arg)->value;
        argc--, argv++;
    }
    // idk
    // if(!colname->length) return Error(ctx, "");
    if(rowstart == IDX_UNSET)
        rowstart = 0;
    if(rowstart) rowstart--;
    if(rowend == IDX_UNSET)
        rowend = -1;
    else if(rowend) rowend--;
    if(argc) return Error(ctx, "Too many args"); // Unreachable??
    buff_set(ctx->a, bc);
    if(sheetname && sheetname->length){
        ForeignRange1DColumn* result = expr_alloc(ctx, EXPR_RANGE1D_COLUMN_FOREIGN);
        if(!result) return NULL;
        result->r.col_name = colname;
        result->sheet_name = sheetname;
        result->r.row_start = rowstart;
        result->r.row_end = rowend;
        return &result->e;
    }
    else {
        Range1DColumn* result = expr_alloc(ctx, EXPR_RANGE1D_COLUMN);
        if(!result) return NULL;
        result->col_name = colname;
        result->row_end = rowend;
        result->row_start = rowstart;
        return &result->e;
    }
}

DRSP_INTERNAL
FORMULAFUNC(drsp_row){
    // This expresses a row range literal, which we don't have.
    if(argc != 3 && argc != 4) return Error(ctx, "");
    DrspAtom startcol = NULL, endcol = NULL, sheetname = NULL;
    intptr_t row_idx;
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    if(argc == 4){
        // first arg is sheet name
        Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        if(arg->kind != EXPR_STRING) return Error(ctx, "");
        sheetname = drsp_intern_str_lower(ctx, ((String*)arg)->str->data, ((String*)arg)->str->length);
        if(!sheetname) return NULL;
        buff_set(ctx->a, bc);
        argc--, argv++;
    }
    {
        // first arg is first colname
        Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        if(arg->kind == EXPR_BLANK)
            startcol = drsp_nil_atom();
        else{
            if(arg->kind != EXPR_STRING) return Error(ctx, "");
            startcol = drsp_intern_str_lower(ctx, ((String*)arg)->str->data, ((String*)arg)->str->length);
            if(!startcol) return NULL;
        }
        buff_set(ctx->a, bc);
        argc--, argv++;
    }
    {
        // second arg is second colname
        Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        if(arg->kind == EXPR_BLANK)
            endcol = drsp_nil_atom();
        else {
            if(arg->kind != EXPR_STRING) return Error(ctx, "");
            endcol = drsp_intern_str_lower(ctx, ((String*)arg)->str->data, ((String*)arg)->str->length);
            if(!endcol) return NULL;
        }
        buff_set(ctx->a, bc);
        argc--, argv++;
    }
    {
        // last arg is row idx
        Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        if(arg->kind != EXPR_NUMBER) return Error(ctx, "");
        // XXX overflow checking
        row_idx = (intptr_t)((Number*)arg)->value;
        if(row_idx > 0) row_idx--;
        buff_set(ctx->a, bc);
        argc--, argv++;
    }
    if(sheetname){
        ForeignRange1DRow* result = expr_alloc(ctx, EXPR_RANGE1D_ROW_FOREIGN);
        if(!result) return NULL;
        result->r.col_start = startcol;
        result->r.col_end = endcol;
        result->r.row_idx = row_idx;
        result->sheet_name = sheetname;
        return &result->e;
    }
    else {
        Range1DRow* result = expr_alloc(ctx, EXPR_RANGE1D_ROW);
        if(!result) return NULL;
        result->col_start = startcol;
        result->col_end = endcol;
        result->row_idx = row_idx;
        return &result->e;
    }
}

DRSP_INTERNAL
FORMULAFUNC(drsp_eval){
    if(argc != 1) return Error(ctx, "eval() accepts 1 argument");
    Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(expr_is_arraylike(arg)){
        arg = convert_to_computed_array(ctx, sd, arg, caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        ComputedArray* c = (ComputedArray*)arg;
        for(intptr_t i = 0; i < c->length; i++){
            Expression* e = c->data[i];
            if(e->kind == EXPR_BLANK) continue;
            if(e->kind != EXPR_STRING) return Error(ctx, "argument to eval() must be a string");
            DrspAtom a = ((String*)e)->str;
            Expression* ev = evaluate_string(ctx, sd, a->data, a->length, caller_row, caller_col);
            if(!ev || ev->kind == EXPR_ERROR) return ev;
            c->data[i] = ev;
        }
        return &c->e;
    }
    if(arg->kind != EXPR_STRING) return Error(ctx, "argument to eval() must be a string");
    DrspAtom a = ((String*)arg)->str;
    Expression* result = evaluate_string(ctx, sd, a->data, a->length, caller_row, caller_col);
    return result;
}

DRSP_INTERNAL
FORMULAFUNC(drsp_pow){
    if(argc != 2) return Error(ctx, "pow() requires 2 arguments");
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(expr_is_arraylike(arg)){
        arg = convert_to_computed_array(ctx, sd, arg, caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        BuffCheckpoint bc = buff_checkpoint(ctx->a);
        Expression* arg2 = evaluate_expr(ctx, sd, argv[1], caller_row, caller_col);
        if(!arg2 || arg2->kind == EXPR_ERROR) return arg;
        ComputedArray* c = (ComputedArray*)arg;
        if(arg2->kind == EXPR_NUMBER){
            double exp = ((Number*)arg2)->value;
            for(intptr_t i = 0; i < c->length; i++){
                Expression* e = c->data[i];
                if(e->kind == EXPR_BLANK) continue;
                if(e->kind != EXPR_NUMBER)
                    return Error(ctx, "argument 1 to pow() must be a number");
                Number* n = (Number*)e;
                n->value = __builtin_pow(n->value, exp);
            }
        }
        else if(expr_is_arraylike(arg2)){
            arg2 = convert_to_computed_array(ctx, sd, arg2, caller_row, caller_col);
            if(!arg2 || arg2->kind == EXPR_BLANK)
                return arg2;
            ComputedArray* exps = (ComputedArray*)arg2;
            if(c->length != exps->length)
                return Error(ctx, "both arguments to pow() must have the same length");
            for(intptr_t i = 0; i < c->length; i++){
                Expression* base = c->data[i];
                Expression* ex = exps->data[i];
                if(base->kind == EXPR_BLANK)
                    continue;
                if(base->kind != EXPR_NUMBER)
                    return Error(ctx, "argument 1 to pow() must be a number");
                if(ex->kind != EXPR_NUMBER)
                    return Error(ctx, "argument 2 to pow() must be a number");
                Number* n = (Number*)base;
                double ex_ = ((Number*)ex)->value;
                n->value = __builtin_pow(n->value, ex_);
            }
        }
        else {
            return Error(ctx, "argument 2 to pow() must be a number");
        }
        buff_set(ctx->a, bc);
        return arg;
    }
    else {
        if(arg->kind != EXPR_NUMBER) {
            buff_set(ctx->a, bc);
            return Error(ctx, "argument 1 to pow() must be a number");
        }
        Expression* arg2 = evaluate_expr(ctx, sd, argv[1], caller_row, caller_col);
        if(!arg2 || arg2 -> kind == EXPR_ERROR){
            buff_set(ctx->a, bc);
            return arg2;
        }
        if(arg2->kind != EXPR_NUMBER){
            buff_set(ctx->a, bc);
            return Error(ctx, "argument 2 to pow() must be a number");
        }
        double val = __builtin_pow(((Number*)arg)->value, ((Number*)arg2)->value);
        buff_set(ctx->a, bc);
        Number* n = expr_alloc(ctx, EXPR_NUMBER);
        if(!n) return NULL;
        n->value = val;
        return &n->e;
    }
}

DRSP_INTERNAL
FORMULAFUNC(drsp_cat){
    if(argc < 2) return Error(ctx, "cat() requires at least 2 arguments");
    DrspAtom catbuff[4];
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    if(argc == 2){
        Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        if(expr_is_arraylike(arg)){
            arg = convert_to_computed_array(ctx, sd, arg, caller_row, caller_col);
            if(!arg || arg->kind == EXPR_ERROR) return arg;
            BuffCheckpoint bc = buff_checkpoint(ctx->a);
            Expression* arg2 = evaluate_expr(ctx, sd, argv[1], caller_row, caller_col);
            if(!arg2 || arg2->kind == EXPR_ERROR) return arg;
            ComputedArray* c = (ComputedArray*)arg;
            if(arg2->kind == EXPR_BLANK){
                for(intptr_t i = 0; i < c->length; i++){
                    Expression* e = c->data[i];
                    if(e->kind == EXPR_BLANK || e->kind == EXPR_STRING)
                        continue;
                    return Error(ctx, "argument 1 to cat() must be a string");
                }
            }
            else if(arg2->kind == EXPR_STRING){
                catbuff[1] = ((String*)arg2)->str;
                for(intptr_t i = 0; i < c->length; i++){
                    Expression* e = c->data[i];
                    if(e->kind == EXPR_BLANK){
                        c->data[i] = arg2;
                        continue;
                    }
                    if(e->kind != EXPR_STRING)
                        return Error(ctx, "argument 1 to cat() must be a string");
                    String* s = (String*)e;
                    catbuff[0] = s->str;
                    int err = sv_cat(ctx, 2, catbuff, &s->str);
                    if(err) return Error(ctx, "oom");
                }
            }
            else if(expr_is_arraylike(arg2)){
                arg2 = convert_to_computed_array(ctx, sd, arg2, caller_row, caller_col);
                if(!arg2 || arg2->kind == EXPR_ERROR)
                    return arg2;
                ComputedArray* rights = (ComputedArray*)arg2;
                if(c->length != rights->length)
                    return Error(ctx, "arguments to cat() must be the same length");
                for(intptr_t i = 0; i < c->length; i++){
                    Expression* l = c->data[i];
                    Expression* r = rights->data[i];
                    if(l->kind != EXPR_BLANK && l->kind != EXPR_STRING)
                        return Error(ctx, "argument 1 to cat() must be a string");
                    if(r->kind != EXPR_BLANK && r->kind != EXPR_STRING)
                        return Error(ctx, "argument 2 to cat() must be a string");
                    if(l->kind == EXPR_BLANK){
                        c->data[i] = r;
                        continue;
                    }
                    if(r->kind == EXPR_BLANK)
                        continue;
                    assert(l->kind == EXPR_STRING);
                    assert(r->kind == EXPR_STRING);
                    String* s = (String*)l;
                    catbuff[0] = s->str;
                    catbuff[1] = ((String*)r)->str;
                    int err = sv_cat(ctx, 2, catbuff, &s->str);
                    if(err) return Error(ctx, "oom");
                }
            }
            else {
                return Error(ctx, "argument 2 to cat() must be a string");
            }
            buff_set(ctx->a, bc);
            return arg;
        }
        else {
            if(arg->kind != EXPR_STRING && arg->kind != EXPR_BLANK) {
                buff_set(ctx->a, bc);
                return Error(ctx, "argument 1 to cat() must be a string");
            }
            Expression* arg2 = evaluate_expr(ctx, sd, argv[1], caller_row, caller_col);
            if(!arg2 || arg2->kind == EXPR_ERROR){
                buff_set(ctx->a, bc);
                return arg2;
            }
            if(expr_is_arraylike(arg2)){
                arg2 = convert_to_computed_array(ctx, sd, arg2, caller_row, caller_col);
                if(!arg2 || arg2->kind == EXPR_ERROR) return arg2;
                ComputedArray* c = (ComputedArray*)arg2;
                if(arg->kind != EXPR_BLANK)
                    catbuff[0] = ((String*)arg)->str;
                for(intptr_t i = 0; i < c->length; i++){
                    Expression* e = c->data[i];
                    if(e->kind == EXPR_BLANK) continue;
                    if(e->kind != EXPR_STRING)
                        return Error(ctx, "argument 2 to cat() must be a string");
                    if(arg->kind == EXPR_BLANK) continue;
                    String* s = (String*)e;
                    catbuff[1] = s->str;
                    int err = sv_cat(ctx, 2, catbuff, &s->str);
                    if(err) return Error(ctx, "oom");
                }
                return arg2;
            }
            else {
                if(arg2->kind == EXPR_BLANK){
                    if(arg->kind == EXPR_BLANK){
                        buff_set(ctx->a, bc);
                        return arg;
                    }
                    DrspAtom v = ((String*)arg)->str;
                    buff_set(ctx->a, bc);
                    String* s = expr_alloc(ctx, EXPR_STRING);
                    if(!s) return NULL;
                    s->str = v;
                    return &s->e;
                }
                if(arg2->kind != EXPR_STRING){
                    buff_set(ctx->a, bc);
                    return Error(ctx, "argument 2 to cat() must be a string");
                }
                if(arg->kind == EXPR_BLANK){
                    DrspAtom v = ((String*)arg2)->str;
                    buff_set(ctx->a, bc);
                    String* s = expr_alloc(ctx, EXPR_STRING);
                    if(!s) return NULL;
                    s->str = v;
                    return &s->e;
                }
                DrspAtom v;
                catbuff[0] = ((String*)arg)->str;
                catbuff[1] = ((String*)arg2)->str;
                int err = sv_cat(ctx, 2, catbuff, &v);
                buff_set(ctx->a, bc);
                if(err) return Error(ctx, "oom");
                String* s = expr_alloc(ctx, EXPR_STRING);
                if(!s) return NULL;
                s->str = v;
                return &s->e;
            }
        }
    }
    else {
        _Bool is_arraylike = false;
        intptr_t column_length = 0;
        for(int i = 0; i < argc; i++){
            argv[i] = evaluate_expr(ctx, sd, argv[i], caller_row, caller_col);
            if(!argv[i] || argv[i]->kind == EXPR_ERROR){
                buff_set(ctx->a, bc);
                return argv[i];
            }
            if(expr_is_arraylike(argv[i])){
                is_arraylike = true;
                argv[i] = convert_to_computed_array(ctx, sd, argv[i], caller_row, caller_col);
                if(!argv[i] || argv[i]->kind == EXPR_ERROR){
                    buff_set(ctx->a, bc);
                    return argv[i];
                }
                ComputedArray* c = (ComputedArray*)argv[i];
                if(c->length > column_length){
                    column_length = c->length;
                }
            }
            else if(argv[i]->kind != EXPR_STRING && argv[i]->kind != EXPR_BLANK){
                buff_set(ctx->a, bc);
                return Error(ctx, "arguments to cat() must be a string");
            }
        }
        if(is_arraylike){
            if(!column_length)
                return Error(ctx, "arguments to cat must be non-zero length");
            ComputedArray* result = computed_array_alloc(ctx, column_length);
            if(!result) return Error(ctx, "oom");
            for(intptr_t r = 0; r < column_length; r++){
                for(int i = 0; i < argc; i++){
                    Expression* e = argv[i];
                    if(e->kind == EXPR_STRING){
                        catbuff[i] = ((String*)e)->str;
                    }
                    else if(e->kind == EXPR_BLANK){
                        catbuff[i] = drsp_nil_atom();
                    }
                    else {
                        ComputedArray* c = (ComputedArray*)argv[i];
                        if(r >= c->length){
                            catbuff[i] = drsp_nil_atom();
                        }
                        else {
                            Expression* item = c->data[r];
                            if(item->kind == EXPR_STRING){
                                catbuff[i] = ((String*)item)->str;
                            }
                            else if(item->kind == EXPR_BLANK){
                                catbuff[i] = drsp_nil_atom();
                            }
                            else {
                                buff_set(ctx->a, bc);
                                return Error(ctx, "arguments to cat() must be strings");
                            }
                        }
                    }
                }
                String* s = expr_alloc(ctx, EXPR_STRING);
                if(!s) return NULL;
                int err = sv_cat(ctx, argc, catbuff, &s->str);
                if(err){
                    buff_set(ctx->a, bc);
                    return Error(ctx, "oom");
                }
                result->data[r] = &s->e;
            }
            return &result->e;
        }
        else {
            for(int i = 0; i < argc; i++){
                Expression* e = argv[i];
                if(e->kind == EXPR_BLANK){
                    catbuff[i] = drsp_nil_atom();
                }
                else {
                    catbuff[i] = ((String*)e)->str;
                }
            }
            DrspAtom sv;
            int err = sv_cat(ctx, argc, catbuff, &sv);
            buff_set(ctx->a, bc);
            if(err) return Error(ctx, "oom");
            String* result = expr_alloc(ctx, EXPR_STRING);
            if(!result) return NULL;
            result->str = sv;
            return &result->e;
        }
    }
}

static inline
Expression*_Nullable
arraylike_tablelookup(DrSpreadCtx* ctx, SheetData* sd, intptr_t caller_row, intptr_t caller_col, Expression* needle, int argc, Expression*_Nonnull*_Nonnull argv){
    needle = convert_to_computed_array(ctx, sd, needle, caller_row, caller_col);
    if(!needle || needle->kind == EXPR_ERROR)
        return needle;
    ComputedArray* cneedle = (ComputedArray*)needle;
    // TODO: Use checkpoints.
    // TODO: lazier computation.
    // BuffCheckpoint bc = buff_checkpoint(ctx->a);
    Expression* haystack = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    if(!haystack || haystack->kind == EXPR_ERROR)
        return haystack;
    haystack = convert_to_computed_array(ctx, sd, haystack, caller_row, caller_col);
    if(!haystack || haystack->kind == EXPR_ERROR)
        return haystack;
    ComputedArray* chaystack = (ComputedArray*)haystack;
    argc--, argv++;
    Expression* values = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    if(!values || values->kind == EXPR_ERROR)
        return values;
    values = convert_to_computed_array(ctx, sd, values, caller_row, caller_col);
    if(!values || values->kind == EXPR_ERROR)
        return values;
    ComputedArray* cvalues = (ComputedArray*)values;
    argc--, argv++;
    Expression* default_ = NULL;
    for(intptr_t i = 0; i < cneedle->length; i++){
        Expression* n = cneedle->data[i];
        ExpressionKind nkind = n->kind;
        if(nkind == EXPR_BLANK)
            continue;
        if(nkind != EXPR_NUMBER && nkind != EXPR_STRING)
            return Error(ctx, "");
        intptr_t idx;
        const intptr_t haylength = chaystack->length;
        Expression*_Nonnull* data = chaystack->data;
        if(nkind == EXPR_NUMBER){
            double d = ((Number*)n)->value;
            for(idx = 0; idx < haylength; idx++){
                const Expression* h = data[idx];
                if(h->kind != EXPR_NUMBER) continue;
                if(d == ((Number*)h)->value)
                    break;
            }
        }
        else {
            DrspAtom s = ((String*)n)->str;
            for(idx = 0; idx < haylength; idx++){
                const Expression* h = data[idx];
                if(h->kind != EXPR_STRING) continue;
                if(s == ((String*)h)->str)
                    break;
            }
        }
        if(idx == haylength){
            if(!argc) return Error(ctx, "needle (argument 1) not found in haystack (argument 2) in tlu()");
            if(!default_){
                default_ = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
                if(!default_ || default_ == EXPR_ERROR) return default_;
                cneedle->data[i] = default_;
            }
        }
        else {
            if(idx >= cvalues->length)
                return Error(ctx, "position of needle in haystack outside the bounds of values in tlu()");
            cneedle->data[i] = cvalues->data[idx];
        }
    }
    return needle;
}

DRSP_INTERNAL
FORMULAFUNC(drsp_tablelookup){
    // "needle", [haystack], [values]
    if(argc != 3 && argc != 4) return Error(ctx, "tlu() requires 3 or 4 arguments");
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    ExpressionKind nkind;
    union {
        DrspAtom s;
        double d;
    } nval = {0};
    {
        Expression* needle = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
        if(!needle || needle->kind == EXPR_ERROR) return needle;
        if(expr_is_arraylike(needle))
            return arraylike_tablelookup(ctx, sd, caller_row, caller_col, needle, argc-1, argv+1);
        nkind = needle->kind;
        if(nkind == EXPR_BLANK){
            if(argc == 4){
                return evaluate_expr(ctx, sd, argv[3], caller_row, caller_col);
            }
        }
        if(nkind != EXPR_NUMBER && nkind != EXPR_STRING) return Error(ctx, "argument 1 to tlu() must be a number or string");
        if(nkind == EXPR_NUMBER)
            nval.d = ((Number*)needle)->value;
        else
            nval.s = ((String*)needle)->str;
        buff_set(ctx->a, bc);
    }
    intptr_t offset = -1;
    {
        Expression* haystack = evaluate_expr(ctx, sd, argv[1], caller_row, caller_col);
        if(!haystack || haystack->kind == EXPR_ERROR) return haystack;
        if(haystack->kind == EXPR_COMPUTED_ARRAY){
            ComputedArray* c = (ComputedArray*)haystack;
            if(nkind == EXPR_STRING){
                for(intptr_t i = 0; i < c->length; i++){
                    Expression* e = c->data[i];
                    if(e->kind != EXPR_STRING) continue;
                    if(nval.s == ((String*)e)->str){
                        offset = i;
                        break;
                    }
                }
            }
            else {
                for(intptr_t i = 0; i < c->length; i++){
                    Expression* e = c->data[i];
                    if(e->kind != EXPR_NUMBER) continue;
                    if(nval.d == ((Number*)e)->value){
                        offset = i;
                        break;
                    }
                }
            }
        }
        else if(haystack->kind == EXPR_RANGE1D_COLUMN || haystack->kind == EXPR_RANGE1D_COLUMN_FOREIGN){
            intptr_t col, start, end;
            SheetData* rsd = sd;
            if(get_range1dcol(ctx, sd, haystack, &col, &start, &end, &rsd, caller_row, caller_col) != 0)
                return Error(ctx, "Invalid range for haystack of tlu()");

            BuffCheckpoint loopbc = buff_checkpoint(ctx->a);
            if(nkind == EXPR_STRING){
                for(intptr_t row = start; row <= end; buff_set(ctx->a,loopbc), row++){
                    Expression* e = evaluate(ctx, rsd, row, col);
                    if(!e) return e;
                    if(e->kind != EXPR_STRING) continue;
                    if(nval.s == ((String*)e)->str){
                        offset = row - start;
                        break;
                    }
                }
            }
            else {
                for(intptr_t row = start; row <= end; buff_set(ctx->a, loopbc), row++){
                    Expression* e = evaluate(ctx, rsd, row, col);
                    if(!e) return e;
                    if(e->kind != EXPR_NUMBER) continue;
                    if(nval.d == ((Number*)e)->value){
                        offset = row - start;
                        break;
                    }
                }
            }
        }
        else if(haystack->kind == EXPR_RANGE1D_ROW || haystack->kind == EXPR_RANGE1D_ROW_FOREIGN){
            intptr_t row, start, end;
            SheetData* rsd = sd;
            if(get_range1drow(ctx, sd, haystack, &row, &start, &end, &rsd, caller_row, caller_col) != 0)
                return Error(ctx, "Invalid range for haystack of tlu()");

            BuffCheckpoint loopbc = buff_checkpoint(ctx->a);
            if(nkind == EXPR_STRING){
                for(intptr_t col = start; col <= end; buff_set(ctx->a,loopbc), col++){
                    Expression* e = evaluate(ctx, rsd, row, col);
                    if(!e) return e;
                    if(e->kind != EXPR_STRING) continue;
                    if(nval.s == ((String*)e)->str){
                        offset = col - start;
                        break;
                    }
                }
            }
            else {
                for(intptr_t col = start; col <= end; buff_set(ctx->a, loopbc), col++){
                    Expression* e = evaluate(ctx, rsd, row, col);
                    if(!e) return e;
                    if(e->kind != EXPR_NUMBER) continue;
                    if(nval.d == ((Number*)e)->value){
                        offset = col - start;
                        break;
                    }
                }
            }
        }
        else {
            return Error(ctx, "haystack is the wrong type");
        }
        buff_set(ctx->a, bc);
        if(offset < 0) {
            if(argc == 4)
                return evaluate_expr(ctx, sd, argv[3], caller_row, caller_col);
            return Error(ctx, "Didn't find needle in haystack");
        }
    }
    {
        Expression* values = evaluate_expr(ctx, sd, argv[2], caller_row, caller_col);
        if(!values || values->kind == EXPR_ERROR) return values;
        if(values->kind == EXPR_COMPUTED_ARRAY){
            ComputedArray* c = (ComputedArray*)values;
            if(offset >= c->length)
                return Error(ctx, "out of bounds");
            return c->data[offset];
        }
        if(values->kind == EXPR_RANGE1D_COLUMN || values->kind == EXPR_RANGE1D_COLUMN_FOREIGN){
            intptr_t col, start, end;
            SheetData* rsd = sd;
            if(get_range1dcol(ctx, sd, values, &col, &start, &end, &rsd, caller_row, caller_col) != 0)
                return Error(ctx, "invalid column range");
            buff_set(ctx->a, bc);
            if(start+offset > end) return Error(ctx, "out of bounds");
            return evaluate(ctx, rsd, start+offset, col);
        }
        else {
            intptr_t row, start, end;
            SheetData* rsd = sd;
            if(get_range1drow(ctx, sd, values, &row, &start, &end, &rsd, caller_row, caller_col) != 0)
                return Error(ctx, "invalid row range");
            buff_set(ctx->a, bc);
            if(start+offset > end) return Error(ctx, "out of bounds");
            return evaluate(ctx, rsd, row, start+offset);
        }
    }
}

DRSP_INTERNAL
FORMULAFUNC(drsp_find){
    // "needle", [haystack]
    if(argc != 2 && argc != 3) return Error(ctx, "find() requires 2 or 3 arguments");
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    ExpressionKind nkind;
    union {
        DrspAtom s;
        double d;
    } nval = {0};
    {
        Expression* needle = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
        if(!needle || needle->kind == EXPR_ERROR) return needle;
        nkind = needle->kind;
        if(nkind != EXPR_NUMBER && nkind != EXPR_STRING && nkind != EXPR_BLANK) return Error(ctx, "first argument to find() must be a number or a string");
        if(nkind == EXPR_NUMBER)
            nval.d = ((Number*)needle)->value;
        else if(nkind == EXPR_STRING)
            nval.s = ((String*)needle)->str;
        buff_set(ctx->a, bc);
    }
    intptr_t offset = -1;
    {
        Expression* haystack = evaluate_expr(ctx, sd, argv[1], caller_row, caller_col);
        if(!haystack || haystack->kind == EXPR_ERROR) return haystack;
        if(haystack->kind == EXPR_COMPUTED_ARRAY){
            ComputedArray* cc = (ComputedArray*)haystack;
            for(intptr_t i = 0; i < cc->length; i++){
                Expression* e = cc->data[i];
                if(e->kind != nkind) continue;
                if(nkind == EXPR_BLANK){
                    offset = i;
                    break;
                }
                if(nkind == EXPR_STRING){
                    if(nval.s == ((String*)e)->str){
                        offset = i;
                        break;
                    }
                }
                else if(nkind == EXPR_NUMBER){
                    if(nval.d == ((Number*)e)->value){
                        offset = i;
                        break;
                    }
                }
            }
        }
        else if(haystack->kind == EXPR_RANGE1D_COLUMN || haystack->kind == EXPR_RANGE1D_COLUMN_FOREIGN){
            intptr_t col, start, end;
            SheetData* rsd = sd;
            if(get_range1dcol(ctx, sd, haystack, &col, &start, &end, &rsd, caller_row, caller_col) != 0)
                return Error(ctx, "Invalid range");

            BuffCheckpoint loopchk = buff_checkpoint(ctx->a);
            for(intptr_t row = start; row <= end; buff_set(ctx->a, loopchk), row++){
                Expression* e = evaluate(ctx, rsd, row, col);
                if(!e) return e;
                if(e->kind != nkind) continue;
                if(nkind == EXPR_BLANK){
                    offset = row - start;
                    break;
                }
                if(nkind == EXPR_STRING){
                    if(nval.s == ((String*)e)->str){
                        offset = row - start;
                        break;
                    }
                }
                else if(nkind == EXPR_NUMBER){
                    if(nval.d == ((Number*)e)->value){
                        offset = row - start;
                        break;
                    }
                }
            }
        }
        else {
            intptr_t row, start, end;
            SheetData* rsd = sd;
            if(get_range1drow(ctx, sd, haystack, &row, &start, &end, &rsd, caller_row, caller_col) != 0)
                return Error(ctx, "Invalid range");

            BuffCheckpoint loopchk = buff_checkpoint(ctx->a);
            for(intptr_t col = start; col <= end; buff_set(ctx->a, loopchk), col++){
                Expression* e = evaluate(ctx, rsd, row, col);
                if(!e) return e;
                if(e->kind != nkind) continue;
                if(nkind == EXPR_BLANK){
                    offset = col - start;
                    break;
                }
                if(nkind == EXPR_STRING){
                    if(nval.s == ((String*)e)->str){
                        offset = col - start;
                        break;
                    }
                }
                else if(nkind == EXPR_NUMBER){
                    if(nval.d == ((Number*)e)->value){
                        offset = col - start;
                        break;
                    }
                }
            }
        }
        buff_set(ctx->a, bc);
    }
    if(offset < 0) {
        if(argc == 3)
            return evaluate_expr(ctx, sd, argv[2], caller_row, caller_col);
        return Error(ctx, "needle not found in haystack in call to find()");
    }
    Number* n = expr_alloc(ctx, EXPR_NUMBER);
    if(!n) return NULL;
    n->value = (double)offset+1;
    return &n->e;
}

DRSP_INTERNAL
FORMULAFUNC(drsp_call){
    if(!argc) return Error(ctx, "call() requires at least 1 argument");
    Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(arg->kind != EXPR_STRING)
        return Error(ctx, "first argument to call() must be a string");
    FormulaFunc* func = lookup_func(((String*)arg)->str);
    if(!func) return Error(ctx, "first argument to call() does not name a function");
    argc--, argv++;
    return func(ctx, sd, caller_row, caller_col, argc, argv);
}

#ifdef DRSP_INTRINS
// GCOV_EXCL_START
DRSP_INTERNAL
FORMULAFUNC(drsp_first){
    if(argc != 1) return Error(ctx, "");
    Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR)
        return arg;
    if(!expr_is_arraylike(arg))
        return Error(ctx, "");
    arg = convert_to_computed_array(ctx, sd, arg, caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR)
        return arg;
    ComputedArray* c = (ComputedArray*)arg;
    if(!c->length)
        return Error(ctx, "");
    return c->data[0];
}

typedef struct PrintBuff PrintBuff;
struct PrintBuff {
    int error;
    intptr_t len;
    char* buff;
};

static inline
void
print(PrintBuff* buff, const char* fmt, ...){
    if(buff->error) return;
    va_list vap;
    va_start(vap, fmt);
    int n = (vsnprintf)(buff->buff, buff->len, fmt, vap);
    if(n < 0) buff->error = 1;
    if(n > buff->len) buff->error = 1;
    else {
        buff->len -= n;
        buff->buff += n;
    }
    va_end(vap);
}


DRSP_INTERNAL
void
repr_expr(PrintBuff* buff, Expression* arg){
    switch(arg->kind){
        case EXPR_ERROR:
            print(buff, "Error()");
            break;
        case EXPR_STRING:{
            String* s = (String*)arg;
            print(buff, "String('%.*s')", (int)s->str->length, s->str->data);
        }break;
        case EXPR_BLANK:
            print(buff, "Null()\n");
            break;
        case EXPR_NUMBER:{
            Number* n = (Number*)arg;
            print(buff, "Number(%g)", n->value);
        }break;
        case EXPR_FUNCTION_CALL:{
            FunctionCall* f = (FunctionCall*)arg;
            // TODO: print function name
            print(buff, "FunctionCall(");
            for(int i = 0; i < f->argc; i++){
                if(i != 0){
                    print(buff, ", ");
                }
                repr_expr(buff, f->argv[i]);
            }
            print(buff, ")");
        }break;
        case EXPR_USER_DEFINED_FUNC_CALL:{
            UserFunctionCall* f = (UserFunctionCall*)arg;
            // TODO: print function name
            print(buff, "UserFuncCall-%.*s(", (int)f->name->length, f->name->data);
            for(int i = 0; i < f->argc; i++){
                if(i != 0){
                    print(buff, ", ");
                }
                repr_expr(buff, f->argv[i]);
            }
            print(buff, ")");
        }break;
        case EXPR_RANGE0D:{
            print(buff, "R0(");
            Range0D* rng = (Range0D*)arg;
            if(rng->row == IDX_DOLLAR)
                print(buff, "[%.*s, $]", (int)rng->col_name->length, rng->col_name->data);
            else
                print(buff, "[%.*s, %zd]", (int)rng->col_name->length, rng->col_name->data, rng->row);
            print(buff, ")");
        }break;
        case EXPR_RANGE0D_FOREIGN:{
            print(buff, "R0F(");
            ForeignRange0D* rng = (ForeignRange0D*)arg;
            print(buff, "[%.*s, %.*s, %zd]",
                    (int)rng->sheet_name->length, rng->sheet_name->data,
                    (int)rng->r.col_name->length, rng->r.col_name->data,
                    rng->r.row);
            print(buff, ")");
        }break;
        case EXPR_RANGE1D_COLUMN:{
            print(buff, "R1C(");
            Range1DColumn* rng = (Range1DColumn*)arg;
            if(rng->row_start == IDX_DOLLAR && rng->row_end == IDX_DOLLAR){
                print(buff, "[%.*s, $:$]",
                        (int)rng->col_name->length, rng->col_name->data);
            }
            else if(rng->row_start == IDX_DOLLAR){
                print(buff, "[%.*s, $:%zd]",
                        (int)rng->col_name->length, rng->col_name->data,
                         rng->row_end);
            }
            else if(rng->row_end == IDX_DOLLAR){
                print(buff, "[%.*s, %zd:$]",
                        (int)rng->col_name->length, rng->col_name->data,
                        rng->row_start);
            }
            else {
                print(buff, "[%.*s, %zd:%zd]",
                        (int)rng->col_name->length, rng->col_name->data,
                        rng->row_start, rng->row_end);
            }
            print(buff, ")");
        }break;
        case EXPR_RANGE1D_COLUMN_FOREIGN:{
            print(buff, "R1CF(");
            ForeignRange1DColumn* rng = (ForeignRange1DColumn*)arg;
            print(buff, "[%.*s, %.*s, %zd:%zd]",
                    (int)rng->sheet_name->length, rng->sheet_name->data,
                    (int)rng->r.col_name->length, rng->r.col_name->data,
                    rng->r.row_start, rng->r.row_end);
            print(buff, ")");
        }break;
        case EXPR_RANGE1D_ROW:{
            print(buff, "R1R(");
            Range1DRow* rng = (Range1DRow*)arg;
            if(rng->row_idx == IDX_DOLLAR){
                print(buff, "[%.*s:%.*s, $]",
                        (int)rng->col_start->length, rng->col_start->data,
                        (int)rng->col_end->length, rng->col_end->data);
            }
            else {
                print(buff, "[%.*s:%.*s, %zd]",
                        (int)rng->col_start->length, rng->col_start->data,
                        (int)rng->col_end->length, rng->col_end->data,
                        rng->row_idx);
            }
            print(buff, ")");
        }break;
        case EXPR_RANGE1D_ROW_FOREIGN:{
            print(buff, "R1RF(");
            ForeignRange1DRow* rng = (ForeignRange1DRow*)arg;
            print(buff, "[%.*s, %.*s:%.*s, %zd]",
                    (int)rng->sheet_name->length, rng->sheet_name->data,
                    (int)rng->r.col_start->length, rng->r.col_start->data,
                    (int)rng->r.col_end->length, rng->r.col_end->data,
                    rng->r.row_idx);
            print(buff, ")");
        }break;
        case EXPR_GROUP:{
            Group* g = (Group*)arg;
            print(buff, "Group(");
            repr_expr(buff, g->expr);
            print(buff, ")");
        }break;
        case EXPR_BINARY:{
            Binary* b = (Binary*)arg;
            print(buff, "Binary(");
            repr_expr(buff, b->lhs);
            switch(b->op){
                case BIN_ADD: print(buff, "+"); break;
                case BIN_SUB: print(buff, "-"); break;
                case BIN_MUL: print(buff, "*"); break;
                case BIN_DIV: print(buff, "/"); break;
                case BIN_LT:  print(buff, "<"); break;
                case BIN_LE:  print(buff, "<="); break;
                case BIN_GT:  print(buff, ">"); break;
                case BIN_GE:  print(buff, ">="); break;
                case BIN_EQ:  print(buff, "="); break;
                case BIN_NE:  print(buff, "!="); break;
            }
            repr_expr(buff, b->rhs);
            print(buff, ")");
        }break;
        case EXPR_UNARY:{
            print(buff, "Unary(");
            Unary* u = (Unary*)arg;
            switch(u->op){
                case UN_PLUS: print(buff, "+"); break;
                case UN_NEG: print(buff, "-"); break;
                case UN_NOT: print(buff, "!"); break;
            }
            repr_expr(buff, u->expr);
            print(buff, ")");
        }break;
        case EXPR_COMPUTED_ARRAY:
            print(buff, "ComputedArray()");
            break;
    }
}

DRSP_INTERNAL
FORMULAFUNC(drsp_repr){
    (void)caller_col; (void)caller_row;
    (void)sd;
    char buffer[4092];
    PrintBuff buff = {0, sizeof buffer, buffer};
    for(int i = 0; i < argc; i++){
        Expression* arg = argv[i];
        repr_expr(&buff, arg);
    }
    if(buff.error) return Error(ctx, "");
    DrspAtom str = drsp_intern_str(ctx, buffer, sizeof(buffer) - buff.len);
    if(!str) return NULL;
    String* s = expr_alloc(ctx, EXPR_STRING);
    if(!s) return NULL;
    s->str = str;
    return &s->e;
}
// GCOV_EXCL_STOP
#endif

DRSP_INTERNAL
FORMULAFUNC(drsp_array){
    if(!argc) return Error(ctx, "array() requires arguments");
    ComputedArray* cc = computed_array_alloc(ctx, argc);
    if(!cc) return NULL;
    for(int i = 0; i < argc; i++){
        Expression* e = evaluate_expr(ctx, sd, argv[i], caller_row, caller_col);
        if(!e || e->kind == EXPR_ERROR)
            return e;
        cc->data[i] = e;
    }
    return &cc->e;
}

DRSP_INTERNAL
FORMULAFUNC(drsp_if){
    if(argc != 3) return Error(ctx, "if() requires 3 arguments");
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    Expression* cond = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    if(!cond || cond->kind == EXPR_ERROR)
        return cond;
    if(expr_is_arraylike(cond)){
        cond = convert_to_computed_array(ctx, sd, cond, caller_row, caller_col);
        if(!cond || cond->kind == EXPR_ERROR)
            return cond;
        ComputedArray* cc = (ComputedArray*)cond;
        Expression* t = NULL;
        Expression* f = NULL;
        // col or based on type
        intptr_t tcol_or_row = IDX_UNSET, tstart = IDX_UNSET, tend = IDX_UNSET;
        SheetData* tsd = sd;
        intptr_t fcol_or_row = IDX_UNSET, fstart = IDX_UNSET, fend = IDX_UNSET;
        SheetData* fsd = sd;
        for(intptr_t i = 0; i < cc->length; i++){
            Expression* e = cc->data[i];
            if(evaled_is_not_scalar(e))
                return Error(ctx, "Must be coercible to true/false");
            if(scalar_expr_is_truthy(e)){
                if(!t){
                    t = evaluate_expr(ctx, sd, argv[1], caller_row, caller_col);
                    if(!t || t->kind == EXPR_ERROR)
                        return t;
                }
                if(t->kind == EXPR_COMPUTED_ARRAY){
                    ComputedArray* tc = (ComputedArray*)t;
                    if(i >= tc->length){
                        return Error(ctx, "true range out of bounds");
                    }
                    cc->data[i] = tc->data[i];
                }
                else if(expr_is_scalar(t)){
                    cc->data[i] = t;
                }
                else if(t->kind == EXPR_RANGE1D_COLUMN || t->kind == EXPR_RANGE1D_COLUMN_FOREIGN){
                    if(tcol_or_row == IDX_UNSET){
                        if(get_range1dcol(ctx, sd, t, &tcol_or_row, &tstart, &tend, &tsd, caller_row, caller_col))
                            return Error(ctx, "Invalid range");
                    }
                    if(tstart + i > tend)
                        return Error(ctx, "true range out of bounds");
                    Expression* te = evaluate(ctx, tsd, tstart+i, tcol_or_row);
                    if(!te || te->kind == EXPR_ERROR)
                        return te;
                    cc->data[i] = te;
                }
                else if(t->kind == EXPR_RANGE1D_ROW || t->kind == EXPR_RANGE1D_ROW_FOREIGN){
                    if(tcol_or_row == IDX_UNSET){
                        if(get_range1drow(ctx, sd, t, &tcol_or_row, &tstart, &tend, &tsd, caller_row, caller_col))
                            return Error(ctx, "Invalid range");
                    }
                    if(tstart + i > tend)
                        return Error(ctx, "true range out of bounds");
                    Expression* te = evaluate(ctx, tsd, tcol_or_row, tstart+i);
                    if(!te || te->kind == EXPR_ERROR)
                        return te;
                    cc->data[i] = te;
                }
                else {
                    return Error(ctx, ""); // unreachable?
                }
            }
            else {
                if(!f){
                    f = evaluate_expr(ctx, sd, argv[2], caller_row, caller_col);
                    if(!f || f->kind == EXPR_ERROR)
                        return f;
                }
                if(f->kind == EXPR_COMPUTED_ARRAY){
                    ComputedArray* fc = (ComputedArray*)f;
                    if(i >= fc->length){
                        return Error(ctx, "false range out of bounds");
                    }
                    cc->data[i] = fc->data[i];
                }
                else if(expr_is_scalar(f)){
                    cc->data[i] = f;
                }
                else if(f->kind == EXPR_RANGE1D_COLUMN || f->kind == EXPR_RANGE1D_COLUMN_FOREIGN){
                    if(fcol_or_row == IDX_UNSET){
                        if(get_range1dcol(ctx, sd, f, &fcol_or_row, &fstart, &fend, &fsd, caller_row, caller_col))
                            return Error(ctx, "Invalid range");
                    }
                    if(fstart + i > fend)
                        return Error(ctx, "false range out of bounds");
                    Expression* fe = evaluate(ctx, fsd, fstart+i, fcol_or_row);
                    if(!fe || fe->kind == EXPR_ERROR)
                        return fe;
                    cc->data[i] = fe;
                }
                else if(f->kind == EXPR_RANGE1D_ROW || f->kind == EXPR_RANGE1D_ROW_FOREIGN){
                    if(fcol_or_row == IDX_UNSET){
                        if(get_range1drow(ctx, sd, f, &fcol_or_row, &fstart, &fend, &fsd, caller_row, caller_col))
                            return Error(ctx, "Invalid range");
                    }
                    if(fstart + i > fend)
                        return Error(ctx, "false range out of bounds");
                    Expression* fe = evaluate(ctx, fsd, fcol_or_row, fstart+i);
                    if(!fe || fe->kind == EXPR_ERROR)
                        return fe;
                    cc->data[i] = fe;
                }
                else {
                    return Error(ctx, ""); // unreachable?
                }
            }
        }
        return cond;
    }
    if(evaled_is_not_scalar(cond)){
        // Unreachable?
        return Error(ctx, "Must be coercible to true/false");
    }
    _Bool truthy;
    switch(cond->kind){
        case EXPR_NUMBER:
            truthy = !!((Number*)cond)->value;
            break;
        case EXPR_STRING:
            truthy = !!((String*)cond)->str->length;
            break;
        case EXPR_BLANK:
            truthy = 0;
            break;
        default:
            __builtin_unreachable();
    }
    buff_set(ctx->a, bc);
    if(truthy)
        return evaluate_expr(ctx, sd, argv[1], caller_row, caller_col);
    else
        return evaluate_expr(ctx, sd, argv[2], caller_row, caller_col);
}

#if defined(DRSPREAD_CLI_C)
#ifdef __APPLE__
#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#include <time.h>
#ifdef __clang__
#pragma clang assume_nonnull begin
#endif
// GCOV_EXCL_START
DRSP_INTERNAL
FORMULAFUNC(drsp_time){
    if(argc != 1) return Error(ctx, "");
    uint64_t start = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);
    Expression* e = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    uint64_t end = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);
    fprintf(stderr, "Took %zu ns\n", (size_t)(end - start));
    return e;
}
// GCOV_EXCL_STOP
#endif
#endif

#ifndef SVI
#define SVI(x) {sizeof(x)-1, x}
#endif

#ifdef DRSP_INTRINS
DRSP_INTERNAL
const FuncInfo FUNC1[] = {
    {SVI("a"),    &drsp_array},
    {SVI("f"),    &drsp_first},
    {SVI("r"),    &drsp_repr},
#if defined(DRSPREAD_CLI_C)
#ifdef __APPLE__
    {SVI("t"),    &drsp_time},
#endif
#endif
};
#endif
DRSP_INTERNAL
const FuncInfo FUNC2[] = {
    {SVI("if"),    &drsp_if},
    {SVI("lu"),    &drsp_tablelookup},
    {SVI("ln"),    &drsp_log},
};
DRSP_INTERNAL
const FuncInfo FUNC3[] = {
    {SVI("sum"),   &drsp_sum},
    {SVI("tlu"),   &drsp_tablelookup},
    {SVI("mod"),   &drsp_mod},
    {SVI("avg"),   &drsp_avg},
    {SVI("min"),   &drsp_min},
    {SVI("max"),   &drsp_max},
    {SVI("abs"),   &drsp_abs},
    {SVI("num"),   &drsp_num},
    {SVI("try"),   &drsp_try},
    {SVI("pow"),   &drsp_pow},
    {SVI("col"),   &drsp_col},
    {SVI("cat"),   &drsp_cat},
    {SVI("row"),   &drsp_row},
    {SVI("log"),   &drsp_log},
};
DRSP_INTERNAL
const FuncInfo FUNC4[] = {
    {SVI("ceil"),  &drsp_ceil},
    {SVI("find"),  &drsp_find},
    {SVI("cell"),  &drsp_cell},
    {SVI("eval"),  &drsp_eval},
    {SVI("call"),  &drsp_call},
    {SVI("sqrt"),  &drsp_sqrt},
    {SVI("mean"),  &drsp_avg},
#ifdef DRSPREAD_CLI_C
#ifdef __APPLE__
    {SVI("time"),  &drsp_time},
#endif
#endif
    {SVI("prod"),  &drsp_prod},
};
DRSP_INTERNAL
const FuncInfo FUNC5[] = {
    {SVI("count"), &drsp_count},
    {SVI("floor"), &drsp_floor},
    {SVI("trunc"), &drsp_trunc},
    {SVI("round"), &drsp_round},
    {SVI("array"), &drsp_array},
};
DRSP_INTERNAL
const FuncInfo FUNC6[] = {
    {SVI("column"), &drsp_col},
};


DRSP_INTERNAL
FormulaFunc*_Nullable
lookup_func(DrspAtom a){
    StringView name = {a->length, a->data};
    switch(name.length){
        #ifdef DRSP_INTRINS
        case 1:
            for(size_t i = 0; i < arrlen(FUNC1); i++){
                if(sv_equals(FUNC1[i].name, name)){
                    return FUNC1[i].func;
                }
            }
            return NULL;
        #endif
        case 2:
            for(size_t i = 0; i < arrlen(FUNC2); i++){
                if(sv_equals(FUNC2[i].name, name)){
                    return FUNC2[i].func;
                }
            }
            return NULL;
        case 3:
            for(size_t i = 0; i < arrlen(FUNC3); i++){
                if(sv_equals(FUNC3[i].name, name)){
                    return FUNC3[i].func;
                }
            }
            return NULL;
        case 4:
            for(size_t i = 0; i < arrlen(FUNC4); i++){
                if(sv_equals(FUNC4[i].name, name)){
                    return FUNC4[i].func;
                }
            }
            return NULL;
        case 5:
            for(size_t i = 0; i < arrlen(FUNC5); i++){
                if(sv_equals(FUNC5[i].name, name)){
                    return FUNC5[i].func;
                }
            }
            return NULL;
        case 6:
            for(size_t i = 0; i < arrlen(FUNC6); i++){
                if(sv_equals(FUNC6[i].name, name)){
                    return FUNC6[i].func;
                }
            }
            return NULL;
    }
    return NULL;
}

#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#endif
