#include "Assertions.hpp"

#include <cstdio>  // for std::fputc, std::fputs, stderr
#include <cstdlib> // for std::exit, EXIT_FAILURE

#include "Polynomial.hpp" // for ZeroOneSolver::Polynomial
#include "Term.hpp"       // for ZeroOneSolver::Term
#include "Utilities.hpp"  // for ZeroOneSolver::contains


void ZeroOneSolver::ensure(bool condition, const char *message) {
    if (!condition) {
        std::fputs(message, stderr);
        std::fputc('\n', stderr);
        std::exit(EXIT_FAILURE);
    }
}


void ZeroOneSolver::prevent(bool condition, const char *message) {
    if (condition) {
        std::fputs(message, stderr);
        std::fputc('\n', stderr);
        std::exit(EXIT_FAILURE);
    }
}


void ZeroOneSolver::ensure_active(
    const std::vector<variable_index_t> &active_indices, variable_index_t index
) {
    constexpr variable_index_t ZERO = static_cast<variable_index_t>(0);
    ensure(
        (index == ZERO) || contains(active_indices, index),
        "ERROR: System contains inactive variable."
    );
}


void ZeroOneSolver::ensure_variable_validity(const System &system) {
    for (const Term &term : system.zeros) {
        ensure_active(system.active_ps, term.p_index);
        ensure_active(system.active_qs, term.q_index);
    }
    for (const Polynomial &poly : system.ones) {
        for (const Term &term : poly) {
            ensure_active(system.active_ps, term.p_index);
            ensure_active(system.active_qs, term.q_index);
        }
    }
    for (const Polynomial &poly : system.unknown) {
        for (const Term &term : poly) {
            ensure_active(system.active_ps, term.p_index);
            ensure_active(system.active_qs, term.q_index);
        }
    }
}
