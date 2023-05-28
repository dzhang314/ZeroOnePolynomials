#ifndef ZERO_ONE_SOLVER_POLYNOMIAL_HPP_INCLUDED
#define ZERO_ONE_SOLVER_POLYNOMIAL_HPP_INCLUDED

#include <vector> // for std::vector

#include "Term.hpp" // for ZeroOneSolver::Term

namespace ZeroOneSolver {


/**
 * A Polynomial is a list of Terms (t_1, ..., t_n) that represents the sum
 * t_1 + ... + t_n. The ordering of Terms within a Polynomial is arbitrary.
 */
struct Polynomial : public std::vector<Term> {


    constexpr bool is_zero() const noexcept { return (size() == 0); }


    constexpr bool is_one() const noexcept {
        return (size() == 1) && front().is_constant();
    }


    constexpr bool is_zero_or_one() const noexcept {
        return is_zero() || is_one();
    }


    void print_plain_text() const;


    void print_latex() const;


    void print_wolfram() const;


}; // struct Polynomial


} // namespace ZeroOneSolver

#endif // ZERO_ONE_SOLVER_POLYNOMIAL_HPP_INCLUDED
