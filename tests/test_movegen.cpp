#include <gtest/gtest.h>
#include "../movegen/movegen.h"
#include "../movegen/attacks.h"
#include "../board/position.h"

using namespace chess;

class MoveGenTest : public ::testing::Test {
protected:
    void SetUp() override {
        Attacks::init();
    }
};

TEST_F(MoveGenTest, StartingPositionMoveCount) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    std::vector<Move> moves;
    MoveGen::generate_moves(pos, moves);
    
    EXPECT_EQ(moves.size(), 20);
}

TEST_F(MoveGenTest, StartingPositionLegalMoves) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    std::vector<Move> moves;
    MoveGen::generate_legal(pos, moves);
    
    EXPECT_EQ(moves.size(), 20);
}

TEST_F(MoveGenTest, PawnMoves) {
    Position pos("8/8/8/8/8/8/P7/8 w - - 0 1");
    std::vector<Move> moves;
    MoveGen::generate_moves(pos, moves);
    
    EXPECT_GE(moves.size(), 2);
    
    bool found_single = false;
    bool found_double = false;
    
    for (const auto& move : moves) {
        if (move.from() == make_square(0, 1) && move.to() == make_square(0, 2)) {
            found_single = true;
        }
        if (move.from() == make_square(0, 1) && move.to() == make_square(0, 3)) {
            found_double = true;
        }
    }
    
    EXPECT_TRUE(found_single);
    EXPECT_TRUE(found_double);
}

TEST_F(MoveGenTest, PawnCaptures) {
    Position pos("8/8/8/8/1p6/P7/8/8 w - - 0 1");
    std::vector<Move> moves;
    MoveGen::generate_captures(pos, moves);
    
    bool found_capture = false;
    for (const auto& move : moves) {
        if (move.from() == make_square(0, 2) && move.to() == make_square(1, 3) && move.is_capture()) {
            found_capture = true;
        }
    }
    
    EXPECT_TRUE(found_capture);
}

TEST_F(MoveGenTest, PawnPromotion) {
    Position pos("8/P7/8/8/8/8/8/8 w - - 0 1");
    std::vector<Move> moves;
    MoveGen::generate_moves(pos, moves);
    
    int promotion_count = 0;
    for (const auto& move : moves) {
        if (move.is_promotion()) {
            promotion_count++;
        }
    }
    
    EXPECT_EQ(promotion_count, 4);
}

TEST_F(MoveGenTest, EnPassant) {
    Position pos("rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3");
    std::vector<Move> moves;
    MoveGen::generate_moves(pos, moves);
    
    bool found_ep = false;
    for (const auto& move : moves) {
        if (move.from() == make_square(4, 4) && move.to() == make_square(5, 5) && move.is_en_passant()) {
            found_ep = true;
        }
    }
    
    EXPECT_TRUE(found_ep);
}

TEST_F(MoveGenTest, KnightMoves) {
    Position pos("8/8/8/8/3N4/8/8/8 w - - 0 1");
    std::vector<Move> moves;
    MoveGen::generate_moves(pos, moves);
    
    EXPECT_EQ(moves.size(), 8);
}

TEST_F(MoveGenTest, BishopMoves) {
    Position pos("8/8/8/8/3B4/8/8/8 w - - 0 1");
    std::vector<Move> moves;
    MoveGen::generate_moves(pos, moves);
    
    EXPECT_EQ(moves.size(), 13);
}

TEST_F(MoveGenTest, RookMoves) {
    Position pos("8/8/8/8/3R4/8/8/8 w - - 0 1");
    std::vector<Move> moves;
    MoveGen::generate_moves(pos, moves);
    
    EXPECT_EQ(moves.size(), 14);
}

TEST_F(MoveGenTest, QueenMoves) {
    Position pos("8/8/8/8/3Q4/8/8/8 w - - 0 1");
    std::vector<Move> moves;
    MoveGen::generate_moves(pos, moves);
    
    EXPECT_EQ(moves.size(), 27);
}

TEST_F(MoveGenTest, KingMoves) {
    Position pos("8/8/8/8/3K4/8/8/8 w - - 0 1");
    std::vector<Move> moves;
    MoveGen::generate_moves(pos, moves);
    
    EXPECT_EQ(moves.size(), 8);
}

TEST_F(MoveGenTest, CastlingWhiteKingside) {
    Position pos("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    std::vector<Move> moves;
    MoveGen::generate_moves(pos, moves);
    
    bool found_castle = false;
    for (const auto& move : moves) {
        if (move.from() == make_square(4, 0) && move.to() == make_square(6, 0) && move.is_castling()) {
            found_castle = true;
        }
    }
    
    EXPECT_TRUE(found_castle);
}

TEST_F(MoveGenTest, CastlingWhiteQueenside) {
    Position pos("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    std::vector<Move> moves;
    MoveGen::generate_moves(pos, moves);
    
    bool found_castle = false;
    for (const auto& move : moves) {
        if (move.from() == make_square(4, 0) && move.to() == make_square(2, 0) && move.is_castling()) {
            found_castle = true;
        }
    }
    
    EXPECT_TRUE(found_castle);
}

TEST_F(MoveGenTest, CastlingBlocked) {
    Position pos("r3k2r/8/8/8/8/8/8/R1N1KB1R w KQkq - 0 1");
    std::vector<Move> moves;
    MoveGen::generate_moves(pos, moves);
    
    for (const auto& move : moves) {
        EXPECT_FALSE(move.is_castling()) << "Castling should be blocked";
    }
}

TEST_F(MoveGenTest, NoCastlingRights) {
    Position pos("r3k2r/8/8/8/8/8/8/R3K2R w - - 0 1");
    std::vector<Move> moves;
    MoveGen::generate_moves(pos, moves);
    
    for (const auto& move : moves) {
        EXPECT_FALSE(move.is_castling()) << "No castling rights available";
    }
}

TEST_F(MoveGenTest, CheckEvasion) {
    // Position with white in check but with legal moves (not checkmate)
    Position pos("rnbqkb1r/pppp1ppp/5n2/4p2Q/4P3/8/PPPP1PPP/RNB1KBNR b KQkq - 1 3");
    std::vector<Move> moves;
    MoveGen::generate_legal(pos, moves);
    
    EXPECT_GT(moves.size(), 0);
    
    for (const auto& move : moves) {
        UndoInfo undo;
        Position test_pos = pos;
        test_pos.make_move(move, undo);
        EXPECT_FALSE(test_pos.is_check(BLACK)) << "Legal move should not leave king in check";
        test_pos.unmake_move(move, undo);
    }
}

TEST_F(MoveGenTest, PinnedPiece) {
    Position pos("4k3/8/8/8/8/8/4r3/4K2R w - - 0 1");
    std::vector<Move> moves;
    MoveGen::generate_legal(pos, moves);
    
    bool rook_can_move_horizontally = false;
    for (const auto& move : moves) {
        if (move.from() == make_square(7, 0) && rank_of(move.to()) == 0) {
            rook_can_move_horizontally = true;
        }
    }
    
    EXPECT_FALSE(rook_can_move_horizontally) << "Pinned rook should not be able to move horizontally";
}

TEST_F(MoveGenTest, CapturesOnly) {
    Position pos("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2");
    std::vector<Move> captures;
    MoveGen::generate_captures(pos, captures);
    
    for (const auto& move : captures) {
        EXPECT_TRUE(move.is_capture() || move.is_en_passant()) << "Captures-only should only include captures";
    }
}

TEST_F(MoveGenTest, QuietMovesOnly) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    std::vector<Move> quiet;
    MoveGen::generate_quiet(pos, quiet);
    
    for (const auto& move : quiet) {
        EXPECT_FALSE(move.is_capture()) << "Quiet moves should not include captures";
    }
    
    EXPECT_EQ(quiet.size(), 20);
}

TEST_F(MoveGenTest, ComplexPosition) {
    Position pos("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    std::vector<Move> moves;
    MoveGen::generate_legal(pos, moves);
    
    EXPECT_GT(moves.size(), 40);
}

TEST_F(MoveGenTest, IsLegal) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    
    Move legal_move(make_square(4, 1), make_square(4, 3));
    EXPECT_TRUE(MoveGen::is_legal(pos, legal_move));
    
    Move illegal_move(make_square(4, 1), make_square(4, 5));
    EXPECT_FALSE(MoveGen::is_legal(pos, illegal_move));
}
