#ifndef DRSPREAD_WASM_H
#define DRSPREAD_WASM_H

#include "drspread.h"
#include "Wasm/jsinter.h"
#pragma clang assume_nonnull begin
#ifdef DRSPREAD_DIRECT_OPS
IMPORT intptr_t sheet_query_cell_kind(SheetHandle, intptr_t, intptr_t);
IMPORT double sheet_cell_number(SheetHandle, intptr_t, intptr_t);
IMPORT PString* sheet_cell_text(SheetHandle, intptr_t, intptr_t);
IMPORT intptr_t sheet_col_height(SheetHandle, intptr_t);
IMPORT intptr_t sheet_row_width(SheetHandle, intptr_t);
IMPORT void sheet_set_display_number(SheetHandle, intptr_t, intptr_t, double);
IMPORT void sheet_set_display_string(SheetHandle, intptr_t, intptr_t, const char*, size_t);
IMPORT void sheet_set_display_error(SheetHandle, intptr_t, intptr_t);
IMPORT intptr_t sheet_name_to_col_idx(SheetHandle, const char*, size_t);
IMPORT intptr_t sheet_next_cell(SheetHandle, intptr_t, intptr_t*, intptr_t*);
IMPORT intptr_t sheet_dims(SheetHandle, intptr_t*, intptr_t*);
IMPORT SheetHandle sheet_name_to_sheet(const char*, size_t);
#endif
#pragma clang assume_nonnull end
#endif
