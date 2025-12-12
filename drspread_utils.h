//
// Copyright © 2023-2025, David Priver <david@davidpriver.com>
//
#ifndef DRSPREAD_UTILS_H
#define DRSPREAD_UTILS_H
#include "drspread_types.h"

#ifdef __clang__
#pragma clang assume_nonnull begin
#endif

static inline
int
get_range1dcol(DrSpreadCtx*ctx, SheetData* sd, Expression* arg, intptr_t* col, intptr_t* rowstart, intptr_t* rowend, SheetData* _Nonnull *_Nonnull rsd, intptr_t caller_row, intptr_t caller_col){
    if(arg->kind != EXPR_RANGE1D_COLUMN && arg->kind != EXPR_RANGE1D_COLUMN_FOREIGN)
        return 1;
    if(arg->kind == EXPR_RANGE1D_COLUMN_FOREIGN){
        DrspAtom sheet_name = ((ForeignRange1DColumn*)arg)->sheet_name;
        SheetData* hd = sheet_lookup_by_name(ctx, sheet_name);
        if(!hd) return 1;
        assert(hd);
        sd = hd;
        *rsd = sd;
    }
    Range1DColumn* rng = (Range1DColumn*)arg;
    intptr_t start = rng->row_start;
    if(start == IDX_DOLLAR) start = caller_row;
    intptr_t colnum;
    if(rng->col_name == drsp_dollar_atom())
        colnum = caller_col;
    else if(!rng->col_name->length)
        colnum = 0;
    else {
        colnum = sp_name_to_col_idx(sd, rng->col_name);
        if(colnum == -1) return 1;
    }
    if(start < 0) start += sp_col_height(sd, colnum);
    intptr_t end = rng->row_end;
    if(end == IDX_DOLLAR) end = caller_row;
    if(end < 0) end += sp_col_height(sd, colnum);
    if(end < start){
        intptr_t tmp = end;
        end = start;
        start = tmp;
    }
    if(end < 0) return 1;
    if(start < 0) return 1;
    // XXX: this is kind of a hack.
    if(end - start > DRSP_MAX_ROWS) return 1;
    *col = colnum;
    *rowstart = start;
    *rowend = end;
    return 0;
}

static inline
int
get_range1drow(DrSpreadCtx*ctx, SheetData* sd, Expression* arg, intptr_t* row, intptr_t* colstart, intptr_t* colend, SheetData* _Nonnull *_Nonnull rsd, intptr_t caller_row, intptr_t caller_col){
    if(arg->kind != EXPR_RANGE1D_ROW && arg->kind != EXPR_RANGE1D_ROW_FOREIGN)
        return 1;
    if(arg->kind == EXPR_RANGE1D_ROW_FOREIGN){
        DrspAtom sheet_name = ((ForeignRange1DRow*)arg)->sheet_name;
        SheetData* hd = sheet_lookup_by_name(ctx, sheet_name);
        if(!hd) return 1;
        sd = hd;
        *rsd = hd;
    }
    Range1DRow* rng = (Range1DRow*)arg;
    intptr_t start;
    if(rng->col_start == drsp_dollar_atom())
        start = caller_col;
    else if(!rng->col_start->length){
        start = 0;
    }
    else
        start = sp_name_to_col_idx(sd, rng->col_start);
    if(start == -1) return 1;
    intptr_t row_idx = rng->row_idx;
    if(row_idx == IDX_DOLLAR) row_idx = caller_row;
    intptr_t end;
    if(rng->col_end == drsp_dollar_atom())
        end = caller_col;
    else if(!rng->col_end->length){
        end = sp_row_width(sd, row_idx);
    }
    else
        end = sp_name_to_col_idx(sd, rng->col_end);
    if(end == -1) return 1;
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

// GCOV_EXCL_START
static inline
_Bool
evaled_is_not_scalar(Expression*_Nullable e){
    if(!e) return 1;
    switch(e->kind){
        case EXPR_ERROR:
            return 1;
        case EXPR_STRING:
        case EXPR_BLANK:
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
        case EXPR_BLANK:
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
        case EXPR_BLANK:
            return 0;
        case EXPR_NUMBER:
            return !!((Number*)e)->value;
        case EXPR_STRING:
            return !!((String*)e)->str->length;
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
        case EXPR_LAZY_ARRAY:
            return 1;
        default:
            return 0;
    }
}

// Get the length of an arraylike expression.
// Returns -1 on error.
static inline
intptr_t
arraylike_length(DrSpreadCtx* ctx, SheetData* sd, Expression* e, intptr_t caller_row, intptr_t caller_col){
    switch(e->kind){
        case EXPR_COMPUTED_ARRAY:
            return ((ComputedArray*)e)->length;
        case EXPR_LAZY_ARRAY:
            return ((LazyArray*)e)->length;
        case EXPR_RANGE1D_COLUMN:
        case EXPR_RANGE1D_COLUMN_FOREIGN: {
            intptr_t col, start, end;
            SheetData* rsd = sd;
            if(get_range1dcol(ctx, sd, e, &col, &start, &end, &rsd, caller_row, caller_col))
                return -1;
            return end - start + 1;
        }
        case EXPR_RANGE1D_ROW:
        case EXPR_RANGE1D_ROW_FOREIGN: {
            intptr_t row, start, end;
            SheetData* rsd = sd;
            if(get_range1drow(ctx, sd, e, &row, &start, &end, &rsd, caller_row, caller_col))
                return -1;
            return end - start + 1;
        }
        default:
            return -1;
    }
}

// Lazy array constructors
static inline
LazyArray*_Nullable
lazy_from_range_col(DrSpreadCtx* ctx, SheetData* range_sd, intptr_t col, intptr_t start, intptr_t length){
    LazyArray* la = expr_alloc(ctx, EXPR_LAZY_ARRAY);
    if(!la) return NULL;
    la->length = length;
    la->kind = LAZY_RANGE_COL;
    la->range_col.sd = range_sd;
    la->range_col.col = col;
    la->range_col.start = start;
    return la;
}

static inline
LazyArray*_Nullable
lazy_from_range_row(DrSpreadCtx* ctx, SheetData* range_sd, intptr_t row, intptr_t start, intptr_t length){
    LazyArray* la = expr_alloc(ctx, EXPR_LAZY_ARRAY);
    if(!la) return NULL;
    la->length = length;
    la->kind = LAZY_RANGE_ROW;
    la->range_row.sd = range_sd;
    la->range_row.row = row;
    la->range_row.start = start;
    return la;
}

static inline
LazyArray*_Nullable
lazy_unary(DrSpreadCtx* ctx, UnaryDoubleOp op, Expression* source, intptr_t length){
    LazyArray* la = expr_alloc(ctx, EXPR_LAZY_ARRAY);
    if(!la) return NULL;
    la->length = length;
    la->kind = LAZY_UNARY;
    la->unary.op = op;
    la->unary.source = source;
    return la;
}

static inline
LazyArray*_Nullable
lazy_binary(DrSpreadCtx* ctx, BinaryKind op, Expression* lhs, Expression* rhs, intptr_t length){
    LazyArray* la = expr_alloc(ctx, EXPR_LAZY_ARRAY);
    if(!la) return NULL;
    la->length = length;
    la->kind = LAZY_BINARY;
    la->binary.op = op;
    la->binary.lhs = lhs;
    la->binary.rhs = rhs;
    return la;
}

static inline
LazyArray*_Nullable
lazy_binary_func(DrSpreadCtx* ctx, BinaryDoubleOp op, Expression* lhs, Expression* rhs, intptr_t length){
    LazyArray* la = expr_alloc(ctx, EXPR_LAZY_ARRAY);
    if(!la) return NULL;
    la->length = length;
    la->kind = LAZY_BINARY_FUNC;
    la->binary_func.op = op;
    la->binary_func.lhs = lhs;
    la->binary_func.rhs = rhs;
    return la;
}

// Convert a range expression to a lazy array
static inline
Expression*_Nullable
range_to_lazy(DrSpreadCtx* ctx, SheetData* sd, Expression* e, intptr_t caller_row, intptr_t caller_col){
    if(e->kind == EXPR_LAZY_ARRAY || e->kind == EXPR_COMPUTED_ARRAY)
        return e;
    if(e->kind == EXPR_RANGE1D_COLUMN || e->kind == EXPR_RANGE1D_COLUMN_FOREIGN){
        intptr_t col, start, end;
        SheetData* rsd = sd;
        if(get_range1dcol(ctx, sd, e, &col, &start, &end, &rsd, caller_row, caller_col))
            return Error(ctx, "Invalid range");
        intptr_t len = end - start + 1;
        if(len <= 0) return Error(ctx, "Empty range");
        if(rsd != sd){
            int err = sheet_add_dependant(ctx, rsd, sd->handle);
            if(err) return Error(ctx, "oom");
        }
        LazyArray* la = lazy_from_range_col(ctx, rsd, col, start, len);
        return la ? &la->e : NULL;
    }
    if(e->kind == EXPR_RANGE1D_ROW || e->kind == EXPR_RANGE1D_ROW_FOREIGN){
        intptr_t row, start, end;
        SheetData* rsd = sd;
        if(get_range1drow(ctx, sd, e, &row, &start, &end, &rsd, caller_row, caller_col))
            return Error(ctx, "Invalid range");
        intptr_t len = end - start + 1;
        if(len <= 0) return Error(ctx, "Empty range");
        if(rsd != sd){
            int err = sheet_add_dependant(ctx, rsd, sd->handle);
            if(err) return Error(ctx, "oom");
        }
        LazyArray* la = lazy_from_range_row(ctx, rsd, row, start, len);
        return la ? &la->e : NULL;
    }
    return Error(ctx, "Not a range");
}

// GCOV_EXCL_STOP

static inline
Expression*_Nullable
convert_to_computed_array(DrSpreadCtx* ctx, SheetData* sd, Expression* e, intptr_t caller_row, intptr_t caller_col){
    if(e->kind == EXPR_COMPUTED_ARRAY)
        return e;
    if(e->kind == EXPR_LAZY_ARRAY){
        // Materialize the lazy array
        LazyArray* la = (LazyArray*)e;
        intptr_t len = la->length;
        if(len <= 0) return Error(ctx, "");
        ComputedArray* cc = computed_array_alloc(ctx, len);
        if(!cc) return NULL;
        for(intptr_t i = 0; i < len; i++){
            Expression* val = arraylike_get(ctx, sd, e, i, caller_row, caller_col);
            if(!val || val->kind == EXPR_ERROR) return val;
            cc->data[i] = val;
        }
        return &cc->e;
    }
    if(e->kind == EXPR_RANGE1D_ROW || e->kind == EXPR_RANGE1D_ROW_FOREIGN){
        SheetData* rsd = sd;
        intptr_t row, colstart, colend;
        if(get_range1drow(ctx, sd, e, &row, &colstart, &colend, &rsd, caller_row, caller_col))
            return Error(ctx, "");
        intptr_t len = colend - colstart + 1;
        // Can't express a zero-length range
        if(len <= 0) return Error(ctx, "");
        ComputedArray* cc = computed_array_alloc(ctx, len);
        if(!cc) return NULL;
        Expression** data = cc->data;
        intptr_t i = 0;
        if(rsd != sd){
            int err = sheet_add_dependant(ctx, rsd, sd->handle);
            if(err) return Error(ctx, "oom");
        }
        for(intptr_t col = colstart; col <= colend; col++){
            Expression* val = evaluate(ctx, rsd, row, col);
            if(!val || val->kind == EXPR_ERROR) return val;
            if(evaled_is_not_scalar(val)) return Error(ctx, "");
            data[i++] = val;
        }
        assert(i == len);
        return &cc->e;
    }
    SheetData* rsd = sd;
    intptr_t col, rstart, rend;
    if(get_range1dcol(ctx, sd, e, &col, &rstart, &rend, &rsd, caller_row, caller_col))
        return Error(ctx, "");
    intptr_t len = rend - rstart + 1;
    // Can't express a zero-length range
    if(len <= 0) return Error(ctx, "");
    ComputedArray* cc = computed_array_alloc(ctx, len);
    if(!cc) return NULL;
    Expression** data = cc->data;
    intptr_t i = 0;
    if(rsd != sd){
        int err = sheet_add_dependant(ctx, rsd, sd->handle);
        if(err) return Error(ctx, "oom");
    }
    for(intptr_t row = rstart; row <= rend; row++){
        Expression* val = evaluate(ctx, rsd, row, col);
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
