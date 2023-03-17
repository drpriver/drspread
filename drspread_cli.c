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
    _Bool printit = 0;
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
        {
            .name = SV("-p"),
            .altname1 = SV("--print"),
            .dest = ARGDEST(&printit),
            .help = "print the evaled spreadsheet then exit.",
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
    DrSpreadCtx* ctx = drsp_create_ctx(&ops);
    for(int i = 0; i < ms.n; i++){
        SpreadSheet* sheet = &ms.sheets[i];
        int e = drsp_set_sheet_name(ctx, (SheetHandle)sheet, sheet->name.text, sheet->name.length);
        if(e) return e;
        for(int i = 0; i < sheet->colnames.n; i++){
            int e = drsp_set_col_name(ctx, (SheetHandle)sheet, i, sheet->colnames.data[i], sheet->colnames.lengths[i]);
            if(e) return e;
        }
        for(intptr_t r = 0; r < sheet->rows; r++){
            const SheetRow* row = &sheet->cells[r];
            for(int c = 0; c < row->n; c++){
                const char* txt = row->data[c];
                size_t len = row->lengths[c];
                int e = drsp_set_cell_str(ctx, (SheetHandle)sheet, r, c, txt, len);
                if(e) return e;
            }
        }
    }
    if(pos_args[1].num_parsed){
        for(int i = 0; i < pos_args[1].num_parsed; i++){
            StringView expr = expressions[i];
            DrSpreadResult val;
            int err = drsp_evaluate_string(ctx, (SheetHandle)&ms.sheets[0], expr.text, expr.length, &val, -1, -1);
            if(err){
                puts("err");
                continue;
            }
            switch(val.kind){
                case DRSP_RESULT_NULL:
                    printf("\n");
                    break;
                case DRSP_RESULT_NUMBER:
                    if((intptr_t)val.d == val.d)
                        printf("%zd\n", (intptr_t)val.d);
                    else
                        printf("%.2f\n", val.d);
                    break;
                case DRSP_RESULT_STRING:
                    printf("'%.*s'\n", (int)val.s.length, val.s.text);
                    break;
                default:
                    printf("err\n");
                    break;
            }
        }
    }
    else {
        for(int i = 0; i < ms.n; i++){
            int nerr = drsp_evaluate_formulas(ctx, (SheetHandle)&ms.sheets[i], NULL, 0);
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
        if(printit) {
            return 0;
        }
        GetInputCtx gi = {
            .prompt = SV("> "),
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
            DrSpreadResult val;
            int err = drsp_evaluate_string(ctx, (SheetHandle)&ms.sheets[0], line, len, &val, -1, -1);
            if(err){
                puts("err");
                continue;
            }
            switch(val.kind){
                case DRSP_RESULT_NULL:
                    printf("\n");
                    break;
                case DRSP_RESULT_NUMBER:
                    if((intptr_t)val.d == val.d)
                        printf("%zd\n", (intptr_t)val.d);
                    else
                        printf("%.2f\n", val.d);
                    break;
                case DRSP_RESULT_STRING:
                    printf("'%.*s'\n", (int)val.s.length, val.s.text);
                    break;
                default:
                    printf("err\n");
                    break;
            }
        }
        gi_dump_history(&gi, "spread.history");
        if(0) cleanup_multisheet(&ms);
    }
    return 0;
}
#define DRSP_INTRINS 1
#include "drspread.c"
#include "get_input.c"
#endif
