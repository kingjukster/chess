#include <gtest/gtest.h>
#include "../book/polyglot.h"
#include "../board/position.h"

using namespace chess;

class BookTest : public ::testing::Test {
protected:
    void SetUp() override {
        book = new PolyglotBook();
    }
    
    void TearDown() override {
        delete book;
    }
    
    PolyglotBook* book;
};

TEST_F(BookTest, InitialState) {
    EXPECT_FALSE(book->is_loaded());
    EXPECT_EQ(book->get_book_depth(), 0);
}

TEST_F(BookTest, LoadNonExistentFile) {
    EXPECT_FALSE(book->load("nonexistent_book.bin"));
    EXPECT_FALSE(book->is_loaded());
}

TEST_F(BookTest, VarietySettings) {
    book->set_variety(0);
    book->set_variety(50);
    book->set_variety(100);
    // No crash = success
}

TEST_F(BookTest, DepthSettings) {
    book->set_max_depth(10);
    book->set_max_depth(20);
    book->set_max_depth(100);
    // No crash = success
}

TEST_F(BookTest, ProbeWithoutLoading) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    
    std::vector<BookMove> moves = book->probe(pos);
    EXPECT_TRUE(moves.empty());
    
    Move move = book->get_move(pos);
    EXPECT_EQ(move, MOVE_NONE);
}

TEST_F(BookTest, BookMoveStructure) {
    Move test_move(make_square(4, 1), make_square(4, 3));
    BookMove bm(test_move, 100);
    
    EXPECT_EQ(bm.move.from(), make_square(4, 1));
    EXPECT_EQ(bm.move.to(), make_square(4, 3));
    EXPECT_EQ(bm.weight, 100);
}

TEST_F(BookTest, CloseWithoutLoading) {
    book->close();
    EXPECT_FALSE(book->is_loaded());
}

// Note: Full integration tests would require an actual Polyglot book file
// These tests verify the API works correctly without a book loaded
