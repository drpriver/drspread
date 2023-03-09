//
// Copyright © 2021-2023, David Priver
//
#ifndef ALLSTD_H
#define ALLSTD_H
// Just put everything in one header and have
// the other headers just include this one.
typedef struct FILE FILE;
#define stdout (FILE*)0x1
#define stdin (FILE*)0x2
#define stderr (FILE*)0x3
typedef __builtin_va_list va_list;
#define va_start(ap, param) __builtin_va_start(ap, param)
#define va_end(ap)          __builtin_va_end(ap)
#define va_arg(ap, type)    __builtin_va_arg(ap, type)
#define va_copy(ap1, ap2)   __builtin_va_copy(ap1, ap2)
typedef typeof(sizeof(1)) size_t;
enum:size_t {SIZE_T_SIZE=sizeof(size_t)};
#include "malloc.h"
_Static_assert(sizeof(size_t) == 4, "");
typedef long ssize_t;
_Static_assert(sizeof(size_t) == sizeof(ssize_t), "");
typedef short int16_t;
typedef unsigned short uint16_t;
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;
typedef long ptrdiff_t;
typedef unsigned long uintptr_t;
typedef long intptr_t;
#define UINTPTR_MAX 0xFFFFFFFF
_Static_assert(UINTPTR_MAX == (uintptr_t)-1, "");
#define INT32_MAX 2147483647
#define UINT16_MAX 65535u
#define UINT32_MAX 4294967295u
_Static_assert(UINT32_MAX == (uint32_t)-1, "");
#define UINT64_MAX 18446744073709551615llu
_Static_assert(UINT64_MAX == (uint64_t)-1, "");

#define INT32_MIN  (-INT32_MAX-1)
_Static_assert(sizeof(int32_t) == sizeof(int), "");
#define INT_MAX INT32_MAX
#define INT_MIN INT32_MIN
#define INT64_MAX 9223372036854775807ll
#define INT64_MIN (-INT64_MAX-1)
_Static_assert(sizeof(uint8_t)==1, "");
_Static_assert(sizeof(int8_t)==1, "");
_Static_assert(sizeof(uint16_t)==2, "");
_Static_assert(sizeof(int16_t)==2, "");
_Static_assert(sizeof(uint32_t)==4, "");
_Static_assert(sizeof(int32_t)==4, "");
_Static_assert(sizeof(int64_t)==8, "");
_Static_assert(sizeof(uint64_t)==8, "");

#define bool _Bool
#define true 1
#define false 0

#define assert(x) (__builtin_expect(!(x), 0), !(x)?abort():(void)0)

#define NULL ((void*)0)

static inline
void* _Nullable
memchr(const void*_Nonnull pointer, int c, size_t nbytes){
    unsigned char*p  = (unsigned char*)pointer;
    while(nbytes--){
        if(*p == c)
            return p;
        p++;
    }
    return NULL;
}

static inline
size_t strlen(const char* p){
    for(size_t i = 0; ; i++){
        if(p[i] == 0)
            return i;
    }
}

static
void*_Nullable
strchr(const char*_Nonnull pointer, int c){
    while(*pointer!= 0){
        if(*pointer == c)
            return pointer;
        pointer++;
    }
    return NULL;
}

static
void*_Null_unspecified
memcpy(void*_Nonnull dst, const void*_Nonnull src, size_t nbytes){
    return __builtin_memcpy(dst, src, nbytes);
    char* d = dst;
    const char* s = src;
    while(nbytes--)
        *d++ = *s++;
    return dst;
}

static inline
void
bzero(void*_Nonnull s, size_t nbytes){
    __builtin_memset(s, 0, nbytes);
    return;
    uint64_t* dl = s;
    while((nbytes-=8) > 7)
        *dl++ = 0;
    char* d = (char*)dl;
    while(nbytes--)
        *d++ = 0;
}

static inline
void*
memset(void*_Nonnull s, int c, size_t n){
    return __builtin_memset(s, c, n);
    unsigned char* str = s;
    unsigned char ch = c;
    while(n--)
        *str++ = ch;
    return s;
}


static inline
int 
vfprintf(FILE* fp, const char*_Nonnull fmt, va_list vargs);

static inline
int printf(const char*_Nonnull fmt, ...){
    __builtin_va_list vap;
    __builtin_va_start(vap, fmt);
    int ret = vfprintf(stdout, fmt, vap);
    __builtin_va_end(vap);
    return ret;
}
static inline
int puts(const char* str){
    (void)str;
    return 0;
}

static inline
int fputs(const char* str, FILE*stream){
    (void)str;
    (void)stream;
    return 0;
}

static inline
int
fprintf(FILE*_Nonnull fp, const char*_Nonnull fmt, ...){
    __builtin_va_list vap;
    __builtin_va_start(vap, fmt);
    int ret = vfprintf(stdout, fmt, vap);
    __builtin_va_end(vap);
    return ret;
}
static inline
size_t fread(void* ptr, size_t size, size_t nitems, FILE* stream){
    (void)ptr, (void)size, (void)nitems, (void)stream;
    return 0;
}

static inline
void
perror(const char* s){
    (void)s;
}

extern
void __attribute__((noreturn))
abort(void);

static inline
int
memcmp(const void* s1, const void* s2, size_t n){
    const unsigned char* pa = s1;
    const unsigned char* pb = s2;
    while(n--){
        unsigned char a = *pa++;
        unsigned char b = *pb++;
        int diff = a - b;
        if(diff)
            return diff;
    }
    return 0;
}

static inline
int
strcmp(const char* s1, const char* s2){
    const unsigned char* pa = s1;
    const unsigned char* pb = s2;
    for(;;){
        unsigned char a = *pa++;
        unsigned char b = *pb++;
        int diff = a - b;
        if(diff)
            return diff;
        if(!a) // both are zero by the above comparison.
            return 0;
    }
    return 0;
}

static inline
void*
memmove(void* dst, const void* src, size_t len){
    return __builtin_memmove(dst,src, len);
    if(src == dst)
        return dst;
    if(src < dst){
        char* d_end = dst + len;
        const char* s_end = src+len;
        while(len--)
            *--d_end = *--s_end;
        return dst;
    }
    else
        return memcpy(dst, src, len);
}

static inline
int abs(int i){
    return i < 0 ? -i : i;
}


#endif
