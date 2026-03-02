#include <gtest/gtest.h>
#include "../uci/uci.h"
#include "../board/position.h"
#include "../movegen/attacks.h"
#include <sstream>

using namespace chess;

class UCITest : public ::testing::Test {
protected:
    void SetUp() override {
        Attacks::init();
    }
};

TEST_F(UCITest, MoveToStringSimple) {
    Move move(make_square(4, 1), make_square(4, 3));
    
    std::string move_str;
    move_str += char('a' + file_of(move.from()));
    move_str += char('1' + rank_of(move.from()));
    move_str += char('a' + file_of(move.to()));
    move_str += char('1' + rank_of(move.to()));
    
    EXPECT_EQ(move_str, "e2e4");
}

TEST_F(UCITest, MoveToStringPromotion) {
    Move move(make_square(0, 6), make_square(0, 7), QUEEN);
    
    std::string move_str;
    move_str += char('a' + file_of(move.from()));
    move_str += char('1' + rank_of(move.from()));
    move_str += char('a' + file_of(move.to()));
    move_str += char('1' + rank_of(move.to()));
    
    if (move.is_promotion()) {
        switch (move.promotion()) {
            case QUEEN: move_str += 'q'; break;
            case ROOK: move_str += 'r'; break;
            case BISHOP: move_str += 'b'; break;
            case KNIGHT: move_str += 'n'; break;
            default: break;
        }
    }
    
    EXPECT_EQ(move_str, "a7a8q");
}

TEST_F(UCITest, MoveToStringCastling) {
    Move move(make_square(4, 0), make_square(6, 0), NO_PIECE, false, true);
    
    std::string move_str;
    move_str += char('a' + file_of(move.from()));
    move_str += char('1' + rank_of(move.from()));
    move_str += char('a' + file_of(move.to()));
    move_str += char('1' + rank_of(move.to()));
    
    EXPECT_EQ(move_str, "e1g1");
}

TEST_F(UCITest, ParseMoveSimple) {
    std::string move_str = "e2e4";
    
    int from_file = move_str[0] - 'a';
    int from_rank = move_str[1] - '1';
    int to_file = move_str[2] - 'a';
    int to_rank = move_str[3] - '1';
    
    Square from = make_square(from_file, from_rank);
    Square to = make_square(to_file, to_rank);
    
    EXPECT_EQ(from, make_square(4, 1));
    EXPECT_EQ(to, make_square(4, 3));
}

TEST_F(UCITest, ParseMovePromotion) {
    std::string move_str = "a7a8q";
    
    int from_file = move_str[0] - 'a';
    int from_rank = move_str[1] - '1';
    int to_file = move_str[2] - 'a';
    int to_rank = move_str[3] - '1';
    
    Square from = make_square(from_file, from_rank);
    Square to = make_square(to_file, to_rank);
    
    PieceType promo = NO_PIECE;
    if (move_str.length() == 5) {
        switch (move_str[4]) {
            case 'q': promo = QUEEN; break;
            case 'r': promo = ROOK; break;
            case 'b': promo = BISHOP; break;
            case 'n': promo = KNIGHT; break;
            default: break;
        }
    }
    
    EXPECT_EQ(from, make_square(0, 6));
    EXPECT_EQ(to, make_square(0, 7));
    EXPECT_EQ(promo, QUEEN);
}

TEST_F(UCITest, ParseMoveCastling) {
    std::string move_str = "e1g1";
    
    int from_file = move_str[0] - 'a';
    int from_rank = move_str[1] - '1';
    int to_file = move_str[2] - 'a';
    int to_rank = move_str[3] - '1';
    
    Square from = make_square(from_file, from_rank);
    Square to = make_square(to_file, to_rank);
    
    EXPECT_EQ(from, make_square(4, 0));
    EXPECT_EQ(to, make_square(6, 0));
}

TEST_F(UCITest, FENStartingPosition) {
    std::string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    Position pos(fen);
    
    EXPECT_EQ(pos.to_fen(), fen);
}

TEST_F(UCITest, FENAfterE4) {
    std::string fen = "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1";
    Position pos(fen);
    
    EXPECT_EQ(pos.side_to_move(), BLACK);
    EXPECT_EQ(pos.ep_square(), make_square(4, 2));
}

TEST_F(UCITest, PositionStartpos) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    
    EXPECT_EQ(pos.side_to_move(), WHITE);
    EXPECT_EQ(pos.castling_rights(), ANY_CASTLING);
}

TEST_F(UCITest, PositionStartposMoves) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    
    UndoInfo undo1;
    Move move1(make_square(4, 1), make_square(4, 3));
    pos.make_move(move1, undo1);
    
    EXPECT_EQ(pos.side_to_move(), BLACK);
    
    UndoInfo undo2;
    Move move2(make_square(4, 6), make_square(4, 4));
    pos.make_move(move2, undo2);
    
    EXPECT_EQ(pos.side_to_move(), WHITE);
}

TEST_F(UCITest, PositionFEN) {
    std::string fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
    Position pos(fen);
    
    EXPECT_EQ(pos.to_fen(), fen);
}

TEST_F(UCITest, MoveRoundTrip) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    
    Move original(make_square(4, 1), make_square(4, 3));
    
    std::string move_str;
    move_str += char('a' + file_of(original.from()));
    move_str += char('1' + rank_of(original.from()));
    move_str += char('a' + file_of(original.to()));
    move_str += char('1' + rank_of(original.to()));
    
    int from_file = move_str[0] - 'a';
    int from_rank = move_str[1] - '1';
    int to_file = move_str[2] - 'a';
    int to_rank = move_str[3] - '1';
    
    Move parsed(make_square(from_file, from_rank), make_square(to_file, to_rank));
    
    EXPECT_EQ(original.from(), parsed.from());
    EXPECT_EQ(original.to(), parsed.to());
}

TEST_F(UCITest, PromotionRoundTrip) {
    Move original(make_square(0, 6), make_square(0, 7), QUEEN);
    
    std::string move_str;
    move_str += char('a' + file_of(original.from()));
    move_str += char('1' + rank_of(original.from()));
    move_str += char('a' + file_of(original.to()));
    move_str += char('1' + rank_of(original.to()));
    
    if (original.is_promotion()) {
        switch (original.promotion()) {
            case QUEEN: move_str += 'q'; break;
            case ROOK: move_str += 'r'; break;
            case BISHOP: move_str += 'b'; break;
            case KNIGHT: move_str += 'n'; break;
            default: break;
        }
    }
    
    EXPECT_EQ(move_str, "a7a8q");
}

TEST_F(UCITest, AllPromotionTypes) {
    std::vector<std::pair<PieceType, char>> promotions = {
        {QUEEN, 'q'},
        {ROOK, 'r'},
        {BISHOP, 'b'},
        {KNIGHT, 'n'}
    };
    
    for (const auto& [piece_type, piece_char] : promotions) {
        Move move(make_square(0, 6), make_square(0, 7), piece_type);
        
        std::string move_str;
        move_str += char('a' + file_of(move.from()));
        move_str += char('1' + rank_of(move.from()));
        move_str += char('a' + file_of(move.to()));
        move_str += char('1' + rank_of(move.to()));
        
        if (move.is_promotion()) {
            switch (move.promotion()) {
                case QUEEN: move_str += 'q'; break;
                case ROOK: move_str += 'r'; break;
                case BISHOP: move_str += 'b'; break;
                case KNIGHT: move_str += 'n'; break;
                default: break;
            }
        }
        
        EXPECT_EQ(move_str.back(), piece_char);
    }
}

TEST_F(UCITest, MoveValidation) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    
    Move valid_move(make_square(4, 1), make_square(4, 3));
    EXPECT_TRUE(valid_move.is_valid());
    
    Move invalid_move(make_square(4, 1), make_square(4, 1));
    EXPECT_FALSE(invalid_move.is_valid());
}

TEST_F(UCITest, CastlingMoveFormat) {
    std::vector<std::pair<std::string, std::pair<Square, Square>>> castling_moves = {
        {"e1g1", {make_square(4, 0), make_square(6, 0)}},
        {"e1c1", {make_square(4, 0), make_square(2, 0)}},
        {"e8g8", {make_square(4, 7), make_square(6, 7)}},
        {"e8c8", {make_square(4, 7), make_square(2, 7)}}
    };
    
    for (const auto& [move_str, squares] : castling_moves) {
        int from_file = move_str[0] - 'a';
        int from_rank = move_str[1] - '1';
        int to_file = move_str[2] - 'a';
        int to_rank = move_str[3] - '1';
        
        Square from = make_square(from_file, from_rank);
        Square to = make_square(to_file, to_rank);
        
        EXPECT_EQ(from, squares.first);
        EXPECT_EQ(to, squares.second);
    }
}

TEST_F(UCITest, SquareNotation) {
    std::vector<std::pair<std::string, Square>> squares = {
        {"a1", make_square(0, 0)},
        {"h1", make_square(7, 0)},
        {"a8", make_square(0, 7)},
        {"h8", make_square(7, 7)},
        {"e4", make_square(4, 3)},
        {"d5", make_square(3, 4)}
    };
    
    for (const auto& [notation, square] : squares) {
        int file = notation[0] - 'a';
        int rank = notation[1] - '1';
        Square parsed = make_square(file, rank);
        
        EXPECT_EQ(parsed, square);
    }
}
