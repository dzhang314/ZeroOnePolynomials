#!/bin/sh

GCC_EXECUTABLE="/opt/homebrew/bin/g++-14"
CLANG_EXECUTABLE="/opt/homebrew/opt/llvm/bin/clang++"
APPLE_CLANG_EXECUTABLE="clang++"

MACRO_DEFINITIONS="-DZERO_ONE_SOLVER_M=14 -DZERO_ONE_SOLVER_N=30 -DZERO_ONE_SOLVER_VERBOSE=false"
GCC_FLAGS="-Wall -Wextra -pedantic -std=c++20 $MACRO_DEFINITIONS"
CLANG_FLAGS="-Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic \
-Wno-unsafe-buffer-usage -Wno-poison-system-directories -std=c++20 $MACRO_DEFINITIONS"

mkdir -p bin

$GCC_EXECUTABLE $GCC_FLAGS -O2 ZeroOneSolver.cpp -o bin/GCC-O2
$GCC_EXECUTABLE $GCC_FLAGS -O2 -flto ZeroOneSolver.cpp -o bin/GCC-O2-LTO
$GCC_EXECUTABLE $GCC_FLAGS -O2 -fwhole-program ZeroOneSolver.cpp -o bin/GCC-O2-WP
$GCC_EXECUTABLE $GCC_FLAGS -O2 -flto -fwhole-program ZeroOneSolver.cpp -o bin/GCC-O2-LTO-WP
$GCC_EXECUTABLE $GCC_FLAGS -O2 -march=native ZeroOneSolver.cpp -o bin/GCC-O2-NATIVE
$GCC_EXECUTABLE $GCC_FLAGS -O2 -march=native -flto ZeroOneSolver.cpp -o bin/GCC-O2-NATIVE-LTO
$GCC_EXECUTABLE $GCC_FLAGS -O2 -march=native -fwhole-program ZeroOneSolver.cpp -o bin/GCC-O2-NATIVE-WP
$GCC_EXECUTABLE $GCC_FLAGS -O2 -march=native -flto -fwhole-program ZeroOneSolver.cpp -o bin/GCC-O2-NATIVE-LTO-WP
$GCC_EXECUTABLE $GCC_FLAGS -O3 ZeroOneSolver.cpp -o bin/GCC-O3
$GCC_EXECUTABLE $GCC_FLAGS -O3 -flto ZeroOneSolver.cpp -o bin/GCC-O3-LTO
$GCC_EXECUTABLE $GCC_FLAGS -O3 -fwhole-program ZeroOneSolver.cpp -o bin/GCC-O3-WP
$GCC_EXECUTABLE $GCC_FLAGS -O3 -flto -fwhole-program ZeroOneSolver.cpp -o bin/GCC-O3-LTO-WP
$GCC_EXECUTABLE $GCC_FLAGS -O3 -march=native ZeroOneSolver.cpp -o bin/GCC-O3-NATIVE
$GCC_EXECUTABLE $GCC_FLAGS -O3 -march=native -flto ZeroOneSolver.cpp -o bin/GCC-O3-NATIVE-LTO
$GCC_EXECUTABLE $GCC_FLAGS -O3 -march=native -fwhole-program ZeroOneSolver.cpp -o bin/GCC-O3-NATIVE-WP
$GCC_EXECUTABLE $GCC_FLAGS -O3 -march=native -flto -fwhole-program ZeroOneSolver.cpp -o bin/GCC-O3-NATIVE-LTO-WP

$CLANG_EXECUTABLE $CLANG_FLAGS -O2 ZeroOneSolver.cpp -o bin/CLANG-O2
$CLANG_EXECUTABLE $CLANG_FLAGS -O2 -flto ZeroOneSolver.cpp -o bin/CLANG-O2-LTO
$CLANG_EXECUTABLE $CLANG_FLAGS -O2 -march=native ZeroOneSolver.cpp -o bin/CLANG-O2-NATIVE
$CLANG_EXECUTABLE $CLANG_FLAGS -O2 -march=native -flto ZeroOneSolver.cpp -o bin/CLANG-O2-NATIVE-LTO
$CLANG_EXECUTABLE $CLANG_FLAGS -O3 ZeroOneSolver.cpp -o bin/CLANG-O3
$CLANG_EXECUTABLE $CLANG_FLAGS -O3 -flto ZeroOneSolver.cpp -o bin/CLANG-O3-LTO
$CLANG_EXECUTABLE $CLANG_FLAGS -O3 -march=native ZeroOneSolver.cpp -o bin/CLANG-O3-NATIVE
$CLANG_EXECUTABLE $CLANG_FLAGS -O3 -march=native -flto ZeroOneSolver.cpp -o bin/CLANG-O3-NATIVE-LTO

$APPLE_CLANG_EXECUTABLE $CLANG_FLAGS -O2 ZeroOneSolver.cpp -o bin/APPLE-CLANG-O2
$APPLE_CLANG_EXECUTABLE $CLANG_FLAGS -O2 -flto ZeroOneSolver.cpp -o bin/APPLE-CLANG-O2-LTO
$APPLE_CLANG_EXECUTABLE $CLANG_FLAGS -O2 -march=native ZeroOneSolver.cpp -o bin/APPLE-CLANG-O2-NATIVE
$APPLE_CLANG_EXECUTABLE $CLANG_FLAGS -O2 -march=native -flto ZeroOneSolver.cpp -o bin/APPLE-CLANG-O2-NATIVE-LTO
$APPLE_CLANG_EXECUTABLE $CLANG_FLAGS -O3 ZeroOneSolver.cpp -o bin/APPLE-CLANG-O3
$APPLE_CLANG_EXECUTABLE $CLANG_FLAGS -O3 -flto ZeroOneSolver.cpp -o bin/APPLE-CLANG-O3-LTO
$APPLE_CLANG_EXECUTABLE $CLANG_FLAGS -O3 -march=native ZeroOneSolver.cpp -o bin/APPLE-CLANG-O3-NATIVE
$APPLE_CLANG_EXECUTABLE $CLANG_FLAGS -O3 -march=native -flto ZeroOneSolver.cpp -o bin/APPLE-CLANG-O3-NATIVE-LTO

hyperfine --warmup 2 --runs 5 \
    "bin/GCC-O2" "bin/GCC-O2-LTO" "bin/GCC-O2-WP" "bin/GCC-O2-LTO-WP" \
    "bin/GCC-O2-NATIVE" "bin/GCC-O2-NATIVE-LTO" "bin/GCC-O2-NATIVE-WP" "bin/GCC-O2-NATIVE-LTO-WP" \
    "bin/GCC-O3" "bin/GCC-O3-LTO" "bin/GCC-O3-WP" "bin/GCC-O3-LTO-WP" \
    "bin/GCC-O3-NATIVE" "bin/GCC-O3-NATIVE-LTO" "bin/GCC-O3-NATIVE-WP" "bin/GCC-O3-NATIVE-LTO-WP" \
    "bin/CLANG-O2" "bin/CLANG-O2-LTO" "bin/CLANG-O2-NATIVE" "bin/CLANG-O2-NATIVE-LTO" \
    "bin/CLANG-O3" "bin/CLANG-O3-LTO" "bin/CLANG-O3-NATIVE" "bin/CLANG-O3-NATIVE-LTO" \
    "bin/APPLE-CLANG-O2" "bin/APPLE-CLANG-O2-LTO" "bin/APPLE-CLANG-O2-NATIVE" "bin/APPLE-CLANG-O2-NATIVE-LTO" \
    "bin/APPLE-CLANG-O3" "bin/APPLE-CLANG-O3-LTO" "bin/APPLE-CLANG-O3-NATIVE" "bin/APPLE-CLANG-O3-NATIVE-LTO"
