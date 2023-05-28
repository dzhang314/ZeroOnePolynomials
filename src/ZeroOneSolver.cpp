#include <cctype>   // for std::isdigit
#include <cstdio>   // for std::fputs, std::printf, std::putchar, std::puts
#include <cstdlib>  // for std::size_t, std::strtoll, EXIT_SUCCESS, EXIT_FAILURE
#include <limits>   // for std::numeric_limits
#include <optional> // for std::optional
#include <string>   // for std::string
#include <vector>   // for std::vector

#include "Assertions.hpp"       // for ZeroOneSolver::{ensure, prevent}
#include "Polynomial.hpp"       // for ZeroOneSolver::Polynomial
#include "Simplification.hpp"   // for ZeroOneSolver::simplify
#include "System.hpp"           // for ZeroOneSolver::System
#include "Term.hpp"             // for ZeroOneSolver::{Term, variable_index_t}
#include "Utilities.hpp"        // for ZeroOneSolver::contains
#include "ZeroSubstitution.hpp" // for ZeroOneSolver::ZeroSubstitution

using ZeroOneSolver::contains;
using ZeroOneSolver::ensure;
using ZeroOneSolver::Polynomial;
using ZeroOneSolver::prevent;
using ZeroOneSolver::simplify;
using ZeroOneSolver::System;
using ZeroOneSolver::Term;
using ZeroOneSolver::variable_index_t;
using ZeroOneSolver::ZeroSubstitution;


/**
 * This program solves systems of quadratic equations of the following form:
 *
 *     t_11 + t_12 + ... + t_1a = 0 or 1
 *     t_21 + t_22 + ... + t_2b = 0 or 1
 *                   ...
 *     t_m1 + t_m2 + ... + t_mn = 0 or 1
 *
 * Here, each term t_k is a monomial of the form 1, p_i, q_j, or p_i * q_j,
 * and the variables p_i and q_j are constrained to values in [0, 1].
 *
 * Systems of equations of this form naturally arise from the 0-1 Polynomial
 * Conjecture, which this program was written to verify (or to find
 * counterexamples to).
 */


static void print_case_id(const std::vector<bool> &case_id) {
    bool first = true;
    for (const bool b : case_id) {
        if (first) {
            first = false;
        } else {
            std::putchar('.');
        }
        std::putchar(b ? '2' : '1');
    }
}


template <bool paranoid>
System move_unknown_to_zero(const System &system, std::size_t index) {
    if constexpr (paranoid) {
        ensure(
            index < system.unknown.size(),
            "ERROR: Polynomial to move is out of bounds."
        );
    }
    const Polynomial &equation = system.unknown[index];
    if constexpr (paranoid) {
        for (const Term &term : equation) {
            prevent(
                term.is_constant(),
                "ERROR: Polynomial to move has a constant term."
            );
        }
    }
    ZeroSubstitution transformation;
    transformation.set_zero(equation);
    return system.apply(transformation);
}


template <bool paranoid>
System move_unknown_to_one(const System &system, std::size_t index) {
    if constexpr (paranoid) {
        ensure(
            index < system.unknown.size(),
            "ERROR: Polynomial to move is out of bounds."
        );
    }
    System result;
    result.active_ps = system.active_ps;
    result.active_qs = system.active_qs;
    result.zeros = system.zeros;
    result.ones = system.ones;
    result.ones.push_back(system.unknown[index]);
    for (std::size_t i = 0; i < system.unknown.size(); ++i) {
        if (i != index) { result.unknown.push_back(system.unknown[i]); }
    }
    return result;
}


enum class PrintMode { PLAIN_TEXT, WOLFRAM, LATEX };


template <PrintMode mode, bool paranoid>
void analyze(std::vector<bool> &case_id, const System &system) {

    if constexpr (mode == PrintMode::LATEX) {
        if (!case_id.empty()) {
            std::fputs("\n\\textbf{Case ", stdout);
            print_case_id(case_id);
            std::fputs(":}", stdout);
            if (system.is_empty()) {
                std::puts(" This case is trivial.");
            } else {
                std::puts(" In this case, we have the following"
                          " system of equations:");
                system.print_latex();
            }
        }
    }

    if (system.is_empty()) { return; }

    std::optional<System> simplified =
        simplify<mode == PrintMode::LATEX, paranoid>(system);

    if (simplified.has_value()) {

        const Term var = simplified->find_unknown_variable();

        if (var.has_p()) {

            if constexpr (mode == PrintMode::LATEX) {
                std::printf(
                    "We consider two cases based on the equation"
                    " $p_{%d} = 0 \\text{ or } 1$, which implies"
                    " $p_{%d} = 0$ (Case ",
                    var.p_index,
                    var.p_index
                );
                case_id.push_back(false);
                print_case_id(case_id);
                case_id.pop_back();
                std::printf(") or $p_{%d} = 1$ (Case ", var.p_index);
                case_id.push_back(true);
                print_case_id(case_id);
                case_id.pop_back();
                std::puts(").");
            }

            case_id.push_back(false);
            analyze<mode, paranoid>(
                case_id, simplified->set_p_zero(var.p_index)
            );
            case_id.pop_back();
            case_id.push_back(true);
            analyze<mode, paranoid>(
                case_id, simplified->set_p_one(var.p_index)
            );
            case_id.pop_back();

        } else if (var.has_q()) {

            if constexpr (mode == PrintMode::LATEX) {
                std::printf(
                    "We consider two cases based on the equation"
                    " $q_{%d} = 0 \\text{ or } 1$, which implies"
                    " $q_{%d} = 0$ (Case ",
                    var.q_index,
                    var.q_index
                );
                case_id.push_back(false);
                print_case_id(case_id);
                case_id.pop_back();
                std::printf(") or $q_{%d} = 1$ (Case ", var.q_index);
                case_id.push_back(true);
                print_case_id(case_id);
                case_id.pop_back();
                std::puts(").");
            }

            case_id.push_back(false);
            analyze<mode, paranoid>(
                case_id, simplified->set_q_zero(var.q_index)
            );
            case_id.pop_back();
            case_id.push_back(true);
            analyze<mode, paranoid>(
                case_id, simplified->set_q_one(var.q_index)
            );
            case_id.pop_back();

        } else if (!simplified->zeros.empty()) {

            const Term zero_term = simplified->zeros.front();

            if constexpr (mode == PrintMode::LATEX) {
                std::fputs(
                    "We consider two cases based on the equation $", stdout
                );
                zero_term.print_latex();
                std::printf(
                    " = 0$, which implies $p_{%d} = 0$ (Case ",
                    zero_term.p_index
                );
                case_id.push_back(false);
                print_case_id(case_id);
                case_id.pop_back();
                std::printf(") or $q_{%d} = 0$ (Case ", zero_term.q_index);
                case_id.push_back(true);
                print_case_id(case_id);
                case_id.pop_back();
                std::puts(").");
            }

            case_id.push_back(false);
            analyze<mode, paranoid>(
                case_id, simplified->set_p_zero(zero_term.p_index)
            );
            case_id.pop_back();
            case_id.push_back(true);
            analyze<mode, paranoid>(
                case_id, simplified->set_q_zero(zero_term.q_index)
            );
            case_id.pop_back();

        } else if (!simplified->unknown.empty()) {

            std::size_t best_index = 0;
            std::size_t best_length = simplified->unknown[0].size();
            for (std::size_t k = 1; k < simplified->unknown.size(); ++k) {
                const std::size_t length = simplified->unknown[k].size();
                if (length < best_length) {
                    best_index = k;
                    best_length = length;
                }
            }

            if constexpr (mode == PrintMode::LATEX) {
                std::fputs(
                    "We consider two cases based on the equation $", stdout
                );
                simplified->unknown[best_index].print_latex();
                std::fputs(" = 0 \\text{ or } 1$, which implies $", stdout);
                simplified->unknown[best_index].print_latex();
                std::fputs(" = 0$ (Case ", stdout);
                case_id.push_back(false);
                print_case_id(case_id);
                case_id.pop_back();
                std::fputs(") or $", stdout);
                simplified->unknown[best_index].print_latex();
                std::fputs(" = 1$ (Case ", stdout);
                case_id.push_back(true);
                print_case_id(case_id);
                case_id.pop_back();
                std::puts(").");
            }

            case_id.push_back(false);
            analyze<mode, paranoid>(
                case_id, move_unknown_to_zero<paranoid>(*simplified, best_index)
            );
            case_id.pop_back();
            case_id.push_back(true);
            analyze<mode, paranoid>(
                case_id, move_unknown_to_one<paranoid>(*simplified, best_index)
            );
            case_id.pop_back();

        } else {
            if constexpr (mode == PrintMode::LATEX) {
                std::puts(
                    "It remains to be shown via a Groebner basis calculation"
                    " that this system of equations has no solutions."
                );
            } else if constexpr (mode == PrintMode::PLAIN_TEXT) {
                for (const Polynomial &poly : simplified->ones) {
                    poly.print_plain_text();
                    std::putchar('\n');
                }
                std::putchar('\n');
            } else if constexpr (mode == PrintMode::WOLFRAM) {
                for (const Polynomial &poly : simplified->ones) {
                    poly.print_wolfram();
                    std::putchar('\n');
                }
                std::putchar('\n');
            }
        }
    }
}


template <bool paranoid>
void print_proof(variable_index_t i, variable_index_t j) {
    std::puts("\\documentclass{article}\n");
    std::puts("\\usepackage{amsmath}");
    std::puts("\\usepackage[margin=0.5in, includefoot]{geometry}");
    std::puts("\\usepackage{parskip}\n");
    std::puts("\\begin{document}\n");
    std::printf(
        "\\textbf{Theorem:} The 0--1 Polynomial Conjecture holds"
        " when $(\\deg P, \\deg Q) = (%d, %d)$.\n\n",
        i,
        j
    );
    std::fputs("\\textit{Proof:} Let $P(x) = 1", stdout);
    for (variable_index_t k = 1; k < i; ++k) {
        std::printf(" + p_{%d} x^{%d}", k, k);
    }
    std::printf(" + x^{%d}$ and $Q(x) = 1", i);
    for (variable_index_t k = 1; k < j; ++k) {
        std::printf(" + q_{%d} x^{%d}", k, k);
    }
    std::printf(
        " + x^{%d}$. If $P(x) Q(x)$ is a 0--1 polynomial,"
        " then the following system of equations holds:\n",
        j
    );
    const System initial_system(i, j);
    initial_system.print_latex();
    std::puts("We must show that all nonnegative solutions of this"
              " system of equations are $\\{0, 1\\}$-valued.\n");
    std::vector<bool> case_id;
    analyze<PrintMode::LATEX, paranoid>(case_id, initial_system);
    std::puts("\n\\end{document}");
}


template <PrintMode mode, bool paranoid>
void print_systems(variable_index_t i, variable_index_t j) {
    const System initial_system(i, j);
    std::vector<bool> case_id;
    analyze<mode, paranoid>(case_id, initial_system);
}


static bool validate_arguments(const std::vector<std::string> &args) {
    const std::vector<std::string> VALID_FLAGS = {
        "--wolfram", "--latex", "--paranoid"};
    if (args.size() < 3) { return false; }
    for (const char &c : args[1]) {
        if (!std::isdigit(c)) { return false; }
    }
    for (const char &c : args[2]) {
        if (!std::isdigit(c)) { return false; }
    }
    for (std::size_t k = 3; k < args.size(); ++k) {
        if (!contains(VALID_FLAGS, args[k])) { return false; }
    }
    return true;
}


static PrintMode get_mode(const std::vector<std::string> &args) {
    PrintMode result = PrintMode::PLAIN_TEXT;
    for (std::size_t k = 3; k < args.size(); ++k) {
        if (args[k] == "--wolfram") {
            result = PrintMode::WOLFRAM;
        } else if (args[k] == "--latex") {
            result = PrintMode::LATEX;
        }
    }
    return result;
}


static bool get_paranoia(const std::vector<std::string> &args) {
    for (std::size_t k = 3; k < args.size(); ++k) {
        if (args[k] == "--paranoid") { return true; }
    }
    return false;
}


int main(int argc, char **argv) {

    const std::vector<std::string> args(argv, argv + argc);

    if (!validate_arguments(args)) {
        std::fprintf(
            stderr,
            "Usage: %s i j [--wolfram | --latex ] [--paranoid]\n",
            argv[0]
        );
        return EXIT_FAILURE;
    }

    constexpr long long upper_bound =
        static_cast<long long>(std::numeric_limits<variable_index_t>::max());

    const long long i_in = std::strtoll(args[1].c_str(), nullptr, 10);
    const long long j_in = std::strtoll(args[2].c_str(), nullptr, 10);
    const bool i_valid = (i_in > 0) && (i_in <= upper_bound);
    const bool j_valid = (j_in > 0) && (j_in <= upper_bound);
    ensure(i_valid && j_valid, "ERROR: Input parameters out of range.");

    const variable_index_t i = static_cast<variable_index_t>(i_in);
    const variable_index_t j = static_cast<variable_index_t>(j_in);
    const PrintMode mode = get_mode(args);
    const bool paranoid = get_paranoia(args);

    switch (mode) {

    case PrintMode::PLAIN_TEXT:
        if (paranoid) {
            print_systems<PrintMode::PLAIN_TEXT, true>(i, j);
        } else {
            print_systems<PrintMode::PLAIN_TEXT, false>(i, j);
        }
        break;

    case PrintMode::WOLFRAM:
        if (paranoid) {
            print_systems<PrintMode::WOLFRAM, true>(i, j);
        } else {
            print_systems<PrintMode::WOLFRAM, false>(i, j);
        }
        break;

    case PrintMode::LATEX:
        if (paranoid) {
            print_proof<true>(i, j);
        } else {
            print_proof<false>(i, j);
        }
        break;
    }

    return EXIT_SUCCESS;
}
