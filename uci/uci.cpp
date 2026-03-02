#define _CRT_SECURE_NO_WARNINGS
#include "uci.h"
#include "../eval/classic_eval.h"
#include "../nnue/nnue_eval.h"
#include "../movegen/movegen.h"
#include "../bench/bench.h"
#include "../eval/tuner.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <chrono>
#include <cstdlib>
#include <vector>
#include <sys/stat.h>

namespace {

// Default paths to try when BookFile/SyzygyPath are empty (relative to cwd)
// Paths relative to cwd; engine often runs from chess/build/Release/ so ../../ reaches chess/
static const std::vector<std::string> DEFAULT_BOOK_PATHS = {
    "../../book.bin", "chess/book.bin", "chess/books/opening.bin", "book.bin", "books/opening.bin",
    "../book.bin", "../chess/book.bin", "../../chess/book.bin"
};
static const std::vector<std::string> DEFAULT_SYZYGY_PATHS = {
    "../../syzygy", "chess/syzygy", "chess/tablebase", "chess/tablebases", "syzygy", "tablebase", "tablebases",
    "../syzygy", "../chess/syzygy", "../../chess/syzygy"
};

std::string try_default_book_path() {
    for (const auto& p : DEFAULT_BOOK_PATHS) {
        std::ifstream f(p, std::ios::binary);
        if (f.good()) { f.close(); return p; }
    }
    return "";
}

std::string try_default_syzygy_path() {
#ifdef _WIN32
    struct _stat64 st = {};
    for (const auto& p : DEFAULT_SYZYGY_PATHS) {
        if (_stat64(p.c_str(), &st) == 0 && (st.st_mode & _S_IFDIR) != 0) {
            return p;
        }
    }
#else
    struct stat st = {};
    for (const auto& p : DEFAULT_SYZYGY_PATHS) {
        if (stat(p.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
            return p;
        }
    }
#endif
    return "";
}

void debug_log(const std::string& msg) {
    const char* tmp = std::getenv("TEMP");
    if (!tmp) tmp = std::getenv("TMP");
    if (!tmp) tmp = ".";
    std::string path = std::string(tmp) + "\\chess_engine_uci.log";
    std::ofstream f(path, std::ios::app);
    if (f) {
        f << msg << std::endl;
    }
}
}

namespace chess {

UCI::UCI() : use_nnue(false), debug_mode(false), tuning_mode(false),
             wtime(0), btime(0), winc(0), binc(0), movetime(0),
             movestogo(0), depth_limit(0), nodes_limit(0), mate(0), infinite(false), 
             nnue_file(""), training_data_file(""),
             use_book(false), book_file(""), book_depth(20), book_variety(50), book_best_move(false),
             use_tablebase(false), tablebase_path(""), tablebase_probe_depth(1), tablebase_probe_limit(7) {
    evaluator = new ClassicEval();
    search = new Search(evaluator);
    tuner = new TexelTuner();
    book = new PolyglotBook();
    tablebase = new SyzygyTablebase();
    pos.evaluator = evaluator;
    
    pos.from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    evaluator->initialize(pos);
}

UCI::~UCI() {
    delete search;
    delete evaluator;
    delete tuner;
    delete book;
    delete tablebase;
}

void UCI::loop() {
    std::string line;
    
    // UCI spec: engine must wait for "uci" command before sending any output
    // (removed startup output - some GUIs like Arena fail if we send before they ask)
    
    while (std::getline(std::cin, line)) {
        debug_log("RECV: " + line);
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        std::string command;
        iss >> command;
        
        if (command == "quit" || command == "exit") {
            break;
        } else if (command == "uci") {
            std::cout << "id name ChessEngine" << std::endl;
            std::cout << "id author Horne" << std::endl;
            std::cout << "option name Use NNUE type check default false" << std::endl;
            std::cout << "option name EvalFile type string default <empty>" << std::endl;
            std::cout << "option name Debug type check default false" << std::endl;
            std::cout << "option name Tuning Mode type check default false" << std::endl;
            std::cout << "option name Training Data type string default <empty>" << std::endl;
            std::cout << "option name MultiPV type spin default 1 min 1 max 256" << std::endl;
            std::cout << "option name Use Null Move type check default true" << std::endl;
            std::cout << "option name Use LMR type check default true" << std::endl;
            std::cout << "option name Use Futility type check default true" << std::endl;
            std::cout << "option name Hash type spin default 128 min 1 max 4096" << std::endl;
            std::cout << "option name OwnBook type check default false" << std::endl;
            std::cout << "option name BookFile type string default <empty>" << std::endl;
            std::cout << "option name BookDepth type spin default 20 min 1 max 100" << std::endl;
            std::cout << "option name BookVariety type spin default 50 min 0 max 100" << std::endl;
            std::cout << "option name BookBestMove type check default false" << std::endl;
            std::cout << "option name SyzygyPath type string default <empty>" << std::endl;
            std::cout << "option name SyzygyProbeDepth type spin default 1 min 1 max 100" << std::endl;
            std::cout << "option name SyzygyProbeLimit type spin default 7 min 3 max 7" << std::endl;
            std::cout << "uciok" << std::endl;
        } else if (command == "isready") {
            std::cout << "readyok" << std::endl;
        } else if (command == "ucinewgame") {
            pos.from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
            evaluator->initialize(pos);
            search->clear_tt();
            book->reset_ply();
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
        } else if (command == "bench") {
            handle_bench(line);
        } else if (command == "d") {
            handle_display();
        } else if (command == "tune") {
            handle_tune(line);
        } else if (command == "exportparams") {
            handle_export_params(line);
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
        book->reset_ply();  // Reset book ply for new position
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
        book->reset_ply();  // Reset book ply for new position
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
    
    wtime = btime = winc = binc = movetime = 0;
    movestogo = depth_limit = nodes_limit = mate = 0;
    infinite = false;
    
    while (iss >> token) {
        if (token == "wtime") iss >> wtime;
        else if (token == "btime") iss >> btime;
        else if (token == "winc") iss >> winc;
        else if (token == "binc") iss >> binc;
        else if (token == "movetime") iss >> movetime;
        else if (token == "movestogo") iss >> movestogo;
        else if (token == "depth") iss >> depth_limit;
        else if (token == "nodes") iss >> nodes_limit;
        else if (token == "mate") iss >> mate;
        else if (token == "infinite") infinite = true;
    }
    
    Move best_move = MOVE_NONE;
    
    // Check opening book first
    if (use_book && book->is_loaded()) {
        best_move = book->get_move(pos, book_best_move);
        if (best_move != MOVE_NONE && best_move.is_valid() && MoveGen::is_legal(pos, best_move)) {
            std::cout << "info string Book move" << std::endl;
            std::cout << "bestmove " << move_to_string(best_move) << std::endl;
            std::cout.flush();
            return;
        }
        if (best_move != MOVE_NONE) {
            best_move = MOVE_NONE;  // Book returned invalid move, fall through to search
        } else {
            std::cout << "info string Book skipped (out of book)" << std::endl;
        }
    } else if (use_book && !book->is_loaded()) {
        std::cout << "info string Book skipped (not loaded)" << std::endl;
    }
    
    // Check tablebase at root
    if (use_tablebase && tablebase->is_initialized()) {
        TBResult wdl;
        int dtz;
        Move tb_move = tablebase->probe_root(pos, wdl, dtz);
        if (tb_move != MOVE_NONE && tb_move.is_valid() && MoveGen::is_legal(pos, tb_move)) {
            std::cout << "info string Tablebase hit - WDL: " << wdl << " DTZ: " << dtz << std::endl;
            std::cout << "bestmove " << move_to_string(tb_move) << std::endl;
            std::cout.flush();
            return;
        }
        if (tb_move != MOVE_NONE) {
            // TB returned invalid move, fall through to search
        }
    }
    
    // Calculate time limit
    int time_limit = 0;
    if (movetime > 0) {
        time_limit = movetime;
    } else if (pos.side_to_move() == WHITE && wtime > 0) {
        int inc = (winc > 0) ? winc : 0;
        if (movestogo > 0) {
            // Divide remaining time over moves left (+1 for this move)
            time_limit = (wtime + inc) / (movestogo + 1);
        } else {
            // No movestogo: use ~5% of time, add increment when provided
            time_limit = (wtime / 20) + inc;
        }
    } else if (pos.side_to_move() == BLACK && btime > 0) {
        int inc = (binc > 0) ? binc : 0;
        if (movestogo > 0) {
            time_limit = (btime + inc) / (movestogo + 1);
        } else {
            time_limit = (btime / 20) + inc;
        }
    }
    if (time_limit == 0 && !infinite) {
        time_limit = 5000;  // 5 second default when no time params given
    }
    
    // Perform search
    int max_depth = depth_limit > 0 ? depth_limit : 10;
    
    // Pass tablebase to search for probing during search
    search->set_tablebase(tablebase);
    best_move = search->iterative_deepening(pos, max_depth, time_limit);
    
    // Validate best move is legal before sending (guard against rare bugs)
    if (!best_move.is_valid() || !MoveGen::is_legal(pos, best_move)) {
        std::vector<Move> legal_moves;
        MoveGen::generate_legal(pos, legal_moves);
        if (!legal_moves.empty()) {
            best_move = legal_moves[0];
            std::cout << "info string Fallback to first legal move (search returned invalid)" << std::endl;
        } else {
            best_move = MOVE_NONE;  // Checkmate or stalemate - send 0000
        }
    }
    
    // Output search stats
    const SearchStats& stats = search->get_stats();
    std::cout << "info depth " << stats.depth << " nodes " << stats.nodes 
              << " score cp " << stats.best_score << std::endl;
    
    // Output best move
    std::string bestmove_str = "bestmove " + move_to_string(best_move);
    std::cout << bestmove_str << std::endl;
    std::cout.flush();
    debug_log("SEND: " + bestmove_str);
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
    } else if (name == "MultiPV") {
        int value;
        iss >> value;
        if (value >= 1 && value <= 256) {
            search->set_multi_pv(value);
        }
    } else if (name == "Use Null Move") {
        std::string value;
        iss >> value;
        search->set_null_move_enabled(value == "true");
    } else if (name == "Use LMR") {
        std::string value;
        iss >> value;
        search->set_lmr_enabled(value == "true");
    } else if (name == "Use Futility") {
        std::string value;
        iss >> value;
        search->set_futility_enabled(value == "true");
    } else if (name == "Hash") {
        int value;
        iss >> value;
        if (value >= 1 && value <= 4096) {
            search->set_hash_size(static_cast<size_t>(value));
            std::cout << "info string Hash size set to " << value << " MB" << std::endl;
        }
    } else if (name == "Debug") {
        std::string value;
        iss >> value;
        debug_mode = (value == "true");
        std::cout << "info string Debug mode " << (debug_mode ? "enabled" : "disabled") << std::endl;
    } else if (name == "Tuning Mode") {
        std::string value;
        iss >> value;
        tuning_mode = (value == "true");
        std::cout << "info string Tuning mode " << (tuning_mode ? "enabled" : "disabled") << std::endl;
    } else if (name == "Training Data") {
        std::string filename;
        iss >> filename;
        training_data_file = filename;
        std::cout << "info string Training data file set to: " << filename << std::endl;
    } else if (name == "OwnBook") {
        std::string value;
        iss >> value;
        use_book = (value == "true");
        std::cout << "info string Opening book " << (use_book ? "enabled" : "disabled") << std::endl;
    } else if (name == "BookFile") {
        std::string filename;
        iss >> filename;
        if (filename.empty() || filename == "<empty>") {
            filename = try_default_book_path();
        }
        book_file = filename;
        if (filename.empty()) {
            book->close();
            std::cout << "info string No book file specified (set path to enable)" << std::endl;
        } else if (book->load(filename)) {
            std::cout << "info string Opening book loaded: " << filename << std::endl;
        } else {
            std::cout << "info string Failed to load opening book: " << filename << std::endl;
        }
    } else if (name == "BookDepth") {
        int value;
        iss >> value;
        book_depth = value;
        book->set_max_depth(value);
        std::cout << "info string Book depth set to " << value << std::endl;
    } else if (name == "BookVariety") {
        int value;
        iss >> value;
        book_variety = value;
        book->set_variety(value);
        std::cout << "info string Book variety set to " << value << std::endl;
    } else if (name == "BookBestMove") {
        std::string value;
        iss >> value;
        book_best_move = (value == "true");
        std::cout << "info string Book best move " << (book_best_move ? "enabled" : "disabled") << std::endl;
    } else if (name == "SyzygyPath") {
        std::string path;
        iss >> path;
        if (path.empty() || path == "<empty>") {
            path = try_default_syzygy_path();
        }
        tablebase_path = path;
        if (path.empty()) {
            use_tablebase = false;
            std::cout << "info string No tablebase path specified (set path to enable)" << std::endl;
        } else if (tablebase->init(path)) {
            use_tablebase = true;
            std::cout << "info string Syzygy tablebases loaded: " << path << std::endl;
            std::cout << "info string Max cardinality: " << tablebase->max_pieces() << std::endl;
        } else {
            use_tablebase = false;
            std::cout << "info string Failed to load Syzygy tablebases: " << path << std::endl;
        }
    } else if (name == "SyzygyProbeDepth") {
        int value;
        iss >> value;
        tablebase_probe_depth = value;
        tablebase->set_probe_depth(value);
        std::cout << "info string Tablebase probe depth set to " << value << std::endl;
    } else if (name == "SyzygyProbeLimit") {
        int value;
        iss >> value;
        tablebase_probe_limit = value;
        tablebase->set_probe_limit(value);
        std::cout << "info string Tablebase probe limit set to " << value << " pieces" << std::endl;
    }
}

void UCI::handle_bench(const std::string& line) {
    std::istringstream iss(line);
    std::string token;
    iss >> token;
    
    int depth = 13;
    if (iss >> token) {
        depth = std::stoi(token);
    }
    
    std::cout << "Running simple benchmark at depth " << depth << std::endl;
    
    // Simple benchmark using current search
    std::vector<std::string> bench_positions = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"
    };
    
    uint64_t total_nodes = 0;
    auto start = std::chrono::steady_clock::now();
    
    for (const auto& fen : bench_positions) {
        Position bench_pos(fen);
        evaluator->initialize(bench_pos);
        search->search(bench_pos, depth, 0);
        total_nodes += search->get_stats().nodes;
    }
    
    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    std::cout << "Benchmark complete:" << std::endl;
    std::cout << "  Total nodes: " << total_nodes << std::endl;
    std::cout << "  Time: " << elapsed << " ms" << std::endl;
    std::cout << "  NPS: " << (total_nodes * 1000 / std::max(1LL, elapsed)) << std::endl;
}

void UCI::handle_display() {
    std::cout << std::endl;
    std::cout << "  +---+---+---+---+---+---+---+---+" << std::endl;
    
    for (int rank = 7; rank >= 0; rank--) {
        std::cout << rank + 1 << " |";
        for (int file = 0; file < 8; file++) {
            Square sq = make_square(file, rank);
            Piece pc = pos.piece_on(sq);
            
            char piece_char = ' ';
            if (pc != NO_PIECE_PIECE) {
                Color c = color_of(pc);
                PieceType pt = type_of(pc);
                
                switch (pt) {
                    case PAWN:   piece_char = 'P'; break;
                    case KNIGHT: piece_char = 'N'; break;
                    case BISHOP: piece_char = 'B'; break;
                    case ROOK:   piece_char = 'R'; break;
                    case QUEEN:  piece_char = 'Q'; break;
                    case KING:   piece_char = 'K'; break;
                    default: break;
                }
                
                if (c == BLACK) {
                    piece_char = tolower(piece_char);
                }
            }
            
            std::cout << " " << piece_char << " |";
        }
        std::cout << std::endl;
        std::cout << "  +---+---+---+---+---+---+---+---+" << std::endl;
    }
    
    std::cout << "    a   b   c   d   e   f   g   h" << std::endl;
    std::cout << std::endl;
    std::cout << "FEN: " << pos.to_fen() << std::endl;
    std::cout << "Key: 0x" << std::hex << pos.zobrist_key() << std::dec << std::endl;
    std::cout << "Side to move: " << (pos.side_to_move() == WHITE ? "White" : "Black") << std::endl;
    
    // Display castling rights
    int cr = pos.castling_rights();
    std::cout << "Castling: ";
    if (cr == 0) {
        std::cout << "-";
    } else {
        if (cr & WHITE_OO) std::cout << "K";
        if (cr & WHITE_OOO) std::cout << "Q";
        if (cr & BLACK_OO) std::cout << "k";
        if (cr & BLACK_OOO) std::cout << "q";
    }
    std::cout << std::endl;
    
    // Display en passant square
    if (pos.ep_square() != SQ_NONE) {
        std::cout << "En passant: " << char('a' + file_of(pos.ep_square())) 
                  << char('1' + rank_of(pos.ep_square())) << std::endl;
    }
    
    std::cout << "50-move counter: " << pos.rule50() << std::endl;
    
    // Display evaluation if not in search
    if (evaluator) {
        int eval = evaluator->evaluate(pos);
        std::cout << "Evaluation: " << eval << " (";
        if (eval > 0) std::cout << "+";
        std::cout << (eval / 100.0) << ")" << std::endl;
    }
    
    std::cout << std::endl;
}

void UCI::handle_tune(const std::string& line) {
    std::istringstream iss(line);
    std::string token;
    iss >> token;
    
    if (!tuning_mode) {
        std::cout << "info string Tuning mode not enabled. Use 'setoption name Tuning Mode value true'" << std::endl;
        return;
    }
    
    if (training_data_file.empty()) {
        std::cout << "info string No training data file specified. Use 'setoption name Training Data value <file>'" << std::endl;
        return;
    }
    
    // Load training data
    if (!tuner->load_training_data(training_data_file)) {
        std::cout << "info string Failed to load training data" << std::endl;
        return;
    }
    
    // Set evaluator for tuning (must be ClassicEval)
    ClassicEval* classic_eval = dynamic_cast<ClassicEval*>(evaluator);
    if (!classic_eval) {
        std::cout << "info string Tuning only works with classic evaluator" << std::endl;
        return;
    }
    
    tuner->set_evaluator(classic_eval);
    
    // Parse tuning parameters
    int iterations = 1000;
    double learning_rate = 1.0;
    
    while (iss >> token) {
        if (token == "iterations") {
            iss >> iterations;
        } else if (token == "lr") {
            iss >> learning_rate;
        }
    }
    
    std::cout << "info string Starting Texel tuning..." << std::endl;
    tuner->tune(iterations, learning_rate);
    std::cout << "info string Tuning complete. Error: " << tuner->get_error() << std::endl;
}

void UCI::handle_export_params(const std::string& line) {
    std::istringstream iss(line);
    std::string token;
    iss >> token;
    
    std::string filename = "eval_params.txt";
    if (iss >> token) {
        filename = token;
    }
    
    ClassicEval* classic_eval = dynamic_cast<ClassicEval*>(evaluator);
    if (!classic_eval) {
        std::cout << "info string Can only export parameters from classic evaluator" << std::endl;
        return;
    }
    
    // Export as JSON
    std::string json = tuning::export_parameters_json(classic_eval->get_params());
    
    std::ofstream file(filename);
    if (file.is_open()) {
        file << json;
        file.close();
        std::cout << "info string Parameters exported to: " << filename << std::endl;
    } else {
        std::cout << "info string Failed to export parameters to: " << filename << std::endl;
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

