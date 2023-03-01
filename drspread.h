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
#ifndef _Null_unspecified
#define _Null_unspecified
#endif
#endif

#ifndef DRSP_EXPORT
#define DRSP_EXPORT extern __attribute__((visibility("default")))
#endif

enum DrspResultKind: intptr_t {
    DRSP_RESULT_NULL = 0, // Empty Cell
    DRSP_RESULT_NUMBER = 1,
    DRSP_RESULT_STRING = 2,
};
typedef enum DrspResultKind DrspResultKind;
// Opaque Struct
// Cast to your actual type in your implementation functions.
// Safer than void*
typedef struct _sheet_handle* SheetHandle;

typedef struct DrSpreadResult DrSpreadResult;
struct  DrSpreadResult {
    // oneof DRSP_RESULT_NULL, DRSP_RESULT_NUMBER, DRSP_RESULT_STRING
    DrspResultKind kind;
    union {
        double d; // DRSP_RESULT_NUMBER
        struct {  // DRSP_RESULT_STRING
            size_t length;
            // NOTE: when used as a return value, this is a malloced.
            // This sucks, but I can't figure out how else to support
            // cat('a', 'b') with the current API.
            // I could hoist the lifetime of the context to the caller,
            // which is probably the best way to handle it.
            const char* text;
        } s;
    };
};

#ifndef __wasm__
typedef struct SheetOps SheetOps;

// Callbacks
struct SheetOps {
    void* ctx;
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

DRSP_EXPORT
int
drsp_evaluate_formulas(SheetHandle sheethandle, const SheetOps* ops, SheetHandle _Null_unspecified*_Nullable sheetdeps, size_t sheetdepslen);

DRSP_EXPORT
int
drsp_evaluate_string(SheetHandle sheethandle, const SheetOps* ops, const char* txt, size_t len, DrSpreadResult* outval, intptr_t row, intptr_t col);
#else
DRSP_EXPORT
int
drsp_evaluate_formulas(SheetHandle sheethandle, SheetHandle _Null_unspecified*_Nullable sheetdeps, size_t sheetdepslen);

DRSP_EXPORT
int
drsp_evaluate_string(SheetHandle sheethandle, const char* txt, size_t len, DrSpreadResult* outval, intptr_t row, intptr_t col);
#endif

#ifdef __clang__
#pragma clang assume_nonnull end
#endif


#endif
