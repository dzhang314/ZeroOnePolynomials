#include "ZeroSubstitution.hpp"

#include <cstdio> // for std::printf, std::putchar

#include "Utilities.hpp" // for ZeroOneSolver::sort_unique


void ZeroOneSolver::ZeroSubstitution::print_latex() {
    sort_unique(zeroed_ps);
    for (const variable_index_t &p_index : zeroed_ps) {
        std::printf("p_{%d} = ", p_index);
    }
    sort_unique(zeroed_qs);
    for (const variable_index_t &q_index : zeroed_qs) {
        std::printf("q_{%d} = ", q_index);
    }
    for (const Term &term : zeroed_terms) {
        std::printf("p_{%d} q_{%d} = ", term.p_index, term.q_index);
    }
    std::putchar('0');
}


void ZeroOneSolver::ZeroSubstitution::print_variables_latex() {
    sort_unique(zeroed_ps);
    for (const variable_index_t &p_index : zeroed_ps) {
        std::printf("p_{%d} = ", p_index);
    }
    sort_unique(zeroed_qs);
    for (const variable_index_t &q_index : zeroed_qs) {
        std::printf("q_{%d} = ", q_index);
    }
    std::putchar('0');
}
