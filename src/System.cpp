#include "System.hpp"

#include <cstdio> // for std::fputs, std::printf, stdout


void ZeroOneSolver::System::print_latex() const {
    {
        std::fputs("\\begin{align}", stdout);
        bool first = true;
        for (variable_index_t p_index : active_ps) {
            if (first) {
                std::fputs(" %", stdout);
                first = false;
            }
            std::printf(" p_{%d}", p_index);
        }
        for (variable_index_t q_index : active_qs) {
            if (first) {
                std::fputs(" %", stdout);
                first = false;
            }
            std::printf(" q_{%d}", q_index);
        }
    }
    {
        bool first = true;
        for (const Term &term : zeros) {
            if (first) {
                std::fputs("\n    ", stdout);
                first = false;
            } else {
                std::fputs(" \\\\\n    ", stdout);
            }
            term.print_latex();
            std::fputs(" &= 0", stdout);
        }
        for (const Polynomial &poly : ones) {
            if (first) {
                std::fputs("\n    ", stdout);
                first = false;
            } else {
                std::fputs(" \\\\\n    ", stdout);
            }
            poly.print_latex();
            std::fputs(" &= 1", stdout);
        }
        for (const Polynomial &poly : unknown) {
            if (first) {
                std::fputs("\n    ", stdout);
                first = false;
            } else {
                std::fputs(" \\\\\n    ", stdout);
            }
            poly.print_latex();
            std::fputs(" &= 0 \\text{ or } 1", stdout);
        }
    }
    std::puts("\n\\end{align*}");
}
