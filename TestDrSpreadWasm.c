#include <allstd.h>
#include "Wasm/jsinter.h"
#include <stb_sprintf.c>
static inline
void* realloc(void* a, size_t sz){
    void* result = malloc(sz);
    memcpy(result, a, sz); // whatever, no mmu in wasm and it's just a read
    free(a);
    return result;
}

static inline
char* strdup(const char* p){
    size_t len = strlen(p);
    char* result = malloc(len+1);
    memcpy(result, p, len+1);
    return result;
}

static inline
char* strsep(char** stringp, const char* delim){
    char* result = *stringp;
    char* s = *stringp;
    if(s){
        for(;*s;s++){
            for(const char* d=delim;*d;d++){
                if(*s == *d){
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

static inline
int
asprintf(char** out, const char* fmt, ...){
    __builtin_va_list vap, vap2;
    __builtin_va_start(vap, fmt);
    __builtin_va_copy(vap2, vap);
    int len = stbsp_vsnprintf(NULL, 0, fmt, vap);
    char* buff = malloc(len+1);
    int ret = stbsp_vsnprintf(buff, len+1, fmt, vap2);
    *out = buff;
    __builtin_va_end(vap);
    __builtin_va_end(vap2);
    return ret;
}

IMPORT
long
fwrite_(const void* restrict buff, size_t sz, FILE* restrict fp);

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
    return 0;
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
static inline
int
chdir(const char* path){
    (void)path;
    return 0;
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
int getpid(void){
    return 0;
}
static inline
int getchar(void){
    return -1;
}
static inline
const char* strrchr(const char* haystack, char needle){
    // wrong but who cares
    (void)needle;
    return haystack-1;
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
