#include <bitset>   // for std::bitset
#include <cstddef>  // for std::size_t
#include <iostream> // for std::cout, std::cerr
#include <vector>   // for std::vector

#include "ZeroOneSolver.hpp"

using ZeroOneSolver::RHS;
using ZeroOneSolver::System;
using ZeroOneSolver::Term;
using ZeroOneSolver::TERM_ZERO;
using ZeroOneSolver::VAR;
using ZeroOneSolver::var_index_t;


template <std::size_t N>
constexpr bool increment(std::bitset<N> &bitset) noexcept {
    for (std::size_t i = 0; i < N; ++i) {
        if (bitset[i]) {
            bitset[i] = false;
        } else {
            bitset[i] = true;
            return true;
        }
    }
    return false;
}


template <var_index_t M, var_index_t N>
void print_leaf_system(const System<M, N> &system) {
    std::bitset<M - 1> p_used;
    std::bitset<N - 1> q_used;
    for (std::size_t e = 0; e < M + N - 1; ++e) {
        if (system.rhs.get(e) == RHS::ZERO) {
            for (std::size_t t = 0; t < M + 1; ++t) {
                assert(system.lhs[e][t] == TERM_ZERO);
            }
        } else {
            assert(system.rhs.get(e) == RHS::ONE);
            bool first = true;
            for (std::size_t t = 0; t < M + 1; ++t) {
                const Term term = system.lhs[e][t];
                if (term == TERM_ZERO) { continue; }
                if (term.p_index) { p_used.set(term.p_index - 1); }
                if (term.q_index) { q_used.set(term.q_index - 1); }
                if (first) {
                    first = false;
                } else {
                    std::cout << " + ";
                }
                std::cout << term;
            }
            std::cout << "\n";
        }
    }
    for (std::size_t i = 0; i < M - 1; ++i) {
        const VAR value = system.p.get(i);
        if ((value == VAR::ZERO) || (value == VAR::ONE)) {
            assert(!p_used.test(i));
        } else {
            assert(value == VAR::UNKNOWN);
            if (!p_used[i]) { std::cout << "0 <= p" << (i + 1) << " <= 1\n"; }
        }
    }
    for (std::size_t i = 0; i < N - 1; ++i) {
        const VAR value = system.q.get(i);
        if ((value == VAR::ZERO) || (value == VAR::ONE)) {
            assert(!q_used.test(i));
        } else {
            assert(value == VAR::UNKNOWN);
            if (!q_used[i]) { std::cout << "0 <= q" << (i + 1) << " <= 1\n"; }
        }
    }
    std::cout << "\n";
}


template <var_index_t M, var_index_t N, bool verbose>
bool find_case_split(
    std::vector<System<M, N>> &stack, const System<M, N> &system
) {

    constexpr std::size_t INVALID_INDEX = ~static_cast<std::size_t>(0);

    for (var_index_t p_index = 1; p_index <= M - 1; ++p_index) {
        if (system.p.get(p_index - 1) == VAR::ZERO_OR_ONE) {
            if constexpr (verbose) {
                std::cerr << "SPLIT ON P" << static_cast<int>(p_index) << "\n";
            }
            stack.push_back(system);
            stack.back().set_p_one(p_index);
            stack.push_back(system);
            stack.back().set_p_zero(p_index);
            return true;
        }
    }

    for (var_index_t q_index = 1; q_index <= N - 1; ++q_index) {
        if (system.q.get(q_index - 1) == VAR::ZERO_OR_ONE) {
            if constexpr (verbose) {
                std::cerr << "SPLIT ON Q" << static_cast<int>(q_index) << "\n";
            }
            stack.push_back(system);
            stack.back().set_q_one(q_index);
            stack.push_back(system);
            stack.back().set_q_zero(q_index);
            return true;
        }
    }

    for (std::size_t e = 0; e < M + N - 1; ++e) {
        const RHS rhs_value = system.rhs.get(e);
        if (rhs_value == RHS::ZERO) {
            for (std::size_t t = 0; t < M + 1; ++t) {
                const Term term = system.lhs[e][t];
                if (term != TERM_ZERO) {
                    if constexpr (verbose) {
                        std::cerr << "SPLIT ON P"
                                  << static_cast<int>(term.p_index) << " * Q"
                                  << static_cast<int>(term.q_index)
                                  << " == 0\n";
                    }
                    assert(term.p_index);
                    assert(term.q_index);
                    stack.push_back(system);
                    stack.back().set_q_zero(term.q_index);
                    stack.push_back(system);
                    stack.back().set_p_zero(term.p_index);
                    return true;
                }
            }
        } else if (rhs_value == RHS::ZERO_OR_ONE) {
            std::size_t term_index = INVALID_INDEX;
            for (std::size_t t = 0; t < M + 1; ++t) {
                if (system.lhs[e][t] != TERM_ZERO) {
                    if (term_index != INVALID_INDEX) {
                        term_index = INVALID_INDEX;
                        break;
                    } else {
                        term_index = t;
                    }
                }
            }
            if (term_index != INVALID_INDEX) {
                const Term term = system.lhs[e][term_index];
                if constexpr (verbose) {
                    std::cerr << "SPLIT ON P" << static_cast<int>(term.p_index)
                              << " * Q" << static_cast<int>(term.q_index)
                              << " == 0 or 1\n";
                }
                assert(term != TERM_ZERO);
                assert(term.p_index);
                assert(term.q_index);
                stack.push_back(system);
                stack.back().set_p_one(term.p_index);
                stack.back().set_q_one(term.q_index);
                stack.push_back(system);
                stack.back().set_q_zero(term.q_index);
                stack.push_back(system);
                stack.back().set_p_zero(term.p_index);
                return true;
            }
        }
    }

    for (std::size_t e = 0; e < M + N - 1; ++e) {
        if (system.rhs.get(e) == RHS::ZERO_OR_ONE) {
            if constexpr (verbose) {
                std::cerr << "SPLIT ON EQUATION " << e << "\n";
            }
            stack.push_back(system);
            stack.back().rhs.set(e, RHS::ONE);
            stack.push_back(system);
            stack.back().rhs.set(e, RHS::ZERO);
            return true;
        }
    }

    return false;
}


template <var_index_t M, var_index_t N, bool verbose>
void analyze_case(const std::bitset<M - 1> &case_index) {
    std::vector<System<M, N>> stack;
    stack.emplace_back();
    stack.back().set_case(case_index);
    while (!stack.empty()) {
        System<M, N> system = stack.back();
        stack.pop_back();
        if (system.simplify()) {
            if (system.has_unknown_variable()) {
                if (!find_case_split<M, N, verbose>(stack, system)) {
                    if constexpr (verbose) { std::cerr << "LEAF SYSTEM\n"; }
                    print_leaf_system(system);
                }
            } else {
                if constexpr (verbose) { std::cerr << "SOLVED SYSTEM\n"; }
            }
        } else {
            if constexpr (verbose) { std::cerr << "INCONSISTENT SYSTEM\n"; }
        }
    }
}


template <var_index_t M, var_index_t N, bool verbose>
void analyze() {
    std::bitset<M - 1> case_index;
    do {
        if constexpr (verbose) {
            std::cerr << "ANALYZING CASE " << case_index.to_string() << "\n";
        }
        analyze_case<M, N, verbose>(case_index);
    } while (increment(case_index));
}


int main() {
    analyze<ZERO_ONE_SOLVER_M, ZERO_ONE_SOLVER_N, ZERO_ONE_SOLVER_VERBOSE>();
}
