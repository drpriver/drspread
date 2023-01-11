//
// Copyright © 2023, David Priver
//
#ifndef DRSPREAD_PARSE_H
#define DRSPREAD_PARSE_H
#include "drspread_types.h"
#ifdef __clang__
#pragma clang assume_nonnull begin
#else
#ifndef _Nullable
#define _Nullable
#endif
#endif
static
Expression*_Nullable
parse(SpreadContext* ctx, const char* txt, size_t length);

static
CellKind
classify_cell(const char* txt, size_t length);

#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#endif
