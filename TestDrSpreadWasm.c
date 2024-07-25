#ifndef TEST_DRSPREAD_WASM_C
#define TEST_DRSPREAD_WASM_C
#include <allstd.h>
#include "Wasm/malloc.h"
#include <stb_sprintf.c>
#ifndef IMPORT
#define IMPORT extern
#endif

IMPORT
long
fwrite_(const void* restrict buff, size_t sz, FILE* restrict fp);

static inline
int
snprintf(char* buff, size_t bufflen, const char* fmt, ...){
    __builtin_va_list vap;
    __builtin_va_start(vap, fmt);
    return stbsp_vsnprintf(buff, bufflen, fmt, vap);
    __builtin_va_end(vap);
}

static inline
int
vfprintf(FILE* fp, const char*_Nonnull fmt, va_list vargs){
    char buff[1024];
    int ret = stbsp_vsnprintf(buff, sizeof buff, fmt, vargs);
    fwrite_(buff, ret, fp);
    return ret;
}
static inline
int vprintf(const char*_Nonnull fmt, va_list vargs){
    return vfprintf(stdout, fmt, vargs);
}
static inline
int vsnprintf(char* buff, size_t bufflen, const char*_Nonnull fmt, va_list vargs){
    return stbsp_vsnprintf(buff, bufflen, fmt, vargs);
}
static inline
long fwrite(const void* restrict buff, size_t sz, size_t n, FILE* restrict fp){
    return fwrite_(buff, sz*n, fp)/sz;
}
static inline
int fputc(int c, FILE* fp){
    fwrite_(&c, 1, fp);
    return 0;
}
static inline
int putchar(int c){
    return fputc(c, stdout);
}
static inline
FILE* fopen(const char* fn, const char* mode){
    (void)fn;
    (void)mode;
    return NULL;
}
int errno = 0;
static inline
const char*
strerror(int err){
    (void)err;
    return "Wasm unimplemented probably";
}
static inline
void fclose(FILE* fp){
    (void)fp;
}
static inline
int getchar(void){
    return -1;
}
static inline
const char* strrchr(const char* haystack, char needle){
    const char* p = NULL;
    const char* p2 = NULL;
    while((p = strchr(haystack, needle)))
        haystack = p+1, p2 = p;
    return p2;
}

extern
void
free_argv(int argc, char** argv){
    for(int i = 0; i < argc; i++){
        free(argv[i]);
    }
    free(argv);
}
// #define SUPPRESS_TEST_MAIN 1
#include "testing.h"

// #define __FILE__ ""
#include "TestDrSpread.c"

static inline void logit(const char* fmt, ...){
    __builtin_va_list vap;
    __builtin_va_start(vap, fmt);
    vprintf(fmt, vap);
    __builtin_va_end(vap);
}

#endif
