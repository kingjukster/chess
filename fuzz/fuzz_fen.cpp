// Fuzzing harness for FEN parsing
// Compile with: clang++ -fsanitize=fuzzer,address -g -O1 fuzz_fen.cpp -o fuzz_fen
// Or with AFL: afl-clang++ -g -O1 fuzz_fen.cpp -o fuzz_fen

#include "../board/position.h"
#include "../movegen/attacks.h"
#include <cstdint>
#include <cstddef>
#include <string>

using namespace chess;

// Initialize attack tables once
static bool initialized = false;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (!initialized) {
        Attacks::init();
        initialized = true;
    }
    
    // Convert input to FEN string
    if (size < 10 || size > 200) {
        return 0;  // Skip too short or too long inputs
    }
    
    std::string fen(reinterpret_cast<const char*>(data), size);
    
    // Try to parse FEN
    Position pos;
    try {
        pos.from_fen(fen);
    } catch (...) {
        // Invalid FEN is expected, just return
        return 0;
    }
    
    // If parsing succeeded, verify the position is consistent
    
    // Check that all pieces are on valid squares
    for (int sq = 0; sq < 64; sq++) {
        Piece pc = pos.piece_on(sq);
        if (pc != NO_PIECE_PIECE) {
            Color c = color_of(pc);
            PieceType pt = type_of(pc);
            
            // Verify piece is in bitboards
            if (!(pos.pieces(c) & (1ULL << sq))) {
                __builtin_trap();  // Piece not in color bitboard
            }
            if (!(pos.pieces(pt) & (1ULL << sq))) {
                __builtin_trap();  // Piece not in type bitboard
            }
        }
    }
    
    // Check king squares
    Square wk = pos.king_square(WHITE);
    Square bk = pos.king_square(BLACK);
    
    if (wk >= 64 || bk >= 64) {
        __builtin_trap();  // Invalid king square
    }
    
    if (pos.piece_on(wk) != make_piece(WHITE, KING)) {
        __builtin_trap();  // King not at king square
    }
    
    if (pos.piece_on(bk) != make_piece(BLACK, KING)) {
        __builtin_trap();  // King not at king square
    }
    
    // Try to convert back to FEN
    std::string fen_out = pos.to_fen();
    
    // Parse the output FEN again
    Position pos2;
    try {
        pos2.from_fen(fen_out);
    } catch (...) {
        __builtin_trap();  // to_fen produced invalid FEN
    }
    
    // Verify positions match
    if (pos2.zobrist_key() != pos.zobrist_key()) {
        __builtin_trap();  // FEN round-trip failed
    }
    
    return 0;
}

#ifdef AFL_FUZZER
// AFL fuzzer main function
int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }
    
    FILE* f = fopen(argv[1], "rb");
    if (!f) {
        perror("fopen");
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    uint8_t* buffer = (uint8_t*)malloc(size);
    if (!buffer) {
        fclose(f);
        return 1;
    }
    
    fread(buffer, 1, size, f);
    fclose(f);
    
    LLVMFuzzerTestOneInput(buffer, size);
    
    free(buffer);
    return 0;
}
#endif
