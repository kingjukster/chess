/*
 * Fathom - Syzygy tablebase probing library
 * Copyright (C) 2015 basil00
 * https://github.com/jdart1/Fathom
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 */

#ifndef TBPROBE_H
#define TBPROBE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t tb_bitboard_t;

#define TB_RESULT_WDL_MASK          0x0000000F
#define TB_RESULT_TO_MASK           0x000003F0
#define TB_RESULT_FROM_MASK         0x0000FC00
#define TB_RESULT_PROMOTES_MASK     0x00070000
#define TB_RESULT_EP_MASK           0x00080000
#define TB_RESULT_DTZ_MASK          0xFFF00000
#define TB_RESULT_WDL_SHIFT         0
#define TB_RESULT_TO_SHIFT          4
#define TB_RESULT_FROM_SHIFT        10
#define TB_RESULT_PROMOTES_SHIFT    16
#define TB_RESULT_EP_SHIFT          19
#define TB_RESULT_DTZ_SHIFT         20

#define TB_GET_WDL(result)          (((result) & TB_RESULT_WDL_MASK) >> TB_RESULT_WDL_SHIFT)
#define TB_GET_TO(result)           (((result) & TB_RESULT_TO_MASK) >> TB_RESULT_TO_SHIFT)
#define TB_GET_FROM(result)         (((result) & TB_RESULT_FROM_MASK) >> TB_RESULT_FROM_SHIFT)
#define TB_GET_PROMOTES(result)     (((result) & TB_RESULT_PROMOTES_MASK) >> TB_RESULT_PROMOTES_SHIFT)
#define TB_GET_EP(result)           (((result) & TB_RESULT_EP_MASK) >> TB_RESULT_EP_SHIFT)
#define TB_GET_DTZ(result)          (((int)(result) & TB_RESULT_DTZ_MASK) >> TB_RESULT_DTZ_SHIFT)

#define TB_LOSS                     0
#define TB_BLESSED_LOSS             1
#define TB_DRAW                     2
#define TB_CURSED_WIN               3
#define TB_WIN                      4

#define TB_PROMOTES_NONE            0
#define TB_PROMOTES_QUEEN           1
#define TB_PROMOTES_ROOK            2
#define TB_PROMOTES_BISHOP          3
#define TB_PROMOTES_KNIGHT          4

#define TB_RESULT_FAILED            0xFFFFFFFF
#define TB_RESULT_CHECKMATE         0xFFFFFFFE
#define TB_RESULT_STALEMATE         0xFFFFFFFD

#define TB_MAX_MOVES                256

extern bool tb_init(const char *path);
extern void tb_free(void);

extern unsigned tb_probe_wdl(
    uint64_t white,
    uint64_t black,
    uint64_t kings,
    uint64_t queens,
    uint64_t rooks,
    uint64_t bishops,
    uint64_t knights,
    uint64_t pawns,
    uint64_t ep,
    bool turn);

extern unsigned tb_probe_root(
    uint64_t white,
    uint64_t black,
    uint64_t kings,
    uint64_t queens,
    uint64_t rooks,
    uint64_t bishops,
    uint64_t knights,
    uint64_t pawns,
    unsigned rule50,
    uint64_t ep,
    bool turn,
    unsigned *results);

extern unsigned tb_max_cardinality(void);

#ifdef __cplusplus
}
#endif

#endif
