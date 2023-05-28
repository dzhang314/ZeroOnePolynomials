#include "Simplification.hpp"

#include <cstdio>   // for std::fputs, std::printf, std::puts, stdout
#include <optional> // for std::nullopt
#include <utility>  // for std::pair

#include "Assertions.hpp"       // for ZeroOneSolver::ensure_variable_validity
#include "Polynomial.hpp"       // for ZeroOneSolver::Polynomial
#include "Term.hpp"             // for ZeroOneSolver::{Term, variable_index_t}
#include "Utilities.hpp"        // for ZeroOneSolver::sort_unique
#include "ZeroSubstitution.hpp" // for ZeroOneSolver::ZeroSubstitution


static void
print_equation(const ZeroOneSolver::Polynomial &poly, bool is_unknown) {
    poly.print_latex();
    std::fputs(is_unknown ? " = 0 \\text{ or } 1" : " = 1", stdout);
}


template <bool verbose, bool paranoid>
std::optional<ZeroOneSolver::System>
ZeroOneSolver::simplify(const System &system) {

    if constexpr (paranoid) { ensure_variable_validity(system); }

    if (system.has_unsatisfiable_equation()) {
        if constexpr (verbose) {
            std::puts("This system of equations is"
                      " inconsistent and has no solutions.");
        }
        return std::nullopt;
    }

    if (system.is_solved()) {
        if constexpr (verbose) {
            std::puts("Every variable in this system of equations is"
                      " directly constrained to values in $\\{0, 1\\}$.");
        }
        return std::nullopt;
    }

    for (const Polynomial &equation : system.ones) {
        if (equation.size() == 1) {
            const Term term = equation.front();
            if (term.is_quadratic()) {
                if constexpr (verbose) {
                    std::printf(
                        "From the equation $p_{%d} q_{%d} = 1$, we may"
                        " conclude that $p_{%d} = 1$ and $q_{%d} = 1$.",
                        term.p_index,
                        term.q_index,
                        term.p_index,
                        term.q_index
                    );
                }
                const System result =
                    system.set_p_one(term.p_index).set_q_one(term.q_index);
                if (result.is_empty()) {
                    if constexpr (verbose) {
                        std::puts(" After performing these substitutions,"
                                  " it is straightforward to verify that"
                                  " the resulting system of equations only"
                                  " admits $\\{0, 1\\}$-valued solutions.");
                    }
                    return std::nullopt;
                } else {
                    if constexpr (verbose) {
                        std::puts(" Performing these substitutions yields"
                                  " the following system of equations:");
                        result.print_latex();
                    }
                    return simplify<verbose, paranoid>(result);
                }
            } else if (term.has_p()) {
                const System result = system.set_p_one(term.p_index);
                if (result.is_empty()) {
                    if constexpr (verbose) {
                        std::printf(
                            "After performing the substitution $p_{%d} = 1$,"
                            " it is straightforward to verify that the"
                            " resulting system of equations only admits"
                            " $\\{0, 1\\}$-valued solutions.\n",
                            term.p_index
                        );
                    }
                    return std::nullopt;
                } else {
                    if constexpr (verbose) {
                        std::printf(
                            "Performing the substitution $p_{%d} = 1$"
                            " yields the following system of equations:\n",
                            term.p_index
                        );
                        result.print_latex();
                    }
                    return simplify<verbose, paranoid>(result);
                }
            } else if (term.has_q()) {
                const System result = system.set_q_one(term.q_index);
                if (result.is_empty()) {
                    if constexpr (verbose) {
                        std::printf(
                            "After performing the substitution $q_{%d} = 1$,"
                            " it is straightforward to verify that the"
                            " resulting system of equations only admits"
                            " $\\{0, 1\\}$-valued solutions.\n",
                            term.q_index
                        );
                    }
                    return std::nullopt;
                } else {
                    if constexpr (verbose) {
                        std::printf(
                            "Performing the substitution $q_{%d} = 1$"
                            " yields the following system of equations:\n",
                            term.q_index
                        );
                        result.print_latex();
                    }
                    return simplify<verbose, paranoid>(result);
                }
            }
        }
    }

    ZeroSubstitution sub;
    std::vector<std::pair<Polynomial, bool>> sub_equations;

    for (const Polynomial &poly : system.ones) {
        bool found_constant_term = false;
        for (const Term &term : poly) {
            if (term.is_constant()) {
                if constexpr (paranoid) {
                    prevent(
                        found_constant_term,
                        "ERROR: Found multiple constant"
                        " terms in a single equation."
                    );
                }
                found_constant_term = true;
            }
        }
        if (found_constant_term) {
            sub.set_zero(poly);
            if constexpr (verbose) { sub_equations.emplace_back(poly, false); }
        }
    }

    for (const Polynomial &poly : system.unknown) {
        bool found_constant_term = false;
        for (const Term &term : poly) {
            if (term.is_constant()) {
                if constexpr (paranoid) {
                    prevent(
                        found_constant_term,
                        "ERROR: Found multiple constant"
                        " terms in a single equation."
                    );
                }
                found_constant_term = true;
            }
        }
        if (found_constant_term) {
            sub.set_zero(poly);
            if constexpr (verbose) { sub_equations.emplace_back(poly, true); }
        }
    }

    if (sub.is_empty()) { return system; }

    const System result = system.apply(sub);

    if constexpr (verbose) {
        if (sub_equations.size() == 1) {
            std::fputs("From the equation $", stdout);
            print_equation(sub_equations[0].first, sub_equations[0].second);
        } else if (sub_equations.size() == 2) {
            std::fputs("From the equations $", stdout);
            print_equation(sub_equations[0].first, sub_equations[0].second);
            std::fputs("$ and $", stdout);
            print_equation(sub_equations[1].first, sub_equations[1].second);
        } else {
            std::fputs("From the equations $", stdout);
            for (std::size_t k = 0; k < sub_equations.size(); ++k) {
                print_equation(sub_equations[k].first, sub_equations[k].second);
                std::fputs(
                    (k + 2 == sub_equations.size()) ? "$, and $" : "$, $",
                    stdout
                );
            }
        }
        std::fputs("$, we may conclude that $", stdout);
        sort_unique(sub.zeroed_ps);
        for (const variable_index_t &p_index : sub.zeroed_ps) {
            std::printf("p_{%d} = ", p_index);
        }
        sort_unique(sub.zeroed_qs);
        for (const variable_index_t &q_index : sub.zeroed_qs) {
            std::printf("q_{%d} = ", q_index);
        }
        if (!result.is_empty()) {
            for (const Term &term : sub.zeroed_terms) {
                std::printf("p_{%d} q_{%d} = ", term.p_index, term.q_index);
            }
        }
        std::fputs("0$.", stdout);
    }

    if (result.is_empty()) {
        if constexpr (verbose) {
            std::puts(" This is the unique solution"
                      " of this system of equations.");
        }
        return std::nullopt;
    } else {
        if constexpr (verbose) {
            std::puts(" This simplifies the preceding"
                      " system of equations to the following:");
            result.print_latex();
        }
        return simplify<verbose, paranoid>(result);
    }
}


// clang-format off
template std::optional<ZeroOneSolver::System> ZeroOneSolver::simplify<false, false>(const System &);
template std::optional<ZeroOneSolver::System> ZeroOneSolver::simplify<false, true>(const System &);
template std::optional<ZeroOneSolver::System> ZeroOneSolver::simplify<true, false>(const System &);
template std::optional<ZeroOneSolver::System> ZeroOneSolver::simplify<true, true>(const System &);
// clang-format on
