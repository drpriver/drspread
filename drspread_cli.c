//
// Copyright © 2023, David Priver
//
#ifndef DRSPREAD_CLI_C
#define DRSPREAD_CLI_C
#include "spreadsheet.h"
#include "drspread.h"
#include "get_input.h"
#include "argument_parsing.h"
#include "term_util.h"
#include <stdio.h>
int
main(int argc, char** argv){
    _Bool multisheet = 0;
    StringView filename;
    StringView expressions[20] = {0};
    ArgToParse pos_args[] = {
        {
            .name = SV("filename"),
            .dest = ARGDEST(&filename),
            .help = "Which file to read.",
            .min_num = 1,
            .max_num = 1,
        },
        {
            .name = SV("expression"),
            .dest = ARGDEST(expressions),
            .help = "Expression to evaluate.",
            .min_num = 0,
            .max_num = arrlen(expressions),
        },
    };
    ArgToParse kw_args[] = {
        {
            .name = SV("-m"),
            .altname1 = SV("--multi"),
            .dest = ARGDEST(&multisheet),
            .help = "Parse the file as containing multiple sheets.",
        },
    };
    enum {HELP=0};
    ArgToParse early_args[] = {
        [HELP] = {
            .name = SV("-h"),
            .altname1 = SV("--help"),
            .help = "Print this help and exit.",
        },
    };
    ArgParser parser = {
        .name = argc?argv[0]:"drspread",
        .description = "A toy spreadsheet evaluator.",
        .positional = {
            .args = pos_args,
            .count = arrlen(pos_args),
        },
        .keyword = {
            .args = kw_args,
            .count = arrlen(kw_args),
        },
        .early_out = {
            .args = early_args,
            .count = arrlen(early_args),
        },
        .styling={.plain = !isatty(fileno(stdout)),},
    };
    Args args = argc?(Args){argc-1, (const char*const*)argv+1}: (Args){0, 0};
    switch(check_for_early_out_args(&parser, &args)){
        case HELP:{
            int columns = get_terminal_size().columns;
            if(columns > 80)
                columns = 80;
            print_argparse_help(&parser, columns);
        } return 1;
        default:
            break;
    }
    enum ArgParseError ae = parse_args(&parser, &args, ARGPARSE_FLAGS_NONE);
    if(ae){
        print_argparse_error(&parser, ae);
        fprintf(stderr, "Use --help to see usage.\n");
        return (int)ae;
    }
    MultiSpreadSheet ms = {0};
    SheetOps ops;
    if(!multisheet){
        int e = read_csv(multisheet_alloc(&ms), filename.text);
        if(e) return e;
        ops = sheet_ops();
    }
    else {
        int e = read_multi_csv(&ms, filename.text);
        if(e) return e;
        ops = multisheet_ops(&ms);
    }
    if(!ms.n){
        fprintf(stderr, "No sheets.\n");
        return 1;
    }
    if(pos_args[1].num_parsed){
        for(int i = 0; i < pos_args[1].num_parsed; i++){
            StringView expr = expressions[i];
            DrSpreadCellValue val;
            int err = drsp_evaluate_string((SheetHandle)&ms.sheets[0], &ops, expr.text, expr.length, &val, -1, -1);
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
        for(int i = 0; i < ms.n; i++){
            int nerr = drsp_evaluate_formulas((SheetHandle)&ms.sheets[i], &ops, NULL, 0);
            // printf("nerr: %d\n", nerr);
            (void)nerr;
        }
        #ifdef BENCHMARKING
            return 0;
        #endif
        for(int i = 0; i < ms.n; i++){
            SpreadSheet* sheet = &ms.sheets[i];
            write_display(sheet, stdout);
        }
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
            if(!len){
                for(int i = 0; i < ms.n; i++){
                    SpreadSheet* sheet = &ms.sheets[i];
                    write_display(sheet, stdout);
                }
                continue;
            }
            gi_add_line_to_history_len(&gi, line, len);
            DrSpreadCellValue val;
            int err = drsp_evaluate_string((SheetHandle)&ms.sheets[0], &ops, line, len, &val, -1, -1);
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
