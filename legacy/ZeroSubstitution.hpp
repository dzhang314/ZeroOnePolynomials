#ifndef ZERO_ONE_SOLVER_ZERO_SUBSTITUTION_HPP_INCLUDED
#define ZERO_ONE_SOLVER_ZERO_SUBSTITUTION_HPP_INCLUDED

#include <vector> // for std::vector

#include "Polynomial.hpp" // for ZeroOneSolver::Polynomial
#include "Term.hpp"       // for ZeroOneSolver::{Term, variable_index_t}
#include "Utilities.hpp"  // for ZeroOneSolver::contains

namespace ZeroOneSolver {


/**
 * A ZeroSubstitution is a list of variables {p_i}, {q_j} and terms {t_k}.
 *
 * ZeroSubstitutions are intended to be used in the following situation:
 * Suppose we are solving a system of equations, and we learn that
 *     t_1 + ... + t_n = 0.
 * Since each term t_k assumes values in [0, 1], we may conclude that
 *     t_1 = ... = t_n = 0.
 * This allows us to simplify the remaining equations in the system by
 * removing all other occurrences of the terms (t_1, ..., t_n).
 */
struct ZeroSubstitution {


    std::vector<variable_index_t> zeroed_ps;
    std::vector<variable_index_t> zeroed_qs;
    std::vector<Term> zeroed_terms;


    explicit constexpr ZeroSubstitution() noexcept
        : zeroed_ps()
        , zeroed_qs()
        , zeroed_terms() {}


    constexpr bool is_empty() noexcept {
        return zeroed_ps.empty() && zeroed_qs.empty() && zeroed_terms.empty();
    }


    constexpr void set_p_zero(variable_index_t index) noexcept {
        zeroed_ps.push_back(index);
    }


    constexpr void set_q_zero(variable_index_t index) noexcept {
        zeroed_qs.push_back(index);
    }


    constexpr void set_zero(const Term &term) noexcept {
        if (term.is_quadratic()) {
            zeroed_terms.push_back(term);
        } else if (term.has_p()) {
            set_p_zero(term.p_index);
        } else if (term.has_q()) {
            set_q_zero(term.q_index);
        }
    }


    constexpr void set_zero(const Polynomial &poly) noexcept {
        for (const Term &term : poly) { set_zero(term); }
    }


    constexpr bool is_zeroed(const Term &term) const noexcept {
        return contains(zeroed_ps, term.p_index) ||
               contains(zeroed_qs, term.q_index) ||
               contains(zeroed_terms, term);
    }


    void print_latex();


    void print_variables_latex();


}; // struct ZeroSubstitution


} // namespace ZeroOneSolver

#endif // ZERO_ONE_SOLVER_ZERO_SUBSTITUTION_HPP_INCLUDED
