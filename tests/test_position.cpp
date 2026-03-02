#include <gtest/gtest.h>
#include "../board/position.h"
#include "../board/bitboard.h"
#include "../movegen/attacks.h"

using namespace chess;

class PositionTest : public ::testing::Test {
protected:
    void SetUp() override {
        Attacks::init();
    }
};

TEST_F(PositionTest, DefaultConstructor) {
    Position pos;
    EXPECT_EQ(pos.side_to_move(), WHITE);
}

TEST_F(PositionTest, StartingPositionFEN) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    
    EXPECT_EQ(pos.side_to_move(), WHITE);
    EXPECT_EQ(pos.castling_rights(), ANY_CASTLING);
    EXPECT_EQ(pos.ep_square(), SQ_NONE);
    EXPECT_EQ(pos.rule50(), 0);
    
    EXPECT_EQ(popcount(pos.pieces(WHITE)), 16);
    EXPECT_EQ(popcount(pos.pieces(BLACK)), 16);
    EXPECT_EQ(popcount(pos.pieces(PAWN)), 16);
    EXPECT_EQ(popcount(pos.pieces(KNIGHT)), 4);
    EXPECT_EQ(popcount(pos.pieces(BISHOP)), 4);
    EXPECT_EQ(popcount(pos.pieces(ROOK)), 4);
    EXPECT_EQ(popcount(pos.pieces(QUEEN)), 2);
    EXPECT_EQ(popcount(pos.pieces(KING)), 2);
}

TEST_F(PositionTest, FENParsing) {
    Position pos("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    
    EXPECT_EQ(pos.side_to_move(), WHITE);
    EXPECT_EQ(pos.castling_rights(), ANY_CASTLING);
    
    EXPECT_EQ(pos.piece_on(make_square(0, 0)), W_ROOK);
    EXPECT_EQ(pos.piece_on(make_square(4, 0)), W_KING);
    EXPECT_EQ(pos.piece_on(make_square(7, 0)), W_ROOK);
    
    EXPECT_EQ(pos.piece_on(make_square(0, 7)), B_ROOK);
    EXPECT_EQ(pos.piece_on(make_square(4, 7)), B_KING);
    EXPECT_EQ(pos.piece_on(make_square(7, 7)), B_ROOK);
}

TEST_F(PositionTest, FENToString) {
    std::string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    Position pos(fen);
    std::string result = pos.to_fen();
    
    EXPECT_EQ(result, fen);
}

TEST_F(PositionTest, FENRoundTrip) {
    std::vector<std::string> fens = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"
    };
    
    for (const auto& fen : fens) {
        Position pos(fen);
        EXPECT_EQ(pos.to_fen(), fen) << "FEN round-trip failed for: " << fen;
    }
}

TEST_F(PositionTest, EnPassantSquare) {
    Position pos("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    EXPECT_EQ(pos.ep_square(), make_square(4, 2));
}

TEST_F(PositionTest, CastlingRights) {
    Position pos1("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    EXPECT_EQ(pos1.castling_rights(), ANY_CASTLING);
    
    Position pos2("r3k2r/8/8/8/8/8/8/R3K2R w Kq - 0 1");
    EXPECT_EQ(pos2.castling_rights(), WHITE_OO | BLACK_OOO);
    
    Position pos3("r3k2r/8/8/8/8/8/8/R3K2R w - - 0 1");
    EXPECT_EQ(pos3.castling_rights(), NO_CASTLING);
}

TEST_F(PositionTest, MakeMovePawn) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    UndoInfo undo;
    
    Move move(make_square(4, 1), make_square(4, 3));
    bool success = pos.make_move(move, undo);
    
    EXPECT_TRUE(success);
    EXPECT_EQ(pos.side_to_move(), BLACK);
    EXPECT_EQ(pos.piece_on(make_square(4, 1)), NO_PIECE_PIECE);
    EXPECT_EQ(pos.piece_on(make_square(4, 3)), W_PAWN);
    EXPECT_EQ(pos.ep_square(), make_square(4, 2));
}

TEST_F(PositionTest, MakeMoveCapture) {
    Position pos("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2");
    UndoInfo undo;
    
    Move move(make_square(4, 3), make_square(3, 4), NO_PIECE, true);
    bool success = pos.make_move(move, undo);
    
    EXPECT_TRUE(success);
    EXPECT_EQ(pos.piece_on(make_square(4, 3)), NO_PIECE_PIECE);
    EXPECT_EQ(pos.piece_on(make_square(3, 4)), W_PAWN);
    EXPECT_EQ(undo.captured_piece, B_PAWN);
}

TEST_F(PositionTest, UnmakeMove) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    std::string original_fen = pos.to_fen();
    UndoInfo undo;
    
    Move move(make_square(4, 1), make_square(4, 3));
    pos.make_move(move, undo);
    pos.unmake_move(move, undo);
    
    EXPECT_EQ(pos.to_fen(), original_fen);
}

TEST_F(PositionTest, UnmakeMoveCapture) {
    Position pos("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2");
    std::string original_fen = pos.to_fen();
    UndoInfo undo;
    
    Move move(make_square(4, 3), make_square(3, 4), NO_PIECE, true);
    pos.make_move(move, undo);
    pos.unmake_move(move, undo);
    
    EXPECT_EQ(pos.to_fen(), original_fen);
}

TEST_F(PositionTest, MakeUnmakeMultipleMoves) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    std::string original_fen = pos.to_fen();
    
    std::vector<Move> moves = {
        Move(make_square(4, 1), make_square(4, 3)),
        Move(make_square(4, 6), make_square(4, 4)),
        Move(make_square(6, 0), make_square(5, 2)),
        Move(make_square(1, 7), make_square(2, 5))
    };
    
    std::vector<UndoInfo> undos(moves.size());
    
    for (size_t i = 0; i < moves.size(); i++) {
        pos.make_move(moves[i], undos[i]);
    }
    
    for (int i = moves.size() - 1; i >= 0; i--) {
        pos.unmake_move(moves[i], undos[i]);
    }
    
    EXPECT_EQ(pos.to_fen(), original_fen);
}

TEST_F(PositionTest, IsCheck) {
    Position pos1("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT_FALSE(pos1.is_check());
    
    Position pos2("rnb1kbnr/pppp1ppp/8/4p3/5PPq/8/PPPPP2P/RNBQKBNR w KQkq - 1 3");
    EXPECT_TRUE(pos2.is_check());
    
    Position pos3("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    EXPECT_FALSE(pos3.is_check());
}

TEST_F(PositionTest, KingSquare) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    
    EXPECT_EQ(pos.king_square(WHITE), make_square(4, 0));
    EXPECT_EQ(pos.king_square(BLACK), make_square(4, 7));
}

TEST_F(PositionTest, PieceQueries) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    
    EXPECT_EQ(popcount(pos.pieces(WHITE, PAWN)), 8);
    EXPECT_EQ(popcount(pos.pieces(BLACK, PAWN)), 8);
    EXPECT_EQ(popcount(pos.pieces(WHITE, KNIGHT)), 2);
    EXPECT_EQ(popcount(pos.pieces(BLACK, KNIGHT)), 2);
    EXPECT_EQ(popcount(pos.pieces(WHITE, BISHOP)), 2);
    EXPECT_EQ(popcount(pos.pieces(BLACK, BISHOP)), 2);
    EXPECT_EQ(popcount(pos.pieces(WHITE, ROOK)), 2);
    EXPECT_EQ(popcount(pos.pieces(BLACK, ROOK)), 2);
    EXPECT_EQ(popcount(pos.pieces(WHITE, QUEEN)), 1);
    EXPECT_EQ(popcount(pos.pieces(BLACK, QUEEN)), 1);
    EXPECT_EQ(popcount(pos.pieces(WHITE, KING)), 1);
    EXPECT_EQ(popcount(pos.pieces(BLACK, KING)), 1);
}

TEST_F(PositionTest, ZobristKeyConsistency) {
    Position pos1("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    Position pos2("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    
    EXPECT_EQ(pos1.zobrist_key(), pos2.zobrist_key());
}

TEST_F(PositionTest, ZobristKeyAfterMove) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    uint64_t key1 = pos.zobrist_key();
    
    UndoInfo undo;
    Move move(make_square(4, 1), make_square(4, 3));
    pos.make_move(move, undo);
    uint64_t key2 = pos.zobrist_key();
    
    EXPECT_NE(key1, key2);
    
    pos.unmake_move(move, undo);
    uint64_t key3 = pos.zobrist_key();
    
    EXPECT_EQ(key1, key3);
}
