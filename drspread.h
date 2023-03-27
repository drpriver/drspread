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

typedef struct DrSpreadCtx DrSpreadCtx;
#ifndef DRSPREAD_DIRECT_OPS
typedef struct SheetOps SheetOps;

// Callbacks
struct SheetOps {
    void* ctx;
    int (*set_display_number)(void*ctx, SheetHandle sheet, intptr_t row, intptr_t col, double value);
    int (*set_display_error)(void*ctx, SheetHandle sheet, intptr_t row, intptr_t col, const char* errmess, size_t errmess_len);
    int (*set_display_string)(void*ctx, SheetHandle sheet, intptr_t row, intptr_t col, const char*, size_t);
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
#endif


#endif
