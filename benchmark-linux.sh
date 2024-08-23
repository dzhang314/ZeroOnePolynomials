#!/bin/sh

mkdir -p bin

MACRO_DEFINITIONS="-DZERO_ONE_SOLVER_M=14 -DZERO_ONE_SOLVER_N=30 -DZERO_ONE_SOLVER_VERBOSE=false"

GCC_COMPILERS=(
    "g++"
    "g++-10"
    "g++-11"
    "g++-12"
    "g++-13"
    "g++-14"
)

GCC_FLAGS="-Wall -Wextra -pedantic -std=c++20 $MACRO_DEFINITIONS"

for COMPILER in "${GCC_COMPILERS[@]}"; do
    if command -v $COMPILER &> /dev/null; then
        echo "Compiling ZeroOneSolver.cpp with $COMPILER..."
        $COMPILER $GCC_FLAGS -O3 ZeroOneSolver.cpp -o bin/${COMPILER}-O3
        echo "Finished compiling bin/${COMPILER}-O3."
        $COMPILER $GCC_FLAGS -O3 -flto ZeroOneSolver.cpp -o bin/${COMPILER}-O3-LTO
        echo "Finished compiling bin/${COMPILER}-O3-LTO."
        $COMPILER $GCC_FLAGS -O3 -fwhole-program ZeroOneSolver.cpp -o bin/${COMPILER}-O3-WP
        echo "Finished compiling bin/${COMPILER}-O3-WP."
        $COMPILER $GCC_FLAGS -O3 -flto -fwhole-program ZeroOneSolver.cpp -o bin/${COMPILER}-O3-LTO-WP
        echo "Finished compiling bin/${COMPILER}-O3-LTO-WP."
        $COMPILER $GCC_FLAGS -O3 -march=native ZeroOneSolver.cpp -o bin/${COMPILER}-O3-NATIVE
        echo "Finished compiling bin/${COMPILER}-O3-NATIVE."
        $COMPILER $GCC_FLAGS -O3 -march=native -flto ZeroOneSolver.cpp -o bin/${COMPILER}-O3-NATIVE-LTO
        echo "Finished compiling bin/${COMPILER}-O3-NATIVE-LTO."
        $COMPILER $GCC_FLAGS -O3 -march=native -fwhole-program ZeroOneSolver.cpp -o bin/${COMPILER}-O3-NATIVE-WP
        echo "Finished compiling bin/${COMPILER}-O3-NATIVE-WP."
        $COMPILER $GCC_FLAGS -O3 -march=native -flto -fwhole-program ZeroOneSolver.cpp -o bin/${COMPILER}-O3-NATIVE-LTO-WP
        echo "Finished compiling bin/${COMPILER}-O3-NATIVE-LTO-WP."
    fi
done

CLANG_COMPILERS=(
    "clang++"
    "clang++-14"
    "clang++-15"
    "clang++-16"
    "clang++-17"
    "clang++-18"
)

CLANG_FLAGS="-Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-c++20-compat \
-Wno-unsafe-buffer-usage -Wno-switch-default -std=c++20 $MACRO_DEFINITIONS"

for COMPILER in "${CLANG_COMPILERS[@]}"; do
    if command -v $COMPILER &> /dev/null; then
        echo "Compiling ZeroOneSolver.cpp with $COMPILER..."
        $COMPILER $CLANG_FLAGS -O3 ZeroOneSolver.cpp -o bin/${COMPILER}-O3
        echo "Finished compiling bin/${COMPILER}-O3."
        $COMPILER $CLANG_FLAGS -O3 -flto ZeroOneSolver.cpp -o bin/${COMPILER}-O3-LTO
        echo "Finished compiling bin/${COMPILER}-O3-LTO."
        $COMPILER $CLANG_FLAGS -O3 -march=native ZeroOneSolver.cpp -o bin/${COMPILER}-O3-NATIVE
        echo "Finished compiling bin/${COMPILER}-O3-NATIVE."
        $COMPILER $CLANG_FLAGS -O3 -march=native -flto ZeroOneSolver.cpp -o bin/${COMPILER}-O3-NATIVE-LTO
        echo "Finished compiling bin/${COMPILER}-O3-NATIVE-LTO."
    fi
done

hyperfine --warmup 2 --runs 5 bin/g++* bin/clang++*
