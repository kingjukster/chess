#include "movegen/movegen.h"
#include "movegen/attacks.h"
#include "board/position.h"
#include <iostream>
#include <iomanip>
#include <string>

using namespace chess;

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

std::string move_to_string(Move move) {
    std::string result;
    Square from = move.from();
    Square to = move.to();
    
    result += char('a' + file_of(from));
    result += char('1' + rank_of(from));
    result += char('a' + file_of(to));
    result += char('1' + rank_of(to));
    
    if (move.is_promotion()) {
        PieceType promo = move.promotion();
        if (promo == QUEEN) result += 'q';
        else if (promo == ROOK) result += 'r';
        else if (promo == BISHOP) result += 'b';
        else if (promo == KNIGHT) result += 'n';
    }
    
    return result;
}

void divide(Position& pos, int depth) {
    std::vector<Move> moves;
    MoveGen::generate_legal(pos, moves);
    
    uint64_t total = 0;
    for (const auto& move : moves) {
        UndoInfo undo;
        pos.make_move(move, undo);
        uint64_t nodes = perft_divide(pos, depth - 1);
        pos.unmake_move(move, undo);
        
        std::cout << move_to_string(move) << ": " << nodes << std::endl;
        total += nodes;
    }
    
    std::cout << "\nTotal: " << total << std::endl;
}

int main(int argc, char* argv[]) {
    Attacks::init();
    
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <FEN> <depth>" << std::endl;
        return 1;
    }
    
    std::string fen = argv[1];
    int depth = std::stoi(argv[2]);
    
    Position pos(fen);
    std::cout << "Position: " << fen << std::endl;
    std::cout << "Depth: " << depth << std::endl;
    std::cout << "\nDivide:\n" << std::endl;
    
    divide(pos, depth);
    
    return 0;
}
