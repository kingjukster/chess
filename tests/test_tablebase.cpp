#include <gtest/gtest.h>
#include "../tablebase/syzygy.h"
#include "../board/position.h"

using namespace chess;

class TablebaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        tb = new SyzygyTablebase();
    }
    
    void TearDown() override {
        delete tb;
    }
    
    SyzygyTablebase* tb;
};

TEST_F(TablebaseTest, InitialState) {
    EXPECT_FALSE(tb->is_initialized());
    EXPECT_EQ(tb->max_pieces(), 0);
    EXPECT_EQ(tb->get_hits(), 0);
}

TEST_F(TablebaseTest, InitWithInvalidPath) {
    EXPECT_FALSE(tb->init(""));
    EXPECT_FALSE(tb->init("/nonexistent/path"));
    EXPECT_FALSE(tb->is_initialized());
}

TEST_F(TablebaseTest, ProbeDepthSettings) {
    tb->set_probe_depth(1);
    EXPECT_EQ(tb->get_probe_depth(), 1);
    
    tb->set_probe_depth(5);
    EXPECT_EQ(tb->get_probe_depth(), 5);
}

TEST_F(TablebaseTest, ProbeLimitSettings) {
    tb->set_probe_limit(3);
    tb->set_probe_limit(5);
    tb->set_probe_limit(7);
    // No crash = success
}

TEST_F(TablebaseTest, ProbeWithoutInit) {
    Position pos("8/8/8/8/8/4k3/8/4K3 w - - 0 1");
    
    TBResult wdl = tb->probe_wdl(pos);
    EXPECT_EQ(wdl, TB_FAILED);
    
    int dtz = tb->probe_dtz(pos);
    EXPECT_EQ(dtz, 0);
    
    TBResult result_wdl;
    int result_dtz;
    Move move = tb->probe_root(pos, result_wdl, result_dtz);
    EXPECT_EQ(move, MOVE_NONE);
    EXPECT_EQ(result_wdl, TB_FAILED);
}

TEST_F(TablebaseTest, ProbeSearchWithoutInit) {
    Position pos("8/8/8/8/8/4k3/8/4K3 w - - 0 1");
    
    int score;
    bool result = tb->probe_search(pos, score);
    EXPECT_FALSE(result);
}

TEST_F(TablebaseTest, ProbeRootMovesWithoutInit) {
    Position pos("8/8/8/8/8/4k3/8/4K3 w - - 0 1");
    
    std::vector<TBMove> moves = tb->probe_root_moves(pos);
    EXPECT_TRUE(moves.empty());
}

TEST_F(TablebaseTest, TBResultEnumValues) {
    // Verify enum values match Fathom library
    EXPECT_EQ(TB_LOSS, 0);
    EXPECT_EQ(TB_BLESSED_LOSS, 1);
    EXPECT_EQ(TB_DRAW, 2);
    EXPECT_EQ(TB_CURSED_WIN, 3);
    EXPECT_EQ(TB_WIN, 4);
    EXPECT_EQ(TB_FAILED, 5);
}

TEST_F(TablebaseTest, TBMoveStructure) {
    Move test_move(make_square(4, 0), make_square(4, 1));
    TBMove tbm(test_move, TB_WIN, 10);
    
    EXPECT_EQ(tbm.move.from(), make_square(4, 0));
    EXPECT_EQ(tbm.move.to(), make_square(4, 1));
    EXPECT_EQ(tbm.wdl, TB_WIN);
    EXPECT_EQ(tbm.dtz, 10);
}

TEST_F(TablebaseTest, ResetStats) {
    tb->reset_stats();
    EXPECT_EQ(tb->get_hits(), 0);
}

TEST_F(TablebaseTest, CloseWithoutInit) {
    tb->close();
    EXPECT_FALSE(tb->is_initialized());
}

// Known endgame positions for testing (when tablebases are available)
class TablebaseEndgameTest : public ::testing::Test {
protected:
    void SetUp() override {
        tb = new SyzygyTablebase();
        // Try to load tablebases from common locations
        // Tests will be skipped if tablebases not available
        tb_available = tb->init("./syzygy") || 
                      tb->init("../syzygy") ||
                      tb->init("../../syzygy");
    }
    
    void TearDown() override {
        delete tb;
    }
    
    SyzygyTablebase* tb;
    bool tb_available;
};

TEST_F(TablebaseEndgameTest, KQvK_Win) {
    if (!tb_available || tb->max_pieces() < 3) {
        GTEST_SKIP() << "Syzygy tablebases not available";
    }
    
    // KQ vs K - should be a win
    Position pos("8/8/8/4k3/8/8/2Q5/4K3 w - - 0 1");
    
    TBResult wdl = tb->probe_wdl(pos);
    EXPECT_EQ(wdl, TB_WIN);
}

TEST_F(TablebaseEndgameTest, KvK_Draw) {
    if (!tb_available || tb->max_pieces() < 2) {
        GTEST_SKIP() << "Syzygy tablebases not available";
    }
    
    // K vs K - should be a draw
    Position pos("8/8/8/4k3/8/8/8/4K3 w - - 0 1");
    
    TBResult wdl = tb->probe_wdl(pos);
    EXPECT_EQ(wdl, TB_DRAW);
}

TEST_F(TablebaseEndgameTest, KRvK_Win) {
    if (!tb_available || tb->max_pieces() < 3) {
        GTEST_SKIP() << "Syzygy tablebases not available";
    }
    
    // KR vs K - should be a win
    Position pos("8/8/8/4k3/8/8/2R5/4K3 w - - 0 1");
    
    TBResult wdl = tb->probe_wdl(pos);
    EXPECT_EQ(wdl, TB_WIN);
}

// Note: Full integration tests require Syzygy tablebase files
// Download from: https://syzygy-tables.info/
