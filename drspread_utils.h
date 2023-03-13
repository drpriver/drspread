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
        SheetHandle _Nullable h = sp_name_to_sheet(ctx, sheet_name.text, sheet_name.length);
        if(!h) return 1;
        hnd = h;
        *rhnd = h;
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
int
get_range1drow(SpreadContext*ctx, SheetHandle hnd, Expression* arg, intptr_t* row, intptr_t* colstart, intptr_t* colend, SheetHandle _Nonnull *_Nonnull rhnd, intptr_t caller_row, intptr_t caller_col){
    if(arg->kind != EXPR_RANGE1D_ROW && arg->kind != EXPR_RANGE1D_ROW_FOREIGN)
        return 1;
    if(arg->kind == EXPR_RANGE1D_ROW_FOREIGN){
        StringView sheet_name = ((ForeignRange1DRow*)arg)->sheet_name;
        SheetHandle _Nullable h = sp_name_to_sheet(ctx, sheet_name.text, sheet_name.length);
        if(!h) return 1;
        hnd = h;
        *rhnd = h;
    }
    Range1DRow* rng = (Range1DRow*)arg;
    StringView sv_start = rng->col_start;
    intptr_t start;
    if(sv_equals(sv_start, SV("$")))
        start = caller_col;
    else
        start = sp_name_to_col_idx(ctx, hnd, sv_start.text, sv_start.length);
    StringView sv_end = rng->col_end;
    intptr_t end;
    if(sv_equals(sv_end, SV("$")))
        end = caller_col;
    else
        end = sp_name_to_col_idx(ctx, hnd, sv_end.text, sv_end.length);
    intptr_t row_idx = rng->row_idx;
    if(row_idx == IDX_DOLLAR) row_idx = caller_row;
    if(end < start){
        intptr_t tmp = end;
        end = start;
        start = tmp;
    }
    *row = row_idx;
    *colstart = start;
    *colend = end;
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
expr_is_arraylike(Expression* e){
    switch(e->kind){
        case EXPR_RANGE1D_COLUMN:
        case EXPR_RANGE1D_COLUMN_FOREIGN:
        case EXPR_COMPUTED_ARRAY:
        case EXPR_RANGE1D_ROW:
        case EXPR_RANGE1D_ROW_FOREIGN:
            return 1;
        default:
            return 0;
    }
}

static inline
Expression*_Nullable
convert_to_computed_array(SpreadContext* ctx, SheetHandle hnd, Expression* e, intptr_t caller_row, intptr_t caller_col){
    if(e->kind == EXPR_COMPUTED_ARRAY)
        return e;
    if(e->kind == EXPR_RANGE1D_ROW || e->kind == EXPR_RANGE1D_ROW_FOREIGN){
        SheetHandle rhnd = hnd;
        intptr_t row, colstart, colend;
        if(get_range1drow(ctx, hnd, e, &row, &colstart, &colend, &rhnd, caller_row, caller_col))
            return Error(ctx, "");
        intptr_t len = colend - colstart + 1;
        // Can't express a zero-length range
        if(len <= 0) return Error(ctx, "");
        ComputedArray* cc = computed_array_alloc(ctx, len);
        if(!cc) return NULL;
        Expression** data = cc->data;
        intptr_t i = 0;
        for(intptr_t col = colstart; col <= colend; col++){
            Expression* val = evaluate(ctx, rhnd, row, col);
            if(!val || val->kind == EXPR_ERROR) return val;
            if(evaled_is_not_scalar(val)) return Error(ctx, "");
            data[i++] = val;
        }
        assert(i == len);
        return &cc->e;
    }
    SheetHandle rhnd = hnd;
    intptr_t col, rstart, rend;
    if(get_range1dcol(ctx, hnd, e, &col, &rstart, &rend, &rhnd, caller_row, caller_col))
        return Error(ctx, "");
    intptr_t len = rend - rstart + 1;
    // Can't express a zero-length range
    if(len <= 0) return Error(ctx, "");
    ComputedArray* cc = computed_array_alloc(ctx, len);
    if(!cc) return NULL;
    Expression** data = cc->data;
    intptr_t i = 0;
    for(intptr_t row = rstart; row <= rend; row++){
        Expression* val = evaluate(ctx, rhnd, row, col);
        if(!val || val->kind == EXPR_ERROR) return val;
        if(evaled_is_not_scalar(val)) return Error(ctx, "");
        data[i++] = val;
    }
    assert(i == len);
    return &cc->e;
}



#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#endif
