#pragma once

#include <cstdint>
#include <cstring>

// CPU feature detection
namespace chess {
namespace simd {

// CPU features
struct CpuFeatures {
    bool sse;
    bool sse2;
    bool sse3;
    bool ssse3;
    bool sse41;
    bool sse42;
    bool avx;
    bool avx2;
    bool avx512;
    bool bmi1;
    bool bmi2;
    bool popcnt;
};

// Detect CPU features at runtime
CpuFeatures detect_cpu_features();

// Get CPU features (cached)
const CpuFeatures& get_cpu_features();

} // namespace simd
} // namespace chess

// SIMD-optimized operations
#if defined(__AVX2__)
    #define USE_AVX2
    #include <immintrin.h>
#elif defined(__SSE4_1__)
    #define USE_SSE41
    #include <smmintrin.h>
#elif defined(__SSSE3__)
    #define USE_SSSE3
    #include <tmmintrin.h>
#elif defined(__SSE2__)
    #define USE_SSE2
    #include <emmintrin.h>
#endif

namespace chess {
namespace simd {

// SIMD-optimized vector addition for NNUE
// Adds two int16_t arrays of size n
void add_vectors_i16(int16_t* dest, const int16_t* src, size_t n);

// SIMD-optimized vector subtraction for NNUE
void sub_vectors_i16(int16_t* dest, const int16_t* src, size_t n);

// SIMD-optimized dot product for NNUE
// Computes dot product of two int16_t arrays and accumulates to int32_t
int32_t dot_product_i16(const int16_t* a, const int16_t* b, size_t n);

// SIMD-optimized ReLU (Rectified Linear Unit)
// Clamps negative values to 0
void relu_i16(int16_t* data, size_t n);

// SIMD-optimized clipped ReLU
// Clamps values to [0, max_val]
void clipped_relu_i16(int16_t* data, size_t n, int16_t max_val);

// SIMD-optimized matrix-vector multiplication for NNUE
// result = weights * input (with bias)
void matmul_i16(int32_t* result, const int16_t* weights, const int16_t* input,
                const int32_t* bias, size_t output_size, size_t input_size);

// Bitboard operations with SIMD
#if defined(USE_AVX2) || defined(USE_SSE2)

// Population count (number of set bits) using hardware instruction
inline int popcount(uint64_t bb) {
#if defined(_MSC_VER)
    return static_cast<int>(__popcnt64(bb));
#else
    return __builtin_popcountll(bb);
#endif
}

// Count trailing zeros
inline int ctz(uint64_t bb) {
#if defined(_MSC_VER)
    unsigned long idx;
    _BitScanForward64(&idx, bb);
    return static_cast<int>(idx);
#else
    return __builtin_ctzll(bb);
#endif
}

// Count leading zeros
inline int clz(uint64_t bb) {
#if defined(_MSC_VER)
    unsigned long idx;
    _BitScanReverse64(&idx, bb);
    return 63 - static_cast<int>(idx);
#else
    return __builtin_clzll(bb);
#endif
}

#else

// Fallback implementations without SIMD
inline int popcount(uint64_t bb) {
    int count = 0;
    while (bb) {
        count++;
        bb &= bb - 1;
    }
    return count;
}

inline int ctz(uint64_t bb) {
    if (bb == 0) return 64;
    int count = 0;
    while ((bb & 1) == 0) {
        count++;
        bb >>= 1;
    }
    return count;
}

inline int clz(uint64_t bb) {
    if (bb == 0) return 64;
    int count = 0;
    while ((bb & (1ULL << 63)) == 0) {
        count++;
        bb <<= 1;
    }
    return count;
}

#endif

// Parallel bits extract (PEXT) - BMI2 instruction
#if defined(__BMI2__)
inline uint64_t pext(uint64_t src, uint64_t mask) {
#if defined(_MSC_VER)
    return _pext_u64(src, mask);
#else
    return __builtin_ia32_pext_di(src, mask);
#endif
}
#else
// Software fallback for PEXT
inline uint64_t pext(uint64_t src, uint64_t mask) {
    uint64_t result = 0;
    int k = 0;
    for (int i = 0; i < 64; i++) {
        if (mask & (1ULL << i)) {
            if (src & (1ULL << i)) {
                result |= (1ULL << k);
            }
            k++;
        }
    }
    return result;
}
#endif

} // namespace simd
} // namespace chess
