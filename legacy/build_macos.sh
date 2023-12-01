mkdir -p bin

SOURCES="src/Term.cpp src/Polynomial.cpp src/ZeroSubstitution.cpp \
src/System.cpp src/Assertions.cpp src/Simplification.cpp src/ZeroOneSolver.cpp"

CLANG_WARN_FLAGS="-Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic \
-Wno-poison-system-directories"

GCC_WARN_FLAGS="-Wall -Wextra -pedantic"

clang++ -std=c++20 ${CLANG_WARN_FLAGS} -O1 -march=native \
    ${SOURCES} -o bin/ZeroOneSolverClangO1

clang++ -std=c++20 ${CLANG_WARN_FLAGS} -O1 -march=native -flto \
    ${SOURCES} -o bin/ZeroOneSolverClangLTO1

clang++ -std=c++20 ${CLANG_WARN_FLAGS} -O2 -march=native \
    ${SOURCES} -o bin/ZeroOneSolverClangO2

clang++ -std=c++20 ${CLANG_WARN_FLAGS} -O2 -march=native -flto \
    ${SOURCES} -o bin/ZeroOneSolverClangLTO2

clang++ -std=c++20 ${CLANG_WARN_FLAGS} -O3 -march=native \
    ${SOURCES} -o bin/ZeroOneSolverClangO3

clang++ -std=c++20 ${CLANG_WARN_FLAGS} -O3 -march=native -flto \
    ${SOURCES} -o bin/ZeroOneSolverClangLTO3

/opt/homebrew/opt/llvm/bin/clang++ -std=c++20 ${CLANG_WARN_FLAGS} -O1 -march=native \
    ${SOURCES} -o bin/ZeroOneSolverHClangO1

/opt/homebrew/opt/llvm/bin/clang++ -std=c++20 ${CLANG_WARN_FLAGS} -O1 -march=native -flto \
    ${SOURCES} -o bin/ZeroOneSolverHClangLTO1

/opt/homebrew/opt/llvm/bin/clang++ -std=c++20 ${CLANG_WARN_FLAGS} -O2 -march=native \
    ${SOURCES} -o bin/ZeroOneSolverHClangO2

/opt/homebrew/opt/llvm/bin/clang++ -std=c++20 ${CLANG_WARN_FLAGS} -O2 -march=native -flto \
    ${SOURCES} -o bin/ZeroOneSolverHClangLTO2

/opt/homebrew/opt/llvm/bin/clang++ -std=c++20 ${CLANG_WARN_FLAGS} -O3 -march=native \
    ${SOURCES} -o bin/ZeroOneSolverHClangO3

/opt/homebrew/opt/llvm/bin/clang++ -std=c++20 ${CLANG_WARN_FLAGS} -O3 -march=native -flto \
    ${SOURCES} -o bin/ZeroOneSolverHClangLTO3

g++-13 -std=c++20 ${GCC_WARN_FLAGS} -O1 -march=native \
    ${SOURCES} -o bin/ZeroOneSolverGCCO1

g++-13 -std=c++20 ${GCC_WARN_FLAGS} -O1 -march=native -flto \
    ${SOURCES} -o bin/ZeroOneSolverGCCLTO1

g++-13 -std=c++20 ${GCC_WARN_FLAGS} -O2 -march=native \
    ${SOURCES} -o bin/ZeroOneSolverGCCO2

g++-13 -std=c++20 ${GCC_WARN_FLAGS} -O2 -march=native -flto \
    ${SOURCES} -o bin/ZeroOneSolverGCCLTO2

g++-13 -std=c++20 ${GCC_WARN_FLAGS} -O3 -march=native \
    ${SOURCES} -o bin/ZeroOneSolverGCCO3

g++-13 -std=c++20 ${GCC_WARN_FLAGS} -O3 -march=native -flto \
    ${SOURCES} -o bin/ZeroOneSolverGCCLTO3
