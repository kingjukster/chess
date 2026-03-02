// Fuzzing harness for move generation
// Compile with: clang++ -fsanitize=fuzzer,address -g -O1 fuzz_movegen.cpp -o fuzz_movegen
// Or with AFL: afl-clang++ -g -O1 fuzz_movegen.cpp -o fuzz_movegen

#include "../board/position.h"
#include "../movegen/movegen.h"
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
        // Invalid FEN, skip
        return 0;
    }
    
    // Generate moves
    std::vector<Move> moves;
    MoveGen::generate_moves(pos, moves);
    
    // Verify each move is pseudo-legal
    for (const Move& move : moves) {
        if (!move.is_valid()) {
            __builtin_trap();  // Crash on invalid move
        }
        
        // Try to make and unmake the move
        UndoInfo undo;
        bool made = pos.make_move(move, undo);
        if (made) {
            pos.unmake_move(move, undo);
        }
    }
    
    // Test perft depth 1 (should match move count)
    uint64_t perft_nodes = MoveGen::perft(pos, 1);
    if (perft_nodes != moves.size()) {
        __builtin_trap();  // Mismatch between move generation methods
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
