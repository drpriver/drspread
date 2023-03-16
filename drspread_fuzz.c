//
// Copyright © 2023, David Priver
//
#include "spreadsheet.h"
#include "drspread.c"

#if 1
// Libfuzz calls this function to do its fuzzing.
// Generally ascii-only is more interesting.
// You can turn that off if you want.
int
LLVMFuzzerTestOneInput(const uint8_t*data, size_t size){
    char* txt = malloc(size+1);
    __builtin_memcpy(txt, data, size);
    txt[size] = 0;
    MultiSpreadSheet ms = {0};
    DrSpreadCtx* ctx = NULL;
    int e = read_multi_csv_from_string(&ms, txt);
    free(txt);
    if(e) {
        goto bad;
        return 0;
    }
    SheetOps ops = multisheet_ops(&ms);
    ctx = drsp_create_ctx(&ops);
    if(!ctx) goto bad;
    for(int i = 0; i < ms.n; i++){
        SpreadSheet* sheet = &ms.sheets[i];
        int e = drsp_set_sheet_name(ctx, (SheetHandle)sheet, sheet->name.text, sheet->name.length);
        if(e) goto bad;
        for(int i = 0; i < sheet->colnames.n; i++){
            int e = drsp_set_col_name(ctx, (SheetHandle)sheet, i, sheet->colnames.data[i], sheet->colnames.lengths[i]);
            if(e) goto bad;
        }
        for(intptr_t r = 0; r < sheet->rows; r++){
            const SheetRow* row = &sheet->cells[r];
            for(int c = 0; c < row->n; c++){
                const char* txt = row->data[c];
                size_t len = row->lengths[c];
                int e = drsp_set_cell_str(ctx, (SheetHandle)sheet, r, c, txt, len);
                if(e) goto bad;
            }
        }
    }
    for(int i = 0; i < ms.n; i++){
        int nerr = drsp_evaluate_formulas(ctx, (SheetHandle)&ms.sheets[i], NULL, 0);
        (void)nerr;
    }
    bad:
    cleanup_multisheet(&ms);
    drsp_destroy_ctx(ctx);
    return 0;
}
#endif
