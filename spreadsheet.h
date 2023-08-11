//
// Copyright © 2023, David Priver <david@davidpriver.com>
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
#include <stdarg.h>
#include "drspread.h"
#include "stringview.h"
#include "parse_numbers.h"
#include "drspread_allocators.h"

#if (defined(_MSC_VER) && !defined(__clang__)) || defined(__IMPORTC__)
#include <math.h>
#define __builtin_lround lround
#endif

#ifdef __clang__
#pragma clang assume_nonnull begin
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#else
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
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
#if defined(__wasm__) || defined(_WIN32)
static inline
char*_Nullable
strsep(char*_Nullable*_Nonnull stringp, const char* delim){
    char* result = *stringp;
    char* s = *stringp;
    if(s){
        for(;*s;s++){
            for(const char* d=delim;*d;d++){
                if(*s == *d){
                    // This is garbage, but whatever, it's just for
                    // demo/testing purposes.
                    if(*d == '\n' && s != result && s[-1] == '\r')
                        s[-1] = '\0';
                    *s = '\0';
                    *stringp = s+1;
                    return result;
                }
            }
        }
    }
    *stringp = NULL;
    return result;
}
#endif

static inline
char*_Nullable
drsp_strdup(const char* txt){
    size_t len = strlen(txt)+1;
    char* t = drsp_alloc(0, NULL, len, _Alignof *t);
    if(!t) return t;
    memcpy(t, txt, len);
    return t;
}

static inline
int
drsp_asprintf(char*_Nullable*_Nonnull out, const char* fmt, ...){
    va_list vap, vap2;
    va_start(vap, fmt);
    va_copy(vap2, vap);
    int len = vsnprintf(NULL, 0, fmt, vap);
    char* buff = drsp_alloc(0, NULL, len+1, _Alignof *buff);
    int ret = vsnprintf(buff, len+1, fmt, vap2);
    *out = buff;
    va_end(vap);
    va_end(vap2);
    return ret;
}


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
    size_t oldsz = ro->n*sizeof txt;
    size_t newsz = ++ro->n*sizeof txt;
    void* d = drsp_alloc(oldsz, ro->data, newsz, _Alignof txt);
    if(!d) abort();
    ro->data = d;
    ro->data[ro->n-1] = txt;
    d = drsp_alloc((ro->n-1)*sizeof txt, ro->lengths, ro->n * sizeof txt, _Alignof txt);
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
    size_t txt_len;
    int paramc, outx, outy;
};

static void
cleanup_row_pair(SheetRow* ro, SheetRow* disp){
    drsp_alloc(ro->n*sizeof *ro->data, ro->data, 0, _Alignof *ro->data);
    drsp_alloc(ro->n*sizeof *ro->lengths, ro->lengths, 0, _Alignof *ro->lengths);
    for(int i = 0; i < disp->n; i++){
        drsp_alloc(disp->lengths[i]+1, disp->data[i], 0, 1);
    }
    drsp_alloc(disp->n*sizeof *disp->data, disp->data, 0, _Alignof *disp->data);
    drsp_alloc(disp->n*sizeof *disp->lengths, disp->lengths, 0, _Alignof *disp->lengths);
}

static
void
cleanup_sheet(SpreadSheet* sheet){
    if(sheet->txt) drsp_alloc(sheet->txt_len+1, sheet->txt, 0, 1);
    for(intptr_t i = 0; i < sheet->rows; i++){
        cleanup_row_pair(&sheet->cells[i], &sheet->display[i]);
    }
    drsp_alloc(sheet->rows * sizeof *sheet->cells, sheet->cells, 0, _Alignof *sheet->cells);
    drsp_alloc(sheet->rows * sizeof *sheet->display, sheet->display, 0, _Alignof *sheet->display);
    drsp_alloc(sheet->colnames.n * sizeof *sheet->colnames.data, sheet->colnames.data, 0, _Alignof * sheet->colnames.data);
    drsp_alloc(sheet->colnames.n * sizeof *sheet->colnames.lengths, sheet->colnames.lengths, 0, _Alignof *sheet->colnames.lengths);
}

static
void
sheet_push_row(SpreadSheet* sheet, SheetRow ro, SheetRow disp){
    ++sheet->rows;

    sheet->cells = drsp_alloc((sheet->rows-1)*sizeof *sheet->cells, sheet->cells, sheet->rows*sizeof *sheet->cells, _Alignof *sheet->cells);
    assert(sheet->cells);
    sheet->cells[sheet->rows-1] = ro;

    sheet->display = drsp_alloc((sheet->rows-1)*sizeof *sheet->display, sheet->display, sizeof(*sheet->display)*sheet->rows, _Alignof *sheet->display);
    assert(sheet->display);
    sheet->display[sheet->rows-1] = disp;
}

static
void
sheet_pop_row(SpreadSheet* sheet){
    if(sheet->rows <= 0) return;
    int end = --sheet->rows;
    if(end < 0) __builtin_unreachable(); // GCC spews a weird warning about malloc size otherwise
    cleanup_row_pair(&sheet->cells[end], &sheet->display[end]);
    sheet->cells = drsp_alloc((end+1)*sizeof *sheet->cells, sheet->cells, end*sizeof *sheet->cells, _Alignof *sheet->cells);
    sheet->display = drsp_alloc((end+1)*sizeof *sheet->display, sheet->display, end*sizeof *sheet->display, _Alignof *sheet->display);

}

typedef struct MultiSpreadSheet MultiSpreadSheet;
struct MultiSpreadSheet {
    char* txt;
    size_t txt_len;
    int n;
    SpreadSheet* sheets;
};

static
void
cleanup_multisheet(MultiSpreadSheet* ms){
    for(int i = 0; i < ms->n; i++)
        cleanup_sheet(&ms->sheets[i]);
    drsp_alloc(ms->n * sizeof *ms->sheets, ms->sheets, 0, _Alignof *ms->sheets);
    if(ms->txt)
        drsp_alloc(ms->txt_len+1, ms->txt, 0, 1);
}

static
SpreadSheet*_Nullable
multisheet_alloc(MultiSpreadSheet* ms){
    SpreadSheet* newsheets = drsp_alloc(ms->n * sizeof *newsheets, ms->sheets, (ms->n+1)*sizeof *newsheets, _Alignof *newsheets);
    if(!newsheets) return NULL;
    assert(newsheets);
    ms->sheets = newsheets;
    SpreadSheet* result = &newsheets[ms->n++];
    memset(result, 0, sizeof *result);
    result->outx = -1;
    result->outy = -1;
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
    // free((void*)ro->data[col]);
    drsp_alloc(strlen(ro->data[col])+1, ro->data[col], 0, _Alignof *ro->data[col]);
    if(val != val){ // nan
        ro->data[col] = drsp_strdup("nan");
        printed = 3;
    }
    else if(__builtin_lround(val) == val)
        printed = drsp_asprintf((char**)&ro->data[col], "%zd", (intptr_t)val);
    else
        printed = drsp_asprintf((char**)&ro->data[col], "%-.1f", val);
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
    drsp_alloc(strlen(ro->data[col])+1, ro->data[col], 0, _Alignof *ro->data[col]);
    // free((void*)ro->data[col]);
    ro->data[col] = drsp_strdup("error");
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
    drsp_alloc(strlen(ro->data[col])+1, ro->data[col], 0, _Alignof *ro->data[col]);
    int printed = drsp_asprintf((char**)&ro->data[col], "%.*s", (int)len, mess);
    if(printed < 0) return 1;
    ro->lengths[col] = printed;
    return 0;
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
    txt = drsp_alloc(0, NULL, len+1, _Alignof *txt);
    if(!txt) goto fail;
    n = fread(txt, 1, len, fp);
    if(n != (size_t)len) goto fail;

    txt[len] = 0;
    fclose(fp);
    return txt;

    fail:
    if(txt) drsp_alloc(len+1, txt, 0, _Alignof *txt);
    fclose(fp);
    return NULL;
}
#endif

static
int
read_csv_from_string(SpreadSheet* sheet, const char* srctxt){
    char* txt = drsp_strdup(srctxt);
    sheet->txt = txt;
    sheet->txt_len = strlen(txt);
    sheet->name = SV("");
    char* line;
    int max_cols = 0;
    for(;(line=strsep(&txt, "\n"));){
        SheetRow ro = {0};
        SheetRow disp = {0};
        char* token;
        while((token = strsep(&line, "\t|"))){
            sheet_row_push(&ro, token);
            sheet_row_push(&disp, drsp_strdup(""));
        }
        if(ro.n > max_cols) max_cols = ro.n;
        sheet_push_row(sheet, ro, disp);
        sheet->maxcols = max_cols;
    }
    if(sheet->rows && sheet->cells[sheet->rows-1].n == 1 && strlen(sheet->cells[sheet->rows-1].data[0])==0){
        sheet_pop_row(sheet);
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
    drsp_alloc(strlen(txt)+1, txt, 0, 1);
    return result;
}
#endif

static
int
read_multi_csv_from_string(MultiSpreadSheet* ms, const char* srctxt){
    char* txt = drsp_strdup(srctxt);
    ms->txt = txt;
    ms->txt_len = strlen(txt);
    char* line;
    int max_cols = 0;
    _Bool need_colnames = 0;
    SpreadSheet* sheet = NULL;
    for(;(line=strsep(&txt, "\n"));){
        size_t len = strlen(line);
        if(len == 3 && memcmp(line, "---", 3) == 0){
            while(sheet && sheet->rows && sheet->cells[sheet->rows-1].n == 1 && strlen(sheet->cells[sheet->rows-1].data[0])==0)
                sheet_pop_row(sheet);
            sheet = NULL;
            continue;
        }
        if(!sheet){
            if(!len) continue;
            max_cols = 0;
            sheet = multisheet_alloc(ms);
            if(!sheet) return 1;
        }
        if(len > 5 && memcmp(line, "func ", 5) == 0){
            line += 5;
            int paramc = 0;
            int params[3] = {0};
            char* tok;
            while((tok = strsep(&line, " "))){
                if(paramc >= 3)
                    break;
                params[paramc++] = parse_int(tok, strlen(tok)).result;
            }
            sheet->paramc = params[0];
            sheet->outy = params[1];
            sheet->outx = params[2];
            continue;
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
            sheet_row_push(&disp, drsp_strdup(""));
        }
        if(ro.n > max_cols) max_cols = ro.n;
        sheet_push_row(sheet, ro, disp);
        sheet->maxcols = max_cols;
    }
    while(sheet && sheet->rows && sheet->cells[sheet->rows-1].n == 1 && strlen(sheet->cells[sheet->rows-1].data[0])==0){
        sheet_pop_row(sheet);
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
    drsp_alloc(strlen(txt)+1, txt, 0, 1);
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
    };
    return ops;
}
#endif

#ifdef __clang__
#pragma clang assume_nonnull end
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#endif
