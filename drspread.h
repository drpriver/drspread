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
            // NOTE: This is a pointer to an interned string and will
            //       live as long as the context.
            const char* text;
        } s;
    };
};

typedef struct DrSpreadCtx DrSpreadCtx;
#ifndef DRSPREAD_DIRECT_OPS
typedef struct SheetOps SheetOps;

// Callbacks
struct SheetOps {
    void* ctx;
    intptr_t (*col_height)(void* ctx, SheetHandle sheet, intptr_t col);
    intptr_t (*row_width)(void*ctx, SheetHandle sheet, intptr_t row);
    int (*set_display_number)(void*ctx, SheetHandle sheet, intptr_t row, intptr_t col, double value);
    int (*set_display_error)(void*ctx, SheetHandle sheet, intptr_t row, intptr_t col, const char* errmess, size_t errmess_len);
    int (*set_display_string)(void*ctx, SheetHandle sheet, intptr_t row, intptr_t col, const char*, size_t);
};
#endif

DRSP_EXPORT
int
drsp_evaluate_formulas(DrSpreadCtx*, SheetHandle sheethandle, SheetHandle _Null_unspecified*_Nullable sheetdeps, size_t sheetdepslen);

DRSP_EXPORT
int
drsp_evaluate_string(DrSpreadCtx*, SheetHandle sheethandle, const char* txt, size_t len, DrSpreadResult* outval, intptr_t row, intptr_t col);


#ifdef DRSPREAD_DIRECT_OPS
DRSP_EXPORT
DrSpreadCtx* _Nullable
drsp_create_ctx(void);
#else
DRSP_EXPORT
DrSpreadCtx* _Nullable
drsp_create_ctx(const SheetOps* ops);
#endif

DRSP_EXPORT
int
drsp_destroy_ctx(DrSpreadCtx*_Nullable);

typedef struct DrspStr DrspStr;

// This has to be called before any other usage of that sheet.
DRSP_EXPORT
int
drsp_set_sheet_name(DrSpreadCtx*restrict ctx, SheetHandle sheet, const char* name, size_t length);

// We support 1 alias
DRSP_EXPORT
int
drsp_set_sheet_alias(DrSpreadCtx*restrict ctx, SheetHandle sheet, const char* name, size_t length);

DRSP_EXPORT
int
drsp_set_cell_str(DrSpreadCtx*restrict ctx, SheetHandle sheet, intptr_t row, intptr_t col, const char* restrict text, size_t length);

DRSP_EXPORT
int
drsp_set_col_name(DrSpreadCtx*restrict ctx, SheetHandle sheet, intptr_t idx, const char* restrict text, size_t length);

// Delete all data associated with a sheet.
DRSP_EXPORT
int
drsp_del_sheet(DrSpreadCtx*restrict ctx, SheetHandle sheet);

#ifdef __clang__
#pragma clang assume_nonnull end
#endif


#endif
