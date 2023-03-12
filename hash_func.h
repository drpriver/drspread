#ifndef HASH_FUNC_H
#define HASH_FUNC_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifndef force_inline
#if defined(__GNUC__) || defined(__clang__)
#define force_inline static inline __attribute__((always_inline))
#else
#define force_inline static inline
#endif
#endif

// dummy structs to allow unaligned loads.
// ubsan complains otherwise. This is sort of a grey
// area.
#if defined(_MSC_VER) && !defined(__clang__)
#pragma pack(push)
#pragma pack(1)
typedef struct packed_uint64 packed_uint64;
struct packed_uint64 {
    uint64_t v;
};

typedef struct packed_uint32 packed_uint32;
struct packed_uint32 {
    uint32_t v;
};

typedef struct packed_uint16 packed_uint16;
struct packed_uint16 {
    uint16_t v;
};
#pragma pack(pop)
#else
typedef struct packed_uint64 packed_uint64;
struct __attribute__((packed)) packed_uint64 {
    uint64_t v;
};

typedef struct packed_uint32 packed_uint32;
struct __attribute__((packed)) packed_uint32 {
    uint32_t v;
};

typedef struct packed_uint16 packed_uint16;
struct __attribute__((packed)) packed_uint16 {
    uint16_t v;
};
#endif



#if defined(__ARM_ACLE) && __ARM_FEATURE_CRC32
#include <arm_acle.h>

#ifdef __clang__
#pragma clang assume_nonnull begin
#endif

static inline
uint32_t
hash_align1(const void* key, size_t len){
    const unsigned char* k = key;
    uint32_t h = 0;
    for(;len >= 8; k+=8, len-=8)
        h = __crc32cd(h, (*(const packed_uint64*)k).v);
    for(;len >= 4; k+=4, len-=4)
        h = __crc32cw(h, (*(const packed_uint32*)k).v);
    for(;len >= 2; k+=2, len-=2)
        h = __crc32ch(h, (*(const packed_uint16*)k).v);
    for(;len >= 1; k+=1, len-=1)
        h = __crc32cb(h, *(const uint8_t*)k);
    return h;
}
static inline
uint32_t
hash_align2(const void* key, size_t len){
    const unsigned char* k = key;
    uint32_t h = 0;
    for(;len >= 8; k+=8, len-=8)
        h = __crc32cd(h, (*(const packed_uint64*)k).v);
    for(;len >= 4; k+=4, len-=4)
        h = __crc32cw(h, (*(const packed_uint32*)k).v);
    for(;len >= 2; k+=2, len-=2)
        h = __crc32ch(h, (*(const packed_uint16*)k).v);
    return h;
}
static inline
uint32_t
hash_align4(const void* key, size_t len){
    const unsigned char* k = key;
    uint32_t h = 0;
    for(;len >= 8; k+=8, len-=8)
        h = __crc32cd(h, (*(const packed_uint64*)k).v);
    for(;len >= 4; k+=4, len-=4)
        h = __crc32cw(h, (*(const packed_uint32*)k).v);
    return h;
}
static inline
uint32_t
hash_align8(const void* key, size_t len){
    const unsigned char* k = key;
    uint32_t h = 0;
    for(;len >= 8; k+=8, len-=8)
        h = __crc32cd(h, (*(const packed_uint64*)k).v);
    return h;
}

static inline
uint32_t
ascii_insensitive_hash_align1(const void* key, size_t len){
    const unsigned char* k = key;
    uint32_t h = 0;
    for(;len >= 8; k+=8, len-=8)
        h = __crc32cd(h, 0x2020202020202020u|(*(const packed_uint64*)k).v);
    for(;len >= 4; k+=4, len-=4)
        h = __crc32cw(h, 0x20202020u|(*(const packed_uint32*)k).v);
    for(;len >= 2; k+=2, len-=2)
        h = __crc32ch(h, 0x2020u|(*(const packed_uint16*)k).v);
    for(;len >= 1; k+=1, len-=1)
        h = __crc32cb(h, 0x20u|*(const uint8_t*)k);
    return h;
}

#elif defined(__x86_64__) && defined(__SSE4_2__)
#include <nmmintrin.h>

#ifdef __clang__
#pragma clang assume_nonnull begin
#endif

static inline
uint32_t
hash_align1(const void* key, size_t len){
    const unsigned char* k = key;
    uint32_t h = 0;
    for(;len >= 8; k+=8, len-=8)
        h = _mm_crc32_u64(h, (*(const packed_uint64*)k).v);
    for(;len >= 4; k+=4, len-=4)
        h = _mm_crc32_u32(h, (*(const packed_uint32*)k).v);
    for(;len >= 2; k+=2, len-=2)
        h = _mm_crc32_u16(h, (*(const packed_uint16*)k).v);
    for(;len >= 1; k+=1, len-=1)
        h = _mm_crc32_u8(h, *(const uint8_t*)k);
    return h;
}
static inline
uint32_t
hash_align2(const void* key, size_t len){
    const unsigned char* k = key;
    uint32_t h = 0;
    for(;len >= 8; k+=8, len-=8)
        h = _mm_crc32_u64(h, (*(const packed_uint64*)k).v);
    for(;len >= 4; k+=4, len-=4)
        h = _mm_crc32_u32(h, (*(const packed_uint32*)k).v);
    for(;len >= 2; k+=2, len-=2)
        h = _mm_crc32_u16(h, (*(const packed_uint16*)k).v);
    return h;
}
static inline
uint32_t
hash_align4(const void* key, size_t len){
    const unsigned char* k = key;
    uint32_t h = 0;
    for(;len >= 8; k+=8, len-=8)
        h = _mm_crc32_u64(h, (*(const packed_uint64*)k).v);
    for(;len >= 4; k+=4, len-=4)
        h = _mm_crc32_u32(h, (*(const packed_uint32*)k).v);
    return h;
}
static inline
uint32_t
hash_align8(const void* key, size_t len){
    const unsigned char* k = key;
    uint32_t h = 0;
    for(;len >= 8; k+=8, len-=8)
        h = _mm_crc32_u64(h, (*(const packed_uint64*)k).v);
    return h;
}

static inline
uint32_t
ascii_insensitive_hash_align1(const void* key, size_t len){
    const unsigned char* k = key;
    uint32_t h = 0;
    for(;len >= 8; k+=8, len-=8)
        h = _mm_crc32_u64(h, 0x2020202020202020u|(*(const packed_uint64*)k).v);
    for(;len >= 4; k+=4, len-=4)
        h = _mm_crc32_u32(h, 0x20202020u|(*(const packed_uint32*)k).v);
    for(;len >= 2; k+=2, len-=2)
        h = _mm_crc32_u16(h, 0x2020u|(*(const packed_uint16*)k).v);
    for(;len >= 1; k+=1, len-=1)
        h = _mm_crc32_u8(h, 0x20u|*(const uint8_t*)k);
    return h;
}
#else // fall back to murmur hash

#ifdef __clang__
#pragma clang assume_nonnull begin
#endif
// cut'n'paste from the wikipedia page on murmur hash
force_inline
uint32_t
murmur_32_scramble(uint32_t k) {
    k *= 0xcc9e2d51;
    k = (k << 15) | (k >> 17);
    k *= 0x1b873593;
    return k;
}
force_inline
uint32_t
hash_align1(const void* key_, size_t len){
    const uint8_t* key = key_;
    uint32_t seed = 4253307714;
	uint32_t h = seed;
    uint32_t k;
    /* Read in groups of 4. */
    for (size_t i = len >> 2; i; i--) {
        k = (*(const packed_uint32*)key).v;
        key += sizeof(uint32_t);
        h ^= murmur_32_scramble(k);
        h = (h << 13) | (h >> 19);
        h = h * 5 + 0xe6546b64;
    }
    /* Read the rest. */
    k = 0;
    for (size_t i = len & 3; i; i--) {
        k <<= 8;
        k |= key[i - 1];
    }
    h ^= murmur_32_scramble(k);
    /* Finalize. */
	h ^= len;
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;
	return h;
}
force_inline
uint32_t
hash_align2(const void* key, size_t len){
    return hash_align1(key, len);
}
force_inline
uint32_t
hash_align4(const void* key, size_t len){
    return hash_align1(key, len);
}
force_inline
uint32_t
hash_align8(const void* key, size_t len){
    return hash_align1(key, len);
}

force_inline
uint32_t
ascii_insensitive_hash_align1(const void* key_, size_t len){
    const uint8_t* key = key_;
    uint32_t seed = 4253307714;
	uint32_t h = seed;
    uint32_t k;
    /* Read in groups of 4. */
    for (size_t i = len >> 2; i; i--) {
        k = 0x20202020u|(*(const packed_uint32*)key).v;
        key += sizeof(uint32_t);
        h ^= murmur_32_scramble(k);
        h = (h << 13) | (h >> 19);
        h = h * 5 + 0xe6546b64;
    }
    /* Read the rest. */
    k = 0;
    for (size_t i = len & 3; i; i--) {
        k <<= 8;
        k |= 0x20u|key[i - 1];
    }
    h ^= murmur_32_scramble(k);
    /* Finalize. */
	h ^= len;
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;
	return h;
}

#endif

#define hash_alignany(key, len) \
      (_Alignof(*key)&7) == 0? hash_align8(key, len) \
    : (_Alignof(*key)&3) == 0? hash_align4(key, len) \
    : (_Alignof(*key)&1) == 0? hash_align2(key, len) \
    :                        hash_align1(key, len)

static inline
uint32_t
fast_reduce32(uint32_t x, uint32_t y){
    return ((uint64_t)x * (uint64_t)y) >> 32;
}

#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#endif
