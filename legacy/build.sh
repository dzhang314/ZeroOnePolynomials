mkdir -p bin

clang++ -std=c++20 -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic \
    -Wno-poison-system-directories \
    -O3 -march=native -flto \
    src/Term.cpp src/Polynomial.cpp src/ZeroSubstitution.cpp src/System.cpp \
    src/Assertions.cpp src/Simplification.cpp src/ZeroOneSolver.cpp \
    -o bin/ZeroOneSolver
