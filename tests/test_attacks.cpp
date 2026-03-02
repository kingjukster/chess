#include <gtest/gtest.h>
#include "../movegen/attacks.h"
#include "../board/bitboard.h"
#include "../board/position.h"

using namespace chess;

class AttacksTest : public ::testing::Test {
protected:
    void SetUp() override {
        Attacks::init();
    }
};

TEST_F(AttacksTest, PawnAttacksWhite) {
    Bitboard attacks = Attacks::pawn_attacks(make_square(4, 3), WHITE);
    EXPECT_EQ(popcount(attacks), 2);
    
    Bitboard expected = (1ULL << make_square(3, 4)) | (1ULL << make_square(5, 4));
    EXPECT_EQ(attacks, expected);
}

TEST_F(AttacksTest, PawnAttacksBlack) {
    Bitboard attacks = Attacks::pawn_attacks(make_square(4, 4), BLACK);
    EXPECT_EQ(popcount(attacks), 2);
    
    Bitboard expected = (1ULL << make_square(3, 3)) | (1ULL << make_square(5, 3));
    EXPECT_EQ(attacks, expected);
}

TEST_F(AttacksTest, PawnAttacksEdge) {
    Bitboard attacks_a = Attacks::pawn_attacks(make_square(0, 3), WHITE);
    EXPECT_EQ(popcount(attacks_a), 1);
    
    Bitboard attacks_h = Attacks::pawn_attacks(make_square(7, 3), WHITE);
    EXPECT_EQ(popcount(attacks_h), 1);
}

TEST_F(AttacksTest, KnightAttacksCenter) {
    Bitboard attacks = Attacks::knight_attacks(make_square(4, 4));
    EXPECT_EQ(popcount(attacks), 8);
}

TEST_F(AttacksTest, KnightAttacksCorner) {
    Bitboard attacks = Attacks::knight_attacks(make_square(0, 0));
    EXPECT_EQ(popcount(attacks), 2);
    
    Bitboard expected = (1ULL << make_square(1, 2)) | (1ULL << make_square(2, 1));
    EXPECT_EQ(attacks, expected);
}

TEST_F(AttacksTest, KnightAttacksEdge) {
    Bitboard attacks = Attacks::knight_attacks(make_square(0, 4));
    EXPECT_EQ(popcount(attacks), 4);
}

TEST_F(AttacksTest, KingAttacksCenter) {
    Bitboard attacks = Attacks::king_attacks(make_square(4, 4));
    EXPECT_EQ(popcount(attacks), 8);
}

TEST_F(AttacksTest, KingAttacksCorner) {
    Bitboard attacks = Attacks::king_attacks(make_square(0, 0));
    EXPECT_EQ(popcount(attacks), 3);
    
    Bitboard expected = (1ULL << make_square(0, 1)) | 
                        (1ULL << make_square(1, 0)) | 
                        (1ULL << make_square(1, 1));
    EXPECT_EQ(attacks, expected);
}

TEST_F(AttacksTest, KingAttacksEdge) {
    Bitboard attacks = Attacks::king_attacks(make_square(4, 0));
    EXPECT_EQ(popcount(attacks), 5);
}

TEST_F(AttacksTest, BishopAttacksEmpty) {
    Bitboard attacks = Attacks::bishop_attacks(make_square(4, 4), 0ULL);
    EXPECT_EQ(popcount(attacks), 13);
}

TEST_F(AttacksTest, BishopAttacksBlocked) {
    Bitboard occupancy = (1ULL << make_square(2, 2)) | (1ULL << make_square(6, 6));
    Bitboard attacks = Attacks::bishop_attacks(make_square(4, 4), occupancy);
    
    EXPECT_TRUE(attacks & (1ULL << make_square(2, 2)));
    EXPECT_TRUE(attacks & (1ULL << make_square(6, 6)));
    EXPECT_FALSE(attacks & (1ULL << make_square(1, 1)));
    EXPECT_FALSE(attacks & (1ULL << make_square(7, 7)));
}

TEST_F(AttacksTest, BishopAttacksCorner) {
    Bitboard attacks = Attacks::bishop_attacks(make_square(0, 0), 0ULL);
    EXPECT_EQ(popcount(attacks), 7);
}

TEST_F(AttacksTest, RookAttacksEmpty) {
    Bitboard attacks = Attacks::rook_attacks(make_square(4, 4), 0ULL);
    EXPECT_EQ(popcount(attacks), 14);
}

TEST_F(AttacksTest, RookAttacksBlocked) {
    Bitboard occupancy = (1ULL << make_square(4, 2)) | (1ULL << make_square(4, 6)) |
                        (1ULL << make_square(2, 4)) | (1ULL << make_square(6, 4));
    Bitboard attacks = Attacks::rook_attacks(make_square(4, 4), occupancy);
    
    EXPECT_TRUE(attacks & (1ULL << make_square(4, 2)));
    EXPECT_TRUE(attacks & (1ULL << make_square(4, 6)));
    EXPECT_TRUE(attacks & (1ULL << make_square(2, 4)));
    EXPECT_TRUE(attacks & (1ULL << make_square(6, 4)));
    
    EXPECT_FALSE(attacks & (1ULL << make_square(4, 1)));
    EXPECT_FALSE(attacks & (1ULL << make_square(4, 7)));
    EXPECT_FALSE(attacks & (1ULL << make_square(1, 4)));
    EXPECT_FALSE(attacks & (1ULL << make_square(7, 4)));
}

TEST_F(AttacksTest, RookAttacksCorner) {
    Bitboard attacks = Attacks::rook_attacks(make_square(0, 0), 0ULL);
    EXPECT_EQ(popcount(attacks), 14);
}

TEST_F(AttacksTest, QueenAttacksEmpty) {
    Bitboard attacks = Attacks::queen_attacks(make_square(4, 4), 0ULL);
    EXPECT_EQ(popcount(attacks), 27);
}

TEST_F(AttacksTest, QueenAttacksBlocked) {
    Bitboard occupancy = (1ULL << make_square(4, 2)) | (1ULL << make_square(6, 6));
    Bitboard attacks = Attacks::queen_attacks(make_square(4, 4), occupancy);
    
    EXPECT_TRUE(attacks & (1ULL << make_square(4, 2)));
    EXPECT_TRUE(attacks & (1ULL << make_square(6, 6)));
    EXPECT_FALSE(attacks & (1ULL << make_square(4, 1)));
    EXPECT_FALSE(attacks & (1ULL << make_square(7, 7)));
}

TEST_F(AttacksTest, AttacksFromPiece) {
    Bitboard knight_attacks = Attacks::attacks_from(KNIGHT, make_square(4, 4), 0ULL);
    EXPECT_EQ(popcount(knight_attacks), 8);
    
    Bitboard bishop_attacks = Attacks::attacks_from(BISHOP, make_square(4, 4), 0ULL);
    EXPECT_EQ(popcount(bishop_attacks), 13);
    
    Bitboard rook_attacks = Attacks::attacks_from(ROOK, make_square(4, 4), 0ULL);
    EXPECT_EQ(popcount(rook_attacks), 14);
    
    Bitboard queen_attacks = Attacks::attacks_from(QUEEN, make_square(4, 4), 0ULL);
    EXPECT_EQ(popcount(queen_attacks), 27);
    
    Bitboard king_attacks = Attacks::attacks_from(KING, make_square(4, 4), 0ULL);
    EXPECT_EQ(popcount(king_attacks), 8);
}

TEST_F(AttacksTest, IsAttackedByPawn) {
    Position pos("8/8/8/8/3P4/8/8/8 w - - 0 1");
    
    EXPECT_TRUE(Attacks::is_attacked(pos, make_square(2, 4), WHITE));
    EXPECT_TRUE(Attacks::is_attacked(pos, make_square(4, 4), WHITE));
    EXPECT_FALSE(Attacks::is_attacked(pos, make_square(3, 4), WHITE));
    EXPECT_FALSE(Attacks::is_attacked(pos, make_square(3, 3), WHITE));
}

TEST_F(AttacksTest, IsAttackedByKnight) {
    Position pos("8/8/8/8/3N4/8/8/8 w - - 0 1");
    
    // Knight on d4 (3,3) attacks: b3, b5, c2, c6, e2, e6, f3, f5
    EXPECT_TRUE(Attacks::is_attacked(pos, make_square(1, 2), WHITE));  // b3
    EXPECT_TRUE(Attacks::is_attacked(pos, make_square(2, 1), WHITE));  // c2
    EXPECT_TRUE(Attacks::is_attacked(pos, make_square(4, 1), WHITE));  // e2
    EXPECT_TRUE(Attacks::is_attacked(pos, make_square(5, 2), WHITE));  // f3
    EXPECT_FALSE(Attacks::is_attacked(pos, make_square(3, 3), WHITE)); // d4 (knight's square)
    EXPECT_FALSE(Attacks::is_attacked(pos, make_square(2, 2), WHITE)); // c3 (not attacked)
}

TEST_F(AttacksTest, IsAttackedByBishop) {
    Position pos("8/8/8/8/3B4/8/8/8 w - - 0 1");
    
    // Bishop on d4 (3,3) attacks diagonals: a1, b2, c3, e5, f6, g7, h8 (NE-SW) and a7, b6, c5, e3, f2, g1 (NW-SE)
    EXPECT_TRUE(Attacks::is_attacked(pos, make_square(0, 0), WHITE));  // a1
    EXPECT_TRUE(Attacks::is_attacked(pos, make_square(1, 1), WHITE));  // b2
    EXPECT_TRUE(Attacks::is_attacked(pos, make_square(2, 2), WHITE));  // c3
    EXPECT_TRUE(Attacks::is_attacked(pos, make_square(4, 4), WHITE));  // e5
    EXPECT_TRUE(Attacks::is_attacked(pos, make_square(6, 6), WHITE));  // g7
    EXPECT_FALSE(Attacks::is_attacked(pos, make_square(3, 4), WHITE)); // d5 (not on diagonal)
    EXPECT_FALSE(Attacks::is_attacked(pos, make_square(0, 1), WHITE)); // a2 (not on diagonal)
}

TEST_F(AttacksTest, IsAttackedByRook) {
    Position pos("8/8/8/8/3R4/8/8/8 w - - 0 1");
    
    EXPECT_TRUE(Attacks::is_attacked(pos, make_square(3, 0), WHITE));
    EXPECT_TRUE(Attacks::is_attacked(pos, make_square(3, 7), WHITE));
    EXPECT_TRUE(Attacks::is_attacked(pos, make_square(0, 3), WHITE));
    EXPECT_TRUE(Attacks::is_attacked(pos, make_square(7, 3), WHITE));
    EXPECT_FALSE(Attacks::is_attacked(pos, make_square(4, 4), WHITE));
}

TEST_F(AttacksTest, InCheck) {
    Position pos1("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT_FALSE(Attacks::in_check(pos1, WHITE));
    EXPECT_FALSE(Attacks::in_check(pos1, BLACK));
    
    Position pos2("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e6 0 2");
    EXPECT_FALSE(Attacks::in_check(pos2, WHITE));
    EXPECT_FALSE(Attacks::in_check(pos2, BLACK));
    
    Position pos3("rnb1kbnr/pppp1ppp/8/4p3/5PPq/8/PPPPP2P/RNBQKBNR w KQkq - 1 3");
    EXPECT_TRUE(Attacks::in_check(pos3, WHITE));
    EXPECT_FALSE(Attacks::in_check(pos3, BLACK));
}
