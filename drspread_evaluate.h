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

enum EvaluateResult {
    EV_ERROR = 0,
    EV_NULL,
    EV_STRING,
    EV_RANGE,
    EV_NUMBER,
};
typedef enum EvaluateResult EvaluateResult;

static
EvaluateResult
evaluate(SpreadContext*, intptr_t row, intptr_t col, double* outval);

static
EvaluateResult
evaluate_string(SpreadContext* ctx, const char* txt, size_t len, double* outval);

#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#endif
