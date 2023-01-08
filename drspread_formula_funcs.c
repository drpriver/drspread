#ifndef DRSPREAD_FORMULA_FUNCS_C
#define DRSPREAD_FORMULA_FUNCS_C
#include "drspread_types.h"
#include "drspread_evaluate.h"
#include <math.h>
#ifdef __clang__
#pragma clang assume_nonnull begin
#endif

#ifndef arrlen
#define arrlen(x) (sizeof(x)/sizeof(x[0]))
#endif

static inline
int
get_rng1dcol(Expression* arg, intptr_t* col, intptr_t* rowstart, intptr_t* rowend){
    if(arg->kind != EXPR_RANGE1D_COLUMN)
        return 1;
    Range1DColumn* rng = (Range1DColumn*)arg;
    intptr_t start = rng->row_start;
    if(start < 0) return 1; // TODO: python style negative indexing
    intptr_t end = rng->row_end;
    if(end < 0) return 1; // TODO: python style negative indexing
    if(end < start){
        intptr_t tmp = end;
        end = start;
        start = tmp;
    }
    *col = rng->col;
    *rowstart = start;
    *rowend = end;
    return 0;
}

static 
FORMULAFUNC(drsp_sum){
    if(argc != 1) return Error(ctx, "");
    Expression* arg = evaluate_expr(ctx, argv[0]);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    intptr_t col, start, end;
    if(get_rng1dcol(arg, &col, &start, &end) != 0)
        return Error(ctx, "");
    double sum = 0;
    // NOTE: inclusive range
    for(intptr_t row = start; row <= end; row++){
        CellKind kind = ctx->ops.query_cell_kind(ctx->ops.ctx, row, col);
        switch(kind){
            case EMPTY:
            case OTHER:
                continue;
            case NUMBER:{
                double val = ctx->ops.cell_number(ctx->ops.ctx, row, col);
                sum += val;
            }continue;
            case FORMULA:{
                double val;
                int err = evaluate(ctx, row, col, &val);
                if(err) return Error(ctx, "");
                sum += val;
            }continue;
        }
    }
    Number* n = expr_alloc(ctx, EXPR_NUMBER);
    n->value = sum;
    return &n->e;
}

static
FORMULAFUNC(drsp_avg){
    if(argc != 1) return Error(ctx, "");
    Expression* arg = evaluate_expr(ctx, argv[0]);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(arg->kind != EXPR_RANGE1D_COLUMN)
        return Error(ctx, "");
    intptr_t col, start, end;
    if(get_rng1dcol(arg, &col, &start, &end) != 0)
        return Error(ctx, "");
    double sum = 0.;
    double count = 0.;
    // NOTE: inclusive range
    for(intptr_t row = start; row <= end; row++){
        CellKind kind = ctx->ops.query_cell_kind(ctx->ops.ctx, row, col);
        switch(kind){
            case EMPTY:
            case OTHER:
                continue;
            case NUMBER:{
                double val = ctx->ops.cell_number(ctx->ops.ctx, row, col);
                sum += val;
                count += 1.0;
            }continue;
            case FORMULA:{
                double val;
                int err = evaluate(ctx, row, col, &val);
                if(err) return Error(ctx, "");
                sum += val;
                count += 1.0;
            }continue;
        }
    }
    Number* n = expr_alloc(ctx, EXPR_NUMBER);
    n->value = sum/count;
    return &n->e;
}
static
FORMULAFUNC(drsp_min){
    (void)argc; (void)argv;
    return Error(ctx, __func__);
}

static
FORMULAFUNC(drsp_max){
    (void)argc; (void)argv;
    return Error(ctx, __func__);
}

static
FORMULAFUNC(drsp_mod){
    if(argc != 1) return Error(ctx, __func__);
    Expression* arg = evaluate_expr(ctx, argv[0]);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(arg->kind != EXPR_NUMBER)
        return Error(ctx, "");
    double score = ((Number*)arg)->value;
    double mod = floor((score - 10)/2);
    Number* n = expr_alloc(ctx, EXPR_NUMBER);
    n->value = mod;
    return &n->e;
}

static
FORMULAFUNC(drsp_floor){
    if(argc != 1) return Error(ctx, __func__);
    Expression* arg = evaluate_expr(ctx, argv[0]);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(arg->kind != EXPR_NUMBER)
        return Error(ctx, "");
    Number* n = expr_alloc(ctx, EXPR_NUMBER);
    n->value = floor(((Number*)arg)->value);
    return &n->e;
}

static
FORMULAFUNC(drsp_ceil){
    if(argc != 1) return Error(ctx, __func__);
    Expression* arg = evaluate_expr(ctx, argv[0]);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(arg->kind != EXPR_NUMBER)
        return Error(ctx, "");
    Number* n = expr_alloc(ctx, EXPR_NUMBER);
    n->value = ceil(((Number*)arg)->value);
    return &n->e;
}

static
FORMULAFUNC(drsp_trunc){
    if(argc != 1) return Error(ctx, __func__);
    Expression* arg = evaluate_expr(ctx, argv[0]);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(arg->kind != EXPR_NUMBER)
        return Error(ctx, "");
    Number* n = expr_alloc(ctx, EXPR_NUMBER);
    n->value = trunc(((Number*)arg)->value);
    return &n->e;
}
static
FORMULAFUNC(drsp_round){
    if(argc != 1) return Error(ctx, __func__);
    Expression* arg = evaluate_expr(ctx, argv[0]);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(arg->kind != EXPR_NUMBER)
        return Error(ctx, "");
    Number* n = expr_alloc(ctx, EXPR_NUMBER);
    n->value = round(((Number*)arg)->value);
    return &n->e;
}
static const FuncInfo FUNCTABLE[] = {
    {{3, "sum"  }, &drsp_sum},
    {{3, "avg"  }, &drsp_avg},
    {{3, "min"  }, &drsp_min},
    {{3, "max"  }, &drsp_max},
    {{3, "mod"  }, &drsp_mod},
    {{5, "floor"}, &drsp_floor},
    {{4, "ceil" }, &drsp_ceil},
    {{5, "trunc"}, &drsp_trunc},
    {{5, "round"}, &drsp_round},
};
static const size_t FUNCTABLE_LENGTH = arrlen(FUNCTABLE);
#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#endif
