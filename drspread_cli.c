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
        .next_cell=&sheet_next,
        .cell_txt=&sheet_txt,
        .set_display_number=&sheet_set_display_number,
        .set_display_error=&sheet_set_display_error,
        .set_display_string=&sheet_set_display_string,
        .name_to_col_idx=&sheet_get_name_to_col_idx,
        .row_width=&sheet_get_row_width,
        .col_height=&sheet_get_col_height,
        .dims=&sheet_get_dims,
    };
    if(argc > 2){
        for(int i = 2; i < argc; i++){
            DrSpreadCellValue val;
            int err = drsp_evaluate_string((SheetHandle)&sheet, &ops, argv[i], strlen(argv[i]), &val, -1, -1);
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
            int err = drsp_evaluate_string((SheetHandle)&sheet, &ops, line, len, &val, -1, -1);
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
                        free((char*)val.s.text);
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
#define DRSP_INTRINS 1
#include "drspread.c"
#include "get_input.c"
#endif
