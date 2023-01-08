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
        .row_width=&get_row_width,
        .col_height=&get_col_height,
    };
    if(argc > 2){
        for(int i = 2; i < argc; i++){
            double val;
            int err = drsp_evaluate_string(&ops, argv[i], strlen(argv[i]), &val);
            if(err){
                puts("err");
            }
            else
                printf("%.1f\n", val);
        }
    }
    else {
        int nerr = drsp_evaluate_formulas(&ops);
        printf("nerr: %d\n", nerr);
        write_display(&sheet, stdout);
    }
    // for(;;){
    // }
    return 0;
}
#include "drspread.c"
