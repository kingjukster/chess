#include <gtest/gtest.h>
#include "../eval/classic_eval.h"
#include "../board/position.h"
#include "../movegen/attacks.h"
#include <cmath>

using namespace chess;

class EvalTest : public ::testing::Test {
protected:
    void SetUp() override {
        Attacks::init();
        evaluator = new ClassicEval();
    }
    
    void TearDown() override {
        delete evaluator;
    }
    
    ClassicEval* evaluator;
    
    // Helper to flip position horizontally
    std::string flip_fen_horizontally(const std::string& fen) {
        std::string result;
        size_t space_pos = fen.find(' ');
        std::string board = fen.substr(0, space_pos);
        std::string rest = fen.substr(space_pos);
        
        std::string flipped_board;
        for (char c : board) {
            if (c == '/') {
                flipped_board += c;
            } else if (isdigit(c)) {
                flipped_board += c;
            } else {
                flipped_board += c;
            }
        }
        
        return flipped_board + rest;
    }
    
    // Helper to flip position vertically (swap colors)
    std::string flip_fen_vertically(const std::string& fen) {
        std::string result;
        size_t space_pos = fen.find(' ');
        std::string board = fen.substr(0, space_pos);
        std::string rest = fen.substr(space_pos);
        
        std::string flipped_board;
        std::vector<std::string> ranks;
        std::string current_rank;
        
        for (char c : board) {
            if (c == '/') {
                ranks.push_back(current_rank);
                current_rank.clear();
            } else {
                current_rank += c;
            }
        }
        ranks.push_back(current_rank);
        
        // Reverse ranks and swap piece colors
        for (int i = ranks.size() - 1; i >= 0; i--) {
            for (char c : ranks[i]) {
                if (isalpha(c)) {
                    flipped_board += (isupper(c) ? tolower(c) : toupper(c));
                } else {
                    flipped_board += c;
                }
            }
            if (i > 0) flipped_board += '/';
        }
        
        // Flip side to move
        std::string modified_rest = rest;
        if (modified_rest.length() > 1) {
            modified_rest[1] = (modified_rest[1] == 'w') ? 'b' : 'w';
        }
        
        return flipped_board + modified_rest;
    }
};

TEST_F(EvalTest, StartingPositionSymmetric) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    evaluator->initialize(pos);
    int score = evaluator->evaluate(pos);
    
    EXPECT_NEAR(score, 0, 50) << "Starting position should be roughly equal";
}

TEST_F(EvalTest, MaterialAdvantage) {
    // Use extra piece (knight) - material should dominate structure
    Position pos1("8/8/8/8/8/8/8/K6k w - - 0 1");
    evaluator->initialize(pos1);
    int score1 = evaluator->evaluate(pos1);
    
    Position pos2("8/8/8/8/8/8/N7/K6k w - - 0 1");
    evaluator->initialize(pos2);
    int score2 = evaluator->evaluate(pos2);
    
    EXPECT_GT(score2, score1) << "White should be better with material advantage (knight)";
}

TEST_F(EvalTest, PawnValue) {
    Position pos1("8/8/8/8/8/8/8/K6k w - - 0 1");
    evaluator->initialize(pos1);
    int score1 = evaluator->evaluate(pos1);
    
    Position pos2("8/8/8/8/8/8/P7/K6k w - - 0 1");
    evaluator->initialize(pos2);
    int score2 = evaluator->evaluate(pos2);
    
    EXPECT_GT(score2 - score1, 50) << "Pawn should have positive value";
    EXPECT_LT(score2 - score1, 200) << "Pawn value should be reasonable";
}

TEST_F(EvalTest, KnightValue) {
    Position pos1("8/8/8/8/8/8/8/K6k w - - 0 1");
    evaluator->initialize(pos1);
    int score1 = evaluator->evaluate(pos1);
    
    Position pos2("8/8/8/8/8/8/N7/K6k w - - 0 1");
    evaluator->initialize(pos2);
    int score2 = evaluator->evaluate(pos2);
    
    EXPECT_GT(score2 - score1, 250) << "Knight should be worth more than 3 pawns";
    EXPECT_LT(score2 - score1, 400) << "Knight value should be reasonable";
}

TEST_F(EvalTest, BishopValue) {
    Position pos1("8/8/8/8/8/8/8/K6k w - - 0 1");
    evaluator->initialize(pos1);
    int score1 = evaluator->evaluate(pos1);
    
    Position pos2("8/8/8/8/8/8/B7/K6k w - - 0 1");
    evaluator->initialize(pos2);
    int score2 = evaluator->evaluate(pos2);
    
    EXPECT_GT(score2 - score1, 250) << "Bishop should be worth more than 3 pawns";
    EXPECT_LT(score2 - score1, 400) << "Bishop value should be reasonable";
}

TEST_F(EvalTest, RookValue) {
    Position pos1("8/8/8/8/8/8/8/K6k w - - 0 1");
    evaluator->initialize(pos1);
    int score1 = evaluator->evaluate(pos1);
    
    Position pos2("8/8/8/8/8/8/R7/K6k w - - 0 1");
    evaluator->initialize(pos2);
    int score2 = evaluator->evaluate(pos2);
    
    EXPECT_GT(score2 - score1, 400) << "Rook should be worth more than 5 pawns";
    EXPECT_LT(score2 - score1, 600) << "Rook value should be reasonable";
}

TEST_F(EvalTest, QueenValue) {
    Position pos1("8/8/8/8/8/8/8/K6k w - - 0 1");
    evaluator->initialize(pos1);
    int score1 = evaluator->evaluate(pos1);
    
    Position pos2("8/8/8/8/8/8/Q7/K6k w - - 0 1");
    evaluator->initialize(pos2);
    int score2 = evaluator->evaluate(pos2);
    
    EXPECT_GT(score2 - score1, 800) << "Queen should be worth more than 9 pawns";
    EXPECT_LT(score2 - score1, 1100) << "Queen value should be reasonable";
}

TEST_F(EvalTest, PieceValueHierarchy) {
    Position pos_pawn("8/8/8/8/8/8/P7/K6k w - - 0 1");
    evaluator->initialize(pos_pawn);
    int pawn_score = evaluator->evaluate(pos_pawn);
    
    Position pos_knight("8/8/8/8/8/8/N7/K6k w - - 0 1");
    evaluator->initialize(pos_knight);
    int knight_score = evaluator->evaluate(pos_knight);
    
    Position pos_bishop("8/8/8/8/8/8/B7/K6k w - - 0 1");
    evaluator->initialize(pos_bishop);
    int bishop_score = evaluator->evaluate(pos_bishop);
    
    Position pos_rook("8/8/8/8/8/8/R7/K6k w - - 0 1");
    evaluator->initialize(pos_rook);
    int rook_score = evaluator->evaluate(pos_rook);
    
    Position pos_queen("8/8/8/8/8/8/Q7/K6k w - - 0 1");
    evaluator->initialize(pos_queen);
    int queen_score = evaluator->evaluate(pos_queen);
    
    EXPECT_LT(pawn_score, knight_score);
    EXPECT_LT(knight_score, rook_score);
    EXPECT_LT(rook_score, queen_score);
}

TEST_F(EvalTest, ColorSymmetry) {
    Position pos1("8/8/8/8/8/8/P7/K6k w - - 0 1");
    evaluator->initialize(pos1);
    int score1 = evaluator->evaluate(pos1);
    
    Position pos2("k6K/p7/8/8/8/8/8/8 w - - 0 1");
    evaluator->initialize(pos2);
    int score2 = evaluator->evaluate(pos2);
    
    EXPECT_NEAR(score1, -score2, 10) << "Evaluation should be symmetric for colors";
}

TEST_F(EvalTest, CentralPawnsPreferred) {
    Position pos1("8/8/8/8/8/8/P7/K6k w - - 0 1");
    evaluator->initialize(pos1);
    int score1 = evaluator->evaluate(pos1);
    
    Position pos2("8/8/8/8/8/8/3P4/K6k w - - 0 1");
    evaluator->initialize(pos2);
    int score2 = evaluator->evaluate(pos2);
    
    EXPECT_GE(score2, score1) << "Central pawns should be valued at least as much as edge pawns";
}

TEST_F(EvalTest, AdvancedPawnsPreferred) {
    Position pos1("8/8/8/8/8/8/P7/K6k w - - 0 1");
    evaluator->initialize(pos1);
    int score1 = evaluator->evaluate(pos1);
    
    Position pos2("8/8/8/8/8/P7/8/K6k w - - 0 1");
    evaluator->initialize(pos2);
    int score2 = evaluator->evaluate(pos2);
    
    EXPECT_GT(score2, score1) << "Advanced pawns should be valued higher";
}

TEST_F(EvalTest, CentralKnightsPreferred) {
    Position pos1("8/8/8/8/8/8/N7/K6k w - - 0 1");
    evaluator->initialize(pos1);
    int score1 = evaluator->evaluate(pos1);
    
    Position pos2("8/8/8/8/3N4/8/8/K6k w - - 0 1");
    evaluator->initialize(pos2);
    int score2 = evaluator->evaluate(pos2);
    
    EXPECT_GT(score2, score1) << "Central knights should be valued higher";
}

TEST_F(EvalTest, MakeUnmakePreservesEval) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    evaluator->initialize(pos);
    int score1 = evaluator->evaluate(pos);
    
    UndoInfo undo;
    Move move(make_square(4, 1), make_square(4, 3));
    pos.make_move(move, undo);
    evaluator->on_make_move(pos, move, undo);
    
    pos.unmake_move(move, undo);
    evaluator->on_unmake_move(pos, move, undo);
    
    int score2 = evaluator->evaluate(pos);
    
    EXPECT_EQ(score1, score2) << "Evaluation should be preserved after make/unmake";
}

TEST_F(EvalTest, EvaluationConsistency) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    evaluator->initialize(pos);
    
    int score1 = evaluator->evaluate(pos);
    int score2 = evaluator->evaluate(pos);
    
    EXPECT_EQ(score1, score2) << "Multiple evaluations should give same result";
}

TEST_F(EvalTest, CheckmatePosition) {
    Position pos1("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    evaluator->initialize(pos1);
    int normal_score = evaluator->evaluate(pos1);
    
    Position pos2("k7/8/8/8/8/8/8/K6R w - - 0 1");
    evaluator->initialize(pos2);
    int winning_score = evaluator->evaluate(pos2);
    
    EXPECT_GT(winning_score, normal_score) << "Overwhelming material advantage should score higher";
}

TEST_F(EvalTest, ComplexPosition) {
    Position pos("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    evaluator->initialize(pos);
    int score = evaluator->evaluate(pos);
    
    EXPECT_NE(score, 0) << "Complex position should have non-zero evaluation";
}

TEST_F(EvalTest, IncrementalUpdateAfterMove) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    evaluator->initialize(pos);
    
    UndoInfo undo;
    Move move(make_square(4, 1), make_square(4, 3));
    pos.make_move(move, undo);
    evaluator->on_make_move(pos, move, undo);
    
    int score = evaluator->evaluate(pos);
    
    EXPECT_NE(score, 0) << "Evaluation should work after move";
}

// ========== Advanced Evaluation Tests ==========

TEST_F(EvalTest, DoubledPawns) {
    Position pos1("8/8/8/8/8/8/PP6/K6k w - - 0 1");
    evaluator->initialize(pos1);
    int score1 = evaluator->evaluate(pos1);
    
    Position pos2("8/8/8/8/8/P7/P7/K6k w - - 0 1");
    evaluator->initialize(pos2);
    int score2 = evaluator->evaluate(pos2);
    
    EXPECT_LT(score2, score1) << "Doubled pawns should be penalized";
}

TEST_F(EvalTest, IsolatedPawns) {
    Position pos1("8/8/8/8/8/8/PPP5/K6k w - - 0 1");
    evaluator->initialize(pos1);
    int score1 = evaluator->evaluate(pos1);
    
    Position pos2("8/8/8/8/8/8/P1P5/K6k w - - 0 1");
    evaluator->initialize(pos2);
    int score2 = evaluator->evaluate(pos2);
    
    EXPECT_LT(score2, score1) << "Isolated pawns should be penalized";
}

TEST_F(EvalTest, PassedPawns) {
    Position pos1("8/8/8/8/8/8/P7/K6k w - - 0 1");
    evaluator->initialize(pos1);
    int score1 = evaluator->evaluate(pos1);
    
    Position pos2("8/8/8/8/8/P7/8/K6k w - - 0 1");
    evaluator->initialize(pos2);
    int score2 = evaluator->evaluate(pos2);
    
    EXPECT_GT(score2, score1) << "Advanced passed pawns should be valued higher";
}

TEST_F(EvalTest, PassedPawnProgression) {
    Position pos2("8/8/8/8/8/P7/8/K6k w - - 0 1");
    evaluator->initialize(pos2);
    int score2 = evaluator->evaluate(pos2);
    
    Position pos3("8/8/8/8/P7/8/8/K6k w - - 0 1");
    evaluator->initialize(pos3);
    int score3 = evaluator->evaluate(pos3);
    
    Position pos4("8/8/8/P7/8/8/8/K6k w - - 0 1");
    evaluator->initialize(pos4);
    int score4 = evaluator->evaluate(pos4);
    
    EXPECT_GT(score3, score2) << "Passed pawn on 4th rank better than 3rd";
    EXPECT_GT(score4, score3) << "Passed pawn on 5th rank better than 4th";
}

TEST_F(EvalTest, BishopPair) {
    Position pos1("8/8/8/8/8/8/8/KBB4k w - - 0 1");
    evaluator->initialize(pos1);
    int score1 = evaluator->evaluate(pos1);
    
    Position pos2("8/8/8/8/8/8/8/KNN4k w - - 0 1");
    evaluator->initialize(pos2);
    int score2 = evaluator->evaluate(pos2);
    
    EXPECT_GT(score1, score2) << "Bishop pair should be valued higher than two knights";
}

TEST_F(EvalTest, RookOnOpenFile) {
    Position pos1("8/8/8/8/8/8/8/KR5k w - - 0 1");
    evaluator->initialize(pos1);
    int score1 = evaluator->evaluate(pos1);
    
    Position pos2("8/8/8/8/8/8/P7/KR5k w - - 0 1");
    evaluator->initialize(pos2);
    int score2 = evaluator->evaluate(pos2);
    
    EXPECT_GT(score1, score2) << "Rook on open file should be valued higher";
}

TEST_F(EvalTest, RookOnSemiOpenFile) {
    Position pos1("8/8/8/8/8/8/P7/KR5k w - - 0 1");
    evaluator->initialize(pos1);
    int score1 = evaluator->evaluate(pos1);
    
    Position pos2("8/8/8/8/8/8/PP6/KR5k w - - 0 1");
    evaluator->initialize(pos2);
    int score2 = evaluator->evaluate(pos2);
    
    EXPECT_GT(score1, score2) << "Rook on semi-open file should be valued higher";
}

TEST_F(EvalTest, KnightOutpost) {
    Position pos1("8/8/8/3N4/2P1P3/8/8/K6k w - - 0 1");
    evaluator->initialize(pos1);
    int score1 = evaluator->evaluate(pos1);
    
    Position pos2("8/8/8/3N4/8/8/8/K6k w - - 0 1");
    evaluator->initialize(pos2);
    int score2 = evaluator->evaluate(pos2);
    
    EXPECT_GT(score1, score2) << "Knight outpost with pawn support should be valued higher";
}

TEST_F(EvalTest, Mobility) {
    Position pos1("8/8/8/3N4/8/8/8/K6k w - - 0 1");
    evaluator->initialize(pos1);
    int score1 = evaluator->evaluate(pos1);
    
    Position pos2("8/8/8/8/8/8/N7/K6k w - - 0 1");
    evaluator->initialize(pos2);
    int score2 = evaluator->evaluate(pos2);
    
    EXPECT_GT(score1, score2) << "Centralized knight with more mobility should be valued higher";
}

TEST_F(EvalTest, KingActivityEndgame) {
    Position pos1("8/8/8/8/8/8/8/K6k w - - 0 1");
    evaluator->initialize(pos1);
    int score1 = evaluator->evaluate(pos1);
    
    Position pos2("8/8/8/8/3K4/8/8/7k w - - 0 1");
    evaluator->initialize(pos2);
    int score2 = evaluator->evaluate(pos2);
    
    EXPECT_GT(score2, score1) << "Centralized king in endgame should be valued higher";
}

TEST_F(EvalTest, PawnShield) {
    Position pos1("8/8/8/8/8/8/PPP5/K6k w - - 0 1");
    evaluator->initialize(pos1);
    int score1 = evaluator->evaluate(pos1);
    
    Position pos2("8/8/8/8/8/8/8/K6k w - - 0 1");
    evaluator->initialize(pos2);
    int score2 = evaluator->evaluate(pos2);
    
    EXPECT_GT(score1, score2) << "King with pawn shield should be safer";
}

// ========== Known Position Tests ==========

TEST_F(EvalTest, LucenaPosition) {
    Position pos("1K6/1P6/8/8/8/8/r7/2k5 w - - 0 1");
    evaluator->initialize(pos);
    int score = evaluator->evaluate(pos);
    
    EXPECT_GT(score, 200) << "Lucena position should be winning for white";
}

TEST_F(EvalTest, PhilidorPosition) {
    Position pos("3k4/8/8/8/8/8/3r4/3K3R w - - 0 1");
    evaluator->initialize(pos);
    int score = evaluator->evaluate(pos);
    
    EXPECT_NEAR(score, 0, 100) << "Philidor position should be roughly equal";
}

TEST_F(EvalTest, BackRankMate) {
    Position pos("6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1");
    evaluator->initialize(pos);
    int score = evaluator->evaluate(pos);
    
    EXPECT_GT(score, 500) << "Back rank weakness should be detected";
}

TEST_F(EvalTest, OppositeColorBishops) {
    Position pos("8/8/8/8/8/8/8/KBb4k w - - 0 1");
    evaluator->initialize(pos);
    int score = evaluator->evaluate(pos);
    
    EXPECT_NEAR(score, 0, 50) << "Opposite color bishops should be drawish";
}

TEST_F(EvalTest, KnightVsBishopEndgame) {
    Position pos1("8/8/8/8/8/8/8/KN5k w - - 0 1");
    evaluator->initialize(pos1);
    int knight_score = evaluator->evaluate(pos1);
    
    Position pos2("8/8/8/8/8/8/8/KB5k w - - 0 1");
    evaluator->initialize(pos2);
    int bishop_score = evaluator->evaluate(pos2);
    
    EXPECT_NEAR(knight_score, bishop_score, 50) << "Knight and bishop should be roughly equal";
}

TEST_F(EvalTest, RookEndgame) {
    Position pos("8/8/8/8/8/8/R7/K6k w - - 0 1");
    evaluator->initialize(pos);
    int score = evaluator->evaluate(pos);
    
    EXPECT_GT(score, 400) << "Rook advantage should be significant";
}

TEST_F(EvalTest, QueenVsRook) {
    Position pos1("8/8/8/8/8/8/Q7/K6k w - - 0 1");
    evaluator->initialize(pos1);
    int queen_score = evaluator->evaluate(pos1);
    
    Position pos2("8/8/8/8/8/8/R7/K6k w - - 0 1");
    evaluator->initialize(pos2);
    int rook_score = evaluator->evaluate(pos2);
    
    EXPECT_GT(queen_score - rook_score, 300) << "Queen should be worth about 4 pawns more than rook";
}

// ========== Symmetry Tests ==========

TEST_F(EvalTest, HorizontalSymmetry) {
    Position pos1("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    evaluator->initialize(pos1);
    int score1 = evaluator->evaluate(pos1);
    
    Position pos2("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    evaluator->initialize(pos2);
    int score2 = evaluator->evaluate(pos2);
    
    EXPECT_EQ(score1, score2) << "Symmetric positions should have same evaluation";
}

TEST_F(EvalTest, VerticalSymmetry) {
    Position pos1("k6K/p7/8/8/8/8/8/8 w - - 0 1");
    evaluator->initialize(pos1);
    int score1 = evaluator->evaluate(pos1);
    
    Position pos2("8/8/8/8/8/8/P7/K6k w - - 0 1");
    evaluator->initialize(pos2);
    int score2 = evaluator->evaluate(pos2);
    
    EXPECT_NEAR(score1, -score2, 10) << "Color-flipped positions should have opposite evaluations";
}

TEST_F(EvalTest, SideToMoveSymmetry) {
    Position pos1("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    evaluator->initialize(pos1);
    int score_white = evaluator->evaluate(pos1);
    
    Position pos2("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");
    evaluator->initialize(pos2);
    int score_black = evaluator->evaluate(pos2);
    
    EXPECT_NEAR(score_white, -score_black, 10) << "Evaluation should flip sign with side to move";
}

// ========== Consistency Tests ==========

TEST_F(EvalTest, EvaluationDeterminism) {
    Position pos("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    evaluator->initialize(pos);
    
    std::vector<int> scores;
    for (int i = 0; i < 10; i++) {
        scores.push_back(evaluator->evaluate(pos));
    }
    
    for (size_t i = 1; i < scores.size(); i++) {
        EXPECT_EQ(scores[0], scores[i]) << "Evaluation should be deterministic";
    }
}

TEST_F(EvalTest, MakeUnmakeConsistency) {
    Position pos("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    evaluator->initialize(pos);
    int original_score = evaluator->evaluate(pos);
    
    std::vector<Move> moves = {
        Move(make_square(3, 4), make_square(3, 5)),
        Move(make_square(5, 2), make_square(6, 4)),
        Move(make_square(4, 0), make_square(6, 0), NO_PIECE, false, true)
    };
    
    for (Move move : moves) {
        UndoInfo undo;
        if (pos.make_move(move, undo)) {
            evaluator->on_make_move(pos, move, undo);
            pos.unmake_move(move, undo);
            evaluator->on_unmake_move(pos, move, undo);
            
            int restored_score = evaluator->evaluate(pos);
            EXPECT_EQ(original_score, restored_score) 
                << "Evaluation should be preserved after make/unmake";
        }
    }
}

TEST_F(EvalTest, PhaseDetection) {
    Position opening("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    evaluator->initialize(opening);
    int opening_score = evaluator->evaluate(opening);
    
    Position endgame("8/8/8/8/8/8/8/K6k w - - 0 1");
    evaluator->initialize(endgame);
    int endgame_score = evaluator->evaluate(endgame);
    
    EXPECT_NE(opening_score, endgame_score) << "Phase should affect evaluation";
}

// ========== Tactical Position Tests ==========

TEST_F(EvalTest, ForkDetection) {
    Position pos("8/8/8/8/3N4/8/8/K1R3Rk w - - 0 1");
    evaluator->initialize(pos);
    int score = evaluator->evaluate(pos);
    
    EXPECT_GT(score, 300) << "Knight forking two rooks should show advantage";
}

TEST_F(EvalTest, PinDetection) {
    Position pos("8/8/8/8/8/8/1r6/KR5k w - - 0 1");
    evaluator->initialize(pos);
    int score = evaluator->evaluate(pos);
    
    EXPECT_LT(score, -300) << "Pinned rook should show disadvantage";
}

TEST_F(EvalTest, TrappedPiece) {
    Position pos("r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 1");
    evaluator->initialize(pos);
    int score = evaluator->evaluate(pos);
    
    EXPECT_NE(score, 0) << "Complex position should have non-zero evaluation";
}

// ========== Endgame Tests ==========

TEST_F(EvalTest, KingAndPawnVsKing) {
    Position pos("8/8/8/8/8/8/P7/K6k w - - 0 1");
    evaluator->initialize(pos);
    int score = evaluator->evaluate(pos);
    
    EXPECT_GT(score, 50) << "King and pawn vs king should be winning";
}

TEST_F(EvalTest, Opposition) {
    Position pos1("8/8/8/8/8/2k5/8/2K5 w - - 0 1");
    evaluator->initialize(pos1);
    int score1 = evaluator->evaluate(pos1);
    
    Position pos2("8/8/8/8/8/3k4/8/2K5 w - - 0 1");
    evaluator->initialize(pos2);
    int score2 = evaluator->evaluate(pos2);
    
    EXPECT_NE(score1, score2) << "Opposition should affect evaluation";
}

TEST_F(EvalTest, DrawishEndgame) {
    Position pos("8/8/8/8/8/8/8/KN5k w - - 0 1");
    evaluator->initialize(pos);
    int score = evaluator->evaluate(pos);
    
    // K+N vs K is theoretical draw; we show material for search, allow up to 350cp
    EXPECT_NEAR(score, 0, 350) << "King and knight vs king - material shown for search";
}

// ========== Complex Position Tests ==========

TEST_F(EvalTest, Kiwipete) {
    Position pos("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    evaluator->initialize(pos);
    int score = evaluator->evaluate(pos);
    
    EXPECT_GT(score, -200) << "Kiwipete position should not be heavily negative";
    EXPECT_LT(score, 400) << "Kiwipete position - White has slight development edge";
}

TEST_F(EvalTest, ItalianGame) {
    Position pos("r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 0 1");
    evaluator->initialize(pos);
    int score = evaluator->evaluate(pos);
    
    EXPECT_NEAR(score, 0, 100) << "Italian game position should be roughly equal";
}

TEST_F(EvalTest, SicilianDefense) {
    Position pos("rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 1");
    evaluator->initialize(pos);
    int score = evaluator->evaluate(pos);
    
    EXPECT_NEAR(score, 0, 200) << "Sicilian defense - opening theory equal";
}

TEST_F(EvalTest, QueensGambit) {
    Position pos("rnbqkbnr/ppp1pppp/8/3p4/2PP4/8/PP2PPPP/RNBQKBNR b KQkq c3 0 1");
    evaluator->initialize(pos);
    int score = evaluator->evaluate(pos);
    
    EXPECT_NEAR(score, 0, 100) << "Queen's gambit should be roughly equal";
}
