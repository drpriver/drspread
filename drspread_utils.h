#ifndef DRSPREAD_UTILS_H
#define DRSPREAD_UTILS_H
#include "drspread_types.h"

#ifdef __clang__
#pragma clang assume_nonnull begin
#endif

static inline
int
get_range1dcol(SpreadContext*ctx, SheetHandle hnd, Expression* arg, intptr_t* col, intptr_t* rowstart, intptr_t* rowend, SheetHandle _Nonnull *_Nonnull rhnd, intptr_t caller_row, intptr_t caller_col){
    if(arg->kind != EXPR_RANGE1D_COLUMN && arg->kind != EXPR_RANGE1D_COLUMN_FOREIGN)
        return 1;
    if(arg->kind == EXPR_RANGE1D_COLUMN_FOREIGN){
        StringView sheet_name = ((ForeignRange1DColumn*)arg)->sheet_name;
        hnd = sp_name_to_sheet(ctx, sheet_name.text, sheet_name.length);
        if(!hnd) return 1;
        *rhnd = hnd;
    }
    Range1DColumn* rng = (Range1DColumn*)arg;
    intptr_t start = rng->row_start;
    if(start == IDX_DOLLAR) start = caller_row;
    intptr_t colnum;
    if(sv_equals(rng->col_name, SV("$"))){
        colnum = caller_col;
    }
    else {
        colnum = sp_name_to_col_idx(ctx, hnd, rng->col_name.text, rng->col_name.length);
    }
    if(start < 0) start += sp_col_height(ctx, hnd, colnum);
    intptr_t end = rng->row_end;
    if(end == IDX_DOLLAR) end = caller_row;
    if(end < 0) end += sp_col_height(ctx, hnd, colnum);
    if(end < start){
        intptr_t tmp = end;
        end = start;
        start = tmp;
    }
    *col = colnum;
    *rowstart = start;
    *rowend = end;
    return 0;
}

static inline
_Bool
evaled_is_not_scalar(Expression*_Nullable e){
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
static inline
_Bool
expr_is_scalar(Expression* e){
    switch(e->kind){
        case EXPR_STRING:
        case EXPR_NULL:
        case EXPR_NUMBER:
            return 1;
        default:
            return 0;
    }
}

static inline
_Bool
scalar_expr_is_truthy(Expression* e){
    switch(e->kind){
        case EXPR_NULL:
            return 0;
        case EXPR_NUMBER:
            return !!((Number*)e)->value;
        case EXPR_STRING:
            return !!((String*)e)->sv.length;
        default:
            __builtin_unreachable();
    }
}


static inline
_Bool
expr_is_columnar(Expression* e){
    switch(e->kind){
        case EXPR_RANGE1D_COLUMN:
        case EXPR_RANGE1D_COLUMN_FOREIGN:
        case EXPR_COMPUTED_COLUMN:
            return 1;
        default:
            return 0;
    }
}

static inline
Expression*_Nullable
convert_to_computed_column(SpreadContext* ctx, SheetHandle hnd, Expression* e, intptr_t caller_row, intptr_t caller_col){
    if(e->kind == EXPR_COMPUTED_COLUMN)
        return e;
    SheetHandle rhnd = hnd;
    intptr_t col, rstart, rend;
    if(get_range1dcol(ctx, hnd, e, &col, &rstart, &rend, &rhnd, caller_row, caller_col))
        return Error(ctx, "");
    intptr_t len = rend - rstart + 1;
    // Can't express a zero-length range
    if(len <= 0) return Error(ctx, "");
    ComputedColumn* cc = buff_alloc(&ctx->a, __builtin_offsetof(ComputedColumn, data)+sizeof(Expression*)*len);
    if(!cc) return NULL;
    cc->e.kind = EXPR_COMPUTED_COLUMN;
    Expression** data = cc->data;
    intptr_t i = 0;
    for(intptr_t row = rstart; row <= rend; row++){
        Expression* val = evaluate(ctx, rhnd, row, col);
        if(!val || val->kind == EXPR_ERROR) return val;
        if(evaled_is_not_scalar(val)) return Error(ctx, "");
        data[i++] = val;
    }
    assert(i == len);
    cc->length = len;
    return &cc->e;
}



#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#endif
