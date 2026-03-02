#include <gtest/gtest.h>
#include "../movegen/movegen.h"
#include "../movegen/attacks.h"
#include "../board/position.h"
#include <fstream>
#include <sstream>

using namespace chess;

class PerftTest : public ::testing::Test {
protected:
    void SetUp() override {
        Attacks::init();
    }
    
    uint64_t perft_divide(Position& pos, int depth) {
        if (depth == 0) return 1;
        
        std::vector<Move> moves;
        MoveGen::generate_legal(pos, moves);
        
        if (depth == 1) return moves.size();
        
        uint64_t nodes = 0;
        for (const auto& move : moves) {
            UndoInfo undo;
            pos.make_move(move, undo);
            nodes += perft_divide(pos, depth - 1);
            pos.unmake_move(move, undo);
        }
        
        return nodes;
    }
};

TEST_F(PerftTest, StartingPositionDepth1) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    uint64_t nodes = MoveGen::perft(pos, 1);
    EXPECT_EQ(nodes, 20) << "Starting position depth 1";
}

TEST_F(PerftTest, StartingPositionDepth2) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    uint64_t nodes = MoveGen::perft(pos, 2);
    EXPECT_EQ(nodes, 400) << "Starting position depth 2";
}

TEST_F(PerftTest, StartingPositionDepth3) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    uint64_t nodes = MoveGen::perft(pos, 3);
    EXPECT_EQ(nodes, 8902) << "Starting position depth 3";
}

TEST_F(PerftTest, StartingPositionDepth4) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    uint64_t nodes = MoveGen::perft(pos, 4);
    EXPECT_EQ(nodes, 197281) << "Starting position depth 4";
}

TEST_F(PerftTest, StartingPositionDepth5) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    uint64_t nodes = MoveGen::perft(pos, 5);
    EXPECT_EQ(nodes, 4865609) << "Starting position depth 5";
}

TEST_F(PerftTest, KiwipeteDepth1) {
    Position pos("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    uint64_t nodes = MoveGen::perft(pos, 1);
    EXPECT_EQ(nodes, 48) << "Kiwipete depth 1";
}

TEST_F(PerftTest, KiwipeteDepth2) {
    Position pos("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    uint64_t nodes = MoveGen::perft(pos, 2);
    EXPECT_EQ(nodes, 2039) << "Kiwipete depth 2";
}

TEST_F(PerftTest, KiwipeteDepth3) {
    Position pos("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    uint64_t nodes = MoveGen::perft(pos, 3);
    EXPECT_EQ(nodes, 97862) << "Kiwipete depth 3";
}

TEST_F(PerftTest, KiwipeteDepth4) {
    Position pos("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    uint64_t nodes = MoveGen::perft(pos, 4);
    EXPECT_EQ(nodes, 4085603) << "Kiwipete depth 4";
}

TEST_F(PerftTest, Position3Depth1) {
    Position pos("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
    uint64_t nodes = MoveGen::perft(pos, 1);
    EXPECT_EQ(nodes, 14) << "Position 3 depth 1";
}

TEST_F(PerftTest, Position3Depth2) {
    Position pos("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
    uint64_t nodes = MoveGen::perft(pos, 2);
    EXPECT_EQ(nodes, 191) << "Position 3 depth 2";
}

TEST_F(PerftTest, Position3Depth3) {
    Position pos("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
    uint64_t nodes = MoveGen::perft(pos, 3);
    EXPECT_EQ(nodes, 2812) << "Position 3 depth 3";
}

TEST_F(PerftTest, Position3Depth4) {
    Position pos("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
    uint64_t nodes = MoveGen::perft(pos, 4);
    EXPECT_EQ(nodes, 43238) << "Position 3 depth 4";
}

TEST_F(PerftTest, Position3Depth5) {
    Position pos("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
    uint64_t nodes = MoveGen::perft(pos, 5);
    EXPECT_EQ(nodes, 674624) << "Position 3 depth 5";
}

TEST_F(PerftTest, Position4Depth1) {
    Position pos("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    uint64_t nodes = MoveGen::perft(pos, 1);
    EXPECT_EQ(nodes, 6) << "Position 4 depth 1";
}

TEST_F(PerftTest, Position4Depth2) {
    Position pos("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    uint64_t nodes = MoveGen::perft(pos, 2);
    EXPECT_EQ(nodes, 264) << "Position 4 depth 2";
}

TEST_F(PerftTest, Position4Depth3) {
    Position pos("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    uint64_t nodes = MoveGen::perft(pos, 3);
    EXPECT_EQ(nodes, 9467) << "Position 4 depth 3";
}

TEST_F(PerftTest, Position4Depth4) {
    Position pos("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    uint64_t nodes = MoveGen::perft(pos, 4);
    EXPECT_EQ(nodes, 422333) << "Position 4 depth 4";
}

TEST_F(PerftTest, Position5Depth1) {
    Position pos("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
    uint64_t nodes = MoveGen::perft(pos, 1);
    EXPECT_EQ(nodes, 44) << "Position 5 depth 1";
}

TEST_F(PerftTest, Position5Depth2) {
    Position pos("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
    uint64_t nodes = MoveGen::perft(pos, 2);
    EXPECT_EQ(nodes, 1486) << "Position 5 depth 2";
}

TEST_F(PerftTest, Position5Depth3) {
    Position pos("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
    uint64_t nodes = MoveGen::perft(pos, 3);
    EXPECT_EQ(nodes, 62379) << "Position 5 depth 3";
}

TEST_F(PerftTest, Position5Depth4) {
    Position pos("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
    uint64_t nodes = MoveGen::perft(pos, 4);
    EXPECT_EQ(nodes, 2103487) << "Position 5 depth 4";
}

TEST_F(PerftTest, Position6Depth1) {
    Position pos("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");
    uint64_t nodes = MoveGen::perft(pos, 1);
    EXPECT_EQ(nodes, 46) << "Position 6 depth 1";
}

TEST_F(PerftTest, Position6Depth2) {
    Position pos("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");
    uint64_t nodes = MoveGen::perft(pos, 2);
    EXPECT_EQ(nodes, 2079) << "Position 6 depth 2";
}

TEST_F(PerftTest, Position6Depth3) {
    Position pos("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");
    uint64_t nodes = MoveGen::perft(pos, 3);
    EXPECT_EQ(nodes, 89890) << "Position 6 depth 3";
}

TEST_F(PerftTest, Position6Depth4) {
    Position pos("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");
    uint64_t nodes = MoveGen::perft(pos, 4);
    EXPECT_EQ(nodes, 3894594) << "Position 6 depth 4";
}

TEST_F(PerftTest, EnPassantCapture) {
    Position pos("rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3");
    uint64_t nodes = MoveGen::perft(pos, 1);
    EXPECT_GE(nodes, 31);
}

TEST_F(PerftTest, CastlingRights) {
    Position pos("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    uint64_t nodes = MoveGen::perft(pos, 1);
    EXPECT_EQ(nodes, 26);
}

TEST_F(PerftTest, PromotionMoves) {
    Position pos("n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1");
    uint64_t nodes = MoveGen::perft(pos, 1);
    EXPECT_GE(nodes, 24);
}

TEST_F(PerftTest, DivideFunction) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    
    std::vector<Move> moves;
    MoveGen::generate_legal(pos, moves);
    
    uint64_t total = 0;
    for (const auto& move : moves) {
        UndoInfo undo;
        pos.make_move(move, undo);
        uint64_t nodes = perft_divide(pos, 1);
        total += nodes;
        pos.unmake_move(move, undo);
    }
    
    EXPECT_EQ(total, 400);
}
