//
// Copyright © 2023, David Priver
//
#ifndef DRSPREAD_FORMULA_FUNCS_C
#define DRSPREAD_FORMULA_FUNCS_C
#include "drspread_types.h"
#include "drspread_evaluate.h"
#include "drspread_utils.h"
#ifdef __clang__
#pragma clang assume_nonnull begin
#endif

#ifndef arrlen
#define arrlen(x) (sizeof(x)/sizeof(x[0]))
#endif

#if defined(TESTING_H) && !defined(DRSP_INTRINS)
#define DRSP_INTRINS 1
#endif

DRSP_INTERNAL
FORMULAFUNC(drsp_sum){
    if(argc != 1) return Error(ctx, "");
    BuffCheckpoint bc = buff_checkpoint(&ctx->a);
    Expression* arg = evaluate_expr(ctx, hnd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    double sum = 0;
    if(arg->kind == EXPR_COMPUTED_COLUMN){
        ComputedColumn* c = (ComputedColumn*)arg;
        for(intptr_t i = 0; i < c->length; i++){
            Expression* e = c->data[i];
            if(e->kind != EXPR_NUMBER)
                continue;
            sum += ((Number*)e)->value;
        }
    }
    else {
        intptr_t col, start, end;
        SheetHandle rhnd = hnd;
        if(get_range1dcol(ctx, hnd, arg, &col, &start, &end, &rhnd, caller_row, caller_col) != 0)
            return Error(ctx, "");
        // NOTE: inclusive range
        for(intptr_t row = start; row <= end; row++){
            BuffCheckpoint bc = buff_checkpoint(&ctx->a);
            Expression* e = evaluate(ctx, rhnd, row, col);
            if(!e || e->kind == EXPR_ERROR) return e;
            if(evaled_is_not_scalar(e)) return Error(ctx, "");
            if(e->kind != EXPR_NUMBER) continue;
            sum += ((Number*)e)->value;
            buff_set(&ctx->a, bc);
        }
    }
    buff_set(&ctx->a, bc);
    Number* n = expr_alloc(ctx, EXPR_NUMBER);
    if(!n) return NULL;
    n->value = sum;
    return &n->e;
}
DRSP_INTERNAL
FORMULAFUNC(drsp_prod){
    if(argc != 1) return Error(ctx, "");
    BuffCheckpoint bc = buff_checkpoint(&ctx->a);
    Expression* arg = evaluate_expr(ctx, hnd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    double prod = 1;
    if(arg->kind == EXPR_COMPUTED_COLUMN){
        ComputedColumn* c = (ComputedColumn*)arg;
        for(intptr_t i = 0; i < c->length; i++){
            Expression* e = c->data[i];
            if(e->kind != EXPR_NUMBER)
                continue;
            prod *= ((Number*)e)->value;
        }
    }
    else {
        intptr_t col, start, end;
        SheetHandle rhnd = hnd;
        if(get_range1dcol(ctx, hnd, arg, &col, &start, &end, &rhnd, caller_row, caller_col) != 0)
            return Error(ctx, "");
        // NOTE: inclusive range
        for(intptr_t row = start; row <= end; row++){
            BuffCheckpoint bc = buff_checkpoint(&ctx->a);
            Expression* e = evaluate(ctx, rhnd, row, col);
            if(!e || e->kind == EXPR_ERROR) return e;
            if(evaled_is_not_scalar(e)) return Error(ctx, "");
            if(e->kind != EXPR_NUMBER) continue;
            prod *= ((Number*)e)->value;
            buff_set(&ctx->a, bc);
        }
    }
    buff_set(&ctx->a, bc);
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
    BuffCheckpoint bc = buff_checkpoint(&ctx->a);
    Expression* arg = evaluate_expr(ctx, hnd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    double sum = 0.;
    double count = 0.;
    if(arg->kind == EXPR_COMPUTED_COLUMN){
        ComputedColumn* c = (ComputedColumn*)arg;
        for(intptr_t i = 0; i < c->length; i++){
            Expression* e = c->data[i];
            if(e->kind != EXPR_NUMBER) continue;
            sum += ((Number*)e)->value;
            count += 1.0;
        }
    }
    else {
        intptr_t col, start, end;
        SheetHandle rhnd = hnd;
        if(get_range1dcol(ctx, hnd, arg, &col, &start, &end, &rhnd, caller_row, caller_col) != 0)
            return Error(ctx, "");
        // NOTE: inclusive range
        for(intptr_t row = start; row <= end; row++){
            BuffCheckpoint bc = buff_checkpoint(&ctx->a);
            Expression* e = evaluate(ctx, rhnd, row, col);
            if(!e || e->kind == EXPR_ERROR) return e;
            if(evaled_is_not_scalar(e)) return Error(ctx, "");
            if(e->kind != EXPR_NUMBER) continue;
            sum += ((Number*)e)->value;
            count += 1.0;
            buff_set(&ctx->a, bc);
        }
    }
    buff_set(&ctx->a, bc);
    n->value = count!=0.?sum/count:0;
    return &n->e;
}

DRSP_INTERNAL
FORMULAFUNC(drsp_count){
    if(argc != 1) return Error(ctx, "");
    Number* n = expr_alloc(ctx, EXPR_NUMBER);
    if(!n) return NULL;
    BuffCheckpoint bc = buff_checkpoint(&ctx->a);
    Expression* arg = evaluate_expr(ctx, hnd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    intptr_t count = 0;
    if(arg->kind == EXPR_COMPUTED_COLUMN){
        ComputedColumn* c = (ComputedColumn*)arg;
        for(intptr_t i = 0; i < c->length; i++){
            Expression* e= c->data[i];
            if(e->kind != EXPR_NUMBER && e->kind != EXPR_STRING)
                continue;
            count += 1;
        }
    }
    else {
        SheetHandle rhnd = hnd;
        intptr_t col, start, end;
        if(get_range1dcol(ctx, hnd, arg, &col, &start, &end, &rhnd, caller_row, caller_col) != 0)
            return Error(ctx, "");
        // NOTE: inclusive range
        for(intptr_t row = start; row <= end; row++){
            BuffCheckpoint bc = buff_checkpoint(&ctx->a);
            Expression* e = evaluate(ctx, rhnd, row, col);
            if(!e || e->kind == EXPR_ERROR) return e;
            if(e->kind != EXPR_NUMBER && e->kind != EXPR_STRING)
                continue;
            count += 1;
            buff_set(&ctx->a, bc);
        }
    }
    buff_set(&ctx->a, bc);
    n->value = (double)count;
    return &n->e;
}

DRSP_INTERNAL
FORMULAFUNC(drsp_min){
    if(!argc) return Error(ctx, "");
    if(argc > 1){
        BuffCheckpoint bc = buff_checkpoint(&ctx->a);
        double v = 1e32;
        for(;argc;argc--, argv++, buff_set(&ctx->a, bc)){
            Expression* arg = evaluate_expr(ctx, hnd, argv[0], caller_row, caller_col);
            if(!arg || arg->kind == EXPR_ERROR)
                return arg;
            if(arg->kind != EXPR_NUMBER)
                return Error(ctx, "");
            double n = ((Number*)arg)->value;
            if(n < v)
                v = n;
        }
        Number* n = expr_alloc(ctx, EXPR_NUMBER);
        n->value = v == 1e32?0:v;
        return &n->e;
    }
    Number* n = expr_alloc(ctx, EXPR_NUMBER);
    if(!n) return NULL;
    BuffCheckpoint bc = buff_checkpoint(&ctx->a);
    Expression* arg = evaluate_expr(ctx, hnd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    double v = 1e32;
    if(arg->kind == EXPR_COMPUTED_COLUMN){
        ComputedColumn* c = (ComputedColumn*)arg;
        for(intptr_t i = 0; i < c->length; i++){
            Expression* e = c->data[i];
            if(e->kind != EXPR_NUMBER) continue;
            if(((Number*)e)->value < v)
                v = ((Number*)e)->value;
        }
    }
    else {
        SheetHandle rhnd = hnd;
        intptr_t col, start, end;
        if(get_range1dcol(ctx, hnd, arg, &col, &start, &end, &rhnd, caller_row, caller_col) != 0)
            return Error(ctx, "");
        // NOTE: inclusive range
        for(intptr_t row = start; row <= end; row++){
            BuffCheckpoint bc = buff_checkpoint(&ctx->a);
            Expression* e = evaluate(ctx, rhnd, row, col);
            if(!e || e->kind == EXPR_ERROR) return e;
            if(evaled_is_not_scalar(e)) return Error(ctx, "");
            if(e->kind != EXPR_NUMBER) continue;
            if(((Number*)e)->value < v)
                v = ((Number*)e)->value;
            buff_set(&ctx->a, bc);
        }
    }
    if(v == 1e32) v = 0.;
    buff_set(&ctx->a, bc);
    n->value = v;
    return &n->e;
}

DRSP_INTERNAL
FORMULAFUNC(drsp_max){
    if(!argc) return Error(ctx, "");
    if(argc > 1){
        BuffCheckpoint bc = buff_checkpoint(&ctx->a);
        double v = -1e32;
        for(;argc;argc--, argv++, buff_set(&ctx->a, bc)){
            Expression* arg = evaluate_expr(ctx, hnd, argv[0], caller_row, caller_col);
            if(!arg || arg->kind == EXPR_ERROR)
                return arg;
            if(arg->kind != EXPR_NUMBER)
                return Error(ctx, "");
            double n = ((Number*)arg)->value;
            if(n > v)
                v = n;
        }
        Number* n = expr_alloc(ctx, EXPR_NUMBER);
        n->value = v == -1e32?0:v;
        return &n->e;
    }
    Number* n = expr_alloc(ctx, EXPR_NUMBER);
    if(!n) return NULL;
    BuffCheckpoint bc = buff_checkpoint(&ctx->a);
    Expression* arg = evaluate_expr(ctx, hnd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    double v = -1e32;
    if(arg->kind == EXPR_COMPUTED_COLUMN){
        ComputedColumn* c = (ComputedColumn*)arg;
        for(intptr_t i = 0; i < c->length; i++){
            Expression* e = c->data[i];
            if(e->kind != EXPR_NUMBER) continue;
            if(((Number*)e)->value > v)
                v = ((Number*)e)->value;
        }
    }
    else {
        SheetHandle rhnd = hnd;
        intptr_t col, start, end;
        if(get_range1dcol(ctx, hnd, arg, &col, &start, &end, &rhnd, caller_row, caller_col) != 0)
            return Error(ctx, "");
        // NOTE: inclusive range
        for(intptr_t row = start; row <= end; row++){
            BuffCheckpoint bc = buff_checkpoint(&ctx->a);
            Expression* e = evaluate(ctx, rhnd, row, col);
            if(!e || e->kind == EXPR_ERROR) return e;
            if(evaled_is_not_scalar(e)) return Error(ctx, "");
            if(e->kind != EXPR_NUMBER) continue;
            if(((Number*)e)->value > v)
                v = ((Number*)e)->value;
            buff_set(&ctx->a, bc);
        }
    }
    if(v == -1e32) v = 0.;
    buff_set(&ctx->a, bc);
    n->value = v;
    return &n->e;
}

DRSP_INTERNAL
FORMULAFUNC(drsp_mod){
    if(argc != 1) return Error(ctx, "");
    BuffCheckpoint bc = buff_checkpoint(&ctx->a);
    Expression* arg = evaluate_expr(ctx, hnd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(expr_is_columnar(arg)){
        arg = convert_to_computed_column(ctx, hnd, arg, caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        ComputedColumn* c = (ComputedColumn*)arg;
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
        buff_set(&ctx->a, bc);
        Number* n = expr_alloc(ctx, EXPR_NUMBER);
        if(!n) return NULL;
        n->value = mod;
        return &n->e;
    }
}

DRSP_INTERNAL
FORMULAFUNC(drsp_floor){
    if(argc != 1) return Error(ctx, "");
    BuffCheckpoint bc = buff_checkpoint(&ctx->a);
    Expression* arg = evaluate_expr(ctx, hnd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(expr_is_columnar(arg)){
        arg = convert_to_computed_column(ctx, hnd, arg, caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        ComputedColumn* c = (ComputedColumn*)arg;
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
        buff_set(&ctx->a, bc);
        Number* n = expr_alloc(ctx, EXPR_NUMBER);
        if(!n) return NULL;
        n->value = __builtin_floor(((Number*)arg)->value);
        return &n->e;
    }
}

DRSP_INTERNAL
FORMULAFUNC(drsp_ceil){
    if(argc != 1) return Error(ctx, "");
    BuffCheckpoint bc = buff_checkpoint(&ctx->a);
    Expression* arg = evaluate_expr(ctx, hnd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(expr_is_columnar(arg)){
        arg = convert_to_computed_column(ctx, hnd, arg, caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        ComputedColumn* c = (ComputedColumn*)arg;
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
        buff_set(&ctx->a, bc);
        Number* n = expr_alloc(ctx, EXPR_NUMBER);
        if(!n) return NULL;
        n->value = __builtin_ceil(((Number*)arg)->value);
        return &n->e;
    }
}

DRSP_INTERNAL
FORMULAFUNC(drsp_trunc){
    if(argc != 1) return Error(ctx, "");
    BuffCheckpoint bc = buff_checkpoint(&ctx->a);
    Expression* arg = evaluate_expr(ctx, hnd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(expr_is_columnar(arg)){
        arg = convert_to_computed_column(ctx, hnd, arg, caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        ComputedColumn* c = (ComputedColumn*)arg;
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
        buff_set(&ctx->a, bc);
        Number* n = expr_alloc(ctx, EXPR_NUMBER);
        if(!n) return NULL;
        n->value = __builtin_trunc(((Number*)arg)->value);
        return &n->e;
    }
}

DRSP_INTERNAL
FORMULAFUNC(drsp_round){
    if(argc != 1) return Error(ctx, "");
    BuffCheckpoint bc = buff_checkpoint(&ctx->a);
    Expression* arg = evaluate_expr(ctx, hnd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(expr_is_columnar(arg)){
        arg = convert_to_computed_column(ctx, hnd, arg, caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        ComputedColumn* c = (ComputedColumn*)arg;
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
        buff_set(&ctx->a, bc);
        Number* n = expr_alloc(ctx, EXPR_NUMBER);
        if(!n) return NULL;
        n->value = __builtin_round(((Number*)arg)->value);
        return &n->e;
    }
}

DRSP_INTERNAL
FORMULAFUNC(drsp_abs){
    if(argc != 1) return Error(ctx, "");
    BuffCheckpoint bc = buff_checkpoint(&ctx->a);
    Expression* arg = evaluate_expr(ctx, hnd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(expr_is_columnar(arg)){
        arg = convert_to_computed_column(ctx, hnd, arg, caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        ComputedColumn* c = (ComputedColumn*)arg;
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
        buff_set(&ctx->a, bc);
        Number* n = expr_alloc(ctx, EXPR_NUMBER);
        if(!n) return NULL;
        n->value = __builtin_fabs(((Number*)arg)->value);
        return &n->e;
    }
}

DRSP_INTERNAL
FORMULAFUNC(drsp_sqrt){
    if(argc != 1) return Error(ctx, "");
    BuffCheckpoint bc = buff_checkpoint(&ctx->a);
    Expression* arg = evaluate_expr(ctx, hnd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(expr_is_columnar(arg)){
        arg = convert_to_computed_column(ctx, hnd, arg, caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        ComputedColumn* c = (ComputedColumn*)arg;
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
        buff_set(&ctx->a, bc);
        Number* n = expr_alloc(ctx, EXPR_NUMBER);
        if(!n) return NULL;
        n->value = __builtin_sqrt(((Number*)arg)->value);
        return &n->e;
    }
}

DRSP_INTERNAL
FORMULAFUNC(drsp_num){
    if(argc != 1 && argc != 2) return Error(ctx, "");
    BuffCheckpoint bc = buff_checkpoint(&ctx->a);
    double default_ = 0.;
    if(argc == 2){
        Expression* d = evaluate_expr(ctx, hnd, argv[1], caller_row, caller_col);
        if(!d || d->kind == EXPR_ERROR)
            return d;
        if(d->kind != EXPR_NUMBER)
            return Error(ctx, "");
        default_ = ((Number*)d)->value;
        buff_set(&ctx->a, bc);
    }
    Expression* arg = evaluate_expr(ctx, hnd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(expr_is_columnar(arg)){
        arg = convert_to_computed_column(ctx, hnd, arg, caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR)
            return arg;
        ComputedColumn* cc = (ComputedColumn*)arg;
        for(intptr_t i = 0; i < cc->length; i++){
            Expression* e = cc->data[i];
            if(e->kind == EXPR_NUMBER)
                continue;
            Number* n = expr_alloc(ctx, EXPR_NUMBER);
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
        buff_set(&ctx->a, bc);
        Number* n = expr_alloc(ctx, EXPR_NUMBER);
        if(!n) return NULL;
        n->value = value;
        return &n->e;
    }
}
DRSP_INTERNAL
FORMULAFUNC(drsp_try){
    if(argc != 2) return Error(ctx, "");
    char* chk = ctx->a.cursor;
    Expression* arg = evaluate_expr(ctx, hnd, argv[0], caller_row, caller_col);
    if(!arg) return NULL;
    if(arg->kind != EXPR_ERROR) return arg;
    ctx->a.cursor = chk;
    return evaluate_expr(ctx, hnd, argv[1], caller_row, caller_col);
}

DRSP_INTERNAL
FORMULAFUNC(drsp_cell){
    SheetHandle foreign = hnd;
    if(argc != 2 && argc != 3) return Error(ctx, "");
    if(argc == 3){
        Expression* sheet = evaluate_expr(ctx, hnd, argv[0], caller_row, caller_col);
        if(!sheet || sheet->kind == EXPR_ERROR) return sheet;
        if(sheet->kind != EXPR_STRING) return Error(ctx, "");
        StringView sv = ((String*)sheet)->sv;
        foreign = sp_name_to_sheet(ctx, sv.text, sv.length);
        if(!foreign) return Error(ctx, "");
        argv++, argc--;
    }
    Expression* col = evaluate_expr(ctx, hnd, argv[0], caller_row, caller_col);
    if(!col || col->kind == EXPR_ERROR) return col;
    if(col->kind != EXPR_STRING) return Error(ctx, "");
    StringView csv = ((String*)col)->sv;
    intptr_t col_idx = sp_name_to_col_idx(ctx, foreign, csv.text, csv.length);
    if(col_idx < 0) return Error(ctx, "");
    Expression* row = evaluate_expr(ctx, hnd, argv[1], caller_row, caller_col);
    if(!row || row->kind == EXPR_ERROR) return row;
    if(row->kind != EXPR_NUMBER) return Error(ctx, "");
    intptr_t row_idx = (intptr_t)((Number*)row)->value;
    row_idx--;
    if(row_idx < 0) return Error(ctx, "");
    return evaluate(ctx, foreign, row_idx, col_idx);
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
    BuffCheckpoint bc = buff_checkpoint(&ctx->a);
    {
        Expression* arg = evaluate_expr(ctx, hnd, argv[0], caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        if(arg->kind != EXPR_STRING) return Error(ctx, "");
        colname = ((String*)arg)->sv;
        argc--, argv++;
    }
    if(argc){
        Expression* arg = evaluate_expr(ctx, hnd, argv[0], caller_row, caller_col);
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
        Expression* arg = evaluate_expr(ctx, hnd, argv[0], caller_row, caller_col);
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
        Expression* arg = evaluate_expr(ctx, hnd, argv[0], caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        if(arg->kind != EXPR_NUMBER)
            return Error(ctx, "");
        rowend = (intptr_t)((Number*)arg)->value;
        argc--, argv++;
    }
    if(!colname.length) return Error(ctx, "");
    if(rowstart == IDX_UNSET)
        rowstart = 0;
    if(rowstart) rowstart--;
    if(rowend == IDX_UNSET)
        rowend = -1;
    else if(rowend) rowend--;
    if(argc) return Error(ctx, "");
    buff_set(&ctx->a, bc);
    if(sheetname.length){
        ForeignRange1DColumn* result = expr_alloc(ctx, EXPR_RANGE1D_COLUMN_FOREIGN);
        result->r.col_name = colname;
        result->sheet_name = sheetname;
        result->r.row_start = rowstart;
        result->r.row_end = rowend;
        return &result->e;
    }
    else {
        Range1DColumn* result = expr_alloc(ctx, EXPR_RANGE1D_COLUMN);
        result->col_name = colname;
        result->row_end = rowend;
        result->row_start = rowstart;
        return &result->e;
    }
}

DRSP_INTERNAL
FORMULAFUNC(drsp_eval){
    if(argc != 1) return Error(ctx, "");
    Expression* arg = evaluate_expr(ctx, hnd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(expr_is_columnar(arg)){
        arg = convert_to_computed_column(ctx, hnd, arg, caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        ComputedColumn* c = (ComputedColumn*)arg;
        for(intptr_t i = 0; i < c->length; i++){
            Expression* e = c->data[i];
            if(e->kind == EXPR_NULL) continue;
            if(e->kind != EXPR_STRING) return Error(ctx, "");
            StringView sv = ((String*)e)->sv;
            Expression* ev = evaluate_string(ctx, hnd, sv.text, sv.length, caller_row, caller_col);
            if(!ev || ev->kind == EXPR_ERROR) return ev;
            c->data[i] = ev;
        }
        return &c->e;
    }
    if(arg->kind != EXPR_STRING) return Error(ctx, "");
    StringView sv = ((String*)arg)->sv;
    Expression* result = evaluate_string(ctx, hnd, sv.text, sv.length, caller_row, caller_col);
    return result;
}

DRSP_INTERNAL
FORMULAFUNC(drsp_pow){
    if(argc != 2) return Error(ctx, "");
    BuffCheckpoint bc = buff_checkpoint(&ctx->a);
    Expression* arg = evaluate_expr(ctx, hnd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(expr_is_columnar(arg)){
        arg = convert_to_computed_column(ctx, hnd, arg, caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        BuffCheckpoint bc = buff_checkpoint(&ctx->a);
        Expression* arg2 = evaluate_expr(ctx, hnd, argv[1], caller_row, caller_col);
        if(!arg2 || arg2->kind == EXPR_ERROR) return arg;
        ComputedColumn* c = (ComputedColumn*)arg;
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
        else if(expr_is_columnar(arg2)){
            arg2 = convert_to_computed_column(ctx, hnd, arg2, caller_row, caller_col);
            if(!arg2 || arg2->kind == EXPR_NULL)
                return arg2;
            ComputedColumn* exps = (ComputedColumn*)arg2;
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
        buff_set(&ctx->a, bc);
        return arg;
    }
    else {
        if(arg->kind != EXPR_NUMBER) {
            buff_set(&ctx->a, bc);
            return Error(ctx, "");
        }
        Expression* arg2 = evaluate_expr(ctx, hnd, argv[1], caller_row, caller_col);
        if(!arg2 || arg2 -> kind == EXPR_ERROR){
            buff_set(&ctx->a, bc);
            return arg2;
        }
        if(arg2->kind != EXPR_NUMBER){
            buff_set(&ctx->a, bc);
            return Error(ctx, "");
        }
        double val = __builtin_pow(((Number*)arg)->value, ((Number*)arg2)->value);
        buff_set(&ctx->a, bc);
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
    BuffCheckpoint bc = buff_checkpoint(&ctx->a);
    if(argc == 2){
        Expression* arg = evaluate_expr(ctx, hnd, argv[0], caller_row, caller_col);
        if(!arg || arg->kind == EXPR_ERROR) return arg;
        if(expr_is_columnar(arg)){
            arg = convert_to_computed_column(ctx, hnd, arg, caller_row, caller_col);
            if(!arg || arg->kind == EXPR_ERROR) return arg;
            BuffCheckpoint bc = buff_checkpoint(&ctx->a);
            Expression* arg2 = evaluate_expr(ctx, hnd, argv[1], caller_row, caller_col);
            if(!arg2 || arg2->kind == EXPR_ERROR) return arg;
            ComputedColumn* c = (ComputedColumn*)arg;
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
            else if(expr_is_columnar(arg2)){
                arg2 = convert_to_computed_column(ctx, hnd, arg2, caller_row, caller_col);
                if(!arg2 || arg2->kind == EXPR_ERROR)
                    return arg2;
                ComputedColumn* rights = (ComputedColumn*)arg2;
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
                    if(err) return expr_alloc(ctx, EXPR_ERROR);
                }
            }
            else {
                return Error(ctx, "");
            }
            buff_set(&ctx->a, bc);
            return arg;
        }
        else {
            if(arg->kind != EXPR_STRING && arg->kind != EXPR_NULL) {
                buff_set(&ctx->a, bc);
                return Error(ctx, "");
            }
            Expression* arg2 = evaluate_expr(ctx, hnd, argv[1], caller_row, caller_col);
            if(!arg2 || arg2->kind == EXPR_ERROR){
                buff_set(&ctx->a, bc);
                return arg2;
            }
            if(expr_is_columnar(arg2)){
                arg2 = convert_to_computed_column(ctx, hnd, arg2, caller_row, caller_col);
                if(!arg2 || arg2->kind == EXPR_ERROR) return arg2;
                ComputedColumn* c = (ComputedColumn*)arg2;
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
                    if(err) return expr_alloc(ctx, EXPR_ERROR);
                }
                return arg2;
            }
            else {
                if(arg2->kind == EXPR_NULL){
                    if(arg->kind == EXPR_NULL){
                        buff_set(&ctx->a, bc);
                        return arg;
                    }
                    StringView v = ((String*)arg)->sv;
                    buff_set(&ctx->a, bc);
                    String* s = expr_alloc(ctx, EXPR_STRING);
                    if(!s) return NULL;
                    s->sv = v;
                    return &s->e;
                }
                if(arg2->kind != EXPR_STRING){
                    buff_set(&ctx->a, bc);
                    return Error(ctx, "");
                }
                if(arg->kind == EXPR_NULL){
                    StringView v = ((String*)arg2)->sv;
                    buff_set(&ctx->a, bc);
                    String* s = expr_alloc(ctx, EXPR_STRING);
                    if(!s) return NULL;
                    s->sv = v;
                    return &s->e;
                }
                StringView v;
                catbuff[0] = ((String*)arg)->sv;
                catbuff[1] = ((String*)arg2)->sv;
                int err = sv_cat(ctx, 2, catbuff, &v);
                buff_set(&ctx->a, bc);
                if(err) return Error(ctx, "");
                String* s = expr_alloc(ctx, EXPR_STRING);
                if(!s) return NULL;
                s->sv = v;
                return &s->e;
            }
        }
    }
    else {
        _Bool is_columnar = false;
        intptr_t column_length = 0;
        for(int i = 0; i < argc; i++){
            argv[i] = evaluate_expr(ctx, hnd, argv[i], caller_row, caller_col);
            if(!argv[i] || argv[i]->kind == EXPR_ERROR){
                buff_set(&ctx->a, bc);
                return argv[i];
            }
            if(expr_is_columnar(argv[i])){
                is_columnar = true;
                argv[i] = convert_to_computed_column(ctx, hnd, argv[i], caller_row, caller_col);
                if(!argv[i] || argv[i]->kind == EXPR_ERROR){
                    buff_set(&ctx->a, bc);
                    return argv[i];
                }
                ComputedColumn* c = (ComputedColumn*)argv[i];
                if(c->length > column_length){
                    column_length = c->length;
                }
            }
            else if(argv[i]->kind != EXPR_STRING && argv[i]->kind != EXPR_NULL){
                buff_set(&ctx->a, bc);
                return Error(ctx, "");
            }
        }
        if(is_columnar){
            if(!column_length)
                return Error(ctx, "");
            ComputedColumn* result = buff_alloc(&ctx->a, __builtin_offsetof(ComputedColumn, data)+sizeof(Expression*)*column_length);
            if(!result) return Error(ctx, "oom");
            result->e.kind = EXPR_COMPUTED_COLUMN;
            result->length = column_length;
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
                        ComputedColumn* c = (ComputedColumn*)argv[i];
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
                                buff_set(&ctx->a, bc);
                                return Error(ctx, "");
                            }
                        }
                    }
                }
                String* s = expr_alloc(ctx, EXPR_STRING);
                if(!s || s->e.kind == EXPR_ERROR) return &s->e;
                int err = sv_cat(ctx, argc, catbuff, &s->sv);
                if(err){
                    buff_set(&ctx->a, bc);
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
            buff_set(&ctx->a, bc);
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
columnar_tablelookup(SpreadContext* ctx, SheetHandle hnd, intptr_t caller_row, intptr_t caller_col, Expression* needle, int argc, Expression*_Nonnull*_Nonnull argv){
    needle = convert_to_computed_column(ctx, hnd, needle, caller_row, caller_col);
    if(!needle || needle->kind == EXPR_ERROR)
        return needle;
    ComputedColumn* cneedle = (ComputedColumn*)needle;
    // TODO: Use checkpoints.
    // TODO: lazier computation.
    // BuffCheckpoint bc = buff_checkpoint(&ctx->a);
    Expression* haystack = evaluate_expr(ctx, hnd, argv[0], caller_row, caller_col);
    if(!haystack || haystack->kind == EXPR_ERROR)
        return haystack;
    haystack = convert_to_computed_column(ctx, hnd, haystack, caller_row, caller_col);
    if(!haystack || haystack->kind == EXPR_ERROR)
        return haystack;
    ComputedColumn* chaystack = (ComputedColumn*)haystack;
    argc--, argv++;
    Expression* values = evaluate_expr(ctx, hnd, argv[0], caller_row, caller_col);
    if(!values || values->kind == EXPR_ERROR)
        return values;
    values = convert_to_computed_column(ctx, hnd, values, caller_row, caller_col);
    if(!values || values->kind == EXPR_ERROR)
        return values;
    ComputedColumn* cvalues = (ComputedColumn*)values;
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
                default_ = evaluate_expr(ctx, hnd, argv[0], caller_row, caller_col);
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
    BuffCheckpoint bc = buff_checkpoint(&ctx->a);
    ExpressionKind nkind;
    union {
        StringView s;
        double d;
    } nval;
    {
        Expression* needle = evaluate_expr(ctx, hnd, argv[0], caller_row, caller_col);
        if(!needle || needle->kind == EXPR_ERROR) return needle;
        if(expr_is_columnar(needle))
            return columnar_tablelookup(ctx, hnd, caller_row, caller_col, needle, argc-1, argv+1);
        nkind = needle->kind;
        if(nkind == EXPR_NULL){
            if(argc == 4){
                return evaluate_expr(ctx, hnd, argv[3], caller_row, caller_col);
            }
        }
        if(nkind != EXPR_NUMBER && nkind != EXPR_STRING) return Error(ctx, "");
        if(nkind == EXPR_NUMBER)
            nval.d = ((Number*)needle)->value;
        else
            nval.s = ((String*)needle)->sv;
        buff_set(&ctx->a, bc);
    }
    intptr_t offset = -1;
    {
        Expression* haystack = evaluate_expr(ctx, hnd, argv[1], caller_row, caller_col);
        if(!haystack || haystack->kind == EXPR_ERROR) return haystack;
        intptr_t col, start, end;
        SheetHandle rhnd = hnd;
        if(get_range1dcol(ctx, hnd, haystack, &col, &start, &end, &rhnd, caller_row, caller_col) != 0)
            return Error(ctx, "");

        char*const loopchk = ctx->a.cursor;
        if(nkind == EXPR_STRING){
            for(intptr_t row = start; row <= end; ctx->a.cursor=loopchk, row++){
                Expression* e = evaluate(ctx, rhnd, row, col);
                if(!e) return e;
                if(e->kind != EXPR_STRING) continue;
                if(sv_equals(nval.s, ((String*)e)->sv)){
                    offset = row - start;
                    break;
                }
            }
        }
        else {
            for(intptr_t row = start; row <= end; ctx->a.cursor=loopchk, row++){
                Expression* e = evaluate(ctx, rhnd, row, col);
                if(!e) return e;
                if(e->kind != EXPR_NUMBER) continue;
                if(nval.d == ((Number*)e)->value){
                    offset = row - start;
                    break;
                }
            }
        }
        buff_set(&ctx->a, bc);
        if(offset < 0) {
            if(argc == 4)
                return evaluate_expr(ctx, hnd, argv[3], caller_row, caller_col);
            return Error(ctx, "Didn't find needle in haystack");
        }
    }
    {
        Expression* values = evaluate_expr(ctx, hnd, argv[2], caller_row, caller_col);
        if(!values || values->kind == EXPR_ERROR) return values;
        intptr_t col, start, end;
        SheetHandle rhnd = hnd;
        if(get_range1dcol(ctx, hnd, values, &col, &start, &end, &rhnd, caller_row, caller_col) != 0)
            return Error(ctx, "invalid column range");
        buff_set(&ctx->a, bc);
        if(start+offset > end) return Error(ctx, "out of bounds");
        return evaluate(ctx, rhnd, start+offset, col);
    }
}

DRSP_INTERNAL
FORMULAFUNC(drsp_find){
    // "needle", [haystack]
    if(argc != 2 && argc != 3) return Error(ctx, "");
    char* chk = ctx->a.cursor;
    ExpressionKind nkind;
    union {
        StringView s;
        double d;
    } nval;
    {
        Expression* needle = evaluate_expr(ctx, hnd, argv[0], caller_row, caller_col);
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
        Expression* haystack = evaluate_expr(ctx, hnd, argv[1], caller_row, caller_col);
        if(!haystack || haystack->kind == EXPR_ERROR) return haystack;
        if(haystack->kind != EXPR_RANGE1D_COLUMN) return Error(ctx, "");
        intptr_t col, start, end;
        SheetHandle rhnd = hnd;
        if(get_range1dcol(ctx, hnd, haystack, &col, &start, &end, &rhnd, caller_row, caller_col) != 0)
            return Error(ctx, "");

        char* loopchk = ctx->a.cursor;
        for(intptr_t row = start; row <= end; ctx->a.cursor=loopchk, row++){
            Expression* e = evaluate(ctx, rhnd, row, col);
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
    if(offset < 0) {
        if(argc == 3)
            return evaluate_expr(ctx, hnd, argv[2], caller_row, caller_col);
        return Error(ctx, "");
    }
    Number* n = expr_alloc(ctx, EXPR_NUMBER);
    n->value = offset+1;
    return &n->e;
}

DRSP_INTERNAL
FORMULAFUNC(drsp_call){
    if(!argc) return Error(ctx, "");
    Expression* arg = evaluate_expr(ctx, hnd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR) return arg;
    if(arg->kind != EXPR_STRING)
        return Error(ctx, "");
    StringView name = ((String*)arg)->sv;

    FormulaFunc* func = NULL;
    for(size_t i = 0; i < FUNCTABLE_LENGTH; i++){
        if(sv_equals(FUNCTABLE[i].name, name)){
            func = FUNCTABLE[i].func;
            break;
        }
    }
    if(!func) return Error(ctx, "");
    argc--, argv++;
    return func(ctx, hnd, caller_row, caller_col, argc, argv);
}

#ifdef DRSP_INTRINS
DRSP_INTERNAL
FORMULAFUNC(drsp_first){
    if(argc != 1) return Error(ctx, "");
    Expression* arg = evaluate_expr(ctx, hnd, argv[0], caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR)
        return arg;
    if(!expr_is_columnar(arg))
        return Error(ctx, "");
    arg = convert_to_computed_column(ctx, hnd, arg, caller_row, caller_col);
    if(!arg || arg->kind == EXPR_ERROR)
        return arg;
    ComputedColumn* c = (ComputedColumn*)arg;
    if(!c->length)
        return Error(ctx, "");
    return c->data[0];
}
#endif

DRSP_INTERNAL
FORMULAFUNC(drsp_array){
    if(!argc) return Error(ctx, "");
    ComputedColumn* cc = buff_alloc(&ctx->a, __builtin_offsetof(ComputedColumn, data)+sizeof(Expression*)*argc);
    if(!cc) return NULL;
    cc->e.kind = EXPR_COMPUTED_COLUMN;
    cc->length = argc;
    for(int i = 0; i < argc; i++){
        Expression* e = evaluate_expr(ctx, hnd, argv[i], caller_row, caller_col);
        if(!e || e->kind == EXPR_ERROR)
            return e;
        cc->data[i] = e;
    }
    return &cc->e;
}

DRSP_INTERNAL
FORMULAFUNC(drsp_if){
    if(argc != 3) return Error(ctx, "");
    BuffCheckpoint bc = buff_checkpoint(&ctx->a);
    Expression* cond = evaluate_expr(ctx, hnd, argv[0], caller_row, caller_col);
    if(!cond || cond->kind == EXPR_ERROR)
        return cond;
    if(expr_is_columnar(cond)){
        cond = convert_to_computed_column(ctx, hnd, cond, caller_row, caller_col);
        if(!cond || cond->kind == EXPR_ERROR)
            return cond;
        ComputedColumn* cc = (ComputedColumn*)cond;
        Expression* t = NULL;
        Expression* f = NULL;
        intptr_t tcol = IDX_UNSET, tstart, tend;
        SheetHandle thnd = hnd;
        intptr_t fcol = IDX_UNSET, fstart, fend;
        SheetHandle fhnd = hnd;
        for(intptr_t i = 0; i < cc->length; i++){
            Expression* e = cc->data[i];
            if(evaled_is_not_scalar(e))
                return Error(ctx, "Must be coercible to true/false");
            if(scalar_expr_is_truthy(e)){
                if(!t){
                    t = evaluate_expr(ctx, hnd, argv[1], caller_row, caller_col);
                    if(!t || t->kind == EXPR_ERROR)
                        return t;
                }
                if(t->kind == EXPR_COMPUTED_COLUMN){
                    ComputedColumn* tc = (ComputedColumn*)t;
                    if(i >= tc->length){
                        return Error(ctx, "");
                    }
                    cc->data[i] = tc->data[i];
                }
                else if(expr_is_scalar(t)){
                    cc->data[i] = t;
                }
                else if(t->kind == EXPR_RANGE1D_COLUMN || t->kind == EXPR_RANGE1D_COLUMN_FOREIGN){
                    if(tcol == IDX_UNSET){
                        if(get_range1dcol(ctx, hnd, t, &tcol, &tstart, &tend, &thnd, caller_row, caller_col))
                            return Error(ctx, "");
                    }
                    if(tstart + i > tend)
                        return Error(ctx, "");
                    Expression* te = evaluate(ctx, thnd, tstart+i, tcol);
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
                    f = evaluate_expr(ctx, hnd, argv[2], caller_row, caller_col);
                    if(!f || f->kind == EXPR_ERROR)
                        return f;
                }
                if(f->kind == EXPR_COMPUTED_COLUMN){
                    ComputedColumn* fc = (ComputedColumn*)f;
                    if(i >= fc->length){
                        return Error(ctx, "");
                    }
                    cc->data[i] = fc->data[i];
                }
                else if(expr_is_scalar(f)){
                    cc->data[i] = f;
                }
                else if(f->kind == EXPR_RANGE1D_COLUMN || f->kind == EXPR_RANGE1D_COLUMN_FOREIGN){
                    if(fcol == IDX_UNSET){
                        if(get_range1dcol(ctx, hnd, f, &fcol, &fstart, &fend, &fhnd, caller_row, caller_col))
                            return Error(ctx, "");
                    }
                    if(fstart + i > fend)
                        return Error(ctx, "");
                    Expression* fe = evaluate(ctx, fhnd, fstart+i, fcol);
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
    buff_set(&ctx->a, bc);
    if(truthy)
        return evaluate_expr(ctx, hnd, argv[1], caller_row, caller_col);
    else
        return evaluate_expr(ctx, hnd, argv[2], caller_row, caller_col);
}

#ifdef DRSPREAD_CLI_C
#ifdef __APPLE__
#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#include <time.h>
#ifdef __clang__
#pragma clang assume_nonnull begin
#endif
DRSP_INTERNAL
FORMULAFUNC(drsp_time){
    if(argc != 1) return Error(ctx, "");
    uint64_t start = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);
    Expression* e = evaluate_expr(ctx, hnd, argv[0], caller_row, caller_col);
    uint64_t end = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);
    fprintf(stderr, "Took %zu ns\n", (size_t)(end - start));
    return e;
}
#endif
#endif

#if 0
#define DRSP_FRAME_FUNCS 1
#endif

#ifdef DRSP_FRAME_FUNCS
DRSP_INTERNAL
FORMULAFUNC(drsp_frame){
    (void)argv; (void)argc;
    (void)caller_row; (void)caller_col;
    (void)hnd;
    uintptr_t sp = (uintptr_t)__builtin_frame_address(0);
    Number* n = expr_alloc(ctx, EXPR_NUMBER);
    if(!n) return NULL;
    n->value = sp;
    return &n->e;
}
DRSP_INTERNAL
FORMULAFUNC(drsp_limit){
    (void)argv; (void)argc;
    (void)caller_row; (void)caller_col;
    (void)hnd;
    Number* n = expr_alloc(ctx, EXPR_NUMBER);
    if(!n) return NULL;
    n->value = ctx->limit;
    return &n->e;
}
#endif



#ifndef SV
#define SV(x) {sizeof(x)-1, x}
#endif

DRSP_INTERNAL
const FuncInfo FUNCTABLE[] = {
    {SV("sum"),   &drsp_sum},
    {SV("tlu"),   &drsp_tablelookup},
    {SV("mod"),   &drsp_mod},
    {SV("if"),    &drsp_if},
    {SV("count"), &drsp_count},
    {SV("avg"),   &drsp_avg},
    {SV("min"),   &drsp_min},
    {SV("max"),   &drsp_max},
    {SV("abs"),   &drsp_abs},
    {SV("floor"), &drsp_floor},
    {SV("ceil"),  &drsp_ceil},
    {SV("trunc"), &drsp_trunc},
    {SV("round"), &drsp_round},
    {SV("find"),  &drsp_find},
    {SV("num"),   &drsp_num},
    {SV("try"),   &drsp_try},
    {SV("pow"),   &drsp_pow},
    {SV("cell"),  &drsp_cell},
    {SV("eval"),  &drsp_eval},
    {SV("col"),   &drsp_col},
    {SV("call"),  &drsp_call},
    {SV("sqrt"),  &drsp_sqrt},
    {SV("array"), &drsp_array},
    {SV("cat"),   &drsp_cat},
    {SV("concat"),&drsp_cat},
#ifdef DRSP_INTRINS
    {SV("_f"),    &drsp_first},
    {SV("_a"),    &drsp_array},
#endif
#ifdef DRSPREAD_CLI_C
#ifdef __APPLE__
    {SV("time"), &drsp_time},
#endif
#endif
    {SV("prod"),  &drsp_prod},
#ifdef DRSP_FRAME_FUNCS
    {SV("_frame"), &drsp_frame},
    {SV("_limit"), &drsp_limit},
#endif
};

DRSP_INTERNAL const size_t FUNCTABLE_LENGTH = arrlen(FUNCTABLE);
#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#endif
