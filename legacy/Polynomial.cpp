#include "Polynomial.hpp"

#include <cstdio> // for std::fputs, stdout


void ZeroOneSolver::Polynomial::print_plain_text() const {
    bool first = true;
    for (const Term &term : *this) {
        if (first) {
            first = false;
        } else {
            std::fputs(" + ", stdout);
        }
        term.print_plain_text();
    }
}


void ZeroOneSolver::Polynomial::print_latex() const {
    bool first = true;
    for (const Term &term : *this) {
        if (first) {
            first = false;
        } else {
            std::fputs(" + ", stdout);
        }
        term.print_latex();
    }
}


void ZeroOneSolver::Polynomial::print_wolfram() const {
    bool first = true;
    for (const Term &term : *this) {
        if (first) {
            first = false;
        } else {
            std::fputs(" + ", stdout);
        }
        term.print_wolfram();
    }
}
