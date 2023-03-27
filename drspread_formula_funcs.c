//
// Copyright © 2023, David Priver
//
#ifndef DRSPREAD_FORMULA_FUNCS_C
#define DRSPREAD_FORMULA_FUNCS_C
#include "drspread_types.h"
#include "drspread_evaluate.h"
#include "drspread_utils.h"
#include "drspread_formula_funcs.h"

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
    if(argc != 1) return Error(ctx, "");
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    double sum = 0;
    if(arg->kind == EXPR_COMPUTED_ARRAY){
        ComputedArray* c = (ComputedArray*)arg;
        for(intptr_t i = 0; i < c->length; i++){
            Expression* e = c->data[i];
            if(e->kind != EXPR_NUMBER)
                continue;
            sum += ((Number*)e)->value;
        }
    }
    else if(arg->kind == EXPR_RANGE1D_COLUMN || arg->kind == EXPR_RANGE1D_COLUMN_FOREIGN){
        intptr_t col, start, end;
        SheetData* rsd = sd;
        if(get_range1dcol(ctx, sd, arg, &col, &start, &end, &rsd, caller_row, caller_col) != 0)
            return Error(ctx, "");
        // NOTE: inclusive range
        for(intptr_t row = start; row <= end; row++){
            BuffCheckpoint bc = buff_checkpoint(ctx->a);
            Expression* e = evaluate(ctx, rsd, row, col);
            if(!e || e->kind == EXPR_ERROR) return e;
            if(evaled_is_not_scalar(e)) return Error(ctx, "");
            if(e->kind != EXPR_NUMBER) continue;
            sum += ((Number*)e)->value;
            buff_set(ctx->a, bc);
        }
    }
    else {
        intptr_t row, start, end;
        SheetData* rsd = sd;
        if(get_range1drow(ctx, sd, arg, &row, &start, &end, &rsd, caller_row, caller_col) != 0)
            return Error(ctx, "");
        // NOTE: inclusive range
        for(intptr_t col = start; col <= end; col++){
            BuffCheckpoint bc = buff_checkpoint(ctx->a);
            Expression* e = evaluate(ctx, rsd, row, col);
            if(!e || e->kind == EXPR_ERROR) return e;
            if(evaled_is_not_scalar(e)) return Error(ctx, "");
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
    if(argc != 1) return Error(ctx, "");
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    double prod = 1;
    if(arg->kind == EXPR_COMPUTED_ARRAY){
        ComputedArray* c = (ComputedArray*)arg;
        for(intptr_t i = 0; i < c->length; i++){
            Expression* e = c->data[i];
            if(e->kind != EXPR_NUMBER)
                continue;
            prod *= ((Number*)e)->value;
        }
    }
    else if(arg->kind == EXPR_RANGE1D_COLUMN || arg->kind == EXPR_RANGE1D_COLUMN_FOREIGN){
        intptr_t col, start, end;
        SheetData* rsd = sd;
        if(get_range1dcol(ctx, sd, arg, &col, &start, &end, &rsd, caller_row, caller_col) != 0)
            return Error(ctx, "");
        // NOTE: inclusive range
        for(intptr_t row = start; row <= end; row++){
            BuffCheckpoint bc = buff_checkpoint(ctx->a);
            Expression* e = evaluate(ctx, rsd, row, col);
            if(!e || e->kind == EXPR_ERROR) return e;
            if(evaled_is_not_scalar(e)) return Error(ctx, "");
            if(e->kind != EXPR_NUMBER) continue;
            prod *= ((Number*)e)->value;
            buff_set(ctx->a, bc);
        }
    }
    else {
        intptr_t row, start, end;
        SheetData* rsd = sd;
        if(get_range1drow(ctx, sd, arg, &row, &start, &end, &rsd, caller_row, caller_col) != 0)
            return Error(ctx, "");
        // NOTE: inclusive range
        for(intptr_t col = start; col <= end; col++){
            BuffCheckpoint bc = buff_checkpoint(ctx->a);
            Expression* e = evaluate(ctx, rsd, row, col);
            if(!e || e->kind == EXPR_ERROR) return e;
            if(evaled_is_not_scalar(e)) return Error(ctx, "");
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
    if(argc != 1) return Error(ctx, "");
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
            if(e->kind != EXPR_NUMBER) continue;
            sum += ((Number*)e)->value;
            count += 1.0;
        }
    }
    else if(arg->kind == EXPR_RANGE1D_COLUMN || arg->kind == EXPR_RANGE1D_COLUMN_FOREIGN){
        intptr_t col, start, end;
        SheetData* rsd = sd;
        if(get_range1dcol(ctx, sd, arg, &col, &start, &end, &rsd, caller_row, caller_col) != 0)
            return Error(ctx, "");
        // NOTE: inclusive range
        for(intptr_t row = start; row <= end; row++){
            BuffCheckpoint bc = buff_checkpoint(ctx->a);
            Expression* e = evaluate(ctx, rsd, row, col);
            if(!e || e->kind == EXPR_ERROR) return e;
            if(evaled_is_not_scalar(e)) return Error(ctx, "");
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
            return Error(ctx, "");
        // NOTE: inclusive range
        for(intptr_t col = start; col <= end; col++){
            BuffCheckpoint bc = buff_checkpoint(ctx->a);
            Expression* e = evaluate(ctx, rsd, row, col);
            if(!e || e->kind == EXPR_ERROR) return e;
            if(evaled_is_not_scalar(e)) return Error(ctx, "");
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
    if(argc != 1) return Error(ctx, "");
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
            return Error(ctx, "");
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
            return Error(ctx, "");
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
    if(!argc) return Error(ctx, "");
    if(argc > 1){
        BuffCheckpoint bc = buff_checkpoint(ctx->a);
        double v = 1e32;
        for(;argc;argc--, argv++, buff_set(ctx->a, bc)){
            Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
            if(!arg || arg->kind == EXPR_ERROR)
                return arg;
            if(arg->kind != EXPR_NUMBER)
                return Error(ctx, "");
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
            if(e->kind != EXPR_NUMBER) continue;
            if(((Number*)e)->value < v)
                v = ((Number*)e)->value;
        }
    }
    else if(arg->kind == EXPR_RANGE1D_COLUMN || arg->kind == EXPR_RANGE1D_COLUMN_FOREIGN){
        SheetData* rsd = sd;
        intptr_t col, start, end;
        if(get_range1dcol(ctx, sd, arg, &col, &start, &end, &rsd, caller_row, caller_col) != 0)
            return Error(ctx, "");
        // NOTE: inclusive range
        for(intptr_t row = start; row <= end; row++){
            BuffCheckpoint bc = buff_checkpoint(ctx->a);
            Expression* e = evaluate(ctx, rsd, row, col);
            if(!e || e->kind == EXPR_ERROR) return e;
            if(evaled_is_not_scalar(e)) return Error(ctx, "");
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
            return Error(ctx, "");
        // NOTE: inclusive range
        for(intptr_t col = start; col <= end; col++){
            BuffCheckpoint bc = buff_checkpoint(ctx->a);
            Expression* e = evaluate(ctx, rsd, row, col);
            if(!e || e->kind == EXPR_ERROR) return e;
            if(evaled_is_not_scalar(e)) return Error(ctx, "");
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
    if(!argc) return Error(ctx, "");
    if(argc > 1){
        BuffCheckpoint bc = buff_checkpoint(ctx->a);
        double v = -1e32;
        for(;argc;argc--, argv++, buff_set(ctx->a, bc)){
            Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
            if(!arg || arg->kind == EXPR_ERROR)
                return arg;
            if(arg->kind != EXPR_NUMBER)
                return Error(ctx, "");
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
            if(e->kind != EXPR_NUMBER) continue;
            if(((Number*)e)->value > v)
                v = ((Number*)e)->value;
        }
    }
    else if(arg->kind == EXPR_RANGE1D_COLUMN || arg->kind == EXPR_RANGE1D_COLUMN_FOREIGN){
        SheetData* rsd = sd;
        intptr_t col, start, end;
        if(get_range1dcol(ctx, sd, arg, &col, &start, &end, &rsd, caller_row, caller_col) != 0)
            return Error(ctx, "");
        // NOTE: inclusive range
        for(intptr_t row = start; row <= end; row++){
            BuffCheckpoint bc = buff_checkpoint(ctx->a);
            Expression* e = evaluate(ctx, rsd, row, col);
            if(!e || e->kind == EXPR_ERROR) return e;
            if(evaled_is_not_scalar(e)) return Error(ctx, "");
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
            return Error(ctx, "");
        // NOTE: inclusive range
        for(intptr_t col = start; col <= end; col++){
            BuffCheckpoint bc = buff_checkpoint(ctx->a);
            Expression* e = evaluate(ctx, rsd, row, col);
            if(!e || e->kind == EXPR_ERROR) return e;
            if(evaled_is_not_scalar(e)) return Error(ctx, "");
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
    if(argc != 1) return Error(ctx, "");
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(expr_is_arraylike(arg)){
        arg = convert_to_computed_array(ctx, sd, arg, caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        ComputedArray* c = (ComputedArray*)arg;
        for(intptr_t i = 0; i < c->length; i++){
            Expression* e = c->data[i];
            if(e->kind == EXPR_NULL)
                continue;
            if(e->kind != EXPR_NUMBER)
                return Error(ctx, "");
            Number* n = (Number*)e;
            n->value = __builtin_floor((n->value - 10)/2);
        }
        return arg;
    }
    else {
        if(arg->kind != EXPR_NUMBER)
            return Error(ctx, "");
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
    if(argc != 1) return Error(ctx, "");
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(expr_is_arraylike(arg)){
        arg = convert_to_computed_array(ctx, sd, arg, caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        ComputedArray* c = (ComputedArray*)arg;
        for(intptr_t i = 0; i < c->length; i++){
            Expression* e = c->data[i];
            if(e->kind == EXPR_NULL)
                continue;
            if(e->kind != EXPR_NUMBER)
                return Error(ctx, "");
            Number* n = (Number*)e;
            n->value = __builtin_floor(n->value);
        }
        return arg;
    }
    else {
        if(arg->kind != EXPR_NUMBER)
            return Error(ctx, "");
        buff_set(ctx->a, bc);
        Number* n = expr_alloc(ctx, EXPR_NUMBER);
        if(!n) return NULL;
        n->value = __builtin_floor(((Number*)arg)->value);
        return &n->e;
    }
}

DRSP_INTERNAL
FORMULAFUNC(drsp_ceil){
    if(argc != 1) return Error(ctx, "");
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(expr_is_arraylike(arg)){
        arg = convert_to_computed_array(ctx, sd, arg, caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        ComputedArray* c = (ComputedArray*)arg;
        for(intptr_t i = 0; i < c->length; i++){
            Expression* e = c->data[i];
            if(e->kind == EXPR_NULL)
                continue;
            if(e->kind != EXPR_NUMBER)
                return Error(ctx, "");
            Number* n = (Number*)e;
            n->value = __builtin_ceil(n->value);
        }
        return arg;
    }
    else {
        if(arg->kind != EXPR_NUMBER)
            return Error(ctx, "");
        buff_set(ctx->a, bc);
        Number* n = expr_alloc(ctx, EXPR_NUMBER);
        if(!n) return NULL;
        n->value = __builtin_ceil(((Number*)arg)->value);
        return &n->e;
    }
}

DRSP_INTERNAL
FORMULAFUNC(drsp_trunc){
    if(argc != 1) return Error(ctx, "");
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(expr_is_arraylike(arg)){
        arg = convert_to_computed_array(ctx, sd, arg, caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        ComputedArray* c = (ComputedArray*)arg;
        for(intptr_t i = 0; i < c->length; i++){
            Expression* e = c->data[i];
            if(e->kind == EXPR_NULL)
                continue;
            if(e->kind != EXPR_NUMBER)
                return Error(ctx, "");
            Number* n = (Number*)e;
            n->value = __builtin_trunc(n->value);
        }
        return arg;
    }
    else {
        if(arg->kind != EXPR_NUMBER)
            return Error(ctx, "");
        buff_set(ctx->a, bc);
        Number* n = expr_alloc(ctx, EXPR_NUMBER);
        if(!n) return NULL;
        n->value = __builtin_trunc(((Number*)arg)->value);
        return &n->e;
    }
}

DRSP_INTERNAL
FORMULAFUNC(drsp_round){
    if(argc != 1) return Error(ctx, "");
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(expr_is_arraylike(arg)){
        arg = convert_to_computed_array(ctx, sd, arg, caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        ComputedArray* c = (ComputedArray*)arg;
        for(intptr_t i = 0; i < c->length; i++){
            Expression* e = c->data[i];
            if(e->kind == EXPR_NULL)
                continue;
            if(e->kind != EXPR_NUMBER)
                return Error(ctx, "");
            Number* n = (Number*)e;
            n->value = __builtin_round(n->value);
        }
        return arg;
    }
    else {
        if(arg->kind != EXPR_NUMBER)
            return Error(ctx, "");
        buff_set(ctx->a, bc);
        Number* n = expr_alloc(ctx, EXPR_NUMBER);
        if(!n) return NULL;
        n->value = __builtin_round(((Number*)arg)->value);
        return &n->e;
    }
}

DRSP_INTERNAL
FORMULAFUNC(drsp_abs){
    if(argc != 1) return Error(ctx, "");
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(expr_is_arraylike(arg)){
        arg = convert_to_computed_array(ctx, sd, arg, caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        ComputedArray* c = (ComputedArray*)arg;
        for(intptr_t i = 0; i < c->length; i++){
            Expression* e = c->data[i];
            if(e->kind == EXPR_NULL)
                continue;
            if(e->kind != EXPR_NUMBER)
                return Error(ctx, "");
            Number* n = (Number*)e;
            n->value = __builtin_fabs(n->value);
        }
        return arg;
    }
    else {
        if(arg->kind != EXPR_NUMBER)
            return Error(ctx, "");
        buff_set(ctx->a, bc);
        Number* n = expr_alloc(ctx, EXPR_NUMBER);
        if(!n) return NULL;
        n->value = __builtin_fabs(((Number*)arg)->value);
        return &n->e;
    }
}

DRSP_INTERNAL
FORMULAFUNC(drsp_sqrt){
    if(argc != 1) return Error(ctx, "");
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(expr_is_arraylike(arg)){
        arg = convert_to_computed_array(ctx, sd, arg, caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        ComputedArray* c = (ComputedArray*)arg;
        for(intptr_t i = 0; i < c->length; i++){
            Expression* e = c->data[i];
            if(e->kind == EXPR_NULL)
                continue;
            if(e->kind != EXPR_NUMBER)
                return Error(ctx, "");
            Number* n = (Number*)e;
            n->value = __builtin_sqrt(n->value);
        }
        return arg;
    }
    else {
        if(arg->kind != EXPR_NUMBER)
            return Error(ctx, "");
        buff_set(ctx->a, bc);
        Number* n = expr_alloc(ctx, EXPR_NUMBER);
        if(!n) return NULL;
        n->value = __builtin_sqrt(((Number*)arg)->value);
        return &n->e;
    }
}

DRSP_INTERNAL
FORMULAFUNC(drsp_num){
    if(argc != 1 && argc != 2) return Error(ctx, "");
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    double default_ = 0.;
    if(argc == 2){
        Expression* d = evaluate_expr(ctx, sd, argv[1], caller_row, caller_col);
        if(!d || d->kind == EXPR_ERROR)
            return d;
        if(d->kind != EXPR_NUMBER)
            return Error(ctx, "");
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
            n->value = default_;
            cc->data[i] = &n->e;
        }
        return arg;
    }
    else {
        double value;
        if(arg->kind == EXPR_NUMBER)
            value = ((Number*)arg)->value;
        else
            value = default_;
        buff_set(ctx->a, bc);
        Number* n = expr_alloc(ctx, EXPR_NUMBER);
        if(!n) return NULL;
        n->value = value;
        return &n->e;
    }
}
DRSP_INTERNAL
FORMULAFUNC(drsp_try){
    if(argc != 2) return Error(ctx, "");
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
    if(argc != 2 && argc != 3) return Error(ctx, "");
    if(argc == 3){
        Expression* sheet = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
        if(!sheet || sheet->kind == EXPR_ERROR) return sheet;
        if(sheet->kind != EXPR_STRING) return Error(ctx, "");
        StringView sv = ((String*)sheet)->sv;
        fsd = sheet_lookup_by_name(ctx, sv.text, sv.length);
        if(!fsd) return Error(ctx, "");
        argv++, argc--;
    }
    Expression* col = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    if(!col || col->kind == EXPR_ERROR) return col;
    if(col->kind != EXPR_STRING) return Error(ctx, "");
    StringView csv = ((String*)col)->sv;
    intptr_t col_idx = sp_name_to_col_idx(fsd, csv.text, csv.length);
    if(col_idx < 0) return Error(ctx, "");
    Expression* row = evaluate_expr(ctx, sd, argv[1], caller_row, caller_col);
    if(!row || row->kind == EXPR_ERROR) return row;
    if(row->kind != EXPR_NUMBER) return Error(ctx, "");
    intptr_t row_idx = (intptr_t)((Number*)row)->value;
    row_idx--;
    if(row_idx < 0) return Error(ctx, "");
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
    if(!argc || argc > 4) return Error(ctx, "");
    StringView colname = {0}, sheetname = {0};
    intptr_t rowstart = IDX_UNSET, rowend = IDX_UNSET;
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    {
        Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        if(arg->kind != EXPR_STRING && arg->kind != EXPR_NULL) return Error(ctx, "");
        if(arg->kind == EXPR_NULL)
            colname = SV("");
        else
            colname = ((String*)arg)->sv;
        argc--, argv++;
    }
    if(argc){
        Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        if(arg->kind == EXPR_STRING){
            sheetname = colname;
            colname = ((String*)arg)->sv;
        }
        else if(arg->kind == EXPR_NUMBER){
            rowstart = (intptr_t)((Number*)arg)->value;
        }
        else {
            return Error(ctx, "");
        }
        argc--, argv++;
    }
    if(argc){
        Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        if(arg->kind != EXPR_NUMBER)
            return Error(ctx, "");
        if(rowstart == IDX_UNSET)
            rowstart = (intptr_t)((Number*)arg)->value;
        else
            rowend = (intptr_t)((Number*)arg)->value;
        argc--, argv++;
    }
    if(argc){
        if(rowend != IDX_UNSET)
            return Error(ctx, "");
        Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        if(arg->kind != EXPR_NUMBER)
            return Error(ctx, "");
        rowend = (intptr_t)((Number*)arg)->value;
        argc--, argv++;
    }
    // idk
    // if(!colname.length) return Error(ctx, "");
    if(rowstart == IDX_UNSET)
        rowstart = 0;
    if(rowstart) rowstart--;
    if(rowend == IDX_UNSET)
        rowend = -1;
    else if(rowend) rowend--;
    if(argc) return Error(ctx, "");
    buff_set(ctx->a, bc);
    if(sheetname.length){
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
    StringView startcol = {0}, endcol = {0}, sheetname = {0};
    intptr_t row_idx;
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    if(argc == 4){
        // first arg is sheet name
        Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        if(arg->kind != EXPR_STRING) return Error(ctx, "");
        sheetname = ((String*)arg)->sv;
        buff_set(ctx->a, bc);
        argc--, argv++;
    }
    {
        // first arg is first colname
        Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        if(arg->kind == EXPR_NULL)
            startcol = SV("");
        else{
            if(arg->kind != EXPR_STRING) return Error(ctx, "");
            startcol = ((String*)arg)->sv;
        }
        buff_set(ctx->a, bc);
        argc--, argv++;
    }
    {
        // second arg is second colname
        Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        if(arg->kind == EXPR_NULL)
            endcol = SV("");
        else {
            if(arg->kind != EXPR_STRING) return Error(ctx, "");
            endcol = ((String*)arg)->sv;
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
    if(sheetname.length){
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
    if(argc != 1) return Error(ctx, "");
    Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(expr_is_arraylike(arg)){
        arg = convert_to_computed_array(ctx, sd, arg, caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        ComputedArray* c = (ComputedArray*)arg;
        for(intptr_t i = 0; i < c->length; i++){
            Expression* e = c->data[i];
            if(e->kind == EXPR_NULL) continue;
            if(e->kind != EXPR_STRING) return Error(ctx, "");
            StringView sv = ((String*)e)->sv;
            Expression* ev = evaluate_string(ctx, sd, sv.text, sv.length, caller_row, caller_col);
            if(!ev || ev->kind == EXPR_ERROR) return ev;
            c->data[i] = ev;
        }
        return &c->e;
    }
    if(arg->kind != EXPR_STRING) return Error(ctx, "");
    StringView sv = ((String*)arg)->sv;
    Expression* result = evaluate_string(ctx, sd, sv.text, sv.length, caller_row, caller_col);
    return result;
}

DRSP_INTERNAL
FORMULAFUNC(drsp_pow){
    if(argc != 2) return Error(ctx, "");
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
                if(e->kind == EXPR_NULL) continue;
                if(e->kind != EXPR_NUMBER)
                    return Error(ctx, "");
                Number* n = (Number*)e;
                n->value = __builtin_pow(n->value, exp);
            }
        }
        else if(expr_is_arraylike(arg2)){
            arg2 = convert_to_computed_array(ctx, sd, arg2, caller_row, caller_col);
            if(!arg2 || arg2->kind == EXPR_NULL)
                return arg2;
            ComputedArray* exps = (ComputedArray*)arg2;
            if(c->length != exps->length)
                return Error(ctx, "");
            for(intptr_t i = 0; i < c->length; i++){
                Expression* base = c->data[i];
                Expression* ex = exps->data[i];
                if(base->kind == EXPR_NULL)
                    continue;
                if(base->kind != EXPR_NUMBER)
                    return Error(ctx, "");
                if(ex->kind != EXPR_NUMBER)
                    return Error(ctx, "");
                Number* n = (Number*)base;
                double ex_ = ((Number*)ex)->value;
                n->value = __builtin_pow(n->value, ex_);
            }
        }
        else {
            return Error(ctx, "");
        }
        buff_set(ctx->a, bc);
        return arg;
    }
    else {
        if(arg->kind != EXPR_NUMBER) {
            buff_set(ctx->a, bc);
            return Error(ctx, "");
        }
        Expression* arg2 = evaluate_expr(ctx, sd, argv[1], caller_row, caller_col);
        if(!arg2 || arg2 -> kind == EXPR_ERROR){
            buff_set(ctx->a, bc);
            return arg2;
        }
        if(arg2->kind != EXPR_NUMBER){
            buff_set(ctx->a, bc);
            return Error(ctx, "");
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
    if(argc < 2) return Error(ctx, "");
    StringView catbuff[4];
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
            if(arg2->kind == EXPR_NULL){
                for(intptr_t i = 0; i < c->length; i++){
                    Expression* e = c->data[i];
                    if(e->kind == EXPR_NULL || e->kind == EXPR_STRING)
                        continue;
                    return Error(ctx, "");
                }
            }
            else if(arg2->kind == EXPR_STRING){
                catbuff[1] = ((String*)arg2)->sv;
                for(intptr_t i = 0; i < c->length; i++){
                    Expression* e = c->data[i];
                    if(e->kind == EXPR_NULL){
                        c->data[i] = arg2;
                        continue;
                    }
                    if(e->kind != EXPR_STRING)
                        return Error(ctx, "");
                    String* s = (String*)e;
                    catbuff[0] = s->sv;
                    int err = sv_cat(ctx, 2, catbuff, &s->sv);
                    if(err) return Error(ctx, "");
                }
            }
            else if(expr_is_arraylike(arg2)){
                arg2 = convert_to_computed_array(ctx, sd, arg2, caller_row, caller_col);
                if(!arg2 || arg2->kind == EXPR_ERROR)
                    return arg2;
                ComputedArray* rights = (ComputedArray*)arg2;
                if(c->length != rights->length)
                    return Error(ctx, "");
                for(intptr_t i = 0; i < c->length; i++){
                    Expression* l = c->data[i];
                    Expression* r = rights->data[i];
                    if(l->kind != EXPR_NULL && l->kind != EXPR_STRING)
                        return Error(ctx, "");
                    if(r->kind != EXPR_NULL && r->kind != EXPR_STRING)
                        return Error(ctx, "");
                    if(l->kind == EXPR_NULL){
                        c->data[i] = r;
                        continue;
                    }
                    if(r->kind == EXPR_NULL)
                        continue;
                    assert(l->kind == EXPR_STRING);
                    assert(r->kind == EXPR_STRING);
                    String* s = (String*)l;
                    catbuff[0] = s->sv;
                    catbuff[1] = ((String*)r)->sv;
                    int err = sv_cat(ctx, 2, catbuff, &s->sv);
                    if(err) return Error(ctx, "");
                }
            }
            else {
                return Error(ctx, "");
            }
            buff_set(ctx->a, bc);
            return arg;
        }
        else {
            if(arg->kind != EXPR_STRING && arg->kind != EXPR_NULL) {
                buff_set(ctx->a, bc);
                return Error(ctx, "");
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
                if(arg->kind != EXPR_NULL)
                    catbuff[0] = ((String*)arg)->sv;
                for(intptr_t i = 0; i < c->length; i++){
                    Expression* e = c->data[i];
                    if(e->kind == EXPR_NULL) continue;
                    if(e->kind != EXPR_STRING)
                        return Error(ctx, "");
                    if(arg->kind == EXPR_NULL) continue;
                    String* s = (String*)e;
                    catbuff[1] = s->sv;
                    int err = sv_cat(ctx, 2, catbuff, &s->sv);
                    if(err) return Error(ctx, "");
                }
                return arg2;
            }
            else {
                if(arg2->kind == EXPR_NULL){
                    if(arg->kind == EXPR_NULL){
                        buff_set(ctx->a, bc);
                        return arg;
                    }
                    StringView v = ((String*)arg)->sv;
                    buff_set(ctx->a, bc);
                    String* s = expr_alloc(ctx, EXPR_STRING);
                    if(!s) return NULL;
                    s->sv = v;
                    return &s->e;
                }
                if(arg2->kind != EXPR_STRING){
                    buff_set(ctx->a, bc);
                    return Error(ctx, "");
                }
                if(arg->kind == EXPR_NULL){
                    StringView v = ((String*)arg2)->sv;
                    buff_set(ctx->a, bc);
                    String* s = expr_alloc(ctx, EXPR_STRING);
                    if(!s) return NULL;
                    s->sv = v;
                    return &s->e;
                }
                StringView v;
                catbuff[0] = ((String*)arg)->sv;
                catbuff[1] = ((String*)arg2)->sv;
                int err = sv_cat(ctx, 2, catbuff, &v);
                buff_set(ctx->a, bc);
                if(err) return Error(ctx, "");
                String* s = expr_alloc(ctx, EXPR_STRING);
                if(!s) return NULL;
                s->sv = v;
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
            else if(argv[i]->kind != EXPR_STRING && argv[i]->kind != EXPR_NULL){
                buff_set(ctx->a, bc);
                return Error(ctx, "");
            }
        }
        if(is_arraylike){
            if(!column_length)
                return Error(ctx, "");
            ComputedArray* result = computed_array_alloc(ctx, column_length);
            if(!result) return Error(ctx, "oom");
            for(intptr_t r = 0; r < column_length; r++){
                for(int i = 0; i < argc; i++){
                    Expression* e = argv[i];
                    if(e->kind == EXPR_STRING){
                        catbuff[i] = ((String*)e)->sv;
                    }
                    else if(e->kind == EXPR_NULL){
                        catbuff[i] = SV("");
                    }
                    else {
                        ComputedArray* c = (ComputedArray*)argv[i];
                        if(r >= c->length){
                            catbuff[i] = SV("");
                        }
                        else {
                            Expression* item = c->data[r];
                            if(item->kind == EXPR_STRING){
                                catbuff[i] = ((String*)item)->sv;
                            }
                            else if(item->kind == EXPR_NULL){
                                catbuff[i] = SV("");
                            }
                            else {
                                buff_set(ctx->a, bc);
                                return Error(ctx, "");
                            }
                        }
                    }
                }
                String* s = expr_alloc(ctx, EXPR_STRING);
                if(!s) return NULL;
                int err = sv_cat(ctx, argc, catbuff, &s->sv);
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
                if(e->kind == EXPR_NULL){
                    catbuff[i] = SV("");
                }
                else {
                    catbuff[i] = ((String*)e)->sv;
                }
            }
            StringView sv;
            int err = sv_cat(ctx, argc, catbuff, &sv);
            buff_set(ctx->a, bc);
            if(err) return Error(ctx, "oom");
            String* result = expr_alloc(ctx, EXPR_STRING);
            if(!result) return NULL;
            result->sv = sv;
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
        if(nkind == EXPR_NULL)
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
            StringView sv = ((String*)n)->sv;
            for(idx = 0; idx < haylength; idx++){
                const Expression* h = data[idx];
                if(h->kind != EXPR_STRING) continue;
                if(sv_equals(sv, ((String*)h)->sv))
                    break;
            }
        }
        if(idx == haylength){
            if(!argc) return Error(ctx, "");
            if(!default_){
                default_ = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
                if(!default_ || default_ == EXPR_ERROR) return default_;
                cneedle->data[i] = default_;
            }
        }
        else {
            if(idx >= cvalues->length)
                return Error(ctx, "");
            cneedle->data[i] = cvalues->data[idx];
        }
    }
    return needle;
}

DRSP_INTERNAL
FORMULAFUNC(drsp_tablelookup){
    // "needle", [haystack], [values]
    if(argc != 3 && argc != 4) return Error(ctx, "");
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    ExpressionKind nkind;
    union {
        StringView s;
        double d;
    } nval = {0};
    {
        Expression* needle = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
        if(!needle || needle->kind == EXPR_ERROR) return needle;
        if(expr_is_arraylike(needle))
            return arraylike_tablelookup(ctx, sd, caller_row, caller_col, needle, argc-1, argv+1);
        nkind = needle->kind;
        if(nkind == EXPR_NULL){
            if(argc == 4){
                return evaluate_expr(ctx, sd, argv[3], caller_row, caller_col);
            }
        }
        if(nkind != EXPR_NUMBER && nkind != EXPR_STRING) return Error(ctx, "");
        if(nkind == EXPR_NUMBER)
            nval.d = ((Number*)needle)->value;
        else
            nval.s = ((String*)needle)->sv;
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
                    if(sv_equals(nval.s, ((String*)e)->sv)){
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
                return Error(ctx, "");

            BuffCheckpoint loopbc = buff_checkpoint(ctx->a);
            if(nkind == EXPR_STRING){
                for(intptr_t row = start; row <= end; buff_set(ctx->a,loopbc), row++){
                    Expression* e = evaluate(ctx, rsd, row, col);
                    if(!e) return e;
                    if(e->kind != EXPR_STRING) continue;
                    if(sv_equals(nval.s, ((String*)e)->sv)){
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
                return Error(ctx, "");

            BuffCheckpoint loopbc = buff_checkpoint(ctx->a);
            if(nkind == EXPR_STRING){
                for(intptr_t col = start; col <= end; buff_set(ctx->a,loopbc), col++){
                    Expression* e = evaluate(ctx, rsd, row, col);
                    if(!e) return e;
                    if(e->kind != EXPR_STRING) continue;
                    if(sv_equals(nval.s, ((String*)e)->sv)){
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
                return Error(ctx, "invalid column range");
            buff_set(ctx->a, bc);
            if(start+offset > end) return Error(ctx, "out of bounds");
            return evaluate(ctx, rsd, row, start+offset);
        }
    }
}

DRSP_INTERNAL
FORMULAFUNC(drsp_find){
    // "needle", [haystack]
    if(argc != 2 && argc != 3) return Error(ctx, "");
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    ExpressionKind nkind;
    union {
        StringView s;
        double d;
    } nval = {0};
    {
        Expression* needle = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
        if(!needle || needle->kind == EXPR_ERROR) return needle;
        nkind = needle->kind;
        if(nkind != EXPR_NUMBER && nkind != EXPR_STRING && nkind != EXPR_NULL) return Error(ctx, "");
        if(nkind == EXPR_NUMBER)
            nval.d = ((Number*)needle)->value;
        else if(nkind == EXPR_STRING)
            nval.s = ((String*)needle)->sv;
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
                if(nkind == EXPR_NULL){
                    offset = i;
                    break;
                }
                if(nkind == EXPR_STRING){
                    if(sv_equals(nval.s, ((String*)e)->sv)){
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
                return Error(ctx, "");

            BuffCheckpoint loopchk = buff_checkpoint(ctx->a);
            for(intptr_t row = start; row <= end; buff_set(ctx->a, loopchk), row++){
                Expression* e = evaluate(ctx, rsd, row, col);
                if(!e) return e;
                if(e->kind != nkind) continue;
                if(nkind == EXPR_NULL){
                    offset = row - start;
                    break;
                }
                if(nkind == EXPR_STRING){
                    if(sv_equals(nval.s, ((String*)e)->sv)){
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
                return Error(ctx, "");

            BuffCheckpoint loopchk = buff_checkpoint(ctx->a);
            for(intptr_t col = start; col <= end; buff_set(ctx->a, loopchk), col++){
                Expression* e = evaluate(ctx, rsd, row, col);
                if(!e) return e;
                if(e->kind != nkind) continue;
                if(nkind == EXPR_NULL){
                    offset = col - start;
                    break;
                }
                if(nkind == EXPR_STRING){
                    if(sv_equals(nval.s, ((String*)e)->sv)){
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
        return Error(ctx, "");
    }
    Number* n = expr_alloc(ctx, EXPR_NUMBER);
    if(!n) return NULL;
    n->value = offset+1;
    return &n->e;
}

DRSP_INTERNAL
FORMULAFUNC(drsp_call){
    if(!argc) return Error(ctx, "");
    Expression* arg = evaluate_expr(ctx, sd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(arg->kind != EXPR_STRING)
        return Error(ctx, "");
    StringView name = ((String*)arg)->sv;

    FormulaFunc* func = lookup_func(name);
    if(!func) return Error(ctx, "");
    argc--, argv++;
    return func(ctx, sd, caller_row, caller_col, argc, argv);
}

#ifdef DRSP_INTRINS
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
    __builtin_va_list vap;
    __builtin_va_start(vap, fmt);
    int n = vsnprintf(buff->buff, buff->len, fmt, vap);
    if(n < 0) buff->error = 1;
    if(n > buff->len) buff->error = 1;
    else {
        buff->len -= n;
        buff->buff += n;
    }
    __builtin_va_end(vap);
}


// GCOV_EXCL_START
DRSP_INTERNAL
void
repr_expr(PrintBuff* buff, Expression* arg){
    switch(arg->kind){
        case EXPR_ERROR:
            print(buff, "Error()");
            break;
        case EXPR_STRING:{
            String* s = (String*)arg;
            print(buff, "String('%.*s')", (int)s->sv.length, s->sv.text);
        }break;
        case EXPR_NULL:
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
            print(buff, "UserFuncCall-%.*s(", (int)f->name.length, f->name.text);
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
            print(buff, "[%.*s, %zd]", (int)rng->col_name.length, rng->col_name.text, rng->row);
            print(buff, ")");
        }break;
        case EXPR_RANGE0D_FOREIGN:{
            print(buff, "R0F(");
            ForeignRange0D* rng = (ForeignRange0D*)arg;
            print(buff, "[%.*s, %.*s, %zd]", 
                    (int)rng->sheet_name.length, rng->sheet_name.text, 
                    (int)rng->r.col_name.length, rng->r.col_name.text, 
                    rng->r.row);
            print(buff, ")");
        }break;
        case EXPR_RANGE1D_COLUMN:{
            print(buff, "R1C(");
            Range1DColumn* rng = (Range1DColumn*)arg;
            print(buff, "[%.*s, %zd:%zd]", 
                    (int)rng->col_name.length, rng->col_name.text, 
                    rng->row_start, rng->row_end);
            print(buff, ")");
        }break;
        case EXPR_RANGE1D_COLUMN_FOREIGN:{
            print(buff, "R1CF(");
            ForeignRange1DColumn* rng = (ForeignRange1DColumn*)arg;
            print(buff, "[%.*s, %.*s, %zd:%zd]", 
                    (int)rng->sheet_name.length, rng->sheet_name.text, 
                    (int)rng->r.col_name.length, rng->r.col_name.text, 
                    rng->r.row_start, rng->r.row_end);
            print(buff, ")");
        }break;
        case EXPR_RANGE1D_ROW:{
            print(buff, "R1R(");
            Range1DRow* rng = (Range1DRow*)arg;
            print(buff, "[%.*s:%.*s, %zd]", 
                    (int)rng->col_start.length, rng->col_start.text,
                    (int)rng->col_end.length, rng->col_end.text,
                    rng->row_idx);
            print(buff, ")");
        }break;
        case EXPR_RANGE1D_ROW_FOREIGN:{
            print(buff, "R1RF(");
            ForeignRange1DRow* rng = (ForeignRange1DRow*)arg;
            print(buff, "[%.*s, %.*s:%.*s, %zd]", 
                    (int)rng->sheet_name.length, rng->sheet_name.text,
                    (int)rng->r.col_start.length, rng->r.col_start.text,
                    (int)rng->r.col_end.length, rng->r.col_end.text,
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
// GCOV_EXCL_STOP

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
    DrspStr* str = drsp_create_str(ctx, buffer, sizeof(buffer) - buff.len);
    if(!str) return NULL;
    String* s = expr_alloc(ctx, EXPR_STRING);
    if(!s) return NULL;
    s->sv.text = str->data;
    s->sv.length = str->length;
    return &s->e;
}
#endif

DRSP_INTERNAL
FORMULAFUNC(drsp_array){
    if(!argc) return Error(ctx, "");
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
    if(argc != 3) return Error(ctx, "");
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
        intptr_t tcol_or_row = IDX_UNSET, tstart, tend;
        SheetData* tsd = sd;
        intptr_t fcol_or_row = IDX_UNSET, fstart, fend;
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
                        return Error(ctx, "");
                    }
                    cc->data[i] = tc->data[i];
                }
                else if(expr_is_scalar(t)){
                    cc->data[i] = t;
                }
                else if(t->kind == EXPR_RANGE1D_COLUMN || t->kind == EXPR_RANGE1D_COLUMN_FOREIGN){
                    if(tcol_or_row == IDX_UNSET){
                        if(get_range1dcol(ctx, sd, t, &tcol_or_row, &tstart, &tend, &tsd, caller_row, caller_col))
                            return Error(ctx, "");
                    }
                    if(tstart + i > tend)
                        return Error(ctx, "");
                    Expression* te = evaluate(ctx, tsd, tstart+i, tcol_or_row);
                    if(!te || te->kind == EXPR_ERROR)
                        return te;
                    cc->data[i] = te;
                }
                else if(t->kind == EXPR_RANGE1D_ROW || t->kind == EXPR_RANGE1D_ROW_FOREIGN){
                    if(tcol_or_row == IDX_UNSET){
                        if(get_range1drow(ctx, sd, t, &tcol_or_row, &tstart, &tend, &tsd, caller_row, caller_col))
                            return Error(ctx, "");
                    }
                    if(tstart + i > tend)
                        return Error(ctx, "");
                    Expression* te = evaluate(ctx, tsd, tcol_or_row, tstart+i);
                    if(!te || te->kind == EXPR_ERROR)
                        return te;
                    cc->data[i] = te;
                }
                else {
                    return Error(ctx, "");
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
                        return Error(ctx, "");
                    }
                    cc->data[i] = fc->data[i];
                }
                else if(expr_is_scalar(f)){
                    cc->data[i] = f;
                }
                else if(f->kind == EXPR_RANGE1D_COLUMN || f->kind == EXPR_RANGE1D_COLUMN_FOREIGN){
                    if(fcol_or_row == IDX_UNSET){
                        if(get_range1dcol(ctx, sd, f, &fcol_or_row, &fstart, &fend, &fsd, caller_row, caller_col))
                            return Error(ctx, "");
                    }
                    if(fstart + i > fend)
                        return Error(ctx, "");
                    Expression* fe = evaluate(ctx, fsd, fstart+i, fcol_or_row);
                    if(!fe || fe->kind == EXPR_ERROR)
                        return fe;
                    cc->data[i] = fe;
                }
                else if(f->kind == EXPR_RANGE1D_ROW || f->kind == EXPR_RANGE1D_ROW_FOREIGN){
                    if(fcol_or_row == IDX_UNSET){
                        if(get_range1drow(ctx, sd, f, &fcol_or_row, &fstart, &fend, &fsd, caller_row, caller_col))
                            return Error(ctx, "");
                    }
                    if(fstart + i > fend)
                        return Error(ctx, "");
                    Expression* fe = evaluate(ctx, fsd, fcol_or_row, fstart+i);
                    if(!fe || fe->kind == EXPR_ERROR)
                        return fe;
                    cc->data[i] = fe;
                }
                else {
                    return Error(ctx, "");
                }
            }
        }
        return cond;
    }
    if(evaled_is_not_scalar(cond)){
        return Error(ctx, "Must be coercible to true/false");
    }
    _Bool truthy;
    switch(cond->kind){
        case EXPR_NUMBER:
            truthy = !!((Number*)cond)->value;
            break;
        case EXPR_STRING:
            truthy = !!((String*)cond)->sv.length;
            break;
        case EXPR_NULL:
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

#ifndef SV
#define SV(x) {sizeof(x)-1, x}
#endif

#ifdef DRSP_INTRINS
DRSP_INTERNAL
const FuncInfo FUNC1[] = {
    {SV("a"),    &drsp_array},
    {SV("f"),    &drsp_first},
    {SV("r"),    &drsp_repr},
#if defined(DRSPREAD_CLI_C)
#ifdef __APPLE__
    {SV("t"),    &drsp_time},
#endif
#endif
};
#endif
DRSP_INTERNAL
const FuncInfo FUNC2[] = {
    {SV("if"),    &drsp_if},
};
DRSP_INTERNAL
const FuncInfo FUNC3[] = {
    {SV("sum"),   &drsp_sum},
    {SV("tlu"),   &drsp_tablelookup},
    {SV("mod"),   &drsp_mod},
    {SV("avg"),   &drsp_avg},
    {SV("min"),   &drsp_min},
    {SV("max"),   &drsp_max},
    {SV("abs"),   &drsp_abs},
    {SV("num"),   &drsp_num},
    {SV("try"),   &drsp_try},
    {SV("pow"),   &drsp_pow},
    {SV("col"),   &drsp_col},
    {SV("cat"),   &drsp_cat},
    {SV("row"),   &drsp_row},
};
DRSP_INTERNAL
const FuncInfo FUNC4[] = {
    {SV("ceil"),  &drsp_ceil},
    {SV("find"),  &drsp_find},
    {SV("cell"),  &drsp_cell},
    {SV("eval"),  &drsp_eval},
    {SV("call"),  &drsp_call},
    {SV("sqrt"),  &drsp_sqrt},
#ifdef DRSPREAD_CLI_C
#ifdef __APPLE__
    {SV("time"), &drsp_time},
#endif
#endif
    {SV("prod"),  &drsp_prod},
};
DRSP_INTERNAL
const FuncInfo FUNC5[] = {
    {SV("count"), &drsp_count},
    {SV("floor"), &drsp_floor},
    {SV("trunc"), &drsp_trunc},
    {SV("round"), &drsp_round},
    {SV("array"), &drsp_array},
};


DRSP_INTERNAL
FormulaFunc*_Nullable
lookup_func(StringView name){
    switch(name.length){
        #ifdef DRSP_INTRINS
        case 1:
            for(size_t i = 0; i < arrlen(FUNC1); i++){
                if(sv_iequals(FUNC1[i].name, name)){
                    return FUNC1[i].func;
                }
            }
            return NULL;
        #endif
        case 2:
            for(size_t i = 0; i < arrlen(FUNC2); i++){
                if(sv_iequals(FUNC2[i].name, name)){
                    return FUNC2[i].func;
                }
            }
            return NULL;
        case 3:
            for(size_t i = 0; i < arrlen(FUNC3); i++){
                if(sv_iequals(FUNC3[i].name, name)){
                    return FUNC3[i].func;
                }
            }
            return NULL;
        case 4:
            for(size_t i = 0; i < arrlen(FUNC4); i++){
                if(sv_iequals(FUNC4[i].name, name)){
                    return FUNC4[i].func;
                }
            }
            return NULL;
        case 5:
            for(size_t i = 0; i < arrlen(FUNC5); i++){
                if(sv_iequals(FUNC5[i].name, name)){
                    return FUNC5[i].func;
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
