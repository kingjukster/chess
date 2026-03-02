cmake_minimum_required(VERSION 3.10)
project(ChessEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type options
option(ENABLE_PROFILING "Enable profiling support" OFF)
option(ENABLE_COVERAGE "Enable code coverage" OFF)
option(ENABLE_SANITIZERS "Enable address and undefined behavior sanitizers" OFF)
option(ENABLE_GPROF "Enable gprof profiling" OFF)

# Build type options
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# CPU architecture detection and SIMD flags
option(USE_AVX2 "Enable AVX2 instructions" ON)
option(USE_SSE41 "Enable SSE4.1 instructions" ON)
option(USE_POPCNT "Enable POPCNT instruction" ON)
option(USE_BMI2 "Enable BMI2 instructions" OFF)

# Source files
set(BOARD_SOURCES
    board/position.cpp
)

set(MOVEGEN_SOURCES
    movegen/attacks.cpp
    movegen/movegen.cpp
)

set(EVAL_SOURCES
    eval/classic_eval.cpp
    eval/tuner.cpp
)

set(NNUE_SOURCES
    nnue/nnue_eval.cpp
    nnue/nnue_loader.cpp
    nnue/stockfish_nnue.cpp
)

set(SEARCH_SOURCES
    search/search.cpp
    search/tt.cpp
    search/thread_pool.cpp
)

set(UCI_SOURCES
    uci/uci.cpp
    uci/uci_new.cpp
)

set(SIMD_SOURCES
    simd/simd.cpp
)

set(BENCH_SOURCES
    bench/bench.cpp
)

set(EPD_SOURCES
    epd/epd_parser.cpp
    epd/epd_runner.cpp
)

set(BOOK_SOURCES
    book/polyglot.cpp
)

set(TABLEBASE_SOURCES
    tablebase/syzygy.cpp
)

# Fathom library for Syzygy tablebase support (no CMake - use FetchContent_Populate)
cmake_policy(SET CMP0169 OLD)  # Allow FetchContent_Populate for repos without CMake
include(FetchContent)
FetchContent_Declare(
    Fathom
    GIT_REPOSITORY https://github.com/jdart1/Fathom.git
    GIT_TAG        master
)
FetchContent_Populate(Fathom)

# Build Fathom as static library (tbprobe.c includes tbchess.c)
add_library(fathom STATIC
    ${fathom_SOURCE_DIR}/src/tbprobe.c
)
target_include_directories(fathom PUBLIC
    ${fathom_SOURCE_DIR}/src
)
target_compile_definitions(fathom PRIVATE TB_NO_HELPER_API TB_NO_THREADS)
if(MSVC)
    # MSVC needs C11 + experimental atomics for Fathom
    set_target_properties(fathom PROPERTIES C_STANDARD 11)
    target_compile_options(fathom PRIVATE /W3 /O2 /std:c11 /experimental:c11atomics)
else()
    target_compile_options(fathom PRIVATE -Wall -O3 -std=c11)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR})

# Create executable
add_executable(BlacklightEngine
    main.cpp
    ${BOARD_SOURCES}
    ${MOVEGEN_SOURCES}
    ${EVAL_SOURCES}
    ${NNUE_SOURCES}
    ${SEARCH_SOURCES}
    ${UCI_SOURCES}
    ${SIMD_SOURCES}
    ${BENCH_SOURCES}
    ${EPD_SOURCES}
    ${BOOK_SOURCES}
    ${TABLEBASE_SOURCES}
)
target_link_libraries(BlacklightEngine PRIVATE fathom)
target_include_directories(BlacklightEngine PRIVATE ${fathom_SOURCE_DIR}/src)

# Compiler-specific options
if(MSVC)
    target_compile_options(BlacklightEngine PRIVATE /W4 /O2)
    
    # SIMD flags for MSVC
    if(USE_AVX2)
        target_compile_options(BlacklightEngine PRIVATE /arch:AVX2)
        target_compile_definitions(BlacklightEngine PRIVATE USE_AVX2)
    elseif(USE_SSE41)
        target_compile_options(BlacklightEngine PRIVATE /arch:SSE2)
        target_compile_definitions(BlacklightEngine PRIVATE USE_SSE41)
    endif()
    
    # Enable multi-processor compilation
    target_compile_options(BlacklightEngine PRIVATE /MP)
    
    # Profiling support for MSVC
    if(ENABLE_PROFILING)
        target_compile_options(BlacklightEngine PRIVATE /Zi)
        target_link_options(BlacklightEngine PRIVATE /PROFILE)
    endif()
    
    # Link-time optimization
    set_property(TARGET BlacklightEngine PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    target_compile_options(BlacklightEngine PRIVATE -Wall -Wextra -O3 -march=native)
    
    # SIMD flags for GCC/Clang
    if(USE_AVX2)
        target_compile_options(BlacklightEngine PRIVATE -mavx2 -mfma)
        target_compile_definitions(BlacklightEngine PRIVATE USE_AVX2)
    elseif(USE_SSE41)
        target_compile_options(BlacklightEngine PRIVATE -msse4.1)
        target_compile_definitions(BlacklightEngine PRIVATE USE_SSE41)
    endif()
    
    if(USE_POPCNT)
        target_compile_options(BlacklightEngine PRIVATE -mpopcnt)
    endif()
    
    if(USE_BMI2)
        target_compile_options(BlacklightEngine PRIVATE -mbmi2)
    endif()
    
    # Link-time optimization
    target_compile_options(BlacklightEngine PRIVATE -flto)
    target_link_options(BlacklightEngine PRIVATE -flto)
    
    # Profiling support
    if(ENABLE_PROFILING)
        target_compile_options(BlacklightEngine PRIVATE -g -fno-omit-frame-pointer)
    endif()
    
    if(ENABLE_GPROF)
        target_compile_options(BlacklightEngine PRIVATE -pg)
        target_link_options(BlacklightEngine PRIVATE -pg)
    endif()
    
    if(ENABLE_SANITIZERS)
        target_compile_options(BlacklightEngine PRIVATE 
            -fsanitize=address 
            -fsanitize=undefined
            -fno-omit-frame-pointer
        )
        target_link_options(BlacklightEngine PRIVATE 
            -fsanitize=address 
            -fsanitize=undefined
        )
    endif()
    
    if(ENABLE_COVERAGE)
        target_compile_options(BlacklightEngine PRIVATE --coverage)
        target_link_options(BlacklightEngine PRIVATE --coverage)
    endif()
    
    # Threading support
    find_package(Threads REQUIRED)
    target_link_libraries(BlacklightEngine PRIVATE Threads::Threads)
endif()

# NNUE converter utility (optional)
option(BUILD_CONVERTER "Build NNUE converter utility" ON)

if(BUILD_CONVERTER)
    add_executable(nnue_converter
        nnue/nnue_converter.cpp
        nnue/nnue_loader.cpp
        nnue/stockfish_nnue.cpp
    )
    
    if(MSVC)
        target_compile_options(nnue_converter PRIVATE /W4 /O2)
    else()
        target_compile_options(nnue_converter PRIVATE -Wall -Wextra -O3)
    endif()
endif()

# Debug tool for testing
# add_executable(test_check_debug
#     ../test_check.cpp
#     ${BOARD_SOURCES}
#     ${MOVEGEN_SOURCES}
# )
# 
# target_include_directories(test_check_debug PRIVATE ${CMAKE_SOURCE_DIR})
# 
# if(MSVC)
#     target_compile_options(test_check_debug PRIVATE /W4 /O2)
# else()
#     target_compile_options(test_check_debug PRIVATE -Wall -Wextra -O3)
# endif()

add_executable(test_pos3_debug
    ../test_pos3.cpp
    ${BOARD_SOURCES}
    ${MOVEGEN_SOURCES}
)

target_include_directories(test_pos3_debug PRIVATE ${CMAKE_SOURCE_DIR})

if(MSVC)
    target_compile_options(test_pos3_debug PRIVATE /W4 /O2)
else()
    target_compile_options(test_pos3_debug PRIVATE -Wall -Wextra -O3)
endif()

add_executable(test_kiwipete_debug
    ../test_kiwipete.cpp
    ${BOARD_SOURCES}
    ${MOVEGEN_SOURCES}
)

target_include_directories(test_kiwipete_debug PRIVATE ${CMAKE_SOURCE_DIR})

if(MSVC)
    target_compile_options(test_kiwipete_debug PRIVATE /W4 /O2)
else()
    target_compile_options(test_kiwipete_debug PRIVATE -Wall -Wextra -O3)
endif()

add_executable(perft_divide
    ../perft_divide.cpp
    ${BOARD_SOURCES}
    ${MOVEGEN_SOURCES}
)

target_include_directories(perft_divide PRIVATE ${CMAKE_SOURCE_DIR})

if(MSVC)
    target_compile_options(perft_divide PRIVATE /W4 /O2)
else()
    target_compile_options(perft_divide PRIVATE -Wall -Wextra -O3)
endif()

add_executable(debug_movegen
    ../debug_movegen.cpp
    ${BOARD_SOURCES}
    ${MOVEGEN_SOURCES}
)

target_include_directories(debug_movegen PRIVATE ${CMAKE_SOURCE_DIR})

if(MSVC)
    target_compile_options(debug_movegen PRIVATE /W4 /O2)
else()
    target_compile_options(debug_movegen PRIVATE -Wall -Wextra -O3)
endif()

add_executable(test_unmake
    ../test_unmake.cpp
    ${BOARD_SOURCES}
    ${MOVEGEN_SOURCES}
)

target_include_directories(test_unmake PRIVATE ${CMAKE_SOURCE_DIR})

if(MSVC)
    target_compile_options(test_unmake PRIVATE /W4 /O2)
else()
    target_compile_options(test_unmake PRIVATE -Wall -Wextra -O3)
endif()

add_executable(test_ep
    ../test_ep.cpp
    ${BOARD_SOURCES}
    ${MOVEGEN_SOURCES}
)

target_include_directories(test_ep PRIVATE ${CMAKE_SOURCE_DIR})

if(MSVC)
    target_compile_options(test_ep PRIVATE /W4 /O2)
else()
    target_compile_options(test_ep PRIVATE -Wall -Wextra -O3)
endif()

add_executable(perft_debug
    ../perft_debug.cpp
    ${BOARD_SOURCES}
    ${MOVEGEN_SOURCES}
)

target_include_directories(perft_debug PRIVATE ${CMAKE_SOURCE_DIR})

if(MSVC)
    target_compile_options(perft_debug PRIVATE /W4 /O2)
else()
    target_compile_options(perft_debug PRIVATE -Wall -Wextra -O3)
endif()

add_executable(test_perft_simple
    ../test_perft_simple.cpp
    ${BOARD_SOURCES}
    ${MOVEGEN_SOURCES}
)

target_include_directories(test_perft_simple PRIVATE ${CMAKE_SOURCE_DIR})

if(MSVC)
    target_compile_options(test_perft_simple PRIVATE /W4 /O2)
else()
    target_compile_options(test_perft_simple PRIVATE -Wall -Wextra -O3)
endif()

add_executable(check_fen
    ../check_fen.cpp
    ${BOARD_SOURCES}
    ${MOVEGEN_SOURCES}
)

target_include_directories(check_fen PRIVATE ${CMAKE_SOURCE_DIR})

if(MSVC)
    target_compile_options(check_fen PRIVATE /W4 /O2)
else()
    target_compile_options(check_fen PRIVATE -Wall -Wextra -O3)
endif()

add_executable(test_perft_twice
    ../test_perft_twice.cpp
    ${BOARD_SOURCES}
    ${MOVEGEN_SOURCES}
)

target_include_directories(test_perft_twice PRIVATE ${CMAKE_SOURCE_DIR})

if(MSVC)
    target_compile_options(test_perft_twice PRIVATE /W4 /O2)
else()
    target_compile_options(test_perft_twice PRIVATE -Wall -Wextra -O3)
endif()

add_executable(test_c7_bug
    ../test_c7_bug.cpp
    ${BOARD_SOURCES}
    ${MOVEGEN_SOURCES}
)

target_include_directories(test_c7_bug PRIVATE ${CMAKE_SOURCE_DIR})

if(MSVC)
    target_compile_options(test_c7_bug PRIVATE /W4 /O2)
else()
    target_compile_options(test_c7_bug PRIVATE -Wall -Wextra -O3)
endif()

add_executable(test_nested_unmake
    ../test_nested_unmake.cpp
    ${BOARD_SOURCES}
    ${MOVEGEN_SOURCES}
)

target_include_directories(test_nested_unmake PRIVATE ${CMAKE_SOURCE_DIR})

if(MSVC)
    target_compile_options(test_nested_unmake PRIVATE /W4 /O2)
else()
    target_compile_options(test_nested_unmake PRIVATE -Wall -Wextra -O3)
endif()

add_executable(debug_perft_tool
    debug_perft.cpp
    ${BOARD_SOURCES}
    ${MOVEGEN_SOURCES}
)

target_include_directories(debug_perft_tool PRIVATE ${CMAKE_SOURCE_DIR})

if(MSVC)
    target_compile_options(debug_perft_tool PRIVATE /W4 /O2)
else()
    target_compile_options(debug_perft_tool PRIVATE -Wall -Wextra -O3)
endif()

add_executable(debug_move_tool
    ../debug_move.cpp
    ${BOARD_SOURCES}
    ${MOVEGEN_SOURCES}
)

target_include_directories(debug_move_tool PRIVATE ${CMAKE_SOURCE_DIR})

if(MSVC)
    target_compile_options(debug_move_tool PRIVATE /W4 /O2)
else()
    target_compile_options(debug_move_tool PRIVATE -Wall -Wextra -O3)
endif()

add_executable(test_make_unmake_tool
    ../test_make_unmake.cpp
    ${BOARD_SOURCES}
    ${MOVEGEN_SOURCES}
)

target_include_directories(test_make_unmake_tool PRIVATE ${CMAKE_SOURCE_DIR})

if(MSVC)
    target_compile_options(test_make_unmake_tool PRIVATE /W4 /O2)
else()
    target_compile_options(test_make_unmake_tool PRIVATE -Wall -Wextra -O3)
endif()

add_executable(test_legal_filter_tool
    ../test_legal_filter.cpp
    ${BOARD_SOURCES}
    ${MOVEGEN_SOURCES}
)

target_include_directories(test_legal_filter_tool PRIVATE ${CMAKE_SOURCE_DIR})

if(MSVC)
    target_compile_options(test_legal_filter_tool PRIVATE /W4 /O2)
else()
    target_compile_options(test_legal_filter_tool PRIVATE -Wall -Wextra -O3)
endif()

add_executable(test_specific_move_tool
    ../test_specific_move.cpp
    ${BOARD_SOURCES}
    ${MOVEGEN_SOURCES}
)

target_include_directories(test_specific_move_tool PRIVATE ${CMAKE_SOURCE_DIR})

if(MSVC)
    target_compile_options(test_specific_move_tool PRIVATE /W4 /O2)
else()
    target_compile_options(test_specific_move_tool PRIVATE -Wall -Wextra -O3)
endif()

add_executable(test_after_g2h3_tool
    ../test_after_g2h3.cpp
    ${BOARD_SOURCES}
    ${MOVEGEN_SOURCES}
)

target_include_directories(test_after_g2h3_tool PRIVATE ${CMAKE_SOURCE_DIR})

if(MSVC)
    target_compile_options(test_after_g2h3_tool PRIVATE /W4 /O2)
else()
    target_compile_options(test_after_g2h3_tool PRIVATE -Wall -Wextra -O3)
endif()

add_executable(test_kiwipete_moves_tool
    ../test_kiwipete_moves.cpp
    ${BOARD_SOURCES}
    ${MOVEGEN_SOURCES}
)

target_include_directories(test_kiwipete_moves_tool PRIVATE ${CMAKE_SOURCE_DIR})

if(MSVC)
    target_compile_options(test_kiwipete_moves_tool PRIVATE /W4 /O2)
else()
    target_compile_options(test_kiwipete_moves_tool PRIVATE -Wall -Wextra -O3)
endif()

add_executable(test_kiwipete_stats_tool
    ../test_kiwipete_stats.cpp
    ${BOARD_SOURCES}
    ${MOVEGEN_SOURCES}
)

target_include_directories(test_kiwipete_stats_tool PRIVATE ${CMAKE_SOURCE_DIR})

if(MSVC)
    target_compile_options(test_kiwipete_stats_tool PRIVATE /W4 /O2)
else()
    target_compile_options(test_kiwipete_stats_tool PRIVATE -Wall -Wextra -O3)
endif()

add_executable(test_promotion_search_tool
    ../test_promotion_search.cpp
    ${BOARD_SOURCES}
    ${MOVEGEN_SOURCES}
    ${EVAL_SOURCES}
    ${SEARCH_SOURCES}
    ${TABLEBASE_SOURCES}
)
target_link_libraries(test_promotion_search_tool PRIVATE fathom)
target_include_directories(test_promotion_search_tool PRIVATE ${CMAKE_SOURCE_DIR} ${fathom_SOURCE_DIR}/src)

if(MSVC)
    target_compile_options(test_promotion_search_tool PRIVATE /W4 /O2)
else()
    target_compile_options(test_promotion_search_tool PRIVATE -Wall -Wextra -O3)
endif()

add_executable(test_promotion_eval_tool
    ../test_promotion_eval.cpp
    ${BOARD_SOURCES}
    ${MOVEGEN_SOURCES}
    ${EVAL_SOURCES}
)

target_include_directories(test_promotion_eval_tool PRIVATE ${CMAKE_SOURCE_DIR})

if(MSVC)
    target_compile_options(test_promotion_eval_tool PRIVATE /W4 /O2)
else()
    target_compile_options(test_promotion_eval_tool PRIVATE -Wall -Wextra -O3)
endif()

add_executable(test_promotion_position_tool
    ../test_promotion_position.cpp
    ${BOARD_SOURCES}
    ${MOVEGEN_SOURCES}
)

target_include_directories(test_promotion_position_tool PRIVATE ${CMAKE_SOURCE_DIR})

if(MSVC)
    target_compile_options(test_promotion_position_tool PRIVATE /W4 /O2)
else()
    target_compile_options(test_promotion_position_tool PRIVATE -Wall -Wextra -O3)
endif()

add_executable(test_promotion_detailed_tool
    ../test_promotion_detailed.cpp
    ${BOARD_SOURCES}
    ${MOVEGEN_SOURCES}
    ${EVAL_SOURCES}
)

target_include_directories(test_promotion_detailed_tool PRIVATE ${CMAKE_SOURCE_DIR})

if(MSVC)
    target_compile_options(test_promotion_detailed_tool PRIVATE /W4 /O2)
else()
    target_compile_options(test_promotion_detailed_tool PRIVATE -Wall -Wextra -O3)
endif()

add_executable(test_promotion_breakdown_tool
    ../test_promotion_breakdown.cpp
    ${BOARD_SOURCES}
    ${MOVEGEN_SOURCES}
    ${EVAL_SOURCES}
)

target_include_directories(test_promotion_breakdown_tool PRIVATE ${CMAKE_SOURCE_DIR})

if(MSVC)
    target_compile_options(test_promotion_breakdown_tool PRIVATE /W4 /O2)
else()
    target_compile_options(test_promotion_breakdown_tool PRIVATE -Wall -Wextra -O3)
endif()

# Google Test setup
option(BUILD_TESTS "Build tests" ON)

if(BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    
    enable_testing()
    
    # Create a library with the chess engine code (excluding main.cpp)
    add_library(chess_lib STATIC
        ${BOARD_SOURCES}
        ${MOVEGEN_SOURCES}
        ${EVAL_SOURCES}
        ${NNUE_SOURCES}
        ${SEARCH_SOURCES}
        ${UCI_SOURCES}
        ${SIMD_SOURCES}
        ${BENCH_SOURCES}
        ${EPD_SOURCES}
        ${BOOK_SOURCES}
        ${TABLEBASE_SOURCES}
    )
    
    target_include_directories(chess_lib PUBLIC ${CMAKE_SOURCE_DIR} ${fathom_SOURCE_DIR}/src)
    target_link_libraries(chess_lib PUBLIC fathom)
    
    if(MSVC)
        target_compile_options(chess_lib PRIVATE /W4 /O2)
        if(USE_AVX2)
            target_compile_options(chess_lib PRIVATE /arch:AVX2)
            target_compile_definitions(chess_lib PRIVATE USE_AVX2)
        endif()
        target_compile_options(chess_lib PRIVATE /MP)
        
        # Profiling support for MSVC
        if(ENABLE_PROFILING)
            target_compile_options(chess_lib PRIVATE /Zi)
        endif()
    else()
        target_compile_options(chess_lib PRIVATE -Wall -Wextra -O3 -march=native)
        if(USE_AVX2)
            target_compile_options(chess_lib PRIVATE -mavx2 -mfma)
            target_compile_definitions(chess_lib PRIVATE USE_AVX2)
        endif()
        if(USE_POPCNT)
            target_compile_options(chess_lib PRIVATE -mpopcnt)
        endif()
        
        # Profiling support
        if(ENABLE_PROFILING)
            target_compile_options(chess_lib PRIVATE -g -fno-omit-frame-pointer)
        endif()
        
        if(ENABLE_GPROF)
            target_compile_options(chess_lib PRIVATE -pg)
        endif()
        
        if(ENABLE_SANITIZERS)
            target_compile_options(chess_lib PRIVATE 
                -fsanitize=address 
                -fsanitize=undefined
                -fno-omit-frame-pointer
            )
        endif()
        
        if(ENABLE_COVERAGE)
            target_compile_options(chess_lib PRIVATE --coverage)
        endif()
        
        find_package(Threads REQUIRED)
        target_link_libraries(chess_lib PUBLIC Threads::Threads)
    endif()
    
    # Test executables
    set(TEST_SOURCES
        tests/test_bitboard.cpp
        tests/test_movegen.cpp
        tests/test_position.cpp
        tests/test_attacks.cpp
        tests/test_eval.cpp
        tests/test_perft.cpp
        tests/test_search.cpp
        tests/test_uci.cpp
        tests/test_book.cpp
        tests/test_tablebase.cpp
    )
    
    add_executable(chess_tests ${TEST_SOURCES})
    
    target_link_libraries(chess_tests
        PRIVATE
        chess_lib
        GTest::gtest_main
    )
    
    if(MSVC)
        target_compile_options(chess_tests PRIVATE /W4)
        
        if(ENABLE_PROFILING)
            target_compile_options(chess_tests PRIVATE /Zi)
        endif()
    else()
        target_compile_options(chess_tests PRIVATE -Wall -Wextra)
        
        if(ENABLE_PROFILING)
            target_compile_options(chess_tests PRIVATE -g -fno-omit-frame-pointer)
        endif()
        
        if(ENABLE_GPROF)
            target_compile_options(chess_tests PRIVATE -pg)
            target_link_options(chess_tests PRIVATE -pg)
        endif()
        
        if(ENABLE_SANITIZERS)
            target_compile_options(chess_tests PRIVATE 
                -fsanitize=address 
                -fsanitize=undefined
                -fno-omit-frame-pointer
            )
            target_link_options(chess_tests PRIVATE 
                -fsanitize=address 
                -fsanitize=undefined
            )
        endif()
        
        if(ENABLE_COVERAGE)
            target_compile_options(chess_tests PRIVATE --coverage)
            target_link_options(chess_tests PRIVATE --coverage)
        endif()
    endif()
    
    include(GoogleTest)
    gtest_discover_tests(chess_tests)
endif()

