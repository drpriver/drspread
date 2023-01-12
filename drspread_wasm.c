//
// Copyright © 2023, David Priver
//
#include "drspread_wasm.h"
#include "drspread.c"
#include "Wasm/jsinter.h"
#ifndef EXPORT
#define EXPORT extern
#endif

EXPORT
void
sheet_evaluate_formulas(intptr_t id){
    drsp_evaluate_formulas((void*)id);
}

EXPORT
int
sheet_evaluate_string(intptr_t id, PString* p, DrSpreadCellValue* result){
    _Static_assert(16 == sizeof *result,"");
    _Static_assert(__builtin_offsetof(DrSpreadCellValue, d)==8, "");
    int err = drsp_evaluate_string((void*)id, (char*)p->text, p->length, result);
    return err;
}
