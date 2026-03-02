#include "../movegen/movegen.h"
#include "../movegen/attacks.h"
#include "../board/position.h"
#include <iostream>
#include <iomanip>
#include <map>
#include <string>

using namespace chess;

std::string move_to_string(Move move) {
    std::string result;
    result += char('a' + file_of(move.from()));
    result += char('1' + rank_of(move.from()));
    result += char('a' + file_of(move.to()));
    result += char('1' + rank_of(move.to()));
    if (move.promotion() != NO_PIECE) {
        switch (move.promotion()) {
            case QUEEN: result += 'q'; break;
            case ROOK: result += 'r'; break;
            case BISHOP: result += 'b'; break;
            case KNIGHT: result += 'n'; break;
            default: break;
        }
    }
    return result;
}

uint64_t perft(Position& pos, int depth) {
    if (depth == 0) return 1;
    
    std::vector<Move> moves;
    MoveGen::generate_legal(pos, moves);
    
    if (depth == 1) return moves.size();
    
    uint64_t nodes = 0;
    for (const auto& move : moves) {
        UndoInfo undo;
        pos.make_move(move, undo);
        nodes += perft(pos, depth - 1);
        pos.unmake_move(move, undo);
    }
    
    return nodes;
}

void divide(Position& pos, int depth) {
    std::vector<Move> moves;
    MoveGen::generate_legal(pos, moves);
    
    std::map<std::string, uint64_t> results;
    uint64_t total = 0;
    
    for (const auto& move : moves) {
        UndoInfo undo;
        pos.make_move(move, undo);
        uint64_t nodes = perft(pos, depth - 1);
        pos.unmake_move(move, undo);
        
        std::string move_str = move_to_string(move);
        results[move_str] = nodes;
        total += nodes;
    }
    
    for (const auto& [move_str, nodes] : results) {
        std::cout << move_str << ": " << nodes << std::endl;
    }
    std::cout << "\nTotal: " << total << std::endl;
}

int main() {
    Attacks::init();
    
    std::cout << "=== Kiwipete Depth 3 Divide ===" << std::endl;
    Position kiwipete("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    divide(kiwipete, 3);
    
    std::cout << "\n\n=== Position 4 Depth 4 Divide ===" << std::endl;
    Position pos4("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    divide(pos4, 4);
    
    return 0;
}
