//
// Copyright © 2023-2025, David Priver <david@davidpriver.com>
//
#ifndef DRSP_ALLOCATORS_C
#define DRSP_ALLOCATORS_C
#include <stdlib.h>
#include "drspread_allocators.h"
#undef free
#undef realloc
#undef malloc

#ifdef RECORD_ALLOCATIONS
#include "debugging.h"
#include <stdio.h>
static inline void logbt(const char*_Nonnull fmt, ...);
static inline void logit(const char*_Nonnull fmt, ...);

static inline void logbt(const char*_Nonnull fmt, ...){
    bt();
    __builtin_va_list vap;
    __builtin_va_start(vap, fmt);
    vfprintf(stderr, fmt, vap);
    __builtin_va_end(vap);
    fputc('\n', stderr);
}
#ifndef __wasm__
static inline void logit(const char*_Nonnull fmt, ...){
    __builtin_va_list vap;
    __builtin_va_start(vap, fmt);
    vfprintf(stderr, fmt, vap);
    __builtin_va_end(vap);
    fputc('\n', stderr);
}
#endif
typedef struct AllocationRecord AllocationRecord;
struct AllocationRecord {
    const void*_Nullable ptr;
    size_t sz;
    size_t align;
    BacktraceArray*_Nullable bt;
};

enum {N_RECORDS=10000};
#if !defined(drsp_thread_local)
#if defined(__wasm__)
#define drsp_thread_local
#else
#define drsp_thread_local _Thread_local
#endif
#endif
drsp_thread_local static AllocationRecord records[N_RECORDS];
drsp_thread_local static size_t n_records = 0;
#endif

static inline
int
drsp_report_leaks(void){
    int leaks = 0;
#ifdef RECORD_ALLOCATIONS
    for(size_t i = 0; i < n_records; i++){
        AllocationRecord* r = &records[i];
        logit("\nleak: %p %zubytes", r->ptr, r->sz);
        dump_bt(r->bt);
        free_bt(r->bt);
        leaks++;
    }
    n_records = 0;
#endif
    return leaks;
}

static inline
void
drsp_record_allocation(size_t old_sz, const void*_Nullable old, size_t new_sz, size_t alignment, void*_Nullable new_pointer){
    #ifdef RECORD_ALLOCATIONS
    // free
    if(!new_sz){
        if(!old_sz && !old) return;
        for(size_t i = 0; i < n_records; i++){
            AllocationRecord* r = &records[i];
            if(r->ptr == old){
                if(r->sz != old_sz){
                    logbt("bad free: size mismatch, original: %zu, freed: %zu", r->sz, old_sz);
                }
                if(r->align != alignment){
                    logbt("bad free: align mismatch, original: %zu, freed: %zu", r->align, alignment);
                }
                free_bt(r->bt);
                if(i != --n_records)
                    *r = records[n_records];
                return;
            }
        }
        logbt("bad free: pointer not allocated %p %zubytes %zualign", old, old_sz, alignment);
        return;
    }
    // malloc
    if(!old_sz){
        if(old){
            logbt("mallocing nonnull, zero-sized original pointer");
        }
        if(!new_sz){
            logbt("mallocing zero-sized pointer");
            return;
        }
        if(n_records == N_RECORDS){
            abort();
        }
        AllocationRecord* r = &records[n_records++];
        r->ptr = new_pointer;
        r->sz = new_sz;
        r->align = alignment;
        r->bt = get_bt();
        return;
    }
    // realloc
    if(old == new_pointer){
        for(size_t i = 0; i < n_records; i++){
            AllocationRecord* r = &records[i];
            if(r->ptr == old){
                if(r->sz != old_sz){
                    logbt("bad realloc: size mismatch, original: %zu, freed: %zu", r->sz, old_sz);
                }
                if(r->align != alignment){
                    logbt("bad realloc: align mismatch, original: %zu, freed: %zu", r->align, alignment);
                }
                free_bt(r->bt);
                r->bt = get_bt();
                r->sz = new_sz;
                r->align = alignment;
                return;
            }
        }
        logbt("bad realloc: pointer not allocated %p %zubytes %zualign", old, old_sz, alignment);
        AllocationRecord* r = &records[n_records++];
        r->bt = get_bt();
        r->ptr = new_pointer;
        r->sz = new_sz;
        r->align = alignment;
        return;
    }
    for(size_t i = 0; i < n_records; i++){
        AllocationRecord* r = &records[i];
        if(r->ptr == old){
            if(r->sz != old_sz){
                logbt("bad realloc: size mismatch, original: %zu, freed: %zu", r->sz, old_sz);
            }
            if(r->align != alignment){
                logbt("bad realloc: align mismatch, original: %zu, freed: %zu", r->align, alignment);
            }
            free_bt(r->bt);
            r->bt = get_bt();
            r->ptr = new_pointer;
            r->sz = new_sz;
            r->align = alignment;
            return;
        }
    }
    logbt("bad realloc: pointer not allocated %p %zubytes %zualign", old, old_sz, alignment);
    AllocationRecord* r = &records[n_records++];
    r->bt = get_bt();
    r->ptr = new_pointer;
    r->sz = new_sz;
    r->align = alignment;
    return;
    #else
    (void)old_sz, (void)old, (void)new_sz, (void)alignment, (void)new_pointer;
    #endif
}


static inline
void*_Nullable
drsp_alloc(size_t old_sz, const void*_Nullable old, size_t new_sz, size_t alignment){
    void* result = NULL;

    if(!new_sz){
        free((void*)old);
        result = NULL;
        goto finish;
    }
    if(!old_sz){
        if(!new_sz) return NULL;
        result = malloc(new_sz);
        goto finish;
    }
    #ifdef __wasm__
    result = sane_realloc((void*)old, old_sz, new_sz);
    #else
    result = realloc((void*)old, new_sz);
    #endif
    finish:
    drsp_record_allocation(old_sz, old, new_sz, alignment, result);
    return result;
}

#endif
