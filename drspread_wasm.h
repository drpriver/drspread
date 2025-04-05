#ifndef DRSPREAD_WASM_H
#define DRSPREAD_WASM_H

#include "drspread.h"
#ifndef IMPORT
#define IMPORT extern
#endif

#pragma clang assume_nonnull begin
#ifdef DRSPREAD_DIRECT_OPS
__attribute__((import_name("sheet_set_display_number")))
IMPORT void sheet_set_display_number(SheetHandle, intptr_t, intptr_t, double);
__attribute__((import_name("sheet_set_display_string")))
IMPORT void sheet_set_display_string(SheetHandle, intptr_t, intptr_t, const char*, size_t);
__attribute__((import_name("sheet_set_display_error")))
IMPORT void sheet_set_display_error(SheetHandle, intptr_t, intptr_t, const char*, size_t);
#endif
#pragma clang assume_nonnull end
#endif
