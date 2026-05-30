/******************************************************************************
 * Copyright (c) 2023-2026 David K. Zhang
 * This file is part of ZeroOnePolynomials by David K. Zhang.
 * ZeroOnePolynomials is distributed under the terms of the MIT license.
 * See the LICENSE file for full license details.
 ******************************************************************************/

#include <bitset>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <vector>

#include "ZeroOneSolver.hpp"

using ZeroOneSolver::RHS;
using ZeroOneSolver::System;
using ZeroOneSolver::Term;
using ZeroOneSolver::TERM_ZERO;
using ZeroOneSolver::VAR;
using ZeroOneSolver::var_index_t;


#ifdef ZERO_ONE_SOLVER_M
constexpr var_index_t M = ZERO_ONE_SOLVER_M;
#else
#error "ZERO_ONE_SOLVER_M must be defined."
#endif


#ifdef ZERO_ONE_SOLVER_N
constexpr var_index_t N = ZERO_ONE_SOLVER_N;
#else
#error "ZERO_ONE_SOLVER_N must be defined."
#endif


// clang-format off
constexpr bool SKIP_ZERO_TERMS = true;
constexpr bool SKIP_ZERO_EQUATIONS = true;


static std::ostream &operator<<(std::ostream &os, const Term &term) {
    if (term == TERM_ZERO) { os << '0'; }
    else if (term.p_index) { os << 'p' << static_cast<int>(term.p_index);
        if (term.q_index) { os << "*q" << static_cast<int>(term.q_index); } }
    else if (term.q_index) { os << 'q' << static_cast<int>(term.q_index); }
    else { os << '1'; }
    return os;
}


static std::ostream &operator<<(std::ostream &os, const System<M, N> &system) {
    for (std::size_t e = 0; e < M + N - 3; ++e) {
        bool first = true;
        for (std::size_t t = 0; t < M + 1; ++t) {
            const Term term = system.lhs[e][t];
            if constexpr (SKIP_ZERO_TERMS) { if (term == TERM_ZERO) { continue; } }
            if (first) { first = false; } else { os << " + "; }
            os << term;
        }
        const RHS rhs = system.rhs.get(e);
        if constexpr (SKIP_ZERO_EQUATIONS) { if (first && (rhs == RHS::ZERO)) { continue; } }
        else { if (first) { os << '0'; } }
        switch (rhs) {
            case RHS::ZERO_OR_ONE: os << " == 0 or 1\n"; break;
            case RHS::ZERO:        os << " == 0\n";      break;
            case RHS::ONE:         os << " == 1\n";      break;
        }
    }
    for (var_index_t i = 1; i <= M - 1; ++i) {
        switch (system.p.get(i - 1)) {
            case VAR::UNKNOWN:     os << "0 <= p" << static_cast<int>(i) << " <= 1\n"; break;
            case VAR::ZERO_OR_ONE: os << 'p' << static_cast<int>(i) << " == 0 or 1\n"; break;
            case VAR::ZERO:        os << 'p' << static_cast<int>(i) << " == 0\n";      break;
            case VAR::ONE:         os << 'p' << static_cast<int>(i) << " == 1\n";      break;
        }
    }
    for (var_index_t j = 1; j <= N - 1; ++j) {
        switch (system.q.get(j - 1)) {
            case VAR::UNKNOWN:     os << "0 <= q" << static_cast<int>(j) << " <= 1\n"; break;
            case VAR::ZERO_OR_ONE: os << 'q' << static_cast<int>(j) << " == 0 or 1\n"; break;
            case VAR::ZERO:        os << 'q' << static_cast<int>(j) << " == 0\n";      break;
            case VAR::ONE:         os << 'q' << static_cast<int>(j) << " == 1\n";      break;
        }
    }
    return os;
}
// clang-format on


static void print_leaf_system(const System<M, N> &system) {
    std::bitset<M - 1> p_used;
    std::bitset<N - 1> q_used;
    for (std::size_t e = 0; e < M + N - 3; ++e) {
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
                if (term.p_index) { p_used[term.p_index - 1] = true; }
                if (term.q_index) { q_used[term.q_index - 1] = true; }
                if (first) {
                    first = false;
                } else {
                    std::cout << " + ";
                }
                std::cout << term;
            }
            std::cout << '\n';
        }
    }
    for (var_index_t i = 1; i <= M - 1; ++i) {
        const VAR value = system.get_p(i);
        if ((value == VAR::ZERO) || (value == VAR::ONE)) {
            assert(!p_used.test(i - 1));
        } else {
            assert(value == VAR::UNKNOWN);
            if (!p_used[i - 1]) {
                std::cout << "0 <= p" << static_cast<int>(i) << " <= 1\n";
                std::cerr << "FOUND SYSTEM WITH UNCONSTRAINED VARIABLE!\n";
                std::cerr << system << std::flush;
            }
        }
    }
    for (var_index_t j = 1; j <= N - 1; ++j) {
        const VAR value = system.get_q(j);
        if ((value == VAR::ZERO) || (value == VAR::ONE)) {
            assert(!q_used.test(j - 1));
        } else {
            assert(value == VAR::UNKNOWN);
            if (!q_used[j - 1]) {
                std::cout << "0 <= q" << static_cast<int>(j) << " <= 1\n";
                std::cerr << "FOUND SYSTEM WITH UNCONSTRAINED VARIABLE!\n";
                std::cerr << system << std::flush;
            }
        }
    }
}


static bool find_case_split(std::vector<System<M, N>> &stack,
                            const System<M, N> &system) {

    // Phase 1.1: Split p_i == 0 or 1 into p_i == 0 and p_i == 1.
    for (var_index_t i = 1; i <= M - 1; ++i) {
        if (system.get_p(i) == VAR::ZERO_OR_ONE) {
            stack.push_back(system);
            stack.back().set_p_one(i);
            stack.push_back(system);
            stack.back().set_p_zero(i);
            return true;
        }
    }

    // Phase 1.2: Split q_j == 0 or 1 into q_j == 0 and q_j == 1.
    for (var_index_t j = 1; j <= N - 1; ++j) {
        if (system.get_q(j) == VAR::ZERO_OR_ONE) {
            stack.push_back(system);
            stack.back().set_q_one(j);
            stack.push_back(system);
            stack.back().set_q_zero(j);
            return true;
        }
    }

    // Phase 2: Split ... + p_i*q_j + ... == 0 into p_i == 0 and q_j == 0.
    for (std::size_t e = 0; e < M + N - 3; ++e) {
        if (system.rhs.get(e) == RHS::ZERO) {
            for (std::size_t t = 0; t < M + 1; ++t) {
                const Term term = system.lhs[e][t];
                if (term != TERM_ZERO) {
                    // Any remaining nonzero term must have the form p_i*q_j
                    // because terms of the form 1, p_i, and q_j are removed
                    // during simplification.
                    assert(term.p_index);
                    assert(term.q_index);
                    stack.push_back(system);
                    stack.back().set_q_zero(term.q_index);
                    stack.push_back(system);
                    stack.back().set_p_zero(term.p_index);
                    return true;
                }
            }
        }
    }

    // Phase 3: Split p_i*q_j == 0 or 1 into
    // p_i == 0, q_j == 0, and p_i == q_j == 1.
    for (std::size_t e = 0; e < M + N - 3; ++e) {
        if (system.rhs.get(e) == RHS::ZERO_OR_ONE) {
            const Term term = system.find_unique_nonzero_term(e);
            if (term != TERM_ZERO) {
                // Any remaining unique nonzero term must have the form p_i*q_j
                // because case splits on p_i == 0 or 1 and q_j == 0 or 1 have
                // already been performed in Phase 1.
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

    // Phase 4: Split LHS == 0 or 1 into LHS == 0 and LHS == 1.
    for (std::size_t e = 0; e < M + N - 3; ++e) {
        if (system.rhs.get(e) == RHS::ZERO_OR_ONE) {
            stack.push_back(system);
            stack.back().rhs.set(e, RHS::ONE);
            stack.push_back(system);
            stack.back().rhs.set(e, RHS::ZERO);
            return true;
        }
    }

    return false;
}


/******************************************************************************
 * Because the coefficients of x^M and x^N in P(x) * Q(x) contain the term 1,
 * we know that all other terms in those coefficients must be zero.
 *
 *     p_1*q_(M-1) == p_2*q_(M-2) == ... == p_(M-1)*q_1 == q_M == 0
 *     p_1*q_(N-1) == p_2*q_(N-2) == ... == p_(M-1)*q_(N-M+1) == q_(N-M) == 0
 *
 * This implies that q_M == q_(N-M) == 0, and for each 1 <= i <= M - 1,
 * p_i == 0 or q_(M-i) == q_(N-i) == 0. Using this observation, we consider
 * 2^(M-1) cases corresponding to these binary choices.
 ******************************************************************************/


consteval System<M, N> initial_system() noexcept {
    System<M, N> system;
    system.set_q_zero(M);
    system.set_q_zero(N - M);
    system.simplify();
    return system;
}


static void analyze(const std::bitset<M - 1> &case_id) {
    std::vector<System<M, N>> stack;
    System<M, N> initial = initial_system();
    for (var_index_t i = 1; i <= M - 1; ++i) {
        if (case_id[i - 1]) {
            initial.set_q_zero(M - i);
            initial.set_q_zero(N - i);
        } else {
            initial.set_p_zero(i);
        }
    }
    stack.push_back(initial);
    while (!stack.empty()) {
        System<M, N> system = stack.back();
        stack.pop_back();
        if (system.simplify() && system.has_unknown_variable()) {
            if (!find_case_split(stack, system)) {
                print_leaf_system(system);
                std::cout << '\n';
            }
        }
    }
}


static bool increment(std::bitset<M - 1> &bitset) noexcept {
    for (std::size_t i = 0; i < M - 1; ++i) {
        if (bitset[i]) {
            bitset[i] = false;
        } else {
            bitset[i] = true;
            return true;
        }
    }
    return false;
}


int main() {
    std::bitset<M - 1> case_id;
    do { analyze(case_id); } while (increment(case_id));
}
