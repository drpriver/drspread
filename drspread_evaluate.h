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

static
Expression*_Nullable
evaluate_expr(SpreadContext*, Expression*);

static
Expression*_Nullable
evaluate(SpreadContext*, intptr_t row, intptr_t col);

static
Expression*_Nullable
evaluate_string(SpreadContext* ctx, const char* txt, size_t len);

#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#endif
