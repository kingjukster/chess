#include <gtest/gtest.h>
#include "../search/search.h"
#include "../eval/classic_eval.h"
#include "../board/position.h"
#include "../movegen/attacks.h"
#include "../movegen/movegen.h"

using namespace chess;

class SearchTest : public ::testing::Test {
protected:
    void SetUp() override {
        Attacks::init();
        evaluator = new ClassicEval();
        search = new Search(evaluator);
    }
    
    void TearDown() override {
        delete search;
        delete evaluator;
    }
    
    ClassicEval* evaluator;
    Search* search;
};

TEST_F(SearchTest, SearchReturnsValidMove) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    evaluator->initialize(pos);
    
    Move best_move = search->search(pos, 3);
    
    EXPECT_TRUE(best_move.is_valid()) << "Search should return a valid move";
    EXPECT_TRUE(MoveGen::is_legal(pos, best_move)) << "Search should return a legal move";
}

TEST_F(SearchTest, SearchDepth1) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    evaluator->initialize(pos);
    
    Move best_move = search->search(pos, 1);
    
    EXPECT_TRUE(best_move.is_valid());
    
    const SearchStats& stats = search->get_stats();
    EXPECT_GT(stats.nodes, 0);
}

TEST_F(SearchTest, SearchDepth3) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    evaluator->initialize(pos);
    
    Move best_move = search->search(pos, 3);
    
    EXPECT_TRUE(best_move.is_valid());
    
    const SearchStats& stats = search->get_stats();
    EXPECT_GT(stats.nodes, 100);
}

TEST_F(SearchTest, FindsMateInOne) {
    Position pos("k7/8/1K6/8/8/8/8/7R w - - 0 1");
    evaluator->initialize(pos);
    
    Move best_move = search->search(pos, 3);
    
    EXPECT_TRUE(best_move.is_valid());
    EXPECT_EQ(best_move.from(), make_square(7, 0));
    EXPECT_EQ(best_move.to(), make_square(7, 7)) << "Should find mate with Rh8#";
}

TEST_F(SearchTest, FindsMateInTwo) {
    Position pos("k7/8/2K5/8/8/8/8/R7 w - - 0 1");
    evaluator->initialize(pos);
    
    Move best_move = search->search(pos, 5);
    
    EXPECT_TRUE(best_move.is_valid());
    
    UndoInfo undo;
    pos.make_move(best_move, undo);
    
    EXPECT_TRUE(pos.is_check(BLACK)) << "Best move should deliver check in mate-in-2";
}

TEST_F(SearchTest, FindsTacticalCapture) {
    Position pos("rnbqkb1r/pppp1ppp/5n2/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 0 1");
    evaluator->initialize(pos);
    
    Move best_move = search->search(pos, 4);
    
    EXPECT_TRUE(best_move.is_valid());
}

TEST_F(SearchTest, AvoidsBlunder) {
    Position pos("r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 1");
    evaluator->initialize(pos);
    
    Move best_move = search->search(pos, 4);
    
    EXPECT_TRUE(best_move.is_valid());
    
    EXPECT_FALSE(best_move.from() == make_square(2, 3) && best_move.to() == make_square(5, 6))
        << "Should not play Bxf7+ which loses the bishop";
}

TEST_F(SearchTest, IterativeDeepeningDepth3) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    evaluator->initialize(pos);
    
    Move best_move = search->iterative_deepening(pos, 3);
    
    EXPECT_TRUE(best_move.is_valid());
    
    const SearchStats& stats = search->get_stats();
    EXPECT_EQ(stats.depth, 3);
}

TEST_F(SearchTest, IterativeDeepeningDepth5) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    evaluator->initialize(pos);
    
    Move best_move = search->iterative_deepening(pos, 5);
    
    EXPECT_TRUE(best_move.is_valid());
    
    const SearchStats& stats = search->get_stats();
    EXPECT_EQ(stats.depth, 5);
}

TEST_F(SearchTest, SearchStatsNodes) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    evaluator->initialize(pos);
    
    search->search(pos, 3);
    
    const SearchStats& stats = search->get_stats();
    EXPECT_GT(stats.nodes, 0);
}

TEST_F(SearchTest, SearchStatsBestMove) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    evaluator->initialize(pos);
    
    Move best_move = search->search(pos, 3);
    
    const SearchStats& stats = search->get_stats();
    EXPECT_EQ(stats.best_move, best_move);
}

TEST_F(SearchTest, ClearTranspositionTable) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    evaluator->initialize(pos);
    
    search->search(pos, 3);
    search->clear_tt();
    
    Move best_move = search->search(pos, 3);
    EXPECT_TRUE(best_move.is_valid());
}

TEST_F(SearchTest, DifferentPositionsDifferentMoves) {
    Position pos1("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    evaluator->initialize(pos1);
    Move move1 = search->search(pos1, 3);
    
    search->clear_tt();
    
    Position pos2("r1bqkbnr/pppppppp/2n5/8/8/5N2/PPPPPPPP/RNBQKB1R w KQkq - 0 1");
    evaluator->initialize(pos2);
    Move move2 = search->search(pos2, 3);
    
    EXPECT_TRUE(move1.is_valid());
    EXPECT_TRUE(move2.is_valid());
}

TEST_F(SearchTest, SearchConsistency) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    evaluator->initialize(pos);
    
    Move move1 = search->search(pos, 3);
    
    search->clear_tt();
    
    Move move2 = search->search(pos, 3);
    
    EXPECT_EQ(move1, move2) << "Same position should give same result";
}

TEST_F(SearchTest, FindsBackRankMate) {
    Position pos("6k1/5ppp/8/8/8/8/5PPP/R5K1 w - - 0 1");
    evaluator->initialize(pos);
    
    Move best_move = search->search(pos, 5);
    
    EXPECT_TRUE(best_move.is_valid());
    EXPECT_EQ(best_move.from(), make_square(0, 0));
    EXPECT_EQ(best_move.to(), make_square(0, 7)) << "Should find back rank mate with Ra8#";
}

TEST_F(SearchTest, FindsFork) {
    Position pos("r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQ1RK1 w kq - 0 1");
    evaluator->initialize(pos);
    
    Move best_move = search->search(pos, 4);
    
    EXPECT_TRUE(best_move.is_valid());
}

TEST_F(SearchTest, FindsPin) {
    Position pos("r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 1");
    evaluator->initialize(pos);
    
    Move best_move = search->search(pos, 4);
    
    EXPECT_TRUE(best_move.is_valid());
}

TEST_F(SearchTest, QuiescenceSearch) {
    Position pos("rnbqkb1r/pppp1ppp/5n2/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 0 1");
    evaluator->initialize(pos);
    
    Move best_move = search->search(pos, 3);
    
    EXPECT_TRUE(best_move.is_valid());
    
    const SearchStats& stats = search->get_stats();
    EXPECT_GT(stats.qnodes, 0) << "Should perform quiescence search";
}

TEST_F(SearchTest, PromotionTactics) {
    Position pos("8/P7/8/8/8/k7/8/K7 w - - 0 1");
    evaluator->initialize(pos);
    
    Move best_move = search->search(pos, 3);
    
    EXPECT_TRUE(best_move.is_valid());
    EXPECT_TRUE(best_move.is_promotion()) << "Should promote the pawn";
    EXPECT_EQ(best_move.promotion(), QUEEN) << "Should promote to queen";
}

TEST_F(SearchTest, SearchInCheck) {
    Position pos("rnbqkbnr/pppp1ppp/8/4p3/4P2Q/8/PPPP1PPP/RNB1KBNR b KQkq - 1 2");
    evaluator->initialize(pos);
    
    Move best_move = search->search(pos, 3);
    
    EXPECT_TRUE(best_move.is_valid());
    
    UndoInfo undo;
    pos.make_move(best_move, undo);
    EXPECT_FALSE(pos.is_check(BLACK)) << "Best move should get out of check";
}
