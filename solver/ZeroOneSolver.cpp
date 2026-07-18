/******************************************************************************
 * Copyright (c) 2023-2026 David K. Zhang
 * This file is part of ZeroOnePolynomials by David K. Zhang.
 * ZeroOnePolynomials is distributed under the terms of the MIT license.
 * See the LICENSE file for full license details.
 ******************************************************************************/

#include <bitset>   // for std::bitset
#include <cassert>  // for assert
#include <cstddef>  // for std::size_t
#include <cstdint>  // for std::uint8_t
#include <cstdlib>  // for std::strtoull
#include <iostream> // for std::ostream, std::cout, std::cerr, std::flush
#include <vector>   // for std::vector

#include "ZeroOneSolver.hpp"

using ZeroOneSolver::RHS;
using ZeroOneSolver::System;
using ZeroOneSolver::Term;
using ZeroOneSolver::TERM_ZERO;
using ZeroOneSolver::VAR;
using ZeroOneSolver::var_index_t;


#ifdef ZERO_ONE_SOLVER_M
static_assert(ZERO_ONE_SOLVER_M > 1);
static_assert(ZERO_ONE_SOLVER_M < 255);
constexpr var_index_t M = ZERO_ONE_SOLVER_M;
#else
#error "ZERO_ONE_SOLVER_M must be defined."
#endif


#ifdef ZERO_ONE_SOLVER_N
static_assert(ZERO_ONE_SOLVER_N > ZERO_ONE_SOLVER_M);
static_assert(ZERO_ONE_SOLVER_N < 256);
constexpr var_index_t N = ZERO_ONE_SOLVER_N;
#else
#error "ZERO_ONE_SOLVER_N must be defined."
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


static std::ostream &operator<<(std::ostream &os, const System<M, N> &system) {
    for (std::size_t e = 0; e < M + N - 3; ++e) {
        bool first = true;
        for (std::size_t t = 0; t < M + 1; ++t) {
            const Term term = system.lhs[e][t];
            if (term == TERM_ZERO) { continue; }
            if (first) {
                first = false;
            } else {
                os << " + ";
            }
            os << term;
        }
        const RHS rhs = system.rhs.get(e);
        if (first && (rhs == RHS::ZERO)) {
            continue;
        } else if (first) {
            os << '0';
        }
        switch (rhs) { // clang-format off
            case RHS::ZERO_OR_ONE: os << " == 0 or 1\n"; break;
            case RHS::ZERO:        os << " == 0\n";      break;
            case RHS::ONE:         os << " == 1\n";      break;
        } // clang-format on
    }
    for (var_index_t i = 1; i <= M - 1; ++i) {
        switch (system.p.get(i - 1)) { // clang-format off
            case VAR::UNKNOWN:     os << "0 <= p" << static_cast<int>(i) << " <= 1\n"; break;
            case VAR::ZERO_OR_ONE: os << 'p' << static_cast<int>(i) << " == 0 or 1\n"; break;
            case VAR::ZERO:        os << 'p' << static_cast<int>(i) << " == 0\n";      break;
            case VAR::ONE:         os << 'p' << static_cast<int>(i) << " == 1\n";      break;
        } // clang-format on
    }
    for (var_index_t j = 1; j <= N - 1; ++j) {
        switch (system.q.get(j - 1)) { // clang-format off
            case VAR::UNKNOWN:     os << "0 <= q" << static_cast<int>(j) << " <= 1\n"; break;
            case VAR::ZERO_OR_ONE: os << 'q' << static_cast<int>(j) << " == 0 or 1\n"; break;
            case VAR::ZERO:        os << 'q' << static_cast<int>(j) << " == 0\n";      break;
            case VAR::ONE:         os << 'q' << static_cast<int>(j) << " == 1\n";      break;
        } // clang-format on
    }
    return os;
}


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


struct StructuralData {

    std::size_t equation_count;
    std::size_t equation_indices[M + N - 3];
    std::uint8_t term_counts[M + N - 3];
    std::uint8_t linear_term_counts[M + N - 3];
    std::bitset<M - 1> linear_p_terms[M + N - 3];
    std::bitset<N - 1> linear_q_terms[M + N - 3];

    explicit StructuralData(const System<M, N> &system) noexcept
        : equation_count(0)
        , equation_indices{}
        , term_counts{}
        , linear_term_counts{}
        , linear_p_terms{}
        , linear_q_terms{} {
        // Identify equations with RHS 1.
        for (std::size_t e = 0; e < M + N - 3; ++e) {
            if (system.rhs.get(e) == RHS::ONE) {
                const std::size_t i = equation_count++;
                equation_indices[i] = e;
                for (std::size_t t = 0; t < M + 1; ++t) {
                    const Term term = system.lhs[e][t];
                    assert(term != ZeroOneSolver::TERM_ONE);
                    if (term == TERM_ZERO) { continue; }
                    // Count terms in each equation with RHS 1.
                    ++term_counts[i];
                    if (term.q_index == 0) {
                        ++linear_term_counts[i];
                        linear_p_terms[i].set(term.p_index - 1);
                    } else if (term.p_index == 0) {
                        ++linear_term_counts[i];
                        linear_q_terms[i].set(term.q_index - 1);
                    }
                }
            }
        }
    }

    bool can_dominate(std::size_t i, std::size_t j) const noexcept {
        return ((i != j) && (term_counts[i] <= linear_term_counts[j]) &&
                (linear_p_terms[i] & ~linear_p_terms[j]).none() &&
                (linear_q_terms[i] & ~linear_q_terms[j]).none() &&
                ((term_counts[i] != term_counts[j]) ||
                 (term_counts[i] != linear_term_counts[i])));
    }

}; // struct StructuralData


struct DominationConsequences {

    std::bitset<M - 1> p_one;
    std::bitset<N - 1> q_zero;
    std::bitset<N - 1> q_one;
    Term split_term;

    constexpr DominationConsequences() noexcept
        : p_one{}
        , q_zero{}
        , q_one{}
        , split_term(TERM_ZERO) {}

    bool has_simplifications() const noexcept {
        return p_one.any() || q_zero.any() || q_one.any();
    }

}; // struct DominationConsequences


enum class DominationStatus : std::uint8_t {
    NO_DOMINATION,
    FOUND_DOMINATION,
    FOUND_CONTRADICTION,
};


static DominationStatus find_domination_consequences(
    DominationConsequences &consequences, const System<M, N> &system,
    std::size_t e_i, std::size_t e_j, const std::bitset<M - 1> &linear_p_terms,
    const std::bitset<N - 1> &linear_q_terms) {

    // Match terms and deduce consequences.
    std::bitset<M - 1> p_matched;
    std::bitset<N - 1> q_matched;
    for (std::size_t t = 0; t < M + 1; ++t) {
        const Term term = system.lhs[e_i][t];
        assert(term != ZeroOneSolver::TERM_ONE);
        if (term == TERM_ZERO) { continue; }
        if (term.q_index == 0) {
            // Linear term; aready checked by can_dominate.
            p_matched.set(term.p_index - 1);
        } else if (term.p_index == 0) {
            // Linear term; already checked by can_dominate.
            q_matched.set(term.q_index - 1);
        } else {
            // Match terms greedily; terms are unique, so we can't get stuck.
            assert(system.p_positive.get(term.p_index - 1));
            if (linear_p_terms.test(term.p_index - 1)) {
                // p_i == p_i*q_j implies q_j == 1.
                p_matched.set(term.p_index - 1);
                consequences.q_one.set(term.q_index - 1);
            } else if (linear_q_terms.test(term.q_index - 1)) {
                // q_j == p_i*q_j implies q_j == 0 or p_i == 1.
                q_matched.set(term.q_index - 1);
                if (system.q_positive.get(term.q_index - 1)) {
                    consequences.p_one.set(term.p_index - 1);
                } else if (consequences.split_term == TERM_ZERO) {
                    consequences.split_term = term;
                }
            } else {
                return DominationStatus::NO_DOMINATION;
            }
        }
    }

    // Deduce additional consequences from unmatched terms.
    for (std::size_t t = 0; t < M + 1; ++t) {
        const Term term = system.lhs[e_j][t];
        assert(term != ZeroOneSolver::TERM_ONE);
        if (term == TERM_ZERO) { continue; }
        if (term.q_index == 0) {
            // Unmatched p_i must be zero, contradicting p_i > 0.
            if (!p_matched.test(term.p_index - 1)) {
                assert(system.p_positive.get(term.p_index - 1));
                return DominationStatus::FOUND_CONTRADICTION;
            }
        } else if (term.p_index == 0) {
            // Unmatched q_j must be zero.
            if (!q_matched.test(term.q_index - 1)) {
                consequences.q_zero.set(term.q_index - 1);
            }
        } else {
            // Unmatched p_i*q_j implies q_j == 0, since p_i > 0.
            assert(system.p_positive.get(term.p_index - 1));
            consequences.q_zero.set(term.q_index - 1);
        }
    }

    return (consequences.q_zero & consequences.q_one).any()
               ? DominationStatus::FOUND_CONTRADICTION
               : DominationStatus::FOUND_DOMINATION;
}


static bool find_domination_move(std::vector<System<M, N>> &stack,
                                 const System<M, N> &system) {

    const StructuralData data(system);
    if (data.equation_count < 2) { return false; }

    Term split_term = TERM_ZERO;
    for (std::size_t i = 0; i < data.equation_count; ++i) {
        const std::size_t e_i = data.equation_indices[i];
        for (std::size_t j = 0; j < data.equation_count; ++j) {
            const std::size_t e_j = data.equation_indices[j];
            if (!data.can_dominate(i, j)) { continue; }

            DominationConsequences consequences;
            const DominationStatus status = find_domination_consequences(
                consequences, system, e_i, e_j, data.linear_p_terms[j],
                data.linear_q_terms[j]);
            if (status == DominationStatus::NO_DOMINATION) { continue; }
            if (status == DominationStatus::FOUND_CONTRADICTION) {
                return true;
            }
            assert(status == DominationStatus::FOUND_DOMINATION);

            if (consequences.has_simplifications()) {
                stack.push_back(system);
                for (var_index_t q = 1; q <= N - 1; ++q) {
                    if (consequences.q_zero.test(q - 1) &&
                        !stack.back().set_q_zero(q)) {
                        stack.pop_back();
                        return true;
                    }
                }
                for (var_index_t p = 1; p <= M - 1; ++p) {
                    if (consequences.p_one.test(p - 1)) {
                        stack.back().set_p_one(p);
                    }
                }
                for (var_index_t q = 1; q <= N - 1; ++q) {
                    if (consequences.q_one.test(q - 1)) {
                        stack.back().set_q_one(q);
                    }
                }
                return true;
            }
            if (split_term == TERM_ZERO) {
                split_term = consequences.split_term;
            }
        }
    }

    if (split_term != TERM_ZERO) {
        stack.push_back(system);
        stack.back().set_q_positive(split_term.q_index);
        stack.back().set_p_one(split_term.p_index);
        stack.push_back(system);
        if (!stack.back().set_q_zero(split_term.q_index)) { stack.pop_back(); }
        return true;
    }
    return false;
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

    // Phase 2: Split ... + p_i*q_j + ... == 0 into p_i == q_j == 0;
    // p_i == 0 and q_j > 0; and p_i > 0 and q_j == 0.
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
                    stack.back().set_p_positive(term.p_index);
                    if (!stack.back().set_q_zero(term.q_index)) {
                        stack.pop_back();
                    }
                    stack.push_back(system);
                    stack.back().set_q_positive(term.q_index);
                    if (!stack.back().set_p_zero(term.p_index)) {
                        stack.pop_back();
                    }
                    stack.push_back(system);
                    if (!(stack.back().set_p_zero(term.p_index) &&
                          stack.back().set_q_zero(term.q_index))) {
                        stack.pop_back();
                    }
                    return true;
                }
            }
        }
    }

    // Phase 3: Split p_i*q_j == 0 or 1 into p_i == q_j == 0;
    // p_i == 0 and q_j > 0; p_i > 0 and q_j == 0; and p_i == q_j == 1.
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
                stack.back().set_p_positive(term.p_index);
                if (!stack.back().set_q_zero(term.q_index)) {
                    stack.pop_back();
                }
                stack.push_back(system);
                stack.back().set_q_positive(term.q_index);
                if (!stack.back().set_p_zero(term.p_index)) {
                    stack.pop_back();
                }
                stack.push_back(system);
                if (!(stack.back().set_p_zero(term.p_index) &&
                      stack.back().set_q_zero(term.q_index))) {
                    stack.pop_back();
                }
                return true;
            }
        }
    }

    // Phase 4: Find "dominating equations", i.e., pairs of equations
    // of the form LHS_i == 1 and LHS_j == 1 where each term in LHS_i
    // matches one-to-one to a larger term in LHS_j.
    if (find_domination_move(stack, system)) { return true; }

    // Phase 5: Split LHS == 0 or 1 into LHS == 0 and LHS == 1.
    std::size_t e_best = M + N - 3;
    std::size_t min_count = M + 2;
    for (std::size_t e = 0; e < M + N - 3; ++e) {
        if (system.rhs.get(e) == RHS::ZERO_OR_ONE) {
            std::size_t term_count = 0;
            for (std::size_t t = 0; t < M + 1; ++t) {
                term_count += (system.lhs[e][t] != TERM_ZERO);
            }
            if (term_count < min_count) {
                e_best = e;
                min_count = term_count;
            }
        }
    }
    if (e_best < M + N - 3) {
        stack.push_back(system);
        stack.back().rhs.set(e_best, RHS::ONE);
        stack.push_back(system);
        stack.back().rhs.set(e_best, RHS::ZERO);
        return true;
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
 * p_i == 0 or p_i > 0 and q_(M-i) == q_(N-i) == 0. Using this observation,
 * we consider 2^(M-1) cases corresponding to these binary choices, indexed
 * by a string of M - 1 bits that we call a case ID.
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
            initial.set_p_positive(i);
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


/******************************************************************************
 * A case ID and its reverse generate isomorphic systems of equations, since
 * reversing the case ID reverses the coefficients of P and Q (the leading
 * term becomes the constant term and vice versa). To eliminate redundancy, we
 * only consider case IDs that do not lexicographically exceed their reverse.
 ******************************************************************************/
static bool exceeds_reverse(const std::bitset<M - 1> &case_id) noexcept {
    for (std::size_t i = 0; i < M - 1; ++i) {
        const bool a = case_id[i];
        const bool b = case_id[M - 2 - i];
        if (a != b) { return b; }
    }
    return false;
}


int main(int argc, char **argv) {
    const bool range_supplied = (argc >= 3);
    unsigned long long remaining_cases =
        range_supplied ? std::strtoull(argv[2], nullptr, 10) : 0;
    std::bitset<M - 1> case_id(range_supplied ? argv[1] : "");
    do {
        if (range_supplied) {
            if (remaining_cases == 0) { break; }
            --remaining_cases;
        }
        if (!exceeds_reverse(case_id)) { analyze(case_id); }
    } while (increment(case_id));
}
