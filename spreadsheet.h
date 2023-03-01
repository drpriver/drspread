//
// Copyright © 2023, David Priver
//
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

#if !defined(likely) && !defined(unlikely)
#if defined(__GNUC__) || defined(__clang__)
#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif
#endif

// This code is only intended for testing and the demo cli app.
// So it leaks memory all over the place.
typedef struct SheetRow SheetRow;
struct SheetRow {
    int n;
    const char*_Nonnull*_Nonnull data;
    size_t* lengths;
};

static
void
sheet_row_push(SheetRow* ro, const char* txt){
    ro->data = realloc(ro->data, ++ro->n*sizeof(txt));
    ro->data[ro->n-1] = txt;
    ro->lengths = realloc(ro->lengths, ro->n*sizeof txt);
    ro->lengths[ro->n-1] = strlen(txt);
}

typedef struct SpreadSheet SpreadSheet;
struct SpreadSheet {
    StringView name;
    SheetRow colnames;
    SheetRow* cells;
    SheetRow* display;
    intptr_t rows;
    intptr_t maxcols;
};

static
int
sheet_next(void* m, SheetHandle hnd, intptr_t i, intptr_t* row, intptr_t* col){
    (void)m;
    (void)i;
    SpreadSheet* sheet =(SpreadSheet*)hnd;
    intptr_t r = *row;
    intptr_t c = *col;
    if(unlikely(r == -1)){
        *row = 0;
        *col = 0;
        return 0;
    }
    c++;
    const SheetRow* ro = &sheet->cells[r];
    if(c >= ro->n) r++, c=0;
    if(unlikely(r >= sheet->rows)) return 1;
    *row = r;
    *col = c;
    return 0;
}

static
const char*
sheet_txt(void*m, SheetHandle hnd, intptr_t row, intptr_t col, size_t* len){
    (void)m;
    SpreadSheet* sheet =(SpreadSheet*)hnd;
    if(unlikely(row < 0 || row >= sheet->rows)){
        *len = 0;
        return "";
    }
    const SheetRow* ro = &sheet->cells[row];
    if(unlikely(col < 0 || col >= ro->n)){
        *len = 0;
        return "";
    }
    const char* t = ro->data[col];
    *len = ro->lengths[col];
    return t;
}

static
int
sheet_set_display_number(void* m, SheetHandle hnd, intptr_t row, intptr_t col, double val){
    (void)m;
    SpreadSheet* sheet =(SpreadSheet*)hnd;
    if(unlikely(row < 0 || row >= sheet->rows)) return 1;
    SheetRow* ro = &sheet->display[row];
    if(unlikely(col < 0 || col >= ro->n)) return 1;
    int printed;
    if((intptr_t)val == val)
        printed = asprintf((char**)&ro->data[col], "%zd", (intptr_t)val) < 0;
    else
        printed = asprintf((char**)&ro->data[col], "%-.1f", val) < 0;
    if(printed < 0) return 1;
    ro->lengths[col] = printed;
    return 0;
}

static
int
sheet_set_display_error(void*m, SheetHandle hnd, intptr_t row, intptr_t col, const char* mess, size_t len){
    (void)m;
    SpreadSheet* sheet =(SpreadSheet*)hnd;
    if(unlikely(row < 0 || row >= sheet->rows)) return 1;
    SheetRow* ro = &sheet->display[row];
    if(unlikely(col < 0 || col >= ro->n)) return 1;
    (void)mess;
    (void)len;
    ro->data[col] = "error";
    ro->lengths[col] = 5;
    return 0;
}
static
int
sheet_set_display_string(void*m, SheetHandle hnd, intptr_t row, intptr_t col, const char* mess, size_t len){
    (void)m;
    SpreadSheet* sheet =(SpreadSheet*)hnd;
    if(unlikely(row < 0 || row >= sheet->rows)) return 1;
    SheetRow* ro = &sheet->display[row];
    if(unlikely(col < 0 || col >= ro->n)) return 1;
    int printed = asprintf((char**)&ro->data[col], "%.*s", (int)len, mess);
    if(printed < 0) return 1;
    ro->lengths[col] = printed;
    return 0;
}

static
intptr_t
sheet_get_name_to_col_idx(void*m, SheetHandle hnd, const char* name, size_t length){
    (void)m;
    (void)hnd;
    (void)length;
    assert(length);
    assert(name[0] >= 'a');
    return name[0] - 'a';
}

static
intptr_t
sheet_get_row_width(void*m, SheetHandle hnd, intptr_t row){
    (void)m;
    SpreadSheet* sheet =(SpreadSheet*)hnd;
    if(unlikely(row < 0 || row >= sheet->rows)) return 0;
    SheetRow* ro = &sheet->display[row];
    return ro->n;
}

static
intptr_t
sheet_get_col_height(void*m, SheetHandle hnd, intptr_t col){
    (void)m;
    (void)col;
    SpreadSheet* sheet =(SpreadSheet*)hnd;
    return sheet->rows;
}

static
int
sheet_get_dims(void*m, SheetHandle hnd, intptr_t* ncols, intptr_t* nrows){
    (void)m;
    SpreadSheet* sheet =(SpreadSheet*)hnd;
    *ncols = sheet->maxcols;
    *nrows = sheet->rows;
    return 0;
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
        SheetRow ro = {0};
        SheetRow disp = {0};
        char* token;
        while((token = strsep(&line, "|"))){
            size_t len = strlen(token);
            if(len && token[len-1] == '\n')
                token[len-1] = 0;
            sheet_row_push(&ro, token);
            sheet_row_push(&disp, "");
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

static inline
int
read_csv_from_string(SpreadSheet* sheet, const char* srctxt){
    char* txt = strdup(srctxt);
    char* line;
    int max_cols = 0;
    for(;(line=strsep(&txt, "\n"));){
        SheetRow ro = {0};
        SheetRow disp = {0};
        char* token;
        while((token = strsep(&line, "|"))){
            size_t len = strlen(token);
            if(len && token[len-1] == '\n')
                token[len-1] = 0;
            sheet_row_push(&ro, token);
            sheet_row_push(&disp, "");
        }
        if(ro.n > max_cols) max_cols = ro.n;
        ++sheet->rows;
        sheet->cells = realloc(sheet->cells, sizeof(*sheet->cells)*sheet->rows);
        sheet->cells[sheet->rows-1] = ro;
        sheet->display = realloc(sheet->display, sizeof(*sheet->display)*sheet->rows);
        sheet->display[sheet->rows-1] = disp;
        sheet->maxcols = max_cols;
    }
    if(sheet->rows && sheet->cells[sheet->rows-1].n == 1 && strlen(sheet->cells[sheet->rows-1].data[0])==0)
        sheet->rows--;
    return 0;
}

static
int
write_display(SpreadSheet* sheet, FILE* out){
    intptr_t C = 0;
    int lens[12] = {4,4,4,4,4,4,4,4,4,4,4,4};
    for(intptr_t row = 0; row < sheet->rows; row++){
        const SheetRow* ro = &sheet->display[row];
        if(ro->n > C) C = ro->n;
        for(int i = 0; i < ro->n && i < 12; i++){
            int len = ro->lengths[i];
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
        const SheetRow* ro = &sheet->display[row];
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
