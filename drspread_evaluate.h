//
// Copyright © 2023, David Priver
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
evaluate_expr(SpreadContext*, SheetHandle, Expression*, intptr_t caller_row, intptr_t caller_col);

DRSP_INTERNAL
Expression*_Nullable
evaluate(SpreadContext*, SheetHandle, intptr_t row, intptr_t col);

DRSP_INTERNAL
Expression*_Nullable
evaluate_string(SpreadContext* ctx, SheetHandle, const char* txt, size_t len, intptr_t row, intptr_t col);

#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#endif
