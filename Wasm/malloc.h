#ifndef MALLOC_H
#define MALLOC_H
extern
void __attribute__((noreturn))
abort(void);

typedef double max_align_t;

typedef typeof(sizeof(1)) size_t;
extern unsigned char __heap_base[];
extern unsigned char*_Nonnull _base_ptr = __heap_base;
extern unsigned char*_Nonnull _heap_end = __heap_base + 1024*1024;

typedef struct FreeAllocation FreeAllocation;
typedef struct MallocBlock MallocBlock;
struct MallocBlock {
    MallocBlock*_Nullable prev;
    MallocBlock*_Nullable next;
    FreeAllocation* free_list;
    size_t used;
    unsigned char memory[];
};

static MallocBlock* _current_block = 0;

typedef struct Allocation Allocation;
struct Allocation {
    size_t size;
    size_t pad;
    _Alignas(max_align_t) unsigned char memory[];
};
_Static_assert(__builtin_offsetof(Allocation, memory) == 8, "");
struct FreeAllocation {
    size_t sz;
    size_t pad;
    FreeAllocation*_Nullable next;
    FreeAllocation*_Nullable prev;
    unsigned char memory[];
};

_Static_assert(__builtin_offsetof(FreeAllocation, memory) == 16, "");


static FreeAllocation*_Nullable free_allocations[31] = {0};

static inline
size_t
next_p2(size_t u){
    size_t r = 1u << (32-__builtin_clz(u-1));
    return r;
}

static inline
size_t
malloc_true_size(size_t size){
    // logit("true_size: %u\n", size);
    size_t orig = size;
    enum: size_t {MAX_ALIGN = sizeof(max_align_t), MAX_ALIGN_SUB_1 = MAX_ALIGN-1};
    if(size & MAX_ALIGN_SUB_1){
        size += MAX_ALIGN - (size & MAX_ALIGN_SUB_1);
    }
    size += 8;
    size = next_p2(size);
    if(size < orig) abort();
    // logit("true_size: %#x -> %#x\n", orig, size);
    return size;
}

static inline
size_t
malloc_bucket(size_t size){
    return 32 - 5 - __builtin_clz(size);
}

static inline
void*_Nonnull
raw_alloc(size_t size){
    // logit("heap_base %p\n", __heap_base);
    // logit("heap_end %p\n", _heap_end);
    // logit("base_ptr %p\n", _base_ptr);
    // if(_heap_end < __heap_base || _base_ptr < __heap_base)
        // abort();
    if(_base_ptr + size >= _heap_end){
        // logit("raw_alloc: _base_ptr: %#p\n", _base_ptr);
        // logit("raw_alloc: size: %zu\n", size);
        // logit("raw_alloc: _heap_end: %#p\n", _heap_end);
        // logit("raw_alloc: remaining: %#zx\n", _heap_end - _base_ptr);
        // logit("raw_alloc: total space: %#zx\n", _heap_end - __heap_base);
        return 0;
    }
    void* result = _base_ptr;
    _base_ptr += size;
    // logit("raw alloc: %#x, %#x\n", result, size);
    // logit("_base_ptr is %zd bytes ahead\n", _base_ptr - __heap_base);
    return result;
}

extern
void
reset_memory(void){
    _base_ptr = __heap_base;
    __builtin_memset(free_allocations, 0, sizeof free_allocations);
}

static
unsigned char zed[64];

extern
void*_Nonnull
malloc(size_t size){
    if(!size) return zed;
    size = malloc_true_size(size);
    // logit("malloc: %#x\n", size);
    size_t bucket = malloc_bucket(size);
    Allocation* a;
    if(free_allocations[bucket]){
        FreeAllocation* f = free_allocations[bucket];
        free_allocations[bucket] = f->next;
        if(f->next) f->next->prev = 0;
        a = (Allocation*)f;
        // logit("malloc: reusing allocation\n");
    }
    else {
        // logit("malloc: raw alloc\n");
        a = raw_alloc(size);
        if(!a) {
            // logit("malloc: raw_alloc failed for %zu\n", size);
            return 0;
        }
    }
    a->size = size;
    // logit("malloc: %#x, %#x\n", a, size);
    // logit("malloc: %#x, %#x\n", a->memory, size);
    return a->memory;
}

extern
void*
calloc(size_t n_items, size_t item_size){
    size_t sz = n_items * item_size;
    void* result = malloc(sz);
    if(result) __builtin_memset(result, 0, sz);
    return result;
}

static
void
free(void* p){
    if(p == zed+8) return;
    if(!p) return;
    unsigned char* mem = p;
    Allocation* a = (Allocation*)(mem - __builtin_offsetof(Allocation, memory));
    // logsz(a->size);
    // logit("free size: %zu\n", a->size);
    // logit("free: %#x, %#x\n", p, a->size);
    size_t bucket = malloc_bucket(a->size);
    FreeAllocation* f = (FreeAllocation*)a;
    f->next = free_allocations[bucket];
    if(free_allocations[bucket]){
        f->prev = free_allocations[bucket];
        free_allocations[bucket]->prev = f;
    }
    else
        f->prev = 0;
    free_allocations[bucket] = f;
}

static inline
void*
sane_realloc(void* p, size_t orig_size, size_t size){
    if(!size){
        free(p);
        return 0;
    }
    if(!p){
        if(orig_size) abort();
        return malloc(size);
    }
    if(size < orig_size) return p; // I don't think I ever actually do this.
    size_t truth = malloc_true_size(orig_size) - sizeof(max_align_t);
    if(truth >= size){
        return p;
    }
    void* result = malloc(size);
    if(!result) return 0;
    if(orig_size){
        __builtin_memcpy(result, p, orig_size);
        free(p);
    }
    return result;
}


extern
size_t
bytes_used(void){
    return (size_t)(_base_ptr - __heap_base);
}


#endif
