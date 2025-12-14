cmake_minimum_required(VERSION 3.10)
project(ChessEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
)

set(NNUE_SOURCES
    nnue/nnue_eval.cpp
    nnue/nnue_loader.cpp
)

set(SEARCH_SOURCES
    search/search.cpp
)

set(UCI_SOURCES
    uci/uci.cpp
)

# Include directories
include_directories(${CMAKE_SOURCE_DIR})

# Create executable
add_executable(chess_engine
    main.cpp
    ${BOARD_SOURCES}
    ${MOVEGEN_SOURCES}
    ${EVAL_SOURCES}
    ${NNUE_SOURCES}
    ${SEARCH_SOURCES}
    ${UCI_SOURCES}
)

# Compiler-specific options
if(MSVC)
    target_compile_options(chess_engine PRIVATE /W4)
else()
    target_compile_options(chess_engine PRIVATE -Wall -Wextra -O3)
endif()

