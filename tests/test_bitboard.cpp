#include <gtest/gtest.h>
#include "../board/bitboard.h"
#include "../board/types.h"

using namespace chess;

class BitboardTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(BitboardTest, PopcountZero) {
    EXPECT_EQ(popcount(0ULL), 0);
}

TEST_F(BitboardTest, PopcountOne) {
    EXPECT_EQ(popcount(1ULL), 1);
    EXPECT_EQ(popcount(2ULL), 1);
    EXPECT_EQ(popcount(4ULL), 1);
    EXPECT_EQ(popcount(0x8000000000000000ULL), 1);
}

TEST_F(BitboardTest, PopcountMultiple) {
    EXPECT_EQ(popcount(3ULL), 2);
    EXPECT_EQ(popcount(7ULL), 3);
    EXPECT_EQ(popcount(0xFFULL), 8);
    EXPECT_EQ(popcount(0xFFFFULL), 16);
    EXPECT_EQ(popcount(0xFFFFFFFFULL), 32);
    EXPECT_EQ(popcount(0xFFFFFFFFFFFFFFFFULL), 64);
}

TEST_F(BitboardTest, PopcountAlternating) {
    EXPECT_EQ(popcount(0xAAAAAAAAAAAAAAAAULL), 32);
    EXPECT_EQ(popcount(0x5555555555555555ULL), 32);
}

TEST_F(BitboardTest, LsbZero) {
    EXPECT_EQ(lsb(0ULL), 64);
}

TEST_F(BitboardTest, LsbSingleBit) {
    EXPECT_EQ(lsb(1ULL), 0);
    EXPECT_EQ(lsb(2ULL), 1);
    EXPECT_EQ(lsb(4ULL), 2);
    EXPECT_EQ(lsb(8ULL), 3);
    EXPECT_EQ(lsb(0x8000000000000000ULL), 63);
}

TEST_F(BitboardTest, LsbMultipleBits) {
    EXPECT_EQ(lsb(3ULL), 0);
    EXPECT_EQ(lsb(6ULL), 1);
    EXPECT_EQ(lsb(12ULL), 2);
    EXPECT_EQ(lsb(0xFFFFFFFFFFFFFFFFULL), 0);
}

TEST_F(BitboardTest, MsbZero) {
    EXPECT_EQ(msb(0ULL), 64);
}

TEST_F(BitboardTest, MsbSingleBit) {
    EXPECT_EQ(msb(1ULL), 0);
    EXPECT_EQ(msb(2ULL), 1);
    EXPECT_EQ(msb(4ULL), 2);
    EXPECT_EQ(msb(8ULL), 3);
    EXPECT_EQ(msb(0x8000000000000000ULL), 63);
}

TEST_F(BitboardTest, MsbMultipleBits) {
    EXPECT_EQ(msb(3ULL), 1);
    EXPECT_EQ(msb(7ULL), 2);
    EXPECT_EQ(msb(15ULL), 3);
    EXPECT_EQ(msb(0xFFFFFFFFFFFFFFFFULL), 63);
}

TEST_F(BitboardTest, PopLsb) {
    Bitboard bb = 0x15ULL;
    EXPECT_EQ(popcount(bb), 3);
    
    Square sq1 = pop_lsb(bb);
    EXPECT_EQ(sq1, 0);
    EXPECT_EQ(popcount(bb), 2);
    
    Square sq2 = pop_lsb(bb);
    EXPECT_EQ(sq2, 2);
    EXPECT_EQ(popcount(bb), 1);
    
    Square sq3 = pop_lsb(bb);
    EXPECT_EQ(sq3, 4);
    EXPECT_EQ(popcount(bb), 0);
    EXPECT_EQ(bb, 0ULL);
}

TEST_F(BitboardTest, PopLsbIteration) {
    Bitboard bb = 0xFFULL;
    int count = 0;
    
    while (bb) {
        Square sq = pop_lsb(bb);
        EXPECT_EQ(sq, count);
        count++;
    }
    
    EXPECT_EQ(count, 8);
}

TEST_F(BitboardTest, SquareHelpers) {
    EXPECT_EQ(make_square(0, 0), 0);
    EXPECT_EQ(make_square(7, 0), 7);
    EXPECT_EQ(make_square(0, 7), 56);
    EXPECT_EQ(make_square(7, 7), 63);
    EXPECT_EQ(make_square(4, 4), 36);
}

TEST_F(BitboardTest, FileOfRankOf) {
    EXPECT_EQ(file_of(0), 0);
    EXPECT_EQ(rank_of(0), 0);
    
    EXPECT_EQ(file_of(7), 7);
    EXPECT_EQ(rank_of(7), 0);
    
    EXPECT_EQ(file_of(56), 0);
    EXPECT_EQ(rank_of(56), 7);
    
    EXPECT_EQ(file_of(63), 7);
    EXPECT_EQ(rank_of(63), 7);
    
    EXPECT_EQ(file_of(36), 4);
    EXPECT_EQ(rank_of(36), 4);
}

TEST_F(BitboardTest, PieceHelpers) {
    Piece wp = make_piece(WHITE, PAWN);
    EXPECT_EQ(color_of(wp), WHITE);
    EXPECT_EQ(type_of(wp), PAWN);
    
    Piece bq = make_piece(BLACK, QUEEN);
    EXPECT_EQ(color_of(bq), BLACK);
    EXPECT_EQ(type_of(bq), QUEEN);
    
    EXPECT_EQ(make_piece(WHITE, KNIGHT), W_KNIGHT);
    EXPECT_EQ(make_piece(BLACK, ROOK), B_ROOK);
}

TEST_F(BitboardTest, MoveConstruction) {
    Move m1(0, 8);
    EXPECT_EQ(m1.from(), 0);
    EXPECT_EQ(m1.to(), 8);
    EXPECT_FALSE(m1.is_capture());
    EXPECT_FALSE(m1.is_promotion());
    EXPECT_FALSE(m1.is_special());
    
    Move m2(8, 16, NO_PIECE, true, false);
    EXPECT_EQ(m2.from(), 8);
    EXPECT_EQ(m2.to(), 16);
    EXPECT_TRUE(m2.is_capture());
    EXPECT_FALSE(m2.is_promotion());
    
    Move m3(48, 56, QUEEN, false, false);
    EXPECT_EQ(m3.from(), 48);
    EXPECT_EQ(m3.to(), 56);
    EXPECT_TRUE(m3.is_promotion());
    EXPECT_EQ(m3.promotion(), QUEEN);
    
    Move m4(4, 6, NO_PIECE, false, true);
    EXPECT_TRUE(m4.is_special());
    EXPECT_TRUE(m4.is_castling());
}

TEST_F(BitboardTest, MoveEquality) {
    Move m1(0, 8);
    Move m2(0, 8);
    Move m3(0, 16);
    
    EXPECT_EQ(m1, m2);
    EXPECT_NE(m1, m3);
    EXPECT_NE(m2, m3);
}

TEST_F(BitboardTest, MoveValidity) {
    Move valid(0, 8);
    EXPECT_TRUE(valid.is_valid());
    
    Move invalid1(0, 0);
    EXPECT_FALSE(invalid1.is_valid());
    
    Move invalid2(0, 64);
    EXPECT_FALSE(invalid2.is_valid());
    
    Move invalid3(64, 0);
    EXPECT_FALSE(invalid3.is_valid());
}
