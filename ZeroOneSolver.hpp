/******************************************************************************
 * Copyright (c) 2023-2024 David K. Zhang
 * This file is part of ZeroOnePolynomials by David K. Zhang.
 * ZeroOnePolynomials is distributed under the terms of the MIT license.
 * See the LICENSE file for full license details.
 ******************************************************************************/

#ifndef ZERO_ONE_SOLVER_HPP_INCLUDED
#define ZERO_ONE_SOLVER_HPP_INCLUDED

#include <cassert> // for assert
#include <cstddef> // for std::byte, std::size_t
#include <cstdint> // for std::uint8_t
#include <utility> // for std::pair

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
 * A `TwoBitPackedArray<T, N>` is an array of `N` elements of type `T` in which
 * each element is represented by two bits and four elements are packed into a
 * single `std::byte`. The type `T` is expected to be `static_cast`-able to and
 * from `std::byte`, e.g., an `enum class` with values 0, 1, 2, and 3.
 ******************************************************************************/
template <typename T, std::size_t N>
class TwoBitPackedArray {

    static constexpr std::byte MASK = static_cast<std::byte>(0x03);

    std::byte data[(N + 3) >> 2] = {}; // Zero-initialize data.

public:

    constexpr T get(std::size_t index) const noexcept {
        assert(index < N);
        const std::size_t byte_index = index >> 2;
        const std::byte byte = data[byte_index];
        const int shift = static_cast<int>(index & 0x03) << 1;
        return static_cast<T>((byte & (MASK << shift)) >> shift);
    }

    constexpr void set(std::size_t index, T item) noexcept {
        assert(index < N);
        const std::size_t byte_index = index >> 2;
        const std::byte byte = data[byte_index];
        const int shift = static_cast<int>(index & 0x03) << 1;
        const std::byte new_byte = static_cast<std::byte>(item) << shift;
        data[byte_index] = (byte & ~(MASK << shift)) | new_byte;
    }

}; // class TwoBitPackedArray<N>


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


/******************************************************************************
 * A `System<M, N>` represents a system of M + N - 1 bilinear equations over
 * the variables {p_1, p_2, ..., p_(M-1)} and {q_1, q_2, ..., q_(N-1)} together
 * with a partial assignment of values to the variables and right-hand sides of
 * each equation. Each equation in the system can contain up to M + 1 terms.
 ******************************************************************************/
template <var_index_t M, var_index_t N>
struct System {


    static_assert((1 < M) && (M < N));


    // Equations with fewer than `M + 1` terms are padded with zeroes.
    Term lhs[M + N - 1][M + 1];
    TwoBitPackedArray<RHS, M + N - 1> rhs;
    TwoBitPackedArray<VAR, M - 1> p;
    TwoBitPackedArray<VAR, N - 1> q;


    /**************************************************************************
     * The `System<M, N>` default constructor sets up the initial system of
     * equations used to study the 0-1 Polynomial Conjecture. Each equation
     * constrains one coefficient of the product P(x) * Q(x), where P and Q
     * have degree M and N, respectively.
     **************************************************************************/
    constexpr System() noexcept {

        // Initialize left-hand sides.
        for (std::size_t e = 0; e < M + N - 1; ++e) {
            for (std::size_t t = 0; t < M + 1; ++t) { lhs[e][t] = TERM_ZERO; }
        }
        // The arrays `rhs`, `p`, and `q` are automatically zero-initialized.

        // Coefficient of x^d for d < M:
        for (int d = 1; d < M; ++d) {
            // p_1*q_(d-1) + p_2*q_(d-2) + ... + p_(d-1)*q_1
            for (int i = 1; i <= d - 1; ++i) { lhs[d - 1][i - 1] = {i, d - i}; }
            lhs[d - 1][d - 1] = {d, 0}; // p_d
            lhs[d - 1][d] = {0, d};     // q_d
        }

        // Coefficient of x^M:
        // p_1*q_(M-1) + p_2*q_(M-2) + ... + p_(M-1)*q_1
        for (int i = 1; i <= M - 1; ++i) { lhs[M - 1][i - 1] = {i, M - i}; }
        lhs[M - 1][M - 1] = {0, M}; // q_M
        lhs[M - 1][M] = TERM_ONE;   // 1
        // This equation immediately implies q_M == 0.

        // Coefficient of x^d for M < d < N:
        for (int d = M + 1; d < N; ++d) {
            // p_1*q_(d-1) + p_2*q_(d-2) + ... + p_(M-1)*q_(d-M+1)
            for (int i = 1; i <= M - 1; ++i) { lhs[d - 1][i - 1] = {i, d - i}; }
            lhs[d - 1][M - 1] = {0, d - M}; // q_(d-M)
            lhs[d - 1][M] = {0, d};         // q_d
        }

        // Coefficient of x^N:
        // p_1*q_(N-1) + p_2*q_(N-2) + ... + p_(M-1)*q_(N-M+1)
        for (int i = 1; i <= M - 1; ++i) { lhs[N - 1][i - 1] = {i, N - i}; }
        lhs[N - 1][M - 1] = {0, N - M}; // q_(N-M)
        lhs[N - 1][M] = TERM_ONE;       // 1
        // This equation immediately implies q_(N-M) == 0.

        // Coefficient of x^d for d > N:
        for (int d = N + 1; d <= M + N - 1; ++d) {
            const int origin = d - (N - 1);
            // p_(d-N+1)*q_(N-1) + p_(d-N+2)*q_(N-2) + ... + p_(M-1)*q_(d-M+1)
            for (int i = origin; i <= M - 1; ++i) {
                lhs[d - 1][i - origin] = {i, d - i};
            }
            lhs[d - 1][M - origin] = {d - N, 0}; // p_(d-N)
            lhs[d - 1][M + N - d] = {0, d - M};  // q_(d-M)
        }
    }


    constexpr VAR get_p(var_index_t i) const noexcept {
        assert((1 <= i) && (i <= M - 1));
        return p.get(i - 1);
    }


    constexpr VAR get_q(var_index_t j) const noexcept {
        assert((1 <= j) && (j <= N - 1));
        return q.get(j - 1);
    }


    constexpr void set_p_zero(var_index_t i) noexcept {
        assert((1 <= i) && (i <= M - 1));
        assert(p.get(i - 1) != VAR::ONE);
        p.set(i - 1, VAR::ZERO);
        // Scan for terms containing p_i and set them to zero.
        for (std::size_t e = 0; e < M + N - 1; ++e) {
            for (std::size_t t = 0; t < M + 1; ++t) {
                if (lhs[e][t].p_index == i) { lhs[e][t] = TERM_ZERO; }
            }
        }
    }


    constexpr void set_q_zero(var_index_t j) noexcept {
        assert((1 <= j) && (j <= N - 1));
        assert(q.get(j - 1) != VAR::ONE);
        q.set(j - 1, VAR::ZERO);
        // Scan for terms containing q_j and set them to zero.
        for (std::size_t e = 0; e < M + N - 1; ++e) {
            for (std::size_t t = 0; t < M + 1; ++t) {
                if (lhs[e][t].q_index == j) { lhs[e][t] = TERM_ZERO; }
            }
        }
    }


    constexpr void set_p_one(var_index_t i) noexcept {
        assert((1 <= i) && (i <= M - 1));
        assert(p.get(i - 1) != VAR::ZERO);
        p.set(i - 1, VAR::ONE);
        // Scan for terms containing p_i and cancel p_i from them.
        for (std::size_t e = 0; e < M + N - 1; ++e) {
            for (std::size_t t = 0; t < M + 1; ++t) {
                if (lhs[e][t].p_index == i) { lhs[e][t].p_index = 0; }
            }
        }
    }


    constexpr void set_q_one(var_index_t j) noexcept {
        assert((1 <= j) && (j <= N - 1));
        assert(q.get(j - 1) != VAR::ZERO);
        q.set(j - 1, VAR::ONE);
        // Scan for terms containing q_j and cancel q_j from them.
        for (std::size_t e = 0; e < M + N - 1; ++e) {
            for (std::size_t t = 0; t < M + 1; ++t) {
                if (lhs[e][t].q_index == j) { lhs[e][t].q_index = 0; }
            }
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
                if (result != TERM_ZERO) { return TERM_ZERO; }
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
                if (result != TERM_ZERO) { return TERM_ZERO; }
                result = term;
            }
        }
        return result;
    }


    constexpr std::pair<Term, Term> find_two_nonzero_terms(std::size_t e
    ) const noexcept {
        Term first = TERM_ZERO;
        Term second = TERM_ZERO;
        for (std::size_t t = 0; t < M + 1; ++t) {
            const Term term = lhs[e][t];
            if (term != TERM_ZERO) {
                if (first == TERM_ZERO) {
                    first = term;
                } else if (second == TERM_ZERO) {
                    second = term;
                } else {
                    return {TERM_ZERO, TERM_ZERO};
                }
            }
        }
        return {first, second};
    }


    static constexpr bool contains_term(
        const Term *array, std::size_t size, const Term &term
    ) noexcept {
        for (std::size_t i = 0; i < size; ++i) {
            if (array[i] == term) { return true; }
        }
        return false;
    }


    constexpr bool simplify() noexcept {

        while (true) {

            // Phase 1: Deduce right-hand sides and eliminate occurrences of 1.
            constexpr std::size_t INVALID_INDEX = ~static_cast<std::size_t>(0);
            for (std::size_t e = 0; e < M + N - 1; ++e) {
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
                        if (one_index != INVALID_INDEX) { return false; }
                        one_index = t;
                    }
                }
                if (!found_nonzero) {
                    // An equation of the form 0 == 1 is unsatisfiable.
                    if (rhs.get(e) == RHS::ONE) { return false; }
                    // If we find no nonzero terms on the left-hand side,
                    // then we set the right-hand side to zero.
                    rhs.set(e, RHS::ZERO);
                } else if (one_index != INVALID_INDEX) {
                    assert(lhs[e][one_index] == TERM_ONE);
                    // ... + 1 + ... == 0 is unsatisfiable.
                    if (rhs.get(e) == RHS::ZERO) { return false; }
                    // If we find 1 on the left-hand side, then we
                    // eliminate it and set the right-hand side to zero.
                    lhs[e][one_index] = TERM_ZERO;
                    rhs.set(e, RHS::ZERO);
                }
            }
            // After Phase 1, we may assume that 1 does not
            // appear on the left-hand side of any equation.

            // Phase 2: Use right-hand sides to deduce values of variables.
            bool set_variable = false;
            for (std::size_t e = 0; e < M + N - 1; ++e) {
                const RHS rhs_value = rhs.get(e);
                if (rhs_value == RHS::ZERO) {
                    // If we see ... + p_i + ... == 0, then we set p_i to 0.
                    // The same holds for ... + q_j + ... == 0.
                    for (std::size_t t = 0; t < M + 1; ++t) {
                        const Term term = lhs[e][t];
                        assert(term != TERM_ONE);
                        if (term.q_index == 0) { // p_i
                            set_p_zero(term.p_index);
                            set_variable = true;
                        } else if (term.p_index == 0) { // q_j
                            set_q_zero(term.q_index);
                            set_variable = true;
                        }
                        if (set_variable) { break; }
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
                        set_variable = true;
                    }
                }
                if (set_variable) { break; }
            }

            // Setting a variable in Phase 2 can enable further
            // simplification. If this occurs, we repeat Phase 1.
            if (set_variable) { continue; }

            // If no variables were set in Phase 2, then we proceed to Phase 3.
            break;
        }

        // At this point, we cannot determine the values of any more variables
        // or right-hand sides. However, it is still possible to conclude that
        // a variable must be either 0 or 1 without determining which value
        // it actually takes. We call this process "eliminating unknowns."

        while (true) {

            // Phase 3: Eliminate unknown variables by all-but-one principle.
            bool eliminated = false;
            for (std::size_t e = 0; e < M + N - 1; ++e) {
                // If every term in an equation except one is already known
                // to be 0 or 1, then the remaining term must also be 0 or 1.
                const Term term = find_unique_unknown_term(e);
                assert(term != TERM_ONE);
                if (term != TERM_ZERO) {
                    assert(is_unknown(term));
                    if (term.q_index == 0) { // p_i
                        eliminated |= set_p_zero_or_one(term.p_index);
                    } else if (term.p_index == 0) { // q_j
                        eliminated |= set_q_zero_or_one(term.q_index);
                    }
                }
            }
            if (eliminated) {
                if (has_unknown_variable()) {
                    // Eliminating one unknown can enable further elimination.
                    // If this occurred, we repeat Phase 3.
                    continue;
                } else {
                    // If no unknowns remain, then simplification is complete.
                    return true;
                }
            }

            // Phase 4: Eliminate unknown variables in subsystems of the form:
            //     v*w == 0 or 1
            //     v + w == 0 or 1
            // The only real solutions of this subsystem are (v, w) == (0, 0),
            // (1, 0), or (1, 1), so we deduce that v and w are both 0 or 1.

            // Phase 4.1: Find equations of the form v*w == 0 or 1.
            Term lone_terms[M + N - 1];
            std::size_t t = 0;
            for (std::size_t e = 0; e < M + N - 1; ++e) {
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
                    lone_terms[t++] = term;
                }
            }

            // Phase 4.2: Find equations of the form v + w == 0 or 1.
            for (std::size_t e = 0; e < M + N - 1; ++e) {
                const auto [x, y] = find_two_nonzero_terms(e);
                if (y != TERM_ZERO) {
                    assert(x != TERM_ZERO);
                    if ((x.q_index == 0) && (y.p_index == 0)) { // p_x*q_y
                        assert(x.p_index);
                        assert(y.q_index);
                        if (contains_term(
                                lone_terms, t, {x.p_index, y.q_index}
                            )) {
                            eliminated |= set_p_zero_or_one(x.p_index);
                            eliminated |= set_q_zero_or_one(y.q_index);
                        }
                    } else if ((x.p_index == 0) &&
                               (y.q_index == 0)) { // p_y*q_x
                        assert(x.q_index);
                        assert(y.p_index);
                        if (contains_term(
                                lone_terms, t, {y.p_index, x.q_index}
                            )) {
                            eliminated |= set_p_zero_or_one(y.p_index);
                            eliminated |= set_q_zero_or_one(x.q_index);
                        }
                    }
                }
            }
            if (eliminated && has_unknown_variable()) {
                // Eliminating one unknown can enable further elimination.
                // If this occurred, we repeat Phase 3.
                continue;
            }

            // At this point, no more unknowns can be eliminated.
            return true;
        }
    }


}; // struct System<M, N>


} // namespace ZeroOneSolver

#endif // ZERO_ONE_SOLVER_HPP_INCLUDED
