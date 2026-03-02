#include "syzygy.h"
#include "tbprobe.h"
#include "../movegen/movegen.h"
#include <algorithm>
#if defined(_WIN32) || defined(_WIN64)
#include <io.h>
#include <sys/stat.h>
#else
#include <sys/stat.h>
#endif

namespace chess {

// Score values for tablebase results
static const int TB_WIN_SCORE = 20000;
static const int TB_LOSS_SCORE = -20000;
static const int TB_CURSED_WIN_SCORE = 1;
static const int TB_BLESSED_LOSS_SCORE = -1;

static bool path_exists(const std::string& path) {
#if defined(_WIN32) || defined(_WIN64)
    struct _stat64 info;
    return (_stat64(path.c_str(), &info) == 0 && (info.st_mode & _S_IFDIR));
#else
    struct stat info;
    return (stat(path.c_str(), &info) == 0 && (info.st_mode & S_IFDIR));
#endif
}

SyzygyTablebase::SyzygyTablebase()
    : initialized(false), max_cardinality(0), probe_depth(1), 
      probe_limit(7), tb_hits(0) {
}

SyzygyTablebase::~SyzygyTablebase() {
    close();
}

bool SyzygyTablebase::init(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    if (!path_exists(path)) {
        return false;
    }
    
    tb_path = path;
    
    if (!tb_init(path.c_str())) {
        initialized = false;
        return false;
    }
    max_cardinality = static_cast<int>(TB_LARGEST);
    initialized = (max_cardinality > 0);
    return true;
}

void SyzygyTablebase::close() {
    if (initialized) {
        tb_free();
        initialized = false;
        max_cardinality = 0;
    }
}

int SyzygyTablebase::count_pieces(const Position& pos) const {
    int count = 0;
    for (Square sq = 0; sq < 64; sq++) {
        if (pos.piece_on(sq) != NO_PIECE_PIECE) {
            count++;
        }
    }
    return count;
}

int SyzygyTablebase::fathom_position(const Position& pos,
                                    uint64_t* white, uint64_t* black,
                                    uint64_t* kings, uint64_t* queens,
                                    uint64_t* rooks, uint64_t* bishops,
                                    uint64_t* knights, uint64_t* pawns,
                                    unsigned* ep, bool* turn) const {
    *white = 0;
    *black = 0;
    *kings = 0;
    *queens = 0;
    *rooks = 0;
    *bishops = 0;
    *knights = 0;
    *pawns = 0;
    
    // Build bitboards for Fathom (a1=0, h8=63, same as engine)
    for (Square sq = 0; sq < 64; sq++) {
        Piece pc = pos.piece_on(sq);
        if (pc == NO_PIECE_PIECE) continue;
        
        uint64_t bb = 1ULL << sq;
        
        Color c = color_of(pc);
        PieceType pt = type_of(pc);
        
        if (c == WHITE) *white |= bb;
        else *black |= bb;
        
        switch (pt) {
            case KING:   *kings |= bb; break;
            case QUEEN:  *queens |= bb; break;
            case ROOK:   *rooks |= bb; break;
            case BISHOP: *bishops |= bb; break;
            case KNIGHT: *knights |= bb; break;
            case PAWN:   *pawns |= bb; break;
            default: break;
        }
    }
    
    // En passant: Fathom expects square index (0-63), 0 if none
    *ep = (pos.ep_square() == SQ_NONE) ? 0U : static_cast<unsigned>(pos.ep_square());
    
    // Side to move
    *turn = (pos.side_to_move() == WHITE);
    
    return count_pieces(pos);
}

Move SyzygyTablebase::fathom_to_move(unsigned fathom_move) const {
    // Fathom move format: from | (to << 6) | (promotion << 12)
    int from = fathom_move & 0x3F;
    int to = (fathom_move >> 6) & 0x3F;
    int promo = (fathom_move >> 12) & 0x7;
    
    PieceType promo_type = NO_PIECE;
    if (promo == 1) promo_type = QUEEN;
    else if (promo == 2) promo_type = ROOK;
    else if (promo == 3) promo_type = BISHOP;
    else if (promo == 4) promo_type = KNIGHT;
    
    // Note: capture and special flags will be determined by the position
    return Move(from, to, promo_type);
}

unsigned SyzygyTablebase::move_to_fathom(Move move) const {
    unsigned from = move.from();
    unsigned to = move.to();
    unsigned promo = 0;
    
    if (move.is_promotion()) {
        PieceType pt = move.promotion();
        if (pt == QUEEN) promo = 1;
        else if (pt == ROOK) promo = 2;
        else if (pt == BISHOP) promo = 3;
        else if (pt == KNIGHT) promo = 4;
    }
    
    return from | (to << 6) | (promo << 12);
}

TBResult SyzygyTablebase::probe_wdl(const Position& pos) {
    if (!initialized) {
        return TB_FAILED;
    }
    
    int pieces = count_pieces(pos);
    if (pieces > max_cardinality || pieces > probe_limit) {
        return TB_FAILED;
    }
    
    uint64_t white, black, kings, queens, rooks, bishops, knights, pawns;
    unsigned ep;
    bool turn;
    
    fathom_position(pos, &white, &black, &kings, &queens, &rooks, &bishops,
                   &knights, &pawns, &ep, &turn);
    
    // tb_probe_wdl requires rule50=0 and castling=0 - returns TB_RESULT_FAILED otherwise
    unsigned result = tb_probe_wdl(white, black, kings, queens, rooks, bishops,
                                   knights, pawns, 0, 0, ep, turn);
    if (result == TB_RESULT_FAILED) {
        return TB_FAILED;
    }
    tb_hits++;
    return static_cast<TBResult>(result);
}

int SyzygyTablebase::probe_dtz(const Position& pos) {
    if (!initialized) {
        return 0;
    }
    
    int pieces = count_pieces(pos);
    if (pieces > max_cardinality || pieces > probe_limit) {
        return 0;
    }
    
    uint64_t white, black, kings, queens, rooks, bishops, knights, pawns;
    unsigned ep;
    bool turn;
    
    fathom_position(pos, &white, &black, &kings, &queens, &rooks, &bishops,
                   &knights, &pawns, &ep, &turn);
    
    // tb_probe_root returns full result; extract DTZ via TB_GET_DTZ
    unsigned result = tb_probe_root(white, black, kings, queens, rooks, bishops,
                                   knights, pawns, pos.rule50(), 0, ep, turn, nullptr);
    if (result == TB_RESULT_FAILED) {
        return 0;
    }
    tb_hits++;
    return static_cast<int>(TB_GET_DTZ(result));
}

Move SyzygyTablebase::probe_root(const Position& pos, TBResult& wdl, int& dtz) {
    wdl = TB_FAILED;
    dtz = 0;
    
    if (!initialized) {
        return MOVE_NONE;
    }
    
    int pieces = count_pieces(pos);
    if (pieces > max_cardinality || pieces > probe_limit) {
        return MOVE_NONE;
    }
    
    uint64_t white, black, kings, queens, rooks, bishops, knights, pawns;
    unsigned ep;
    bool turn;
    
    fathom_position(pos, &white, &black, &kings, &queens, &rooks, &bishops,
                   &knights, &pawns, &ep, &turn);
    
    unsigned results[TB_MAX_MOVES];
    unsigned result = tb_probe_root(white, black, kings, queens, rooks, bishops,
                                   knights, pawns, pos.rule50(), 0, ep, turn, results);
    if (result == TB_RESULT_FAILED) {
        return MOVE_NONE;
    }
    tb_hits++;
    unsigned best_move = TB_GET_FROM(result) | (TB_GET_TO(result) << 6) |
                        (TB_GET_PROMOTES(result) << 12);
    wdl = static_cast<TBResult>(TB_GET_WDL(result));
    dtz = static_cast<int>(TB_GET_DTZ(result));
    return fathom_to_move(best_move);
}

std::vector<TBMove> SyzygyTablebase::probe_root_moves(const Position& pos) {
    std::vector<TBMove> tb_moves;
    
    if (!initialized) {
        return tb_moves;
    }
    
    int pieces = count_pieces(pos);
    if (pieces > max_cardinality || pieces > probe_limit) {
        return tb_moves;
    }
    
    uint64_t white, black, kings, queens, rooks, bishops, knights, pawns;
    unsigned ep;
    bool turn;
    
    fathom_position(pos, &white, &black, &kings, &queens, &rooks, &bishops,
                   &knights, &pawns, &ep, &turn);
    
    unsigned results[TB_MAX_MOVES];
    unsigned result = tb_probe_root(white, black, kings, queens, rooks, bishops,
                                   knights, pawns, pos.rule50(), 0, ep, turn, results);
    if (result == TB_RESULT_FAILED) {
        return tb_moves;
    }
    tb_hits++;
    for (unsigned i = 0; results[i] != TB_RESULT_FAILED; i++) {
        unsigned move_data = results[i];
        unsigned fathom_move = TB_GET_FROM(move_data) | (TB_GET_TO(move_data) << 6) |
                              (TB_GET_PROMOTES(move_data) << 12);
        Move move = fathom_to_move(fathom_move);
        TBResult wdl_val = static_cast<TBResult>(TB_GET_WDL(move_data));
        int dtz_val = static_cast<int>(TB_GET_DTZ(move_data));
        tb_moves.push_back(TBMove(move, wdl_val, dtz_val));
    }
    
    return tb_moves;
}

int SyzygyTablebase::wdl_to_score(TBResult wdl, int ply) const {
    switch (wdl) {
        case TB_WIN:
            return TB_WIN_SCORE - ply;
        case TB_CURSED_WIN:
            return TB_CURSED_WIN_SCORE;
        case TB_DRAW:
            return 0;
        case TB_BLESSED_LOSS:
            return TB_BLESSED_LOSS_SCORE;
        case TB_LOSS:
            return TB_LOSS_SCORE + ply;
        default:
            return 0;
    }
}

bool SyzygyTablebase::probe_search(const Position& pos, int& score) {
    if (!initialized) {
        return false;
    }
    
    int pieces = count_pieces(pos);
    if (pieces > max_cardinality || pieces > probe_limit) {
        return false;
    }
    
    TBResult wdl = probe_wdl(pos);
    if (wdl == TB_FAILED) {
        return false;
    }
    
    // Convert WDL to score (ply is estimated from rule50)
    score = wdl_to_score(wdl, pos.rule50());
    return true;
}

} // namespace chess
