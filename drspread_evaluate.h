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
int evaluate(SpreadContext*, intptr_t row, intptr_t col, double* outval);

#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#endif
