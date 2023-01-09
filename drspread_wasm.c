#include "drspread.c"
#include "Wasm/jsinter.h"

extern
intptr_t sheet_cell_kind(intptr_t, intptr_t, intptr_t);

static
CellKind 
js_cell_kind(void* ctx, intptr_t row, intptr_t col){
    return (CellKind)sheet_cell_kind((intptr_t)ctx, row, col);
}

extern double sheet_cell_number(intptr_t, intptr_t, intptr_t);

static
double
js_cell_number(void* ctx, intptr_t row, intptr_t col){
    return sheet_cell_number((intptr_t)ctx, row, col);
}

extern
PString*
sheet_cell_text(intptr_t, intptr_t, intptr_t);

static
const char*
js_cell_text(void* ctx, intptr_t row, intptr_t col, size_t* len){
    PString* p = sheet_cell_text((intptr_t)ctx, row, col);
    *len = p->length;
    return (char*)p->text;
}

extern
intptr_t sheet_col_height(intptr_t, intptr_t);

static 
intptr_t
js_col_height(void* ctx, intptr_t col){
    return sheet_col_height((intptr_t)ctx, col);
}


extern
intptr_t sheet_row_width(intptr_t, intptr_t);

static 
intptr_t
js_row_width(void* ctx, intptr_t row){
    return sheet_row_width((intptr_t)ctx, row);
}

extern
void sheet_set_display_number(intptr_t, intptr_t, intptr_t, double);

static 
int
js_set_display_number(void* ctx, intptr_t row, intptr_t col, double value){
    sheet_set_display_number((intptr_t)ctx, row, col, value);
    return 0;
}

extern
void sheet_set_display_string(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);

static 
int
js_set_display_string(void* ctx, intptr_t row, intptr_t col, const char* txt, size_t len){
    sheet_set_display_string((intptr_t)ctx, row, col, (intptr_t)txt, (intptr_t)len);
    return 0;
}

extern
void sheet_set_display_error(intptr_t, intptr_t, intptr_t);

static 
int
js_set_display_error(void* ctx, intptr_t row, intptr_t col, const char* txt, size_t len){
    (void)txt;
    (void)len;
    sheet_set_display_error((intptr_t)ctx, row, col);
    return 0;
}

extern
intptr_t sheet_name_to_col_idx(intptr_t, intptr_t, intptr_t);

static
intptr_t
js_name_to_col_idx(void* ctx, const char* name, size_t len){
    return sheet_name_to_col_idx((intptr_t)ctx, (intptr_t)name, (intptr_t)len);
}

extern
intptr_t sheet_next_cell(intptr_t, intptr_t, intptr_t, intptr_t);

static
int
js_next_cell(void* ctx, intptr_t i, intptr_t* row, intptr_t* col){
    return sheet_next_cell((intptr_t)ctx, i, (intptr_t)row, (intptr_t)col);
}

extern
intptr_t sheet_dims(intptr_t, intptr_t, intptr_t);

static
int
js_dims(void* ctx, intptr_t*ncols, intptr_t*nrows){
    return sheet_dims((intptr_t)ctx, (intptr_t)ncols, (intptr_t)nrows);
}

static const SheetOps op_base = {
    .ctx = (void*)0,
    .query_cell_kind = &js_cell_kind,
    .cell_number = &js_cell_number,
    .cell_txt = &js_cell_text,
    .col_height = &js_col_height,
    .row_width = &js_row_width,
    .dims = &js_dims,
    .set_display_number = &js_set_display_number,
    .set_display_error = &js_set_display_error,
    .set_display_string = &js_set_display_string,
    .next_cell = &js_next_cell,
    .name_to_col_idx = &js_name_to_col_idx,
};

void
sheet_evaluate_formulas(intptr_t id){
    SheetOps ops = op_base;
    ops.ctx = (void*)id;
    drsp_evaluate_formulas(&ops);
}

double 
sheet_evaluate_string(intptr_t id, PString* p){
    double result = __builtin_nan("");
    SheetOps ops = op_base;
    ops.ctx = (void*)id;
    drsp_evaluate_string(&ops, (char*)p->text, p->length, &result);
    return result;
}
