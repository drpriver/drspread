//
// Copyright © 2023, David Priver
//
#include "allstd.h"
#include "drspread.h"
#include "drspread_wasm.h"
#include "drspread.c"
#include "Wasm/jsinter.h"

#ifndef DRSP_IMPORT
#define DRSP_IMPORT extern
#endif

DRSP_EXPORT
void
sheet_evaluate_formulas(intptr_t id, SheetHandle _Null_unspecified*_Nullable sheetdeps, size_t sheetdepslen){
    drsp_evaluate_formulas((void*)id, sheetdeps, sheetdepslen);
}

DRSP_EXPORT
int
sheet_evaluate_string(intptr_t id, PString* p, DrSpreadCellValue* result){
    _Static_assert(16 == sizeof *result,"");
    _Static_assert(__builtin_offsetof(DrSpreadCellValue, d)==8, "");
    _Static_assert(__builtin_offsetof(DrSpreadCellValue, s.length)==8, "");
    _Static_assert(__builtin_offsetof(DrSpreadCellValue, s.text)==12, "");
    int err = drsp_evaluate_string((void*)id, (char*)p->text, p->length, result, -1, -1);
    return err;
}
