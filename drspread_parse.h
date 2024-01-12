//
// Copyright © 2023-2024, David Priver <david@davidpriver.com>
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
DRSP_INTERNAL
Expression*_Nullable
parse(DrSpreadCtx* ctx, DrspAtom a);

#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#endif
