#ifndef SPREADSHEET_H
#define SPREADSHEET_H
#if defined(__linux__)
#define _GNU_SOURCE
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "drspread.h"
#include "stringview.h"
#include "parse_numbers.h"

#ifdef __clang__
#pragma clang assume_nonnull begin
#else
#ifndef _Nonnull
#define _Nonnull
#endif
#ifndef _Nullable
#define _Nullable
#endif
#endif

struct Row {
    int n;
    const char*_Nonnull*_Nonnull data;
};

static
void row_push(struct Row* ro, const char* txt){
    ro->data = realloc(ro->data, ++ro->n*sizeof(txt));
    ro->data[ro->n-1] = txt;
}

typedef struct SpreadSheet SpreadSheet;
struct SpreadSheet {
    struct Row* cells;
    struct Row* display;
    intptr_t rows;
    intptr_t maxcols;
};

static
int next(void* ctx, intptr_t* row, intptr_t* col){
    SpreadSheet* sheet = ctx;
    intptr_t r = *row;
    intptr_t c = *col;
    if(r == -1){
        *row = 0;
        *col = 0;
        return 0;
    }
    c++;
    const struct Row* ro = &sheet->cells[r];
    if(c >= ro->n) r++, c=0;
    if(r >= sheet->rows) return 1;
    *row = r;
    *col = c;
    return 0;
}

static
const char* txt(void* ctx, intptr_t row, intptr_t col, size_t* len){
    SpreadSheet* sheet = ctx;
    if(row < 0 || row >= sheet->rows){
        *len = 0;
        return "";
    }
    const struct Row* ro = &sheet->cells[row];
    if(col < 0 || col >= ro->n){
        *len = 0;
        return "";
    }
    const char* t = ro->data[col];
    *len = strlen(t);
    return t;
}

static
int
display_number(void* ctx, intptr_t row, intptr_t col, double val){
    SpreadSheet* sheet = ctx;
    if(row < 0 || row >= sheet->rows) return 1;
    struct Row* ro = &sheet->display[row];
    if(col < 0 || col >= ro->n) return 1;
    return asprintf((char**)&ro->data[col], "%-.0f", __builtin_round(val)) < 0;
}

static
int
display_error(void* ctx, intptr_t row, intptr_t col, const char* mess, size_t len){
    SpreadSheet* sheet = ctx;
    if(row < 0 || row >= sheet->rows) return 1;
    struct Row* ro = &sheet->display[row];
    if(col < 0 || col >= ro->n) return 1;
    (void)mess;
    (void)len;
    ro->data[col] = sheet->cells[row].data[col];
    return 0;
}
static
int
display_string(void* ctx, intptr_t row, intptr_t col, const char* mess, size_t len){
    SpreadSheet* sheet = ctx;
    if(row < 0 || row >= sheet->rows) return 1;
    struct Row* ro = &sheet->display[row];
    if(col < 0 || col >= ro->n) return 1;
    asprintf((char**)&ro->data[col], "%.*s", (int)len, mess);
    return 0;
}

static
intptr_t
get_name_to_col_idx(void* ctx, const char* name, size_t length){
    (void)ctx;
    (void)length;
    assert(length);
    assert(name[0] >= 'a');
    return name[0] - 'a';
}

static
intptr_t
get_row_width(void* ctx, intptr_t row){
    SpreadSheet* sheet = ctx;
    if(row < 0 || row >= sheet->rows) return 0;
    struct Row* ro = &sheet->display[row];
    return ro->n;
}

static
intptr_t
get_col_height(void* ctx, intptr_t col){
    (void)col;
    SpreadSheet* sheet = ctx;
    return sheet->rows;
}

static
int
get_dims(void* ctx, intptr_t* ncols, intptr_t* nrows){
    SpreadSheet* sheet = ctx;
    *ncols = sheet->maxcols;
    *nrows = sheet->rows;
    return 0;
}

static
double
cell_number(void* ctx, intptr_t row, intptr_t col){
    // printf("%s: row,col: %zd,%zd\n", __func__, row, col);
    SpreadSheet* sheet = ctx;
    if(row < 0 || row >= sheet->rows) return 0;
    struct Row* ro = &sheet->cells[row];
    if(col < 0 || col >= ro->n) return 0;
    StringView s = {strlen(ro->data[col]), ro->data[col]};
    s = stripped(s);
    DoubleResult dr = parse_double(s.text, s.length);
    return dr.result;
}

static
CellKind
cell_kind(void* ctx, intptr_t row, intptr_t col){
    SpreadSheet* sheet = ctx;
    if(row < 0 || row >= sheet->rows) return CELL_EMPTY;
    struct Row* ro = &sheet->cells[row];
    if(col < 0 || col >= ro->n) return CELL_EMPTY;
    const char* txt = ro->data[col];
    if(!txt) return CELL_EMPTY;
    while(*txt == ' ') txt++;
    StringView s = {strlen(txt), txt};
    s = stripped(s);
    if(s.length == 0) return CELL_EMPTY;
    if(s.text[0] == '=') return CELL_FORMULA;
    if(!parse_double(s.text, s.length).errored) return CELL_NUMBER;
    return CELL_OTHER;
}


static
int
read_csv(SpreadSheet* sheet, const char* filename){
    FILE* fp = fopen(filename, "rb");
    if(!fp) return 1;
    int max_cols = 0;
    for(;;){
        char* line = NULL;
        size_t quant = 0;
        ssize_t len = getline(&line, &quant, fp);
        if(len <= -1) break;
        struct Row ro = {0};
        struct Row disp = {0};
        char* token;
        while((token = strsep(&line, "|"))){
            size_t len = strlen(token);
            if(len && token[len-1] == '\n')
                token[len-1] = 0;
            row_push(&ro, token);
            row_push(&disp, "");
        }
        if(ro.n > max_cols) max_cols = ro.n;
        ++sheet->rows;
        sheet->cells = realloc(sheet->cells, sizeof(*sheet->cells)*sheet->rows);
        sheet->cells[sheet->rows-1] = ro;
        sheet->display = realloc(sheet->display, sizeof(*sheet->display)*sheet->rows);
        sheet->display[sheet->rows-1] = disp;
        sheet->maxcols = max_cols;
    }
    return 0;
}

static
int
write_display(SpreadSheet* sheet, FILE* out){
    intptr_t C = 0;
    int lens[12] = {4,4,4,4,4,4,4,4,4,4,4,4};
    for(intptr_t row = 0; row < sheet->rows; row++){
        const struct Row* ro = &sheet->display[row];
        if(ro->n > C) C = ro->n;
        for(int i = 0; i < ro->n && i < 12; i++){
            int len = strlen(ro->data[i]);
            if(lens[i] < len) lens[i] = len;
        }
    }
    fprintf(out, "    | ");
    if(C > 12) C = 12;
    for(intptr_t i = 0; i < C; i++){
        fprintf(out, " %-*c |", lens[i], (int)(i + 'a'));
    }
    fputc('\n', out);
    fprintf(out, "----|-");
    for(intptr_t i = 0; i < C; i++){
        for(int j = 0; j < lens[i]+2; j++){
            fputc('-', out);
        }
        fputc('|', out);
    }
    fputc('\n', out);
    for(intptr_t row = 0; row < sheet->rows; row++){
        const struct Row* ro = &sheet->display[row];
        fprintf(out, "%3zd | ", row+1);
        for(int col = 0; col < ro->n; col++){
            fprintf(out, " %4s%s", ro->data[col], " |");
        }
        fputc('\n', out);
    }
    fflush(out);
    return 0;
}

#ifdef __clang__
#pragma clang assume_nonnull end
#endif

#endif
