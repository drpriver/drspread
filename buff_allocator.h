//
// Copyright © 2023-2024, David Priver <david@davidpriver.com>
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

#ifndef force_inline
#if defined(__GNUC__) || defined(__clang__)
#define force_inline static inline __attribute__((always_inline))
#else
#define force_inline static inline
#endif
#endif

typedef struct BuffAllocator BuffAllocator;
struct BuffAllocator {
    char* const data;
    char* cursor;
    char* const end;
};

typedef struct BuffCheckpoint BuffCheckpoint;
struct BuffCheckpoint {
    char* const ptr;
};

static inline
BuffCheckpoint
buff_checkpoint(const BuffAllocator* b){
    return (BuffCheckpoint){b->cursor};
}

static inline
void
buff_set(BuffAllocator* b, BuffCheckpoint c){
    b->cursor = c.ptr;
}


force_inline
// static inline
#ifdef __GNUC__
__attribute__((__malloc__))
#endif
void*_Nullable
buff_alloc(BuffAllocator* a, size_t sz){
    if(a->cursor + sz > a->end)
        return NULL;
    char* result = a->cursor;
    a->cursor += sz;
    return result;
}



#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#endif
