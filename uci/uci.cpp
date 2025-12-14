#include "uci.h"
#include "../eval/classic_eval.h"
#include "../nnue/nnue_eval.h"
#include "../movegen/movegen.h"
#include <iostream>
#include <sstream>
#include <algorithm>

namespace chess {

UCI::UCI() : use_nnue(false), wtime(0), btime(0), winc(0), binc(0),
             movestogo(0), depth_limit(0), nodes_limit(0), mate(0), infinite(false), nnue_file("") {
    // Start with classic eval
    evaluator = new ClassicEval();
    search = new Search(evaluator);
    pos.evaluator = evaluator;
    
    pos.from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    evaluator->initialize(pos);
}

UCI::~UCI() {
    delete search;
    delete evaluator;
}

void UCI::loop() {
    std::string line;
    
    std::cout << "id name ChessEngine" << std::endl;
    std::cout << "id author NNUE Engine" << std::endl;
    std::cout << "option name Use NNUE type check default false" << std::endl;
    std::cout << "option name EvalFile type string default <empty>" << std::endl;
    std::cout << "uciok" << std::endl;
    
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        std::string command;
        iss >> command;
        
        if (command == "quit" || command == "exit") {
            break;
        } else if (command == "uci") {
            std::cout << "id name ChessEngine" << std::endl;
            std::cout << "id author NNUE Engine" << std::endl;
            std::cout << "option name Use NNUE type check default false" << std::endl;
            std::cout << "option name EvalFile type string default <empty>" << std::endl;
            std::cout << "uciok" << std::endl;
        } else if (command == "isready") {
            std::cout << "readyok" << std::endl;
        } else if (command == "ucinewgame") {
            pos.from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
            evaluator->initialize(pos);
            search->clear_tt();
        } else if (command == "position") {
            handle_position(line);
        } else if (command == "go") {
            handle_go(line);
        } else if (command == "stop") {
            // Stop search (handled in search)
        } else if (command == "setoption") {
            handle_setoption(line);
        } else if (command == "perft") {
            std::string depth_str;
            iss >> depth_str;
            int depth = std::stoi(depth_str);
            uint64_t nodes = MoveGen::perft(pos, depth);
            std::cout << "perft " << depth << " " << nodes << std::endl;
        }
    }
}

void UCI::handle_position(const std::string& line) {
    std::istringstream iss(line);
    std::string token;
    iss >> token; // "position"
    
    iss >> token;
    if (token == "startpos") {
        pos.from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        evaluator->initialize(pos);
    } else if (token == "fen") {
        std::string fen;
        for (int i = 0; i < 6; i++) {
            std::string part;
            iss >> part;
            if (i > 0) fen += " ";
            fen += part;
        }
        pos.from_fen(fen);
        evaluator->initialize(pos);
    }
    
    iss >> token; // "moves"
    if (token == "moves") {
        while (iss >> token) {
            Move move = parse_move(token);
            if (move.is_valid()) {
                UndoInfo undo;
                pos.make_move(move, undo);
            }
        }
    }
}

void UCI::handle_go(const std::string& line) {
    std::istringstream iss(line);
    std::string token;
    iss >> token; // "go"
    
    wtime = btime = winc = binc = 0;
    movestogo = depth_limit = nodes_limit = mate = 0;
    infinite = false;
    
    while (iss >> token) {
        if (token == "wtime") iss >> wtime;
        else if (token == "btime") iss >> btime;
        else if (token == "winc") iss >> winc;
        else if (token == "binc") iss >> binc;
        else if (token == "movestogo") iss >> movestogo;
        else if (token == "depth") iss >> depth_limit;
        else if (token == "nodes") iss >> nodes_limit;
        else if (token == "mate") iss >> mate;
        else if (token == "infinite") infinite = true;
    }
    
    // Calculate time limit
    int time_limit = 0;
    if (pos.side_to_move() == WHITE && wtime > 0) {
        time_limit = wtime / 20; // Use 5% of time
    } else if (pos.side_to_move() == BLACK && btime > 0) {
        time_limit = btime / 20;
    }
    
    // Perform search
    int max_depth = depth_limit > 0 ? depth_limit : 10;
    
    Move best_move = search->iterative_deepening(pos, max_depth, time_limit);
    
    // Output search stats
    const SearchStats& stats = search->get_stats();
    std::cout << "info depth " << stats.depth << " nodes " << stats.nodes 
              << " score cp " << stats.best_score << std::endl;
    
    // Output best move
    std::cout << "bestmove " << move_to_string(best_move) << std::endl;
    std::cout.flush();
}

void UCI::handle_setoption(const std::string& line) {
    std::istringstream iss(line);
    std::string token;
    iss >> token; // "setoption"
    iss >> token; // "name"
    
    std::string name;
    while (iss >> token && token != "value") {
        if (!name.empty()) name += " ";
        name += token;
    }
    
    if (name == "Use NNUE") {
        std::string value;
        iss >> value;
        if (value == "true") {
            delete evaluator;
            delete search;
            NnueEval* nnue = new NnueEval();
            
            // Load network file if specified
            if (!nnue_file.empty()) {
                nnue->load_network(nnue_file);
            }
            
            evaluator = nnue;
            search = new Search(evaluator);
            pos.evaluator = evaluator;
            evaluator->initialize(pos);
            use_nnue = true;
        } else {
            delete evaluator;
            delete search;
            evaluator = new ClassicEval();
            search = new Search(evaluator);
            pos.evaluator = evaluator;
            evaluator->initialize(pos);
            use_nnue = false;
        }
    } else if (name == "EvalFile") {
        std::string filename;
        iss >> filename;
        nnue_file = filename;
        
        // If NNUE is already active, reload the network
        if (use_nnue && evaluator) {
            NnueEval* nnue = dynamic_cast<NnueEval*>(evaluator);
            if (nnue) {
                nnue->load_network(filename);
                evaluator->initialize(pos);  // Reinitialize with new weights
            }
        }
    }
}

Move UCI::parse_move(const std::string& move_str) {
    if (move_str.length() < 4) return MOVE_NONE;
    
    int from_file = move_str[0] - 'a';
    int from_rank = move_str[1] - '1';
    int to_file = move_str[2] - 'a';
    int to_rank = move_str[3] - '1';
    
    if (from_file < 0 || from_file >= 8 || from_rank < 0 || from_rank >= 8 ||
        to_file < 0 || to_file >= 8 || to_rank < 0 || to_rank >= 8) {
        return MOVE_NONE;
    }
    
    Square from = make_square(from_file, from_rank);
    Square to = make_square(to_file, to_rank);
    
    PieceType promo = NO_PIECE;
    if (move_str.length() >= 5) {
        switch (move_str[4]) {
            case 'n': promo = KNIGHT; break;
            case 'b': promo = BISHOP; break;
            case 'r': promo = ROOK; break;
            case 'q': promo = QUEEN; break;
        }
    }
    
    bool capture = pos.piece_on(to) != NO_PIECE_PIECE;
    bool special = false;
    
    // Check for castling
    Piece pc = pos.piece_on(from);
    if (type_of(pc) == KING && abs(file_of(to) - file_of(from)) == 2) {
        special = true;
    }
    
    // Check for en passant
    if (type_of(pc) == PAWN && to == pos.ep_square()) {
        special = true;
        capture = true;
    }
    
    return Move(from, to, promo, capture, special);
}

std::string UCI::move_to_string(Move move) {
    if (!move.is_valid()) return "0000";
    
    Square from = move.from();
    Square to = move.to();
    
    std::string result;
    result += char('a' + file_of(from));
    result += char('1' + rank_of(from));
    result += char('a' + file_of(to));
    result += char('1' + rank_of(to));
    
    if (move.is_promotion()) {
        switch (move.promotion()) {
            case KNIGHT: result += 'n'; break;
            case BISHOP: result += 'b'; break;
            case ROOK: result += 'r'; break;
            case QUEEN: result += 'q'; break;
            default: break;
        }
    }
    
    return result;
}

} // namespace chess

