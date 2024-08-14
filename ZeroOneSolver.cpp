/******************************************************************************
 * Copyright (c) 2023-2024 David K. Zhang
 * This file is part of ZeroOnePolynomials by David K. Zhang.
 * ZeroOnePolynomials is distributed under the terms of the MIT license.
 * See the LICENSE file for full license details.
 ******************************************************************************/

#include <bitset>
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
#warning "ZERO_ONE_SOLVER_M not defined; using default value 7."
constexpr var_index_t M = 13;
#endif


#ifdef ZERO_ONE_SOLVER_N
constexpr var_index_t N = ZERO_ONE_SOLVER_N;
#else
#warning "ZERO_ONE_SOLVER_N not defined; using default value 13."
constexpr var_index_t N = 17;
#endif


static std::ostream &operator<<(std::ostream &os, const Term &term) {
    if (term == TERM_ZERO) {
        os << '0';
    } else if (term.p_index) {
        os << 'p' << static_cast<int>(term.p_index);
        if (term.q_index) { os << "*q" << static_cast<int>(term.q_index); }
    } else if (term.q_index) {
        os << 'q' << static_cast<int>(term.q_index);
    } else {
        os << '1';
    }
    return os;
}


constexpr bool SKIP_ZERO_TERMS = false;


static std::ostream &operator<<(std::ostream &os, const System<M, N> &system) {
    // clang-format off
    for (std::size_t e = 0; e < M + N - 1; ++e) {
        bool first = true;
        for (std::size_t t = 0; t < M + 1; ++t) {
            const Term term = system.lhs[e][t];
            if constexpr (SKIP_ZERO_TERMS) { if (term == TERM_ZERO) { continue; } }
            if (first) { first = false; } else { os << " + "; }
            os << term;
        }
        switch (system.rhs.get(e)) {
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
    // clang-format on
    return os;
}


static void print_leaf_system(const System<M, N> &system) {
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
            std::cout << '\n';
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
    std::cout << '\n';
}


static bool
find_case_split(std::vector<System<M, N>> &stack, const System<M, N> &system) {

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

    for (std::size_t e = 0; e < M + N - 1; ++e) {
        const RHS rhs_value = system.rhs.get(e);
        if (rhs_value == RHS::ZERO) {
            // ... + p_i*q_j + ... == 0
            for (std::size_t t = 0; t < M + 1; ++t) {
                const Term term = system.lhs[e][t];
                if (term != TERM_ZERO) {
                    // Any remaining nonzero term must have the form p_i*q_j
                    // because terms of the form 1, p_i, and q_j are removed
                    // during simplification.
                    stack.push_back(system);
                    stack.back().set_q_zero(term.q_index);
                    stack.push_back(system);
                    stack.back().set_p_zero(term.p_index);
                    return true;
                }
            }
        } else if (rhs_value == RHS::ZERO_OR_ONE) {
            // p_i*q_j == 0 or 1
            const Term term = system.find_unique_nonzero_term(e);
            if (term != TERM_ZERO) {
                // Any remaining unique nonzero term must have the form p_i*q_j
                // because case splits on p_i == 0 or 1 and q_j == 0 or 1 have
                // already been performed in the previous step.
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

    // ... == 0 or 1
    for (std::size_t e = 0; e < M + N - 1; ++e) {
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


static void analyze(const System<M, N> &initial) {
    std::vector<System<M, N>> stack;
    stack.push_back(initial);
    while (!stack.empty()) {
        System<M, N> system = stack.back();
        stack.pop_back();
        if (system.simplify() && system.has_unknown_variable()) {
            if (!find_case_split(stack, system)) { print_leaf_system(system); }
        }
    }
}


/******************************************************************************
 * Because the coefficients of x^M and x^N in P(x) * Q(x) contain the term 1,
 * we know that all other terms in those coefficients must be zero.
 *
 *     p_1*q_(M-1) == p_2*q_(M-2) == ... == p_(M-1)*q_1 == q_M == 0
 *     p_1*q_(N-1) == p_2*q_(N-2) == ... == p_(M-1)*q_(N-M+1) == q_N == 0
 *
 * This implies that q_M == q_N == 0, and for each 1 <= i <= M - 1, p_i == 0 or
 * or q_(M-i) == q_(N-i) == 0. Using this observation, we consider 2^(M-1)
 * cases corresponding to these binary choices.
 ******************************************************************************/
static void case_split_p(var_index_t depth, System<M, N> system);
static void case_split_q(var_index_t depth, System<M, N> system);


static void case_split_p(var_index_t depth, System<M, N> system) {
    system.set_p_zero(depth);
    if (system.simplify() && system.has_unknown_variable()) {
        if (depth < M - 1) {
            case_split_p(depth + 1, system);
            case_split_q(depth + 1, system);
        } else {
            analyze(system);
        }
    }
}


static void case_split_q(var_index_t depth, System<M, N> system) {
    system.set_q_zero(M - depth);
    system.set_q_zero(N - depth);
    if (system.simplify() && system.has_unknown_variable()) {
        if (depth < M - 1) {
            case_split_p(depth + 1, system);
            case_split_q(depth + 1, system);
        } else {
            analyze(system);
        }
    }
}


consteval System<M, N> initial_system() noexcept {
    System<M, N> system;
    if (system.simplify()) { return system; }
    // Intentionally produce a compile-time error if simplification fails.
}


int main() {
    case_split_p(1, initial_system());
    case_split_q(1, initial_system());
}
