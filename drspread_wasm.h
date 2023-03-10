#ifndef DRSPREAD_WASM_H
#define DRSPREAD_WASM_H

#include "drspread.h"
#include "Wasm/jsinter.h"
#pragma clang assume_nonnull begin
#ifdef DRSPREAD_DIRECT_OPS
IMPORT void sheet_set_display_number(SheetHandle, intptr_t, intptr_t, double);
IMPORT void sheet_set_display_string(SheetHandle, intptr_t, intptr_t, const char*, size_t);
IMPORT void sheet_set_display_error(SheetHandle, intptr_t, intptr_t);
#endif
#pragma clang assume_nonnull end
#endif
