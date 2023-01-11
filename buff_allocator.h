//
// Copyright © 2023, David Priver
//
#ifndef BUFF_ALLOCATOR_H
#define BUFF_ALLOCATOR_H
#include <stddef.h>
#ifdef __clang__
#pragma clang assume_nonnull begin
#else
#ifndef _Nullable
#define _Nullable
#endif
#endif

typedef struct BuffAllocator BuffAllocator;
struct BuffAllocator {
    char* const data;
    char* cursor;
    char* const end;
};


static inline
__attribute__((__malloc__))
void*_Nullable
buff_alloc(BuffAllocator* a, size_t sz){
    if(a->cursor + sz > a->end) return NULL;
    char* result = a->cursor;
    a->cursor += sz;
    return result;
}



#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#endif
