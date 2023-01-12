//
// Copyright © 2023, David Priver
//
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

enum CellKind: intptr_t {
    CELL_EMPTY = 0, // Empty Cell
    CELL_NUMBER = 1,
    CELL_FORMULA = 2,
    CELL_OTHER = 3,
    CELL_UNKNOWN = 4, // caller doesn't know what this is, but can provide a string
};
typedef enum CellKind CellKind;
// Opaque Struct
// Cast to your actual type in your implementation functions.
// Safer than void*
typedef struct _sheet_handle* SheetHandle;

#ifndef __wasm__
typedef struct SheetOps SheetOps;

// Callbacks
struct SheetOps {
    void* ctx;
    CellKind (*_Nullable query_cell_kind)(void*ctx, SheetHandle sheet, intptr_t row, intptr_t col);
    double (*cell_number)(void* ctx, SheetHandle sheet, intptr_t row, intptr_t col);
    const char*_Nullable (*_Nonnull cell_txt)(void*ctx, SheetHandle sheet, intptr_t row, intptr_t col, size_t* len);
    intptr_t (*col_height)(void* ctx, SheetHandle sheet, intptr_t col);
    intptr_t (*row_width)(void*ctx, SheetHandle sheet, intptr_t row);
    int (*dims)(void*ctx, SheetHandle sheet, intptr_t* ncols, intptr_t* nrows);
    int (*set_display_number)(void*ctx, SheetHandle sheet, intptr_t row, intptr_t col, double value);
    int (*set_display_error)(void*ctx, SheetHandle sheet, intptr_t row, intptr_t col, const char* errmess, size_t errmess_len);
    int (*set_display_string)(void*ctx, SheetHandle sheet, intptr_t row, intptr_t col, const char*, size_t);
    int (*next_cell)(void*ctx, SheetHandle sheet, intptr_t nth, intptr_t* row, intptr_t* col);
    intptr_t(*name_to_col_idx)(void*ctx, SheetHandle sheet, const char*, size_t);
    void*_Nullable (*_Nullable name_to_sheet)(void* ctx, const char*, size_t);
};

int
drsp_evaluate_formulas(SheetHandle sheethandle, const SheetOps* ops);

typedef struct DrSpreadCellValue DrSpreadCellValue;
struct  DrSpreadCellValue {
    CellKind kind; // oneof CELL_EMPTY, CELL_NUMBER, CELL_OTHER
    union {
        double d; // CELL_NUMBER
        struct {  // CELL_OTHER
            size_t length;
            const char* text;
        } s;
    };
};

int
drsp_evaluate_string(SheetHandle sheethandle, const SheetOps* ops, const char* txt, size_t len, DrSpreadCellValue* outval);
#else
int
drsp_evaluate_formulas(SheetHandle sheethandle);

typedef struct DrSpreadCellValue DrSpreadCellValue;
struct  DrSpreadCellValue {
    CellKind kind; // oneof CELL_EMPTY, CELL_NUMBER, CELL_OTHER
    union {
        double d; // CELL_NUMBER
        struct {  // CELL_OTHER
            size_t length;
            const char* text;
        } s;
    };
};

int
drsp_evaluate_string(SheetHandle sheethandle, const char* txt, size_t len, DrSpreadCellValue* outval);
#endif

#ifdef __clang__
#pragma clang assume_nonnull end
#endif


#endif
