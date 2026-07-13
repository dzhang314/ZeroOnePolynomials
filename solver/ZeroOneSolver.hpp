/******************************************************************************
 * Copyright (c) 2023-2026 David K. Zhang
 * This file is part of ZeroOnePolynomials by David K. Zhang.
 * ZeroOnePolynomials is distributed under the terms of the MIT license.
 * See the LICENSE file for full license details.
 ******************************************************************************/

#ifndef ZERO_ONE_SOLVER_HPP_INCLUDED
#define ZERO_ONE_SOLVER_HPP_INCLUDED

#include <array>   // for std::array
#include <cassert> // for assert
#include <cstddef> // for std::size_t
#include <cstdint> // for std::uint8_t, std::uint16_t
#include <utility> // for std::pair

#include "PackedBooleanArray.hpp"
#include "TwoBitPackedArray.hpp"

namespace ZeroOneSolver {


/******************************************************************************
 * ZeroOneSolver is a high-performance C++20 program that analyzes systems of
 * bilinear equations of the following form:
 *
 *     p_1 + q_1 == 0 or 1
 *     p_1*q_1 + p_2 + q_2 == 0 or 1
 *     p_1*q_2 + p_2*q_1 + p_3 + q_3 == 0 or 1
 *     p_1*q_3 + p_2*q_2 + p_3*q_1 + p_4 + q_4 == 0 or 1
 *     ...
 *     p_(M-2)*q_(N-1) + p_(M-1)*q_(N-2) + p_(M-3) + q_(N-3) == 0 or 1
 *     p_(M-1)*q_(N-1) + p_(M-2) + q_(N-2) == 0 or 1
 *     p_(M-1) + q_(N-1) == 0 or 1
 *
 * Here, {p_1, p_2, ..., p_(M-1)} and {q_1, q_2, ..., q_(N-1)} are real
 * variables taking values in the closed interval [0, 1]. This system arises in
 * the study of the 0-1 Polynomial Conjecture from the coefficients of the
 * product P(x) * Q(x) of the following polynomials:
 *
 *     P(x) = x^M + p_(M-1)*x^(M-1) + p_(M-2)*x^(M-2) + ... + p_1*x + 1
 *     Q(x) = x^N + q_(N-1)*x^(N-1) + q_(N-2)*x^(N-2) + ... + q_1*x + 1
 *
 * Note that P and Q are assumed to be monic in the statement of the 0-1
 * Polynomial Conjecture. Moreover, we can assume without loss of generality
 * that both P and Q have nonzero constant term. This implies that they both
 * have constant term 1, since p_0 and q_0 lie in [0, 1] and p_0*q_0 == 1.
 *
 * ZeroOneSolver attempts to prove that all solutions of such systems are
 * {0, 1}-valued, i.e., no variable ever lies strictly between 0 and 1. This
 * shows that the 0-1 Polynomial Conjecture holds for given values of M and N.
 ******************************************************************************/


// We assume that the parameters M and N, which define the number of variables
// in the system, are known at compile time and satisfy 1 < M < N < 256.
using var_index_t = std::uint8_t;


/******************************************************************************
 * A `Term` is an ordered pair of `var_index_t` values that represents a
 * monomial of the form 0, 1, p_i, q_j, or p_i*q_j.
 *
 *     `Term(255, 255)` represents 0.
 *     `Term(0, 0)` represents 1.
 *     `Term(i, 0)` where 1 <= i <= 254 represents p_i.
 *     `Term(0, j)` where 1 <= j <= 254 represents q_j.
 *     `Term(i, j)` where 1 <= i, j <= 254 represents p_i*q_j.
 *
 * All remaining forms (`Term(i, 255)` and `Term(255, j)`) are invalid.
 ******************************************************************************/
struct Term {

    var_index_t p_index;
    var_index_t q_index;

    constexpr Term() noexcept
        : p_index(0)
        , q_index(0) {}

    constexpr Term(int p, int q) noexcept
        : p_index(static_cast<var_index_t>(p))
        , q_index(static_cast<var_index_t>(q)) {
        assert((0 <= p) && (p <= 255));
        assert((0 <= q) && (q <= 255));
        assert(!((p == 255) ^ (q == 255)));
    }

    constexpr bool operator==(const Term &) const noexcept = default;
    constexpr bool operator!=(const Term &) const noexcept = default;

}; // struct Term


constexpr Term TERM_ZERO = {0xFF, 0xFF};
constexpr Term TERM_ONE = {0x00, 0x00};


/******************************************************************************
 * The right-hand side of each equation is either 0 or 1. Initially, all
 * right-hand sides are assigned the indefinite value `RHS::ZERO_OR_ONE`.
 * ZeroOneSolver then performs a backtracking search in which some right-hand
 * sides are assigned the definite values `RHS::ZERO` or `RHS::ONE`.
 ******************************************************************************/
enum class RHS : std::uint8_t {
    ZERO_OR_ONE = 0x00, // Default value; must be zero.
    ZERO = 0x01,
    ONE = 0x02,
}; // enum class RHS


/******************************************************************************
 * Each variable `p_i` or `q_j` is initially unknown and is assigned the value
 * `VAR::UNKNOWN`. As the backtracking search progresses, some variables are
 * deduced to have the values `VAR::ZERO` or `VAR::ONE`. In some cases, a
 * variable can be assigned the value `VAR::ZERO_OR_ONE`, indicating that the
 * variable has been deduced to be either 0 or 1 and cannot lie in between.
 ******************************************************************************/
enum class VAR : std::uint8_t {
    UNKNOWN = 0x00, // Default value; must be zero.
    ZERO_OR_ONE = 0x01,
    ZERO = 0x02,
    ONE = 0x03,
}; // enum class VAR


enum class SimplifyStatus : std::uint8_t {
    NO_SIMPLIFICATION,
    FOUND_SIMPLIFICATION,
    FOUND_CONTRADICTION,
}; // enum class SimplifyStatus


template <var_index_t M, var_index_t N>
consteval std::array<std::array<Term, M + 1>, M + N - 3>
initial_lhs() noexcept {

    // Equations with fewer than `M + 1` terms are padded with zeroes.
    std::array<std::array<Term, M + 1>, M + N - 3> lhs;
    for (std::size_t e = 0; e < M + N - 3; ++e) {
        for (std::size_t t = 0; t < M + 1; ++t) { lhs[e][t] = TERM_ZERO; }
    }

    // Coefficient of x^d for d < M:
    for (var_index_t d = 1; d < M; ++d) {
        // p_1*q_(d-1) + p_2*q_(d-2) + ... + p_(d-1)*q_1
        for (var_index_t i = 1; i <= d - 1; ++i) {
            lhs[d - 1][i - 1] = {i, d - i};
        }
        lhs[d - 1][d - 1] = {d, 0}; // p_d
        lhs[d - 1][d] = {0, d};     // q_d
    }

    // Coefficient of x^M:
    // p_1*q_(M-1) + p_2*q_(M-2) + ... + p_(M-1)*q_1
    // for (var_index_t i = 1; i <= M - 1; ++i) {
    //     lhs[M - 1][i - 1] = {i, M - i};
    // }
    // lhs[M - 1][M - 1] = {0, M}; // q_M
    // lhs[M - 1][M] = TERM_ONE;   // 1
    // This equation immediately implies q_M == 0.

    // Coefficient of x^d for M < d < N:
    for (var_index_t d = M + 1; d < N; ++d) {
        // p_1*q_(d-1) + p_2*q_(d-2) + ... + p_(M-1)*q_(d-M+1)
        for (var_index_t i = 1; i <= M - 1; ++i) {
            lhs[d - 2][i - 1] = {i, d - i};
        }
        lhs[d - 2][M - 1] = {0, d - M}; // q_(d-M)
        lhs[d - 2][M] = {0, d};         // q_d
    }

    // Coefficient of x^N:
    // p_1*q_(N-1) + p_2*q_(N-2) + ... + p_(M-1)*q_(N-M+1)
    // for (var_index_t i = 1; i <= M - 1; ++i) {
    //     lhs[N - 1][i - 1] = {i, N - i};
    // }
    // lhs[N - 1][M - 1] = {0, N - M}; // q_(N-M)
    // lhs[N - 1][M] = TERM_ONE;       // 1
    // This equation immediately implies q_(N-M) == 0.

    // Coefficient of x^d for d > N:
    for (var_index_t d = 1; d <= M - 1; ++d) {
        // p_(d+1)*q_(N-1) + p_(d+2)*q_(N-2) + ... + p_(M-1)*q_(N+d-(M-1))
        const std::size_t equation_index =
            static_cast<std::size_t>(N - 3) + static_cast<std::size_t>(d);
        for (var_index_t i = d + 1; i <= M - 1; ++i) {
            lhs[equation_index][i - d - 1] = {i, N + d - i};
        }
        lhs[equation_index][M - d - 1] = {d, 0};       // p_(d)
        lhs[equation_index][M - d] = {0, d + (N - M)}; // q_(d+(N-M))
    }

    return lhs;
}


enum class Factor { P, Q };


struct TermPosition {
    std::uint16_t equation_index;
    std::uint16_t term_index;
};


template <var_index_t M, var_index_t N, Factor FACTOR>
struct OccurrenceTable {

    static constexpr std::size_t VAR_COUNT =
        (FACTOR == Factor::P) ? M - 1 : N - 1;
    static constexpr std::size_t CAPACITY =
        (FACTOR == Factor::P) ? N - 1 : M + 1;

    std::array<std::array<TermPosition, CAPACITY>, VAR_COUNT> positions;
    std::array<std::uint8_t, VAR_COUNT> counts;

    consteval OccurrenceTable() noexcept
        : positions{}
        , counts{} {
        constexpr auto lhs = initial_lhs<M, N>();
        for (std::size_t e = 0; e < M + N - 3; ++e) {
            for (std::size_t t = 0; t < M + 1; ++t) {
                const Term term = lhs[e][t];
                const var_index_t i =
                    (FACTOR == Factor::P) ? term.p_index : term.q_index;
                if ((i != 0x00) && (i != 0xFF)) {
                    assert(counts[i - 1] < CAPACITY);
                    positions[i - 1][counts[i - 1]++] = {
                        static_cast<std::uint16_t>(e),
                        static_cast<std::uint16_t>(t),
                    };
                }
            }
        }
    }

}; // struct OccurrenceTable


/******************************************************************************
 * A `System<M, N>` represents a system of M + N - 3 bilinear equations over
 * the variables {p_1, p_2, ..., p_(M-1)} and {q_1, q_2, ..., q_(N-1)} together
 * with a partial assignment of values to the variables and right-hand sides of
 * each equation. Each equation in the system can contain up to M + 1 terms.
 ******************************************************************************/
template <var_index_t M, var_index_t N>
struct System {


    static_assert((1 < M) && (M < N));


    static constexpr OccurrenceTable<M, N, Factor::P> P_TABLE{};
    static constexpr OccurrenceTable<M, N, Factor::Q> Q_TABLE{};
    std::array<std::array<Term, M + 1>, M + N - 3> lhs;
    TwoBitPackedArray<RHS, M + N - 3> rhs;
    TwoBitPackedArray<VAR, M - 1> p;
    TwoBitPackedArray<VAR, N - 1> q;
    PackedBooleanArray<M - 1> p_positive;
    PackedBooleanArray<N - 1> q_positive;


    /**************************************************************************
     * The `System<M, N>` default constructor sets up the initial system of
     * equations used to study the 0-1 Polynomial Conjecture. Each equation
     * constrains one coefficient of the product P(x) * Q(x), where P and Q
     * have degree M and N, respectively.
     **************************************************************************/
    consteval System() noexcept {
        lhs = initial_lhs<M, N>();
        // The arrays `rhs`, `p`, `q`, `p_positive`, and
        // `q_positive` are automatically zero-initialized.
    }


    constexpr VAR get_p(var_index_t i) const noexcept {
        assert((1 <= i) && (i <= M - 1));
        return p.get(i - 1);
    }


    constexpr VAR get_q(var_index_t j) const noexcept {
        assert((1 <= j) && (j <= N - 1));
        return q.get(j - 1);
    }


    constexpr bool set_p_zero(var_index_t i) noexcept {
        assert((1 <= i) && (i <= M - 1));
        assert(p.get(i - 1) != VAR::ONE);
        if (p_positive.get(i - 1)) { return false; }
        p.set(i - 1, VAR::ZERO);
        for (std::size_t k = 0; k < P_TABLE.counts[i - 1]; ++k) {
            const auto [e, t] = P_TABLE.positions[i - 1][k];
            lhs[e][t] = TERM_ZERO;
        }
        return true;
    }


    constexpr bool set_q_zero(var_index_t j) noexcept {
        assert((1 <= j) && (j <= N - 1));
        assert(q.get(j - 1) != VAR::ONE);
        if (q_positive.get(j - 1)) { return false; }
        q.set(j - 1, VAR::ZERO);
        for (std::size_t k = 0; k < Q_TABLE.counts[j - 1]; ++k) {
            const auto [e, t] = Q_TABLE.positions[j - 1][k];
            lhs[e][t] = TERM_ZERO;
        }
        return true;
    }


    constexpr void set_p_one(var_index_t i) noexcept {
        assert((1 <= i) && (i <= M - 1));
        assert(p.get(i - 1) != VAR::ZERO);
        p.set(i - 1, VAR::ONE);
        for (std::size_t k = 0; k < P_TABLE.counts[i - 1]; ++k) {
            const auto [e, t] = P_TABLE.positions[i - 1][k];
            if (lhs[e][t].p_index == i) { lhs[e][t].p_index = 0; }
        }
    }


    constexpr void set_q_one(var_index_t j) noexcept {
        assert((1 <= j) && (j <= N - 1));
        assert(q.get(j - 1) != VAR::ZERO);
        q.set(j - 1, VAR::ONE);
        for (std::size_t k = 0; k < Q_TABLE.counts[j - 1]; ++k) {
            const auto [e, t] = Q_TABLE.positions[j - 1][k];
            if (lhs[e][t].q_index == j) { lhs[e][t].q_index = 0; }
        }
    }


    constexpr bool set_p_zero_or_one(var_index_t i) noexcept {
        assert((1 <= i) && (i <= M - 1));
        if (p.get(i - 1) == VAR::UNKNOWN) {
            p.set(i - 1, VAR::ZERO_OR_ONE);
            return true;
        }
        return false;
    }


    constexpr bool set_q_zero_or_one(var_index_t j) noexcept {
        assert((1 <= j) && (j <= N - 1));
        if (q.get(j - 1) == VAR::UNKNOWN) {
            q.set(j - 1, VAR::ZERO_OR_ONE);
            return true;
        }
        return false;
    }


    constexpr void set_p_positive(var_index_t i) noexcept {
        assert((1 <= i) && (i <= M - 1));
        assert(p.get(i - 1) != VAR::ZERO);
        p_positive.set(i - 1);
    }


    constexpr void set_q_positive(var_index_t j) noexcept {
        assert((1 <= j) && (j <= N - 1));
        assert(q.get(j - 1) != VAR::ZERO);
        q_positive.set(j - 1);
    }


    constexpr bool is_unknown(const Term &term) const noexcept {
        if (term == TERM_ZERO) { return false; }
        if (term.p_index) {
            if (p.get(term.p_index - 1) == VAR::UNKNOWN) { return true; }
        }
        if (term.q_index) {
            if (q.get(term.q_index - 1) == VAR::UNKNOWN) { return true; }
        }
        return false;
    }


    constexpr bool has_unknown_variable() const noexcept {
        for (std::size_t i = 0; i < M - 1; ++i) {
            if (p.get(i) == VAR::UNKNOWN) { return true; }
        }
        for (std::size_t i = 0; i < N - 1; ++i) {
            if (q.get(i) == VAR::UNKNOWN) { return true; }
        }
        return false;
    }


    constexpr Term find_unique_nonzero_term(std::size_t e) const noexcept {
        Term result = TERM_ZERO;
        for (std::size_t t = 0; t < M + 1; ++t) {
            const Term term = lhs[e][t];
            if (term != TERM_ZERO) {
                if (result != TERM_ZERO) {
                    result = TERM_ZERO;
                    break;
                }
                result = term;
            }
        }
        return result;
    }


    constexpr Term find_unique_unknown_term(std::size_t e) const noexcept {
        Term result = TERM_ZERO;
        for (std::size_t t = 0; t < M + 1; ++t) {
            const Term term = lhs[e][t];
            if (is_unknown(term)) {
                if (result != TERM_ZERO) {
                    result = TERM_ZERO;
                    break;
                }
                result = term;
            }
        }
        return result;
    }


    constexpr std::pair<Term, Term>
    find_two_nonzero_terms(std::size_t e) const noexcept {
        std::pair<Term, Term> result = {TERM_ZERO, TERM_ZERO};
        Term &first = result.first;
        Term &second = result.second;
        for (std::size_t t = 0; t < M + 1; ++t) {
            const Term term = lhs[e][t];
            if (term != TERM_ZERO) {
                if (first == TERM_ZERO) {
                    first = term;
                } else if (second == TERM_ZERO) {
                    second = term;
                } else {
                    first = TERM_ZERO;
                    second = TERM_ZERO;
                    break;
                }
            }
        }
        return result;
    }


    static constexpr bool contains_term(const Term *array, std::size_t size,
                                        const Term &term) noexcept {
        for (std::size_t i = 0; i < size; ++i) {
            if (array[i] == term) { return true; }
        }
        return false;
    }


    constexpr bool simplify_positive_binary() noexcept {
        // Phase 0: If a variable is both positive and binary, then it is 1.
        bool result = false;
        for (var_index_t i = 1; i <= M - 1; ++i) {
            if (p_positive.get(i - 1) && (p.get(i - 1) == VAR::ZERO_OR_ONE)) {
                set_p_one(i);
                result = true;
            }
        }
        for (var_index_t j = 1; j <= N - 1; ++j) {
            if (q_positive.get(j - 1) && (q.get(j - 1) == VAR::ZERO_OR_ONE)) {
                set_q_one(j);
                result = true;
            }
        }
        return result;
    }


    constexpr SimplifyStatus simplify_phase_1() noexcept {
        // Phase 1: Deduce right-hand sides and eliminate occurrences of 1.
        SimplifyStatus result = SimplifyStatus::NO_SIMPLIFICATION;
        constexpr std::size_t INVALID_INDEX = ~static_cast<std::size_t>(0);
        for (std::size_t e = 0; e < M + N - 3; ++e) {
            // Scan the left-hand side of each equation for nonzero terms
            // and 1, keeping track of the index at which 1 occurs.
            bool found_nonzero = false;
            std::size_t one_index = INVALID_INDEX;
            for (std::size_t t = 0; t < M + 1; ++t) {
                const Term term = lhs[e][t];
                if (term != TERM_ZERO) { found_nonzero = true; }
                if (term == TERM_ONE) {
                    // An equation with multiple copies of 1
                    // on its left-hand side is unsatisfiable.
                    if (one_index != INVALID_INDEX) {
                        return SimplifyStatus::FOUND_CONTRADICTION;
                    }
                    one_index = t;
                }
            }
            if (!found_nonzero) {
                // An equation of the form 0 == 1 is unsatisfiable.
                if (rhs.get(e) == RHS::ONE) {
                    return SimplifyStatus::FOUND_CONTRADICTION;
                }
                // If we find no nonzero terms on the left-hand side,
                // then we set the right-hand side to zero.
                if (rhs.get(e) == RHS::ZERO_OR_ONE) {
                    rhs.set(e, RHS::ZERO);
                    result = SimplifyStatus::FOUND_SIMPLIFICATION;
                }
            } else if (one_index != INVALID_INDEX) {
                assert(lhs[e][one_index] == TERM_ONE);
                // ... + 1 + ... == 0 is unsatisfiable.
                if (rhs.get(e) == RHS::ZERO) {
                    return SimplifyStatus::FOUND_CONTRADICTION;
                }
                // If we find 1 on the left-hand side, then we
                // eliminate it and set the right-hand side to zero.
                lhs[e][one_index] = TERM_ZERO;
                rhs.set(e, RHS::ZERO);
                result = SimplifyStatus::FOUND_SIMPLIFICATION;
            }
        }
        return result;
    }


    constexpr SimplifyStatus simplify_phase_2() noexcept {
        // Phase 2: Use right-hand sides to deduce values of variables.
        SimplifyStatus result = SimplifyStatus::NO_SIMPLIFICATION;
        for (std::size_t e = 0; e < M + N - 3; ++e) {
            const RHS rhs_value = rhs.get(e);
            if (rhs_value == RHS::ZERO) {
                // If we see ... + p_i + ... == 0, then we set p_i to 0.
                // The same holds for ... + q_j + ... == 0. Moreover, if
                // we see ... + p_i*q_j + ... == 0 where p_i is positive,
                // then we set q_j to 0 and vice versa.
                for (std::size_t t = 0; t < M + 1; ++t) {
                    const Term term = lhs[e][t];
                    assert(term != TERM_ONE);
                    if (term == TERM_ZERO) { continue; }
                    if (term.q_index == 0) { // p_i
                        if (!set_p_zero(term.p_index)) {
                            return SimplifyStatus::FOUND_CONTRADICTION;
                        }
                        result = SimplifyStatus::FOUND_SIMPLIFICATION;
                    } else if (term.p_index == 0) { // q_j
                        if (!set_q_zero(term.q_index)) {
                            return SimplifyStatus::FOUND_CONTRADICTION;
                        }
                        result = SimplifyStatus::FOUND_SIMPLIFICATION;
                    } else if (p_positive.get(term.p_index - 1)) { // p_i > 0
                        if (!set_q_zero(term.q_index)) {
                            return SimplifyStatus::FOUND_CONTRADICTION;
                        }
                        result = SimplifyStatus::FOUND_SIMPLIFICATION;
                    } else if (q_positive.get(term.q_index - 1)) { // q_j > 0
                        if (!set_p_zero(term.p_index)) {
                            return SimplifyStatus::FOUND_CONTRADICTION;
                        }
                        result = SimplifyStatus::FOUND_SIMPLIFICATION;
                    }
                }
            } else if (rhs_value == RHS::ONE) {
                // If we see p_i == 1, then we set p_i to 1. The same holds
                // for q_j == 1 and p_i*q_j == 1 (since p_i and q_j are
                // constrained to the interval [0, 1]).
                const Term term = find_unique_nonzero_term(e);
                assert(term != TERM_ONE);
                if (term != TERM_ZERO) {
                    if (term.p_index) { set_p_one(term.p_index); }
                    if (term.q_index) { set_q_one(term.q_index); }
                    return SimplifyStatus::FOUND_SIMPLIFICATION;
                }
            }
        }
        return result;
    }


    constexpr SimplifyStatus simplify_phase_3() noexcept {
        // Phase 3: Eliminate unknown variables by all-but-one principle.
        SimplifyStatus result = SimplifyStatus::NO_SIMPLIFICATION;
        for (std::size_t e = 0; e < M + N - 3; ++e) {
            // If every term in an equation except one is already known
            // to be 0 or 1, then the remaining term must also be 0 or 1.
            const Term term = find_unique_unknown_term(e);
            assert(term != TERM_ONE);
            if (term != TERM_ZERO) {
                assert(is_unknown(term));
                if (term.q_index == 0) { // p_i
                    if (set_p_zero_or_one(term.p_index)) {
                        result = SimplifyStatus::FOUND_SIMPLIFICATION;
                    }
                } else if (term.p_index == 0) { // q_j
                    if (set_q_zero_or_one(term.q_index)) {
                        result = SimplifyStatus::FOUND_SIMPLIFICATION;
                    }
                }
            }
        }
        return result;
    }


    constexpr SimplifyStatus simplify_phase_4() noexcept {
        // Phase 4: Eliminate unknown variables in subsystems of the form:
        //     v*w == 0 or 1
        //     v + w == 0 or 1
        // The only real solutions of this subsystem are (v, w) == (0, 0),
        // (0, 1), or (1, 0), so we deduce that v and w are both 0 or 1.

        // Phase 4.1: Find equations of the form v*w == 0 or 1.
        Term unique[M + N - 3];
        std::size_t t = 0;
        for (std::size_t e = 0; e < M + N - 3; ++e) {
            // If v*w is the only unknown term in the equation
            // ... + v*w + ... == 0 or 1, then v*w == 0 or 1.
            const Term term = find_unique_unknown_term(e);
            assert(term != TERM_ONE);
            if (term != TERM_ZERO) {
                assert(is_unknown(term));
                // In Phase 4, all unique unknown terms have the form
                // p_i*q_j, since any unique unknown term of the form
                // p_i or q_j would have been eliminated in Phase 3.
                assert(term.p_index);
                assert(term.q_index);
                unique[t++] = term;
            }
        }

        // Phase 4.2: Find equations of the form v + w == 0 or 1.
        SimplifyStatus result = SimplifyStatus::NO_SIMPLIFICATION;
        for (std::size_t e = 0; e < M + N - 3; ++e) {
            const auto [x, y] = find_two_nonzero_terms(e);
            if (y != TERM_ZERO) {
                assert(x != TERM_ZERO);
                if ((x.q_index == 0) && (y.p_index == 0)) { // p_x*q_y
                    assert(x.p_index);
                    assert(y.q_index);
                    if (contains_term(unique, t, {x.p_index, y.q_index})) {
                        if (set_p_zero_or_one(x.p_index)) {
                            result = SimplifyStatus::FOUND_SIMPLIFICATION;
                        }
                        if (set_q_zero_or_one(y.q_index)) {
                            result = SimplifyStatus::FOUND_SIMPLIFICATION;
                        }
                    }
                } else if ((x.p_index == 0) && (y.q_index == 0)) { // p_y*q_x
                    assert(x.q_index);
                    assert(y.p_index);
                    if (contains_term(unique, t, {y.p_index, x.q_index})) {
                        if (set_p_zero_or_one(y.p_index)) {
                            result = SimplifyStatus::FOUND_SIMPLIFICATION;
                        }
                        if (set_q_zero_or_one(x.q_index)) {
                            result = SimplifyStatus::FOUND_SIMPLIFICATION;
                        }
                    }
                }
            }
        }
        return result;
    }


    constexpr bool simplify() noexcept {

        simplify_positive_binary();

        while (true) {

            if (simplify_phase_1() == SimplifyStatus::FOUND_CONTRADICTION) {
                return false;
            }
            // After Phase 1, we may assume that 1 does not
            // appear on the left-hand side of any equation.

            const SimplifyStatus phase_2 = simplify_phase_2();
            if (phase_2 == SimplifyStatus::FOUND_CONTRADICTION) {
                return false;
            }
            // Setting a variable in Phase 2 can enable further
            // simplification. If this occurs, we repeat Phase 1.
            if (phase_2 == SimplifyStatus::FOUND_SIMPLIFICATION) { continue; }
            // If no variables were set in Phase 2, then we proceed to Phase 3.

            // At this point, we cannot determine the values of any more
            // variables or right-hand sides. However, it is still possible
            // to conclude that a variable must be either 0 or 1 without
            // determining which of the two values it takes. We call this
            // process "eliminating unknowns."
            while (true) {

                const SimplifyStatus phase_3 = simplify_phase_3();
                // Eliminating unknowns can lead us to deduce that a variable
                // is both positive and binary. If so, we return to phase 1.
                if (simplify_positive_binary()) { break; }
                if (phase_3 == SimplifyStatus::FOUND_SIMPLIFICATION) {
                    // If no unknowns remain, then simplification is complete.
                    if (!has_unknown_variable()) { return true; }
                    // Eliminating unknowns can enable further elimination.
                    // We repeat Phase 3 to check.
                    continue;
                }

                const SimplifyStatus phase_4 = simplify_phase_4();
                // Eliminating unknowns can lead us to deduce that a variable
                // is both positive and binary. If so, we return to phase 1.
                if (simplify_positive_binary()) { break; }
                // Eliminating unknowns can enable further elimination.
                // We repeat Phase 3 to check.
                if ((phase_4 == SimplifyStatus::FOUND_SIMPLIFICATION) &&
                    has_unknown_variable()) {
                    continue;
                }

                // At this point, no more unknowns can be eliminated.
                return true;
            }
        }
    }


}; // struct System<M, N>


} // namespace ZeroOneSolver

#endif // ZERO_ONE_SOLVER_HPP_INCLUDED
