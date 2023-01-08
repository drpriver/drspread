#include <stdio.h>
#include "spreadsheet.h"
#include "drspread.h"
int
main(int argc, char** argv){
    if(argc < 2) return 1;
    SpreadSheet sheet = {0};
    int e = read_csv(&sheet, argv[1]);
    if(e) return e;
    SheetOps ops = {
        .ctx = &sheet,
        .next_cell=&next,
        .cell_txt=&txt,
        .set_display_number=&display_number,
        .set_display_error=&display_error,
        .name_to_col_idx=&get_name_to_col_idx,
        .query_cell_kind=&cell_kind,
        .cell_number=&cell_number,
    };
    int err = drsp_evaluate_formulas(&ops);
    printf("err: %d\n", err);
    write_display(&sheet, stdout);
    return err;
}
#include "drspread.c"
