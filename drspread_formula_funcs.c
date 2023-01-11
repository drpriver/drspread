//
// Copyright © 2023, David Priver
//
#ifndef DRSPREAD_FORMULA_FUNCS_C
#define DRSPREAD_FORMULA_FUNCS_C
#include "drspread_types.h"
#include "drspread_evaluate.h"
#ifdef __clang__
#pragma clang assume_nonnull begin
#endif

#ifndef arrlen
#define arrlen(x) (sizeof(x)/sizeof(x[0]))
#endif

static inline
int
get_range1dcol(SpreadContext*ctx, Expression* arg, intptr_t* col, intptr_t* rowstart, intptr_t* rowend){
    if(arg->kind != EXPR_RANGE1D_COLUMN)
        return 1;
    Range1DColumn* rng = (Range1DColumn*)arg;
    intptr_t start = rng->row_start;
    if(start < 0) start += ctx->ops.col_height(ctx->ops.ctx, rng->col);
    intptr_t end = rng->row_end;
    if(end < 0) end += ctx->ops.col_height(ctx->ops.ctx, rng->col);
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

static inline
_Bool evaled_is_not_scalar(Expression*_Nullable e){
    if(!e) return 1;
    switch(e->kind){
        case EXPR_ERROR:
            return 1;
        case EXPR_STRING:
        case EXPR_NULL:
        case EXPR_NUMBER:
            return 0;
        default:
            return 1;
    }

}

static
FORMULAFUNC(drsp_sum){
    // printf("enter %s\n", __func__);
    if(argc != 1) return Error(ctx, "");
    Number* n = expr_alloc(ctx, EXPR_NUMBER);
    if(!n) return NULL;
    char* chk = ctx->a.cursor;
    Expression* arg = evaluate_expr(ctx, argv[0]);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    intptr_t col, start, end;
    if(get_range1dcol(ctx, arg, &col, &start, &end) != 0)
        return Error(ctx, "");
    double sum = 0;
    // NOTE: inclusive range
    for(intptr_t row = start; row <= end; row++){
        char* chk = ctx->a.cursor;
        Expression* e = evaluate(ctx, row, col);
        if(!e || e->kind == EXPR_ERROR) return e;
        if(evaled_is_not_scalar(e)) return Error(ctx, "");
        if(e->kind != EXPR_NUMBER) continue;
        sum += ((Number*)e)->value;
        ctx->a.cursor = chk;
    }
    ctx->a.cursor = chk;
    n->value = sum;
    // printf("leave %s\n", __func__);
    return &n->e;
}

static
FORMULAFUNC(drsp_avg){
    if(argc != 1) return Error(ctx, "");
    Number* n = expr_alloc(ctx, EXPR_NUMBER);
    if(!n) return NULL;
    char* chk = ctx->a.cursor;
    Expression* arg = evaluate_expr(ctx, argv[0]);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(arg->kind != EXPR_RANGE1D_COLUMN)
        return Error(ctx, "");
    intptr_t col, start, end;
    if(get_range1dcol(ctx, arg, &col, &start, &end) != 0)
        return Error(ctx, "");
    double sum = 0.;
    double count = 0.;
    // NOTE: inclusive range
    for(intptr_t row = start; row <= end; row++){
        char* chk = ctx->a.cursor;
        Expression* e = evaluate(ctx, row, col);
        if(!e || e->kind == EXPR_ERROR) return e;
        if(evaled_is_not_scalar(e)) return Error(ctx, "");
        if(e->kind != EXPR_NUMBER) continue;
        sum += ((Number*)e)->value;
        count += 1.0;
        ctx->a.cursor = chk;
    }
    ctx->a.cursor = chk;
    n->value = sum/count;
    return &n->e;
}
static
FORMULAFUNC(drsp_min){
    if(argc != 1) return Error(ctx, "");
    Number* n = expr_alloc(ctx, EXPR_NUMBER);
    if(!n) return NULL;
    char* chk = ctx->a.cursor;
    Expression* arg = evaluate_expr(ctx, argv[0]);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(arg->kind != EXPR_RANGE1D_COLUMN)
        return Error(ctx, "");
    intptr_t col, start, end;
    if(get_range1dcol(ctx, arg, &col, &start, &end) != 0)
        return Error(ctx, "");
    double v = 1e32;
    // NOTE: inclusive range
    for(intptr_t row = start; row <= end; row++){
        char* chk = ctx->a.cursor;
        Expression* e = evaluate(ctx, row, col);
        if(!e || e->kind == EXPR_ERROR) return e;
        if(evaled_is_not_scalar(e)) return Error(ctx, "");
        if(e->kind != EXPR_NUMBER) continue;
        if(((Number*)e)->value < v)
            v = ((Number*)e)->value;
        ctx->a.cursor = chk;
    }
    if(v == 1e32) v = 0.;
    ctx->a.cursor = chk;
    n->value = v;
    return &n->e;
}

static
FORMULAFUNC(drsp_max){
    if(argc != 1) return Error(ctx, "");
    Number* n = expr_alloc(ctx, EXPR_NUMBER);
    if(!n) return NULL;
    char* chk = ctx->a.cursor;
    Expression* arg = evaluate_expr(ctx, argv[0]);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(arg->kind != EXPR_RANGE1D_COLUMN)
        return Error(ctx, "");
    intptr_t col, start, end;
    if(get_range1dcol(ctx, arg, &col, &start, &end) != 0)
        return Error(ctx, "");
    double v = -1e32;
    // NOTE: inclusive range
    for(intptr_t row = start; row <= end; row++){
        char* chk = ctx->a.cursor;
        Expression* e = evaluate(ctx, row, col);
        if(!e || e->kind == EXPR_ERROR) return e;
        if(evaled_is_not_scalar(e)) return Error(ctx, "");
        if(e->kind != EXPR_NUMBER) continue;
        if(((Number*)e)->value > v)
            v = ((Number*)e)->value;
        ctx->a.cursor = chk;
    }
    if(v == -1e32) v = 0.;
    ctx->a.cursor = chk;
    n->value = v;
    return &n->e;
}

static
FORMULAFUNC(drsp_mod){
    if(argc != 1) return Error(ctx, "");
    Number* n = expr_alloc(ctx, EXPR_NUMBER);
    if(!n) return NULL;
    char* chk = ctx->a.cursor;
    Expression* arg = evaluate_expr(ctx, argv[0]);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(arg->kind != EXPR_NUMBER)
        return Error(ctx, "");
    double score = ((Number*)arg)->value;
    double mod = __builtin_floor((score - 10)/2);
    ctx->a.cursor = chk;
    n->value = mod;
    return &n->e;
}

static
FORMULAFUNC(drsp_floor){
    if(argc != 1) return Error(ctx, "");
    Number* n = expr_alloc(ctx, EXPR_NUMBER);
    if(!n) return NULL;
    char* chk = ctx->a.cursor;
    Expression* arg = evaluate_expr(ctx, argv[0]);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(arg->kind != EXPR_NUMBER)
        return Error(ctx, "");
    ctx->a.cursor = chk;
    n->value = __builtin_floor(((Number*)arg)->value);
    return &n->e;
}

static
FORMULAFUNC(drsp_ceil){
    if(argc != 1) return Error(ctx, "");
    Number* n = expr_alloc(ctx, EXPR_NUMBER);
    if(!n) return NULL;
    char* chk = ctx->a.cursor;
    Expression* arg = evaluate_expr(ctx, argv[0]);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(arg->kind != EXPR_NUMBER)
        return Error(ctx, "");
    ctx->a.cursor = chk;
    n->value = __builtin_ceil(((Number*)arg)->value);
    return &n->e;
}

static
FORMULAFUNC(drsp_trunc){
    if(argc != 1) return Error(ctx, "");
    Number* n = expr_alloc(ctx, EXPR_NUMBER);
    if(!n) return NULL;
    char* chk = ctx->a.cursor;
    Expression* arg = evaluate_expr(ctx, argv[0]);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(arg->kind != EXPR_NUMBER)
        return Error(ctx, "");
    ctx->a.cursor = chk;
    n->value = __builtin_trunc(((Number*)arg)->value);
    return &n->e;
}
static
FORMULAFUNC(drsp_round){
    if(argc != 1) return Error(ctx, "");
    Number* n = expr_alloc(ctx, EXPR_NUMBER);
    if(!n) return NULL;
    char* chk = ctx->a.cursor;
    Expression* arg = evaluate_expr(ctx, argv[0]);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(arg->kind != EXPR_NUMBER)
        return Error(ctx, "");
    ctx->a.cursor = chk;
    n->value = __builtin_round(((Number*)arg)->value);
    return &n->e;
}
static
FORMULAFUNC(drsp_abs){
    if(argc != 1) return Error(ctx, "");
    Number* n = expr_alloc(ctx, EXPR_NUMBER);
    if(!n) return NULL;
    char* chk = ctx->a.cursor;
    Expression* arg = evaluate_expr(ctx, argv[0]);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(arg->kind != EXPR_NUMBER)
        return Error(ctx, "");
    ctx->a.cursor = chk;
    n->value = __builtin_fabs(((Number*)arg)->value);
    return &n->e;
}

static
FORMULAFUNC(drsp_tablelookup){
    // "needle", [haystack], [values]
    if(argc != 3 && argc != 4) return Error(ctx, "");
    char* chk = ctx->a.cursor;
    ExpressionKind nkind;
    union {
        StringView s;
        double d;
    } nval;
    {
        Expression* needle = evaluate_expr(ctx, argv[0]);
        if(!needle || needle->kind == EXPR_ERROR) return needle;
        nkind = needle->kind;
        if(nkind != EXPR_NUMBER && nkind != EXPR_STRING) return Error(ctx, "");
        if(nkind == EXPR_NUMBER)
            nval.d = ((Number*)needle)->value;
        else
            nval.s = ((String*)needle)->sv;
        ctx->a.cursor = chk;
    }
    intptr_t offset = -1;
    {
        Expression* haystack = evaluate_expr(ctx, argv[1]);
        if(!haystack || haystack->kind == EXPR_ERROR) return haystack;
        if(haystack->kind != EXPR_RANGE1D_COLUMN) return Error(ctx, "");
        intptr_t col, start, end;
        if(get_range1dcol(ctx, haystack, &col, &start, &end) != 0)
            return Error(ctx, "");

        char* loopchk = ctx->a.cursor;
        for(intptr_t row = start; row <= end; ctx->a.cursor=loopchk, row++){
            Expression* e = evaluate(ctx, row, col);
            if(!e) return e;
            if(e->kind != nkind) continue;
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
        ctx->a.cursor = chk;
        if(offset < 0) {
            if(argc == 4)
                return evaluate_expr(ctx, argv[3]);
            return Error(ctx, "Didn't find needle in haystack");
        }
    }
    {
        Expression* values = evaluate_expr(ctx, argv[2]);
        if(!values || values->kind == EXPR_ERROR) return values;
        if(values->kind != EXPR_RANGE1D_COLUMN) return Error(ctx, "values must be a column range");
        intptr_t col, start, end;
        if(get_range1dcol(ctx, values, &col, &start, &end) != 0)
            return Error(ctx, "invalid column range");
        ctx->a.cursor = chk;
        if(start+offset > end) return Error(ctx, "out of bounds");
        return evaluate(ctx, start+offset, col);
    }
}

static
FORMULAFUNC(drsp_find){
    // "needle", [haystack]
    if(argc != 2) return Error(ctx, "");
    char* chk = ctx->a.cursor;
    ExpressionKind nkind;
    union {
        StringView s;
        double d;
    } nval;
    {
        Expression* needle = evaluate_expr(ctx, argv[0]);
        if(!needle || needle->kind == EXPR_ERROR) return needle;
        nkind = needle->kind;
        if(nkind != EXPR_NUMBER && nkind != EXPR_STRING && nkind != EXPR_NULL) return Error(ctx, "");
        if(nkind == EXPR_NUMBER)
            nval.d = ((Number*)needle)->value;
        else if(nkind == EXPR_STRING)
            nval.s = ((String*)needle)->sv;
        ctx->a.cursor = chk;
    }
    intptr_t offset = -1;
    {
        Expression* haystack = evaluate_expr(ctx, argv[1]);
        if(!haystack || haystack->kind == EXPR_ERROR) return haystack;
        if(haystack->kind != EXPR_RANGE1D_COLUMN) return Error(ctx, "");
        intptr_t col, start, end;
        if(get_range1dcol(ctx, haystack, &col, &start, &end) != 0)
            return Error(ctx, "");

        char* loopchk = ctx->a.cursor;
        for(intptr_t row = start; row <= end; ctx->a.cursor=loopchk, row++){
            Expression* e = evaluate(ctx, row, col);
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
        ctx->a.cursor = chk;
    }
    if(offset < 0) return Error(ctx, "");
    Number* n = expr_alloc(ctx, EXPR_NUMBER);
    n->value = offset;
    return &n->e;
}

#ifndef SV
#define SV(x) {sizeof(x)-1, x}
#endif

static const FuncInfo FUNCTABLE[] = {
    {{3, "sum"  }, &drsp_sum},
    {{3, "avg"  }, &drsp_avg},
    {{3, "min"  }, &drsp_min},
    {{3, "max"  }, &drsp_max},
    {{3, "mod"  }, &drsp_mod},
    {{3, "abs"  }, &drsp_abs},
    {{5, "floor"}, &drsp_floor},
    {{4, "ceil" }, &drsp_ceil},
    {{5, "trunc"}, &drsp_trunc},
    {{5, "round"}, &drsp_round},
    {{3, "tlu"  }, &drsp_tablelookup},
    {{4, "find" }, &drsp_find},
};
static const size_t FUNCTABLE_LENGTH = arrlen(FUNCTABLE);
#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#endif
