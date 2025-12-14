#pragma once

#include "types.h"
#include <cstdint>

#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace chess {

// Bitboard utilities
inline int popcount(Bitboard bb) {
#ifdef _MSC_VER
    return static_cast<int>(__popcnt64(bb));
#else
    return __builtin_popcountll(bb);
#endif
}

inline int lsb(Bitboard bb) {
    if (bb == 0) return 64;
#ifdef _MSC_VER
    unsigned long idx;
    _BitScanForward64(&idx, bb);
    return static_cast<int>(idx);
#else
    return __builtin_ctzll(bb);
#endif
}

inline int msb(Bitboard bb) {
    if (bb == 0) return 64;
#ifdef _MSC_VER
    unsigned long idx;
    _BitScanReverse64(&idx, bb);
    return static_cast<int>(idx);
#else
    return 63 - __builtin_clzll(bb);
#endif
}

inline Square pop_lsb(Bitboard& bb) {
    Square sq = lsb(bb);
    bb &= bb - 1;
    return sq;
}

} // namespace chess

