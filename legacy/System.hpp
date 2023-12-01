#ifndef ZERO_ONE_SOLVER_SYSTEM_HPP_INCLUDED
#define ZERO_ONE_SOLVER_SYSTEM_HPP_INCLUDED

#include <cstddef> // for std::size_t
#include <utility> // for std::move
#include <vector>  // for std::vector

#include "Polynomial.hpp"       // for ZeroOneSolver::Polynomial
#include "Term.hpp"             // for ZeroOneSolver::{Term, variable_index_t}
#include "Utilities.hpp"        // for ZeroOneSolver::{contains, drop, drop_all}
#include "ZeroSubstitution.hpp" // for ZeroOneSolver::ZeroSubstitution

namespace ZeroOneSolver {


struct System {


    std::vector<variable_index_t> active_ps;
    std::vector<variable_index_t> active_qs;
    std::vector<Term> zeros;
    std::vector<Polynomial> ones;
    std::vector<Polynomial> unknown;


    explicit constexpr System() noexcept
        : active_ps()
        , active_qs()
        , zeros()
        , ones()
        , unknown() {}


    explicit constexpr System(
        variable_index_t p_degree, variable_index_t q_degree
    ) noexcept
        : active_ps(static_cast<std::size_t>(p_degree - 1))
        , active_qs(static_cast<std::size_t>(q_degree - 1))
        , zeros()
        , ones()
        , unknown(static_cast<std::size_t>(p_degree + q_degree - 1)) {
        constexpr variable_index_t ZERO = static_cast<variable_index_t>(0);
        constexpr variable_index_t ONE = static_cast<variable_index_t>(1);
        for (variable_index_t p = 1; p < p_degree; ++p) {
            active_ps[static_cast<std::size_t>(p - ONE)] = p;
        }
        for (variable_index_t q = 1; q < q_degree; ++q) {
            active_qs[static_cast<std::size_t>(q - ONE)] = q;
        }
        for (variable_index_t p = ZERO; p <= p_degree; ++p) {
            for (variable_index_t q = ZERO; q <= q_degree; ++q) {
                // Omit constant and leading terms of product polynomial.
                if ((p == ZERO) && (q == ZERO)) { continue; }
                if ((p == p_degree) && (q == q_degree)) { continue; }
                unknown[static_cast<std::size_t>(p + q - ONE)].emplace_back(
                    (p == p_degree) ? ZERO : p, (q == q_degree) ? ZERO : q
                );
            }
        }
    }


    /**
     * @return true if this System contains no active variables or equations.
     */
    constexpr bool is_empty() const noexcept {
        return active_ps.empty() && active_qs.empty() && zeros.empty() &&
               ones.empty() && unknown.empty();
    }


    /**
     * @return true if this System contains a equation of the form 0 = 1 or
     *         1 + 1 + t_1 + ... + t_n = 0 or 1. Equations of these forms are
     *         unsatisfiable and imply that this System is inconsistent.
     */
    constexpr bool has_unsatisfiable_equation() const noexcept {
        for (const Polynomial &poly : ones) {
            if (poly.empty()) { return true; }
            bool found_constant_term = false;
            for (const Term &term : poly) {
                if (term.is_constant()) {
                    if (found_constant_term) {
                        return true;
                    } else {
                        found_constant_term = true;
                    }
                }
            }
        }
        for (const Polynomial &poly : unknown) {
            bool found_constant_term = false;
            for (const Term &term : poly) {
                if (term.is_constant()) {
                    if (found_constant_term) {
                        return true;
                    } else {
                        found_constant_term = true;
                    }
                }
            }
        }
        return false;
    }


    /**
     * @return true if every active variable in this System is solved.
     *
     * A variable v is said to be solved if this system contains an equation
     * of the form v = 0 or 1.
     */
    constexpr bool is_solved() const noexcept {
        std::vector<variable_index_t> solved_ps;
        std::vector<variable_index_t> solved_qs;
        for (const Polynomial &poly : ones) {
            if (poly.size() == 1) {
                const Term term = poly.front();
                if (term.has_p()) { solved_ps.push_back(term.p_index); }
                if (term.has_q()) { solved_qs.push_back(term.q_index); }
            }
        }
        for (const Polynomial &poly : unknown) {
            if (poly.size() == 1) {
                const Term term = poly.front();
                if (term.is_linear()) {
                    if (term.has_p()) { solved_ps.push_back(term.p_index); }
                    if (term.has_q()) { solved_qs.push_back(term.q_index); }
                }
            }
        }
        return drop_all(active_ps, solved_ps).empty() &&
               drop_all(active_qs, solved_qs).empty();
    }


    /**
     * Eliminate the variables and terms specified in a given ZeroSubstitution.
     * Returns a new System with those variables and terms eliminated; this
     * System is left unmodified.
     */
    constexpr System apply(const ZeroSubstitution &sub) const noexcept {
        System result;
        result.active_ps = drop_all(active_ps, sub.zeroed_ps);
        result.active_qs = drop_all(active_qs, sub.zeroed_qs);
        for (const Term &term : zeros) {
            if (!sub.is_zeroed(term)) { result.zeros.push_back(term); }
        }
        for (const Term &term : sub.zeroed_terms) {
            if (!contains(sub.zeroed_ps, term.p_index) &&
                !contains(sub.zeroed_qs, term.q_index)) {
                result.zeros.push_back(term);
            }
        }
        for (const Polynomial &poly : ones) {
            Polynomial sub_poly;
            for (const Term &term : poly) {
                if (!sub.is_zeroed(term)) { sub_poly.push_back(term); }
            }
            if (!sub_poly.is_one()) {
                result.ones.push_back(std::move(sub_poly));
            }
        }
        for (const Polynomial &poly : unknown) {
            Polynomial sub_poly;
            for (const Term &term : poly) {
                if (!sub.is_zeroed(term)) { sub_poly.push_back(term); }
            }
            if (!sub_poly.is_zero_or_one()) {
                result.unknown.push_back(std::move(sub_poly));
            }
        }
        return result;
    }


    /**
     * Special case of apply(ZeroSubstitution) for a single variable p_i.
     */
    constexpr System set_p_zero(variable_index_t p_index) const noexcept {
        System result;
        result.active_ps = drop(active_ps, p_index);
        result.active_qs = active_qs;
        for (const Term &term : zeros) {
            if (term.p_index != p_index) { result.zeros.push_back(term); }
        }
        for (const Polynomial &poly : ones) {
            Polynomial sub_poly;
            for (const Term &term : poly) {
                if (term.p_index != p_index) { sub_poly.push_back(term); }
            }
            if (!sub_poly.is_one()) {
                result.ones.push_back(std::move(sub_poly));
            }
        }
        for (const Polynomial &poly : unknown) {
            Polynomial sub_poly;
            for (const Term &term : poly) {
                if (term.p_index != p_index) { sub_poly.push_back(term); }
            }
            if (!sub_poly.is_zero_or_one()) {
                result.unknown.push_back(std::move(sub_poly));
            }
        }
        return result;
    }


    /**
     * Special case of apply(ZeroSubstitution) for a single variable q_j.
     */
    constexpr System set_q_zero(variable_index_t q_index) const noexcept {
        System result;
        result.active_ps = active_ps;
        result.active_qs = drop(active_qs, q_index);
        for (const Term &term : zeros) {
            if (term.q_index != q_index) { result.zeros.push_back(term); }
        }
        for (const Polynomial &poly : ones) {
            Polynomial sub_poly;
            for (const Term &term : poly) {
                if (term.q_index != q_index) { sub_poly.push_back(term); }
            }
            if (!sub_poly.is_one()) {
                result.ones.push_back(std::move(sub_poly));
            }
        }
        for (const Polynomial &poly : unknown) {
            Polynomial sub_poly;
            for (const Term &term : poly) {
                if (term.q_index != q_index) { sub_poly.push_back(term); }
            }
            if (!sub_poly.is_zero_or_one()) {
                result.unknown.push_back(std::move(sub_poly));
            }
        }
        return result;
    }


    constexpr System set_p_one(variable_index_t p_index) const noexcept {
        constexpr variable_index_t ZERO = static_cast<variable_index_t>(0);
        System result;
        result.active_ps = drop(active_ps, p_index);
        result.active_qs = active_qs;
        ZeroSubstitution transformation;
        for (const Term &term : zeros) {
            if (term.p_index == p_index) {
                transformation.set_q_zero(term.q_index);
            } else {
                result.zeros.push_back(term);
            }
        }
        for (const Polynomial &poly : ones) {
            Polynomial sub_poly;
            for (const Term &term : poly) {
                sub_poly.emplace_back(
                    (term.p_index == p_index) ? ZERO : term.p_index,
                    term.q_index
                );
            }
            if (!sub_poly.is_one()) {
                result.ones.push_back(std::move(sub_poly));
            }
        }
        for (const Polynomial &poly : unknown) {
            Polynomial sub_poly;
            for (const Term &term : poly) {
                sub_poly.emplace_back(
                    (term.p_index == p_index) ? ZERO : term.p_index,
                    term.q_index
                );
            }
            if (!sub_poly.is_zero_or_one()) {
                result.unknown.push_back(std::move(sub_poly));
            }
        }
        return result.apply(transformation);
    }


    constexpr System set_q_one(variable_index_t q_index) const noexcept {
        constexpr variable_index_t ZERO = static_cast<variable_index_t>(0);
        System result;
        result.active_ps = active_ps;
        result.active_qs = drop(active_qs, q_index);
        ZeroSubstitution transformation;
        for (const Term &term : zeros) {
            if (term.q_index == q_index) {
                transformation.set_p_zero(term.p_index);
            } else {
                result.zeros.push_back(term);
            }
        }
        for (const Polynomial &poly : ones) {
            Polynomial sub_poly;
            for (const Term &term : poly) {
                sub_poly.emplace_back(
                    term.p_index,
                    (term.q_index == q_index) ? ZERO : term.q_index
                );
            }
            if (!sub_poly.is_one()) {
                result.ones.push_back(std::move(sub_poly));
            }
        }
        for (const Polynomial &poly : unknown) {
            Polynomial sub_poly;
            for (const Term &term : poly) {
                sub_poly.emplace_back(
                    term.p_index,
                    (term.q_index == q_index) ? ZERO : term.q_index
                );
            }
            if (!sub_poly.is_zero_or_one()) {
                result.unknown.push_back(std::move(sub_poly));
            }
        }
        return result.apply(transformation);
    }


    constexpr bool constrains_p(variable_index_t p_index) const noexcept {
        for (const Term &term : zeros) {
            if (term.p_index == p_index) { return true; }
        }
        for (const Polynomial &poly : ones) {
            for (const Term &term : poly) {
                if (term.p_index == p_index) { return true; }
            }
        }
        for (const Polynomial &poly : unknown) {
            for (const Term &term : poly) {
                if (term.p_index == p_index) { return true; }
            }
        }
        return false;
    }


    constexpr bool constrains_q(variable_index_t q_index) const noexcept {
        for (const Term &term : zeros) {
            if (term.q_index == q_index) { return true; }
        }
        for (const Polynomial &poly : ones) {
            for (const Term &term : poly) {
                if (term.q_index == q_index) { return true; }
            }
        }
        for (const Polynomial &poly : unknown) {
            for (const Term &term : poly) {
                if (term.q_index == q_index) { return true; }
            }
        }
        return false;
    }


    constexpr bool has_free_variable() const noexcept {
        for (const variable_index_t &p_index : active_ps) {
            if (!constrains_p(p_index)) { return true; }
        }
        for (const variable_index_t &q_index : active_qs) {
            if (!constrains_q(q_index)) { return true; }
        }
        return false;
    }


    constexpr Term find_unknown_variable() const noexcept {
        constexpr variable_index_t ZERO = static_cast<variable_index_t>(0);
        for (const Polynomial &poly : unknown) {
            if (poly.size() == 1) {
                const Term term = poly.front();
                if (term.is_linear()) { return term; }
            }
        }
        return Term(ZERO, ZERO);
    }


    void print_latex() const;


    void print_active_variables_plain_text() const;


    void print_active_variables_wolfram() const;


}; // struct System


} // namespace ZeroOneSolver

#endif // ZERO_ONE_SOLVER_SYSTEM_HPP_INCLUDED
