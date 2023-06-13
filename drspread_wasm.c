//
// Copyright © 2023, David Priver <david@davidpriver.com>
//
#define DRSPREAD_DIRECT_OPS 1
#include "allstd.h"
#include "drspread.h"
#include "drspread_wasm.h"
#include "Wasm/jsinter.h"

_Static_assert(16 == sizeof(DrSpreadResult), "");
_Static_assert(__builtin_offsetof(DrSpreadResult, d)==8, "");
_Static_assert(__builtin_offsetof(DrSpreadResult, s.length)==8, "");
_Static_assert(__builtin_offsetof(DrSpreadResult, s.text)==12, "");

DRSP_EXPORT unsigned char wasm_str_buff[1024];
unsigned char wasm_str_buff[1024] = {0};

DRSP_EXPORT DrSpreadResult wasm_result;
DrSpreadResult wasm_result = {0};

DRSP_EXPORT int32_t wasm_parambuff_row[4];
int32_t wasm_parambuff_row[4] = {0};

DRSP_EXPORT int32_t wasm_parambuff_col[4];
int32_t wasm_parambuff_col[4] = {0};

#include "drspread.c"
