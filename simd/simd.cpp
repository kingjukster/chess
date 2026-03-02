#include "simd.h"
#include <algorithm>

#if defined(_MSC_VER)
    #include <intrin.h>
#elif defined(__GNUC__) || defined(__clang__)
    #include <cpuid.h>
#endif

namespace chess {
namespace simd {

CpuFeatures detect_cpu_features() {
    CpuFeatures features = {};
    
#if defined(_MSC_VER)
    int cpu_info[4];
    __cpuid(cpu_info, 0);
    int num_ids = cpu_info[0];
    
    if (num_ids >= 1) {
        __cpuidex(cpu_info, 1, 0);
        features.sse = (cpu_info[3] & (1 << 25)) != 0;
        features.sse2 = (cpu_info[3] & (1 << 26)) != 0;
        features.sse3 = (cpu_info[2] & (1 << 0)) != 0;
        features.ssse3 = (cpu_info[2] & (1 << 9)) != 0;
        features.sse41 = (cpu_info[2] & (1 << 19)) != 0;
        features.sse42 = (cpu_info[2] & (1 << 20)) != 0;
        features.avx = (cpu_info[2] & (1 << 28)) != 0;
        features.popcnt = (cpu_info[2] & (1 << 23)) != 0;
    }
    
    if (num_ids >= 7) {
        __cpuidex(cpu_info, 7, 0);
        features.avx2 = (cpu_info[1] & (1 << 5)) != 0;
        features.bmi1 = (cpu_info[1] & (1 << 3)) != 0;
        features.bmi2 = (cpu_info[1] & (1 << 8)) != 0;
        features.avx512 = (cpu_info[1] & (1 << 16)) != 0;
    }
    
#elif defined(__GNUC__) || defined(__clang__)
    unsigned int eax, ebx, ecx, edx;
    
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        features.sse = (edx & bit_SSE) != 0;
        features.sse2 = (edx & bit_SSE2) != 0;
        features.sse3 = (ecx & bit_SSE3) != 0;
        features.ssse3 = (ecx & bit_SSSE3) != 0;
        features.sse41 = (ecx & bit_SSE4_1) != 0;
        features.sse42 = (ecx & bit_SSE4_2) != 0;
        features.avx = (ecx & bit_AVX) != 0;
        features.popcnt = (ecx & bit_POPCNT) != 0;
    }
    
    if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx)) {
        features.avx2 = (ebx & bit_AVX2) != 0;
        features.bmi1 = (ebx & bit_BMI) != 0;
        features.bmi2 = (ebx & bit_BMI2) != 0;
        features.avx512 = (ebx & bit_AVX512F) != 0;
    }
#endif
    
    return features;
}

const CpuFeatures& get_cpu_features() {
    static CpuFeatures features = detect_cpu_features();
    return features;
}

//-----------------------------------------------------------------------------
// SIMD-optimized vector operations
//-----------------------------------------------------------------------------

#if defined(USE_AVX2)

void add_vectors_i16(int16_t* dest, const int16_t* src, size_t n) {
    size_t i = 0;
    
    // Process 16 elements at a time with AVX2
    for (; i + 16 <= n; i += 16) {
        __m256i d = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(dest + i));
        __m256i s = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(src + i));
        __m256i result = _mm256_add_epi16(d, s);
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(dest + i), result);
    }
    
    // Handle remaining elements
    for (; i < n; i++) {
        dest[i] += src[i];
    }
}

void sub_vectors_i16(int16_t* dest, const int16_t* src, size_t n) {
    size_t i = 0;
    
    // Process 16 elements at a time with AVX2
    for (; i + 16 <= n; i += 16) {
        __m256i d = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(dest + i));
        __m256i s = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(src + i));
        __m256i result = _mm256_sub_epi16(d, s);
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(dest + i), result);
    }
    
    // Handle remaining elements
    for (; i < n; i++) {
        dest[i] -= src[i];
    }
}

int32_t dot_product_i16(const int16_t* a, const int16_t* b, size_t n) {
    __m256i sum = _mm256_setzero_si256();
    size_t i = 0;
    
    // Process 16 elements at a time
    for (; i + 16 <= n; i += 16) {
        __m256i va = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(a + i));
        __m256i vb = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(b + i));
        __m256i prod = _mm256_madd_epi16(va, vb);
        sum = _mm256_add_epi32(sum, prod);
    }
    
    // Horizontal sum
    __m128i sum128 = _mm_add_epi32(_mm256_castsi256_si128(sum), _mm256_extracti128_si256(sum, 1));
    sum128 = _mm_hadd_epi32(sum128, sum128);
    sum128 = _mm_hadd_epi32(sum128, sum128);
    int32_t result = _mm_cvtsi128_si32(sum128);
    
    // Handle remaining elements
    for (; i < n; i++) {
        result += static_cast<int32_t>(a[i]) * static_cast<int32_t>(b[i]);
    }
    
    return result;
}

void relu_i16(int16_t* data, size_t n) {
    __m256i zero = _mm256_setzero_si256();
    size_t i = 0;
    
    for (; i + 16 <= n; i += 16) {
        __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(data + i));
        __m256i result = _mm256_max_epi16(v, zero);
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(data + i), result);
    }
    
    for (; i < n; i++) {
        if (data[i] < 0) data[i] = 0;
    }
}

void clipped_relu_i16(int16_t* data, size_t n, int16_t max_val) {
    __m256i zero = _mm256_setzero_si256();
    __m256i max_vec = _mm256_set1_epi16(max_val);
    size_t i = 0;
    
    for (; i + 16 <= n; i += 16) {
        __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(data + i));
        __m256i result = _mm256_max_epi16(v, zero);
        result = _mm256_min_epi16(result, max_vec);
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(data + i), result);
    }
    
    for (; i < n; i++) {
        data[i] = std::max(int16_t(0), std::min(data[i], max_val));
    }
}

#elif defined(USE_SSE41)

void add_vectors_i16(int16_t* dest, const int16_t* src, size_t n) {
    size_t i = 0;
    
    // Process 8 elements at a time with SSE
    for (; i + 8 <= n; i += 8) {
        __m128i d = _mm_loadu_si128(reinterpret_cast<const __m128i*>(dest + i));
        __m128i s = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src + i));
        __m128i result = _mm_add_epi16(d, s);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(dest + i), result);
    }
    
    for (; i < n; i++) {
        dest[i] += src[i];
    }
}

void sub_vectors_i16(int16_t* dest, const int16_t* src, size_t n) {
    size_t i = 0;
    
    for (; i + 8 <= n; i += 8) {
        __m128i d = _mm_loadu_si128(reinterpret_cast<const __m128i*>(dest + i));
        __m128i s = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src + i));
        __m128i result = _mm_sub_epi16(d, s);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(dest + i), result);
    }
    
    for (; i < n; i++) {
        dest[i] -= src[i];
    }
}

int32_t dot_product_i16(const int16_t* a, const int16_t* b, size_t n) {
    __m128i sum = _mm_setzero_si128();
    size_t i = 0;
    
    for (; i + 8 <= n; i += 8) {
        __m128i va = _mm_loadu_si128(reinterpret_cast<const __m128i*>(a + i));
        __m128i vb = _mm_loadu_si128(reinterpret_cast<const __m128i*>(b + i));
        __m128i prod = _mm_madd_epi16(va, vb);
        sum = _mm_add_epi32(sum, prod);
    }
    
    // Horizontal sum
    sum = _mm_hadd_epi32(sum, sum);
    sum = _mm_hadd_epi32(sum, sum);
    int32_t result = _mm_cvtsi128_si32(sum);
    
    for (; i < n; i++) {
        result += static_cast<int32_t>(a[i]) * static_cast<int32_t>(b[i]);
    }
    
    return result;
}

void relu_i16(int16_t* data, size_t n) {
    __m128i zero = _mm_setzero_si128();
    size_t i = 0;
    
    for (; i + 8 <= n; i += 8) {
        __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data + i));
        __m128i result = _mm_max_epi16(v, zero);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(data + i), result);
    }
    
    for (; i < n; i++) {
        if (data[i] < 0) data[i] = 0;
    }
}

void clipped_relu_i16(int16_t* data, size_t n, int16_t max_val) {
    __m128i zero = _mm_setzero_si128();
    __m128i max_vec = _mm_set1_epi16(max_val);
    size_t i = 0;
    
    for (; i + 8 <= n; i += 8) {
        __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data + i));
        __m128i result = _mm_max_epi16(v, zero);
        result = _mm_min_epi16(result, max_vec);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(data + i), result);
    }
    
    for (; i < n; i++) {
        data[i] = std::max(int16_t(0), std::min(data[i], max_val));
    }
}

#else

// Scalar fallback implementations
void add_vectors_i16(int16_t* dest, const int16_t* src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        dest[i] += src[i];
    }
}

void sub_vectors_i16(int16_t* dest, const int16_t* src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        dest[i] -= src[i];
    }
}

int32_t dot_product_i16(const int16_t* a, const int16_t* b, size_t n) {
    int32_t result = 0;
    for (size_t i = 0; i < n; i++) {
        result += static_cast<int32_t>(a[i]) * static_cast<int32_t>(b[i]);
    }
    return result;
}

void relu_i16(int16_t* data, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (data[i] < 0) data[i] = 0;
    }
}

void clipped_relu_i16(int16_t* data, size_t n, int16_t max_val) {
    for (size_t i = 0; i < n; i++) {
        data[i] = std::max(int16_t(0), std::min(data[i], max_val));
    }
}

#endif

void matmul_i16(int32_t* result, const int16_t* weights, const int16_t* input,
                const int32_t* bias, size_t output_size, size_t input_size) {
    for (size_t i = 0; i < output_size; i++) {
        result[i] = bias ? bias[i] : 0;
        result[i] += dot_product_i16(weights + i * input_size, input, input_size);
    }
}

} // namespace simd
} // namespace chess
