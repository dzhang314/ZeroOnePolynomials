#include "Term.hpp"

#include <cstdio> // for std::printf, std::putchar


void ZeroOneSolver::Term::print_plain_text() const {
    if (is_quadratic()) {
        std::printf("p_%d * q_%d", p_index, q_index);
    } else if (has_p()) {
        std::printf("p_%d", p_index);
    } else if (has_q()) {
        std::printf("q_%d", q_index);
    } else {
        std::putchar('1');
    }
}


void ZeroOneSolver::Term::print_latex() const {
    if (is_quadratic()) {
        std::printf("p_{%d} q_{%d}", p_index, q_index);
    } else if (has_p()) {
        std::printf("p_{%d}", p_index);
    } else if (has_q()) {
        std::printf("q_{%d}", q_index);
    } else {
        std::putchar('1');
    }
}


void ZeroOneSolver::Term::print_wolfram() const {
    if (is_quadratic()) {
        std::printf("p[%d] q[%d]", p_index, q_index);
    } else if (has_p()) {
        std::printf("p[%d]", p_index);
    } else if (has_q()) {
        std::printf("q[%d]", q_index);
    } else {
        std::putchar('1');
    }
}
