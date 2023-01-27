//
// Copyright © 2023, David Priver
//
#ifndef DRSPREAD_CLI_C
#define DRSPREAD_CLI_C
#include "spreadsheet.h"
#include "drspread.h"
#include "get_input.h"
#include <stdio.h>
int
main(int argc, char** argv){
    if(argc < 2) return 1;
    SpreadSheet sheet = {0};
    int e = read_csv(&sheet, argv[1]);
    if(e) return e;
    SheetOps ops = {
        .ctx = NULL,
        .next_cell=&next,
        .cell_txt=&txt,
        .set_display_number=&display_number,
        .set_display_error=&display_error,
        .set_display_string=&display_string,
        .name_to_col_idx=&get_name_to_col_idx,
        .row_width=&get_row_width,
        .col_height=&get_col_height,
        .dims=&get_dims,
    };
    if(argc > 2){
        for(int i = 2; i < argc; i++){
            DrSpreadCellValue val;
            int err = drsp_evaluate_string((SheetHandle)&sheet, &ops, argv[i], strlen(argv[i]), &val);
            if(err){
                puts("err");
            }
            else{
                switch(val.kind){
                    case CELL_EMPTY:
                        printf("\n");
                        break;
                    case CELL_NUMBER:
                        if((intptr_t)val.d == val.d)
                            printf("%zd\n", (intptr_t)val.d);
                        else
                            printf("%.2f\n", val.d);
                        break;
                    case CELL_OTHER:
                        printf("'%.*s'\n", (int)val.s.length, val.s.text);
                        break;
                    default:
                        printf("err\n");
                        break;
                }
            }
        }
    }
    else {
        int nerr = drsp_evaluate_formulas((SheetHandle)&sheet, &ops, NULL, 0);
        // printf("nerr: %d\n", nerr);
        (void)nerr;
        #ifdef BENCHMARKING
            return 0;
        #endif
        write_display(&sheet, stdout);
        GetInputCtx gi = {
            .prompt.text = "> ",
            .prompt.length = 2,
        };
        gi_load_history(&gi, "spread.history");
        for(;;){
            ssize_t len = gi_get_input(&gi);
            fputs("\r", stdout);
            fflush(stdout);
            if(len < 0) break;
            char* line = gi.buff;
            if(len == 1 && *line == 'q') break;
            gi_add_line_to_history_len(&gi, line, len);
            DrSpreadCellValue val;
            int err = drsp_evaluate_string((SheetHandle)&sheet, &ops, line, len, &val);
            if(err) puts("err");
            else {
                switch(val.kind){
                    case CELL_EMPTY:
                        printf("\n");
                        break;
                    case CELL_NUMBER:
                        if((intptr_t)val.d == val.d)
                            printf("%zd\n", (intptr_t)val.d);
                        else
                            printf("%.2f\n", val.d);
                        break;
                    case CELL_OTHER:
                        printf("'%.*s'\n", (int)val.s.length, val.s.text);
                        break;
                    default:
                        printf("err\n");
                        break;
                }
            }
        }
        gi_dump_history(&gi, "spread.history");
    }
    return 0;
}
#include "drspread.c"
#include "get_input.c"
#endif
