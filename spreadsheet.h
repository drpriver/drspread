//
// Copyright © 2023, David Priver
//
#ifndef SPREADSHEET_H
#define SPREADSHEET_H

#if defined(__linux__)
#define _GNU_SOURCE 1
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
    void* d = realloc(ro->data, ++ro->n*sizeof(txt));
    if(!d) abort();
    ro->data = d;
    ro->data[ro->n-1] = txt;
    d = realloc(ro->lengths, ro->n*sizeof txt);
    if(!d) abort();
    ro->lengths = d;
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
    char* txt;
};

static void
cleanup_row_pair(SheetRow* ro, SheetRow* disp){
    free(ro->data);
    free(ro->lengths);
    for(int i = 0; i < disp->n; i++){
        free((void*)disp->data[i]);
    }
    free(disp->data);
    free(disp->lengths);
}

static
void
cleanup_sheet(SpreadSheet* sheet){
    if(sheet->txt) free(sheet->txt);
    for(intptr_t i = 0; i < sheet->rows; i++){
        cleanup_row_pair(&sheet->cells[i], &sheet->display[i]);
    }
    free(sheet->cells);
    free(sheet->display);
    free(sheet->colnames.data);
    free(sheet->colnames.lengths);
}

static
void
sheet_push_row(SpreadSheet* sheet, SheetRow ro, SheetRow disp){
    ++sheet->rows;

    sheet->cells = realloc(sheet->cells, sizeof(*sheet->cells)*sheet->rows);
    assert(sheet->cells);
    sheet->cells[sheet->rows-1] = ro;

    sheet->display = realloc(sheet->display, sizeof(*sheet->display)*sheet->rows);
    assert(sheet->display);
    sheet->display[sheet->rows-1] = disp;
}

typedef struct MultiSpreadSheet MultiSpreadSheet;
struct MultiSpreadSheet {
    char* txt;
    int n;
    SpreadSheet* sheets;
};

static
void
cleanup_multisheet(MultiSpreadSheet* ms){
    for(int i = 0; i < ms->n; i++)
        cleanup_sheet(&ms->sheets[i]);
    free(ms->sheets);
    free(ms->txt);
}

static
SpreadSheet*_Nullable
multisheet_alloc(MultiSpreadSheet* ms){
    SpreadSheet* newsheets = realloc(ms->sheets, (ms->n+1)*sizeof *newsheets);
    if(!newsheets) return NULL;
    assert(newsheets);
    ms->sheets = newsheets;
    SpreadSheet* result = &newsheets[ms->n++];
    memset(result, 0, sizeof *result);
    return result;
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
    free((void*)ro->data[col]);
    if(__builtin_lround(val) == val)
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
    free((void*)ro->data[col]);
    ro->data[col] = strdup("error");
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
    free((void*)ro->data[col]);
    int printed = asprintf((char**)&ro->data[col], "%.*s", (int)len, mess);
    if(printed < 0) return 1;
    ro->lengths[col] = printed;
    return 0;
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


#ifndef __wasm__
static
char*_Nullable
read_file(const char* filename){
    char* txt = NULL;
    int err = 0;
    long len = 0;
    size_t n = 0;
    FILE* fp = fopen(filename, "rb");
    if(!fp) goto fail;
    err = fseek(fp, 0, SEEK_END);
    if(err) goto fail;
    len = ftell(fp);
    if(len < 0) goto fail;
    err = fseek(fp, 0, SEEK_SET);
    if(err) goto fail;
    txt = malloc(len+1);
    if(!txt) goto fail;
    n = fread(txt, 1, len, fp);
    if(n != (size_t)len) goto fail;

    txt[len] = 0;
    fclose(fp);
    return txt;

    fail:
    free(txt);
    fclose(fp);
    return NULL;
}
#endif

static
int
read_csv_from_string(SpreadSheet* sheet, const char* srctxt){
    char* txt = strdup(srctxt);
    sheet->txt = txt;
    char* line;
    int max_cols = 0;
    for(;(line=strsep(&txt, "\n"));){
        SheetRow ro = {0};
        SheetRow disp = {0};
        char* token;
        while((token = strsep(&line, "\t|"))){
            sheet_row_push(&ro, token);
            sheet_row_push(&disp, strdup(""));
        }
        if(ro.n > max_cols) max_cols = ro.n;
        sheet_push_row(sheet, ro, disp);
        sheet->maxcols = max_cols;
    }
    if(sheet->rows && sheet->cells[sheet->rows-1].n == 1 && strlen(sheet->cells[sheet->rows-1].data[0])==0){
        cleanup_row_pair(&sheet->cells[sheet->rows-1], &sheet->display[sheet->rows-1]);
        sheet->rows--;
    }
    return 0;
}


#ifndef __wasm__
static
int
read_csv(SpreadSheet* sheet, const char* filename){
    char* txt = read_file(filename);
    if(!txt) return 1;
    int result = read_csv_from_string(sheet, txt);
    free(txt);
    return result;
}
#endif

static
int
read_multi_csv_from_string(MultiSpreadSheet* ms, const char* srctxt){
    char* txt = strdup(srctxt);
    ms->txt = txt;
    char* line;
    int max_cols = 0;
    _Bool need_colnames = 0;
    SpreadSheet* sheet = NULL;
    for(;(line=strsep(&txt, "\n"));){
        size_t len = strlen(line);
        if(len == 3 && memcmp(line, "---", 3) == 0){
            while(sheet && sheet->rows && sheet->cells[sheet->rows-1].n == 1 && strlen(sheet->cells[sheet->rows-1].data[0])==0)
                sheet->rows--;
            sheet = NULL;
            continue;
        }
        if(!sheet){
            if(!len) continue;
            max_cols = 0;
            sheet = multisheet_alloc(ms);
            if(!sheet) return 1;
        }
        if(!sheet->name.length){
            sheet->name.length = len;
            sheet->name.text = line;
            need_colnames = 1;
            continue;
        }
        char* token;
        if(need_colnames){
            while((token = strsep(&line, "\t|"))){
                while(*token == ' ')
                    token++;
                size_t len = strlen(token);
                while(len && token[len-1] == ' ')
                    token[--len] = 0;
                // assert(len);
                sheet_row_push(&sheet->colnames, token);
            }
            max_cols = sheet->colnames.n;
            need_colnames = 0;
            continue;
        }
        SheetRow ro = {0};
        SheetRow disp = {0};
        while((token = strsep(&line, "\t|"))){
            sheet_row_push(&ro, token);
            sheet_row_push(&disp, strdup(""));
        }
        if(ro.n > max_cols) max_cols = ro.n;
        sheet_push_row(sheet, ro, disp);
        sheet->maxcols = max_cols;
    }
    while(sheet && sheet->rows && sheet->cells[sheet->rows-1].n == 1 && strlen(sheet->cells[sheet->rows-1].data[0])==0){
        cleanup_row_pair(&sheet->cells[sheet->rows-1], &sheet->display[sheet->rows-1]);
        sheet->rows--;
    }
    return 0;
}


#ifndef __wasm__
static
int
read_multi_csv(MultiSpreadSheet* ms, const char* filename){
    char* txt = read_file(filename);
    if(!txt) return 1;
    int result = read_multi_csv_from_string(ms, txt);
    free(txt);
    return result;
}
#endif

#ifndef __wasm__
static
int
write_display(SpreadSheet* sheet, FILE* out){
    if(sheet->name.length){
        fprintf(out, "-*- %.*s -*-\n", (int)sheet->name.length, sheet->name.text);
        fprintf(out, "-------------------\n");
    }
    intptr_t C = 0;
    int lens[12] = {4,4,4,4,4,4,4,4,4,4,4,4};
    {
        const SheetRow* ro = &sheet->colnames;
        if(ro->n > C) C = ro->n;
        for(int i = 0; i < ro->n && i < 12; i++){
            int len = ro->lengths[i];
            if(lens[i] < len) lens[i] = len;
        }
    }
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
        if(i < sheet->colnames.n){
            fprintf(out, " %-*.*s |", lens[i], (int)sheet->colnames.lengths[i], sheet->colnames.data[i]);
        }
        else
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
            fprintf(out, " %-*s%s", lens[col < 12?col:11], ro->data[col], " |");
        }
        fputc('\n', out);
    }
    fflush(out);
    return 0;
}
#endif

#ifndef DRSPREAD_DIRECT_OPS
static
SheetOps
multisheet_ops(MultiSpreadSheet* ms){
    SheetOps ops = {
        .ctx = ms,
        .set_display_number=&sheet_set_display_number,
        .set_display_error=&sheet_set_display_error,
        .set_display_string=&sheet_set_display_string,
        .row_width=&sheet_get_row_width,
        .col_height=&sheet_get_col_height,
    };
    return ops;
}


static
SheetOps
sheet_ops(void){
    SheetOps ops = {
        .ctx = NULL,
        .set_display_number=&sheet_set_display_number,
        .set_display_error=&sheet_set_display_error,
        .set_display_string=&sheet_set_display_string,
        .row_width=&sheet_get_row_width,
        .col_height=&sheet_get_col_height,
    };
    return ops;
}
#endif

#ifdef __clang__
#pragma clang assume_nonnull end
#endif

#endif
