#include <allstd.h>
#include "Wasm/jsinter.h"
#include <stb_sprintf.c>
static inline
void* realloc(void* a, size_t sz){
    void* result = malloc(sz);
    memcpy(result, a, sz); // whatever, no mmu in wasm and it's just a read
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
#if 0
int
test_main(int argc, char** argv, void* unused){
    (void)argc;
    (void)argv;
    (void)unused;
    if(!TestOutFileCount) TestRegisterOutFile(stdout);
    const char* gray   = "<span class=gray>";
    const char* blue   = "<span class=blue>";
    const char* green  = "<span class=green>";
    const char* red    = "<span class=red>";
    const char* reset  = "</span>";
    const char* bold   = "<b>";
    const char* nobold = "</b>";
    _test_color_gray = gray;
    _test_color_reset = reset;
    _test_color_green = green;
    _test_color_red = red;

    size_t tests_to_run[arrlen(test_funcs)] = {0};
    for(size_t i = 0; i < test_funcs_count; i++)
        tests_to_run[i] = i;
    struct TestResults result = {0};
    run_the_tests(tests_to_run, test_funcs_count, &result);

    const char* text = result.funcs_executed == 1?
        "test function executed"
        : "test functions executed";
    TestPrintf("%s%s%s: %s%zu%s %s\n",
            gray, "TestDrSpread.c", reset,
            blue, result.funcs_executed, reset,
            text);

    text = result.executed == 1? "test executed" : "tests executed";
    TestPrintf("%s%s%s: %s%zu%s %s\n",
            gray, "TestDrSpread.c", reset,
            blue, result.executed, reset,
            text);

    text = result.assert_failures == 1?
        "test function aborted early"
        : "test functions aborted early";
    const char* color = result.assert_failures?red:green;
    TestPrintf("%s%s%s: %s%zu%s %s\n",
            gray, "TestDrSpread.c", reset,
            color, result.assert_failures, reset,
            text);

    color = result.failures?red:green;
    text = result.failures == 1? "test failed" : "tests failed";
    TestPrintf("%s%s%s: %s%zu%s %s\n",
            gray, "TestDrSpread.c", reset,
            color, result.failures, reset,
            text);
    for(size_t i = 0; i < result.n_failed_tests; i++){
        StringView name = test_funcs[result.failed_tests[i]].test_name;
        TestPrintf("%s%.*s%s %sfailed%s.\n",
                bold, (int)name.length, name.text, nobold,
                red, reset);
    }

    return 0;
}
#endif

// #define __FILE__ ""
#include "TestDrSpread.c"

static inline void logit(const char* fmt, ...){
    __builtin_va_list vap;
    __builtin_va_start(vap, fmt);
    vprintf(fmt, vap);
    __builtin_va_end(vap);
}
