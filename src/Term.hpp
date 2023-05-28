#ifndef ZERO_ONE_SOLVER_TERM_HPP_INCLUDED
#define ZERO_ONE_SOLVER_TERM_HPP_INCLUDED

#include <compare> // for operator<=>
#include <cstdint> // for std::int16_t

namespace ZeroOneSolver {


using variable_index_t = std::int16_t;


/**
 * A Term represents a monomial of the form 1, p_i, q_j, or p_i * q_j.
 *
 * The presence of each variable in a given Term is indicated by a nonzero
 * value of the corresponding index. Thus:
 *     Term(0, 0) represents 1.
 *     Term(i, 0) for i != 0 represents p_i.
 *     Term(0, j) for j != 0 represents q_j.
 *     Term(i, j) for i != 0 and j != 0 represents p_i * q_j.
 *
 * The variables p_0 and q_0 cannot be represented in a Term, so all subscripts
 * should always be assumed to start from 1.
 */
struct Term {


    variable_index_t p_index;
    variable_index_t q_index;


    explicit constexpr Term(variable_index_t p, variable_index_t q) noexcept
        : p_index(p)
        , q_index(q) {}


    constexpr bool operator<=>(const Term &) const noexcept = default;


    constexpr bool has_p() const noexcept {
        constexpr variable_index_t ZERO = static_cast<variable_index_t>(0);
        return (p_index != ZERO);
    }


    constexpr bool has_q() const noexcept {
        constexpr variable_index_t ZERO = static_cast<variable_index_t>(0);
        return (q_index != ZERO);
    }


    constexpr bool is_constant() const noexcept {
        return !(has_p() || has_q());
    }


    constexpr bool is_linear() const noexcept { return has_p() ^ has_q(); }


    constexpr bool is_quadratic() const noexcept { return has_p() && has_q(); }


    void print_plain_text() const;


    void print_latex() const;


    void print_wolfram() const;


}; // struct Term


} // namespace ZeroOneSolver

#endif // ZERO_ONE_SOLVER_TERM_HPP_INCLUDED
