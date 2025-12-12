//
// Copyright © 2023-2024, David Priver <david@davidpriver.com>
//
#ifndef DRSPREAD_EVALUATE_H
#define DRSPREAD_EVALUATE_H
#include "drspread_types.h"
#include <stdint.h>

#ifdef __clang__
#pragma clang assume_nonnull begin
#else
#ifndef _Nullable
#define _Nullable
#endif
#endif

DRSP_INTERNAL
Expression*_Nullable
evaluate_expr(DrSpreadCtx*, SheetData*, Expression*, intptr_t caller_row, intptr_t caller_col);

DRSP_INTERNAL
Expression*_Nullable
evaluate(DrSpreadCtx*, SheetData*, intptr_t row, intptr_t col);

DRSP_INTERNAL
Expression*_Nullable
evaluate_string(DrSpreadCtx* ctx, SheetData*, const char* txt, size_t len, intptr_t row, intptr_t col);

DRSP_INTERNAL
Expression*_Nullable
call_udf(DrSpreadCtx* ctx, SheetData*, size_t nargs, Expression*_Nonnull*_Nonnull args);

// Get element at index from any arraylike expression (ComputedArray, LazyArray, Range)
DRSP_INTERNAL
Expression*_Nullable
arraylike_get(DrSpreadCtx* ctx, SheetData* sd, Expression* arr, intptr_t index, intptr_t caller_row, intptr_t caller_col);

#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#endif
