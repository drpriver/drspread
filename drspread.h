//
// Copyright © 2023-2024, David Priver <david@davidpriver.com>
//
#ifndef DRSPREAD_H
#define DRSPREAD_H

#define DRSPREAD_VERSION "0.1.0"
#define DRSPREAD_VERSION_MAJOR 0
#define DRSPREAD_VERSION_MINOR 2
#define DRSPREAD_VERSION_MICRO 0

#include <stdint.h>
#include <stddef.h>
#ifdef __clang__
#pragma clang assume_nonnull begin
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmicrosoft-fixed-enum"
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
#if !defined(_WIN32) && !defined(DRSP_DYLIB) // Don't re-export the symbols, but make them visible in .o files.
#define DRSP_EXPORT extern __attribute__((visibility("hidden")))
#elif !defined(_WIN32) && defined(DRSP_DYLIB) // Export from the dylib
#define DRSP_EXPORT extern __attribute__((visibility("default")))
#elif defined(DRSPREAD_C) && defined(DRSP_DYLIB) // building the lib, Export from dll
#define DRSP_EXPORT extern __declspec(dllexport)
#elif defined(DRSP_DYLIB) // Will be imported from a dll
#define DRSP_EXPORT extern __declspec(dllimport)
#else // Building a static library/.o file on win32
#define DRSP_EXPORT extern
#endif
#endif

#ifndef DRSP_TYPED_ENUM
#if defined(__clang__) || __STDC_VERSION__ >= 202300L
#define DRSP_TYPED_ENUM(name, type) enum name: type; typedef enum name name; enum name : type
#else
#define DRSP_TYPED_ENUM(name, type) typedef type name; enum name
#endif
#endif

DRSP_TYPED_ENUM(DrspResultKind, uintptr_t){
    DRSP_RESULT_NULL = 0, // Empty Cell
    DRSP_RESULT_NUMBER = 1,
    DRSP_RESULT_STRING = 2,
};
enum {DRSP_MAX_ROWS=10065000};

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
        // NOTE: This is a pointer to an interned string and will
        //       live as long as the context.
        struct {
            size_t length;
            const char* text;
        }s;  // DRSP_RESULT_STRING
    };
};
enum {DRSP_IDX_EXTRA_DIMENSIONAL = -2147483645  }; // INT32_MIN+3

typedef struct DrSpreadCtx DrSpreadCtx;
#ifndef DRSPREAD_DIRECT_OPS
typedef struct SheetOps SheetOps;
// NOTE: row will be DRSP_IDX_EXTRA_DIMENSIONAL for the extra-dimensional cells.
typedef int (DrspSetDisplayNumber)(void* ctx, SheetHandle sheet, intptr_t row, intptr_t col, double value);
typedef int (DrspSetDisplayError)(void* ctx, SheetHandle sheet, intptr_t row, intptr_t col, const char*, size_t);
typedef int (DrspSetDisplayString)(void* ctx, SheetHandle sheet, intptr_t row, intptr_t col, const char*, size_t);

// Callbacks
struct SheetOps {
    void* ctx;
    DrspSetDisplayNumber* set_display_number;
    DrspSetDisplayError* set_display_error;
    DrspSetDisplayString* set_display_string;
};
#endif

DRSP_EXPORT
int
drsp_evaluate_formulas(DrSpreadCtx*);

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
typedef const struct DrspStr* DrspAtom;

// NOTE: strips leading and trailing whitespace
DRSP_EXPORT
DrspAtom _Nullable
drsp_atomize(DrSpreadCtx* restrict, const char* restrict, size_t length);

DRSP_EXPORT
const char* _Nullable
drsp_atom_get_str(DrSpreadCtx* restrict, DrspAtom restrict, size_t* restrict length);

// If the sheet does not exist, this also creates the sheet.
// This means that sheets always have a name.
// Names do not have to be unique, but looking up sheets by name
// will arbitrarily pick one.
// This has to be called before any other usage of that sheet.
DRSP_EXPORT
int
drsp_set_sheet_name(DrSpreadCtx*restrict ctx, SheetHandle sheet, const char* name, size_t length);

// We support 1 alias
// Calling this more than once with the same sheet will override the
// previous alias.
DRSP_EXPORT
int
drsp_set_sheet_alias(DrSpreadCtx*restrict ctx, SheetHandle sheet, const char* name, size_t length);

// NOTE: strips leading and trailing whitespace
DRSP_EXPORT
int
drsp_set_cell_str(DrSpreadCtx*restrict ctx, SheetHandle sheet, intptr_t row, intptr_t col, const char* restrict text, size_t length);

DRSP_EXPORT
int
drsp_set_cell_atom(DrSpreadCtx*restrict ctx, SheetHandle sheet, intptr_t row, intptr_t col, DrspAtom str);

// Sets the text of a cell that is not actually in the 2d cell grid.
// Useful for things like summaries.
DRSP_EXPORT
int
drsp_set_extra_dimensional_str(DrSpreadCtx* restrict ctx, SheetHandle sheet, intptr_t id, const char* restrict text, size_t length);

DRSP_EXPORT
int
drsp_set_named_cell(DrSpreadCtx* restrict ctx, SheetHandle sheet, const char* restrict name, size_t name_len, intptr_t row, intptr_t col);

DRSP_EXPORT
int
drsp_clear_named_cell(DrSpreadCtx* restrict ctx, SheetHandle sheet, const char* restrict name, size_t name_len);

// Set to a zero-length string to delete the column name.
DRSP_EXPORT
int
drsp_set_col_name(DrSpreadCtx*restrict ctx, SheetHandle sheet, intptr_t idx, const char* restrict text, size_t length);

// Delete all data associated with a sheet.
DRSP_EXPORT
int
drsp_del_sheet(DrSpreadCtx*restrict ctx, SheetHandle sheet);

enum DrspSheetFlags {
    DRSP_SHEET_FLAGS_NONE = 0x0,
    DRSP_SHEET_FLAGS_IS_FUNCTION = 0x1,
};

DRSP_EXPORT
int
drsp_set_sheet_flags(DrSpreadCtx*restrict ctx, SheetHandle sheet, unsigned flags);

DRSP_EXPORT
int
drsp_set_sheet_flag(DrSpreadCtx*restrict ctx, SheetHandle sheet, unsigned flag, _Bool on);

DRSP_EXPORT
unsigned
drsp_get_sheet_flags(DrSpreadCtx*restrict ctx, SheetHandle sheet);

// overrides all function params
DRSP_EXPORT
int
drsp_set_function_params(DrSpreadCtx*restrict ctx, SheetHandle function, size_t n_params, const intptr_t* rows, const intptr_t* cols);

DRSP_EXPORT
int
drsp_clear_function_params(DrSpreadCtx* restrict ctx, SheetHandle);

DRSP_EXPORT
int
drsp_set_function_output(DrSpreadCtx* restrict ctx, SheetHandle function, intptr_t row, intptr_t col);

#ifdef __clang__
#pragma clang assume_nonnull end
#pragma clang diagnostic pop
#endif


#endif
