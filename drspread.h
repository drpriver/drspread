#ifndef DRSPREAD_H
#define DRSPREAD_H

#include <stdint.h>
#include <stddef.h>
#ifdef __clang__
#pragma clang assume_nonnull begin
#else
#ifndef _Nullable
#define _Nullable
#endif
#ifndef _Nonnull
#define _Nonnull
#endif
#endif

enum CellKind {
    CELL_EMPTY = 0, // Empty Cell
    CELL_NUMBER = 1,
    CELL_FORMULA = 2,
    CELL_OTHER = 3,
};
typedef enum CellKind CellKind;

typedef struct SheetOps SheetOps;

// Callbacks
struct SheetOps {
    void* ctx;
    CellKind (*query_cell_kind)(void* ctx, intptr_t row, intptr_t col);
    double (*cell_number)(void* ctx, intptr_t row, intptr_t col);
    const char*_Nullable (*_Nonnull cell_txt)(void* ctx, intptr_t row, intptr_t col, size_t* len);
    intptr_t (*col_height)(void* ctx, intptr_t col);
    intptr_t (*row_width)(void* ctx, intptr_t row);
    int (*dims)(void* ctx, intptr_t* ncols, intptr_t* nrows);
    int (*set_display_number)(void* ctx, intptr_t row, intptr_t col, double value);
    int (*set_display_error)(void* ctx, intptr_t row, intptr_t col, const char* errmess, size_t errmess_len);
    int (*set_display_string)(void* ctx, intptr_t row, intptr_t col, const char*, size_t);
    int (*next_cell)(void* ctx, intptr_t* row, intptr_t* col);
    intptr_t(*name_to_col_idx)(void* ctx, const char*, size_t);
};

int
drsp_evaluate_formulas(const SheetOps* ops);

int
drsp_evaluate_string(const SheetOps* ops, const char* txt, size_t len, double* outval);

#ifdef __clang__
#pragma clang assume_nonnull end
#endif


#endif
