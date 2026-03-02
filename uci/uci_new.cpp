#include "uci_new.h"
#include "../eval/classic_eval.h"
#include "../nnue/nnue_eval.h"
#include "../movegen/movegen.h"
#include "../simd/simd.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <iomanip>

namespace chess {

UCINew::UCINew() 
    : use_nnue_(false), threads_(1), hash_size_mb_(128), 
      multi_pv_(1), ponder_(false), searching_(false) {
    
    // Initialize evaluator
    evaluator_ = new ClassicEval();
    
    // Initialize transposition table
    tt_ = std::make_unique<TranspositionTable>();
    tt_->resize(hash_size_mb_);
    
    // Initialize thread pool
    pool_ = std::make_unique<ThreadPool>(evaluator_, tt_.get());
    pool_->set_threads(threads_);
    
    // Set up starting position
    pos_.from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    pos_.evaluator = evaluator_;
    evaluator_->initialize(pos_);
}

UCINew::~UCINew() {
    if (searching_) {
        pool_->stop_search();
        pool_->wait_for_search();
    }
    
    if (search_thread_ && search_thread_->joinable()) {
        search_thread_->join();
    }
    
    delete evaluator_;
}

void UCINew::loop() {
    std::string line;
    
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        std::string command;
        iss >> command;
        
        if (command == "quit" || command == "exit") {
            if (searching_) {
                pool_->stop_search();
                pool_->wait_for_search();
            }
            break;
        }
        else if (command == "uci") {
            handle_uci();
        }
        else if (command == "isready") {
            handle_isready();
        }
        else if (command == "ucinewgame") {
            handle_ucinewgame();
        }
        else if (command == "position") {
            handle_position(line);
        }
        else if (command == "go") {
            handle_go(line);
        }
        else if (command == "stop") {
            handle_stop();
        }
        else if (command == "ponderhit") {
            handle_ponderhit();
        }
        else if (command == "setoption") {
            handle_setoption(line);
        }
        else if (command == "bench") {
            handle_bench(line);
        }
        else if (command == "perft") {
            handle_perft(line);
        }
        else if (command == "d") {
            handle_d();
        }
        else if (command == "eval") {
            handle_eval();
        }
        else if (command == "compiler") {
            handle_compiler();
        }
    }
}

void UCINew::handle_uci() {
    std::cout << "id name ChessEngine MT" << std::endl;
    std::cout << "id author Horne" << std::endl;
    print_options();
    std::cout << "uciok" << std::endl;
}

void UCINew::print_options() {
    std::cout << "option name Threads type spin default 1 min 1 max 256" << std::endl;
    std::cout << "option name Hash type spin default 128 min 1 max 32768" << std::endl;
    std::cout << "option name Clear Hash type button" << std::endl;
    std::cout << "option name Ponder type check default false" << std::endl;
    std::cout << "option name MultiPV type spin default 1 min 1 max 256" << std::endl;
    std::cout << "option name Use NNUE type check default false" << std::endl;
    std::cout << "option name EvalFile type string default <empty>" << std::endl;
}

void UCINew::handle_isready() {
    std::cout << "readyok" << std::endl;
}

void UCINew::handle_ucinewgame() {
    if (searching_) {
        pool_->stop_search();
        pool_->wait_for_search();
    }
    
    pos_.from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    evaluator_->initialize(pos_);
    tt_->clear();
}

void UCINew::handle_position(const std::string& line) {
    std::istringstream iss(line);
    std::string token;
    iss >> token; // "position"
    
    iss >> token;
    if (token == "startpos") {
        pos_.from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        evaluator_->initialize(pos_);
    }
    else if (token == "fen") {
        std::string fen;
        for (int i = 0; i < 6; i++) {
            std::string part;
            if (!(iss >> part)) break;
            if (i > 0) fen += " ";
            fen += part;
        }
        pos_.from_fen(fen);
        evaluator_->initialize(pos_);
    }
    
    iss >> token;
    if (token == "moves") {
        while (iss >> token) {
            Move move = parse_move(token);
            if (move.is_valid()) {
                UndoInfo undo;
                pos_.make_move(move, undo);
            }
        }
    }
}

void UCINew::handle_go(const std::string& line) {
    std::istringstream iss(line);
    std::string token;
    iss >> token; // "go"
    
    SearchLimits limits;
    
    while (iss >> token) {
        if (token == "wtime") {
            iss >> limits.time[WHITE];
        }
        else if (token == "btime") {
            iss >> limits.time[BLACK];
        }
        else if (token == "winc") {
            iss >> limits.inc[WHITE];
        }
        else if (token == "binc") {
            iss >> limits.inc[BLACK];
        }
        else if (token == "movestogo") {
            iss >> limits.movestogo;
        }
        else if (token == "depth") {
            iss >> limits.depth;
        }
        else if (token == "nodes") {
            iss >> limits.nodes;
        }
        else if (token == "mate") {
            iss >> limits.mate;
        }
        else if (token == "movetime") {
            iss >> limits.movetime;
        }
        else if (token == "infinite") {
            limits.infinite = true;
        }
        else if (token == "ponder") {
            limits.ponder = true;
        }
    }
    
    // Start search in separate thread
    if (search_thread_ && search_thread_->joinable()) {
        search_thread_->join();
    }
    
    searching_ = true;
    search_thread_ = std::make_unique<std::thread>([this, limits]() {
        pool_->start_search(pos_, limits);
        pool_->wait_for_search();
        
        Move best_move = pool_->best_move();
        
        // Output best move
        std::cout << "bestmove " << move_to_string(best_move);
        
        // TODO: Add ponder move support
        // if (ponder_move.is_valid()) {
        //     std::cout << " ponder " << move_to_string(ponder_move);
        // }
        
        std::cout << std::endl;
        std::cout.flush();
        
        searching_ = false;
    });
}

void UCINew::handle_stop() {
    if (searching_) {
        pool_->stop_search();
    }
}

void UCINew::handle_ponderhit() {
    if (searching_) {
        pool_->ponderhit();
    }
}

void UCINew::handle_setoption(const std::string& line) {
    std::istringstream iss(line);
    std::string token;
    iss >> token; // "setoption"
    iss >> token; // "name"
    
    std::string name;
    while (iss >> token && token != "value") {
        if (!name.empty()) name += " ";
        name += token;
    }
    
    if (name == "Threads") {
        int value;
        iss >> value;
        threads_ = std::max(1, std::min(256, value));
        pool_->set_threads(threads_);
        std::cout << "info string Threads set to " << threads_ << std::endl;
    }
    else if (name == "Hash") {
        int value;
        iss >> value;
        hash_size_mb_ = std::max(1, std::min(32768, value));
        tt_->resize(hash_size_mb_);
        std::cout << "info string Hash set to " << hash_size_mb_ << " MB" << std::endl;
    }
    else if (name == "Clear Hash") {
        tt_->clear();
        std::cout << "info string Hash cleared" << std::endl;
    }
    else if (name == "Ponder") {
        std::string value;
        iss >> value;
        ponder_ = (value == "true");
        std::cout << "info string Ponder " << (ponder_ ? "enabled" : "disabled") << std::endl;
    }
    else if (name == "MultiPV") {
        int value;
        iss >> value;
        multi_pv_ = std::max(1, std::min(256, value));
        std::cout << "info string MultiPV set to " << multi_pv_ << std::endl;
    }
    else if (name == "Use NNUE") {
        std::string value;
        iss >> value;
        bool use_nnue = (value == "true");
        if (use_nnue != use_nnue_) {
            switch_evaluator(use_nnue);
        }
    }
    else if (name == "EvalFile") {
        std::string filename;
        iss >> filename;
        nnue_file_ = filename;
        
        if (use_nnue_ && evaluator_) {
            NnueEval* nnue = dynamic_cast<NnueEval*>(evaluator_);
            if (nnue && !filename.empty()) {
                nnue->load_network(filename);
                evaluator_->initialize(pos_);
                std::cout << "info string NNUE network loaded from " << filename << std::endl;
            }
        }
    }
}

void UCINew::switch_evaluator(bool use_nnue) {
    if (searching_) {
        pool_->stop_search();
        pool_->wait_for_search();
    }
    
    delete evaluator_;
    
    if (use_nnue) {
        NnueEval* nnue = new NnueEval();
        if (!nnue_file_.empty()) {
            nnue->load_network(nnue_file_);
        }
        evaluator_ = nnue;
        use_nnue_ = true;
        std::cout << "info string Switched to NNUE evaluation" << std::endl;
    }
    else {
        evaluator_ = new ClassicEval();
        use_nnue_ = false;
        std::cout << "info string Switched to classical evaluation" << std::endl;
    }
    
    pos_.evaluator = evaluator_;
    evaluator_->initialize(pos_);
    
    // Recreate thread pool with new evaluator
    pool_ = std::make_unique<ThreadPool>(evaluator_, tt_.get());
    pool_->set_threads(threads_);
}

void UCINew::handle_bench(const std::string& line) {
    std::istringstream iss(line);
    std::string token;
    iss >> token; // "bench"
    
    int depth = 13;
    if (iss >> token) {
        depth = std::stoi(token);
    }
    
    std::cout << "Starting benchmark at depth " << depth << std::endl;
    
    Benchmark::BenchResult result = Benchmark::run_bench(pool_.get(), depth);
    Benchmark::print_results(result);
}

void UCINew::handle_perft(const std::string& line) {
    std::istringstream iss(line);
    std::string token;
    iss >> token; // "perft"
    
    int depth = 5;
    if (iss >> token) {
        depth = std::stoi(token);
    }
    
    std::cout << "Running perft(" << depth << ")..." << std::endl;
    uint64_t nodes = MoveGen::perft(pos_, depth);
    std::cout << "Nodes: " << nodes << std::endl;
}

void UCINew::handle_d() {
    // Display current position
    std::cout << "Position FEN: " << pos_.to_fen() << std::endl;
    std::cout << "Zobrist key: 0x" << std::hex << pos_.zobrist_key() << std::dec << std::endl;
    
    // Display board (simple text representation)
    std::cout << "\n  +---+---+---+---+---+---+---+---+" << std::endl;
    for (int rank = 7; rank >= 0; rank--) {
        std::cout << (rank + 1) << " |";
        for (int file = 0; file < 8; file++) {
            Square sq = make_square(file, rank);
            Piece pc = pos_.piece_on(sq);
            
            char piece_char = ' ';
            if (pc != NO_PIECE_PIECE) {
                const char pieces[] = " PNBRQK";
                piece_char = pieces[type_of(pc)];
                if (color_of(pc) == BLACK) {
                    piece_char = tolower(piece_char);
                }
            }
            std::cout << " " << piece_char << " |";
        }
        std::cout << std::endl;
        std::cout << "  +---+---+---+---+---+---+---+---+" << std::endl;
    }
    std::cout << "    a   b   c   d   e   f   g   h" << std::endl;
}

void UCINew::handle_eval() {
    int score = evaluator_->evaluate(pos_);
    std::cout << "Evaluation: " << score << " cp (from " 
              << (pos_.side_to_move() == WHITE ? "white" : "black") 
              << "'s perspective)" << std::endl;
}

void UCINew::handle_compiler() {
    std::cout << "Compiled with:" << std::endl;
    
#if defined(__clang__)
    std::cout << "  Compiler: Clang " << __clang_major__ << "." 
              << __clang_minor__ << "." << __clang_patchlevel__ << std::endl;
#elif defined(__GNUC__)
    std::cout << "  Compiler: GCC " << __GNUC__ << "." 
              << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__ << std::endl;
#elif defined(_MSC_VER)
    std::cout << "  Compiler: MSVC " << _MSC_VER << std::endl;
#endif

    std::cout << "  C++ Standard: " << __cplusplus << std::endl;
    
    const auto& features = simd::get_cpu_features();
    std::cout << "  CPU Features:" << std::endl;
    std::cout << "    SSE:    " << (features.sse ? "yes" : "no") << std::endl;
    std::cout << "    SSE2:   " << (features.sse2 ? "yes" : "no") << std::endl;
    std::cout << "    SSE3:   " << (features.sse3 ? "yes" : "no") << std::endl;
    std::cout << "    SSSE3:  " << (features.ssse3 ? "yes" : "no") << std::endl;
    std::cout << "    SSE4.1: " << (features.sse41 ? "yes" : "no") << std::endl;
    std::cout << "    SSE4.2: " << (features.sse42 ? "yes" : "no") << std::endl;
    std::cout << "    AVX:    " << (features.avx ? "yes" : "no") << std::endl;
    std::cout << "    AVX2:   " << (features.avx2 ? "yes" : "no") << std::endl;
    std::cout << "    BMI1:   " << (features.bmi1 ? "yes" : "no") << std::endl;
    std::cout << "    BMI2:   " << (features.bmi2 ? "yes" : "no") << std::endl;
    std::cout << "    POPCNT: " << (features.popcnt ? "yes" : "no") << std::endl;
}

Move UCINew::parse_move(const std::string& move_str) {
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
    
    bool capture = pos_.piece_on(to) != NO_PIECE_PIECE;
    bool special = false;
    
    Piece pc = pos_.piece_on(from);
    if (type_of(pc) == KING && abs(file_of(to) - file_of(from)) == 2) {
        special = true;
    }
    
    if (type_of(pc) == PAWN && to == pos_.ep_square()) {
        special = true;
        capture = true;
    }
    
    return Move(from, to, promo, capture, special);
}

std::string UCINew::move_to_string(Move move) {
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
