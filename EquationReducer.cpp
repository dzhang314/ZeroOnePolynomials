#include <cstdint>    // for std::intNN_t, std::intmax_t
#include <cstdlib>    // for EXIT_SUCCESS, EXIT_FAILURE
#include <filesystem> // for std::filesystem::rename
#include <fstream>    // for std::ofstream
#include <iostream>   // for std::ostream, std::cout, std::cerr, std::endl
#include <string>     // for std::string, std::stoull
#include <utility>    // for std::move
#include <vector>     // for std::vector


////////////////////////////////////////////////////////////////////////////////


template <typename T>
struct Term {


    T p_index;
    T q_index;


    explicit constexpr Term(T p, T q) noexcept : p_index(p), q_index(q) {}


    constexpr bool is_zeroed_p(const std::vector<T> &zeroed_ps) const noexcept {
        if (p_index) {
            for (const T p : zeroed_ps) {
                if (p_index == p) {
                    return true;
                }
            }
        }
        return false;
    }


    constexpr bool is_zeroed_q(const std::vector<T> &zeroed_qs) const noexcept {
        if (q_index) {
            for (const T q : zeroed_qs) {
                if (q_index == q) {
                    return true;
                }
            }
        }
        return false;
    }


    constexpr bool is_zeroed(
        const std::vector<T> &zeroed_ps, const std::vector<T> &zeroed_qs
    ) const noexcept {
        return is_zeroed_p(zeroed_ps) || is_zeroed_q(zeroed_qs);
    }


    constexpr bool is_zeroed(const std::vector<Term> &zeroed_terms
    ) const noexcept {
        for (const Term &term : zeroed_terms) {
            if ((p_index == term.p_index) && (q_index == term.q_index)) {
                return true;
            }
        }
        return false;
    }


}; // struct Term<T>


template <typename T>
std::ostream &operator<<(std::ostream &os, const Term<T> &t) {
    if (t.p_index) {
        if (t.q_index) {
            os << "p_" << static_cast<std::intmax_t>(t.p_index) << " * "
               << "q_" << static_cast<std::intmax_t>(t.q_index);
        } else {
            os << "p_" << static_cast<std::intmax_t>(t.p_index);
        }
    } else {
        if (t.q_index) {
            os << "q_" << static_cast<std::intmax_t>(t.q_index);
        } else {
            os << '1';
        }
    }
    return os;
}


////////////////////////////////////////////////////////////////////////////////


template <typename T>
struct Equation {


    std::vector<Term<T>> terms;


    constexpr Equation() noexcept : terms() {}


    Equation(Equation &&) = default;


    void add_term(T p, T q) noexcept { terms.emplace_back(p, q); }


    constexpr bool is_constant() const noexcept {
        constexpr T ZERO = static_cast<T>(0);
        for (const Term<T> &term : terms) {
            if ((term.p_index != ZERO) || (term.q_index != ZERO)) {
                return false;
            }
        }
        return true;
    }


    constexpr bool has_constant_term() const noexcept {
        constexpr T ZERO = static_cast<T>(0);
        for (const Term<T> &term : terms) {
            if ((term.p_index == ZERO) && (term.q_index == ZERO)) {
                return true;
            }
        }
        return false;
    }


}; // struct Equation<T>


template <typename T>
std::ostream &operator<<(std::ostream &os, const Equation<T> &eq) {
    bool first = true;
    for (const Term<T> &term : eq.terms) {
        if (first) {
            first = false;
        } else {
            os << " + ";
        }
        os << term;
    }
    return os;
}


////////////////////////////////////////////////////////////////////////////////


template <typename T>
std::vector<Equation<T>> initial_equations(T p_degree, T q_degree) {

    constexpr T ZERO = static_cast<T>(0);
    constexpr T ONE = static_cast<T>(1);
    using index_t = typename std::vector<Equation<T>>::size_type;

    std::vector<Equation<T>> result(
        static_cast<index_t>(p_degree + q_degree - 1)
    );

    for (T p = ZERO; p <= p_degree; ++p) {
        for (T q = ZERO; q <= q_degree; ++q) {
            if ((p == ZERO) && (q == ZERO)) {
                continue; // omit constant term of product polynomial
            }
            if ((p == p_degree) && (q == q_degree)) {
                continue; // omit leading term of product polynomial
            }
            result[static_cast<index_t>(p + q - ONE)].add_term(
                (p == p_degree) ? ZERO : p, (q == q_degree) ? ZERO : q
            );
        }
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////


template <typename T>
struct State {


    std::vector<Term<T>> zeroed_terms;
    std::vector<Equation<T>> equations;


    explicit constexpr State() noexcept : zeroed_terms(), equations() {}


    explicit constexpr State(T p_degree, T q_degree) noexcept
        : zeroed_terms(), equations(initial_equations(p_degree, q_degree)) {}


    State set_p_zero(T p_index) const noexcept {
        State result;
        for (const Term<T> &term : zeroed_terms) {
            if (term.p_index != p_index) {
                result.zeroed_terms.push_back(term);
            }
        }
        for (const Equation<T> &equation : equations) {
            Equation<T> new_equation;
            for (const Term<T> &term : equation.terms) {
                if (term.p_index != p_index) {
                    new_equation.terms.push_back(term);
                }
            }
            if (!new_equation.is_constant()) {
                result.equations.push_back(std::move(new_equation));
            }
        }
        return result;
    }


    State set_q_zero(T q_index) const noexcept {
        State result;
        for (const Term<T> &term : zeroed_terms) {
            if (term.q_index != q_index) {
                result.zeroed_terms.push_back(term);
            }
        }
        for (const Equation<T> &equation : equations) {
            Equation<T> new_equation;
            for (const Term<T> &term : equation.terms) {
                if (term.q_index != q_index) {
                    new_equation.terms.push_back(term);
                }
            }
            if (!new_equation.is_constant()) {
                result.equations.push_back(std::move(new_equation));
            }
        }
        return result;
    }


    State set_p_one(T p_index) const noexcept {

        constexpr T ZERO = static_cast<T>(0);

        State result;
        std::vector<T> zeroed_qs;

        for (const Term<T> &term : zeroed_terms) {
            if (term.p_index == p_index) {
                zeroed_qs.push_back(term.q_index);
            }
        }

        for (const Term<T> &term : zeroed_terms) {
            if (!term.is_zeroed_q(zeroed_qs)) {
                result.zeroed_terms.push_back(term);
            }
        }

        for (const Equation<T> &equation : equations) {
            Equation<T> new_equation;
            for (const Term<T> &term : equation.terms) {
                if (!term.is_zeroed_q(zeroed_qs)) {
                    new_equation.add_term(
                        (term.p_index == p_index) ? ZERO : term.p_index,
                        term.q_index
                    );
                }
            }
            if (!new_equation.is_constant()) {
                result.equations.push_back(std::move(new_equation));
            }
        }
        return result;
    }


    State set_q_one(T q_index) const noexcept {

        constexpr T ZERO = static_cast<T>(0);

        State result;
        std::vector<T> zeroed_ps;

        for (const Term<T> &term : zeroed_terms) {
            if (term.q_index == q_index) {
                zeroed_ps.push_back(term.p_index);
            }
        }

        for (const Term<T> &term : zeroed_terms) {
            if (!term.is_zeroed_p(zeroed_ps)) {
                result.zeroed_terms.push_back(term);
            }
        }

        for (const Equation<T> &equation : equations) {
            Equation<T> new_equation;
            for (const Term<T> &term : equation.terms) {
                if (!term.is_zeroed_p(zeroed_ps)) {
                    new_equation.add_term(
                        term.p_index,
                        (term.q_index == q_index) ? ZERO : term.q_index
                    );
                }
            }
            if (!new_equation.is_constant()) {
                result.equations.push_back(std::move(new_equation));
            }
        }
        return result;
    }


    State remove_constant_terms() const noexcept {

        State result;
        std::vector<T> zeroed_ps;
        std::vector<T> zeroed_qs;
        std::vector<Term<T>> new_zeroed_terms;

        for (const Equation<T> &equation : equations) {
            if (equation.has_constant_term()) {
                for (const Term<T> &term : equation.terms) {
                    if (term.p_index) {
                        if (term.q_index) {
                            new_zeroed_terms.push_back(term);
                        } else {
                            zeroed_ps.push_back(term.p_index);
                        }
                    } else if (term.q_index) {
                        zeroed_qs.push_back(term.q_index);
                    }
                }
            }
        }

        for (const Term<T> &term : zeroed_terms) {
            if (!term.is_zeroed(zeroed_ps, zeroed_qs)) {
                result.zeroed_terms.push_back(term);
            }
        }
        for (const Term<T> &term : new_zeroed_terms) {
            if (!term.is_zeroed(zeroed_ps, zeroed_qs)) {
                result.zeroed_terms.push_back(term);
            }
        }

        for (const Equation<T> &equation : equations) {
            if (!equation.has_constant_term()) {
                Equation<T> new_equation;
                for (const Term<T> &term : equation.terms) {
                    if (!term.is_zeroed(zeroed_ps, zeroed_qs)) {
                        if (!term.is_zeroed(new_zeroed_terms)) {
                            new_equation.terms.push_back(term);
                        }
                    }
                }
                if (!new_equation.is_constant()) {
                    result.equations.push_back(std::move(new_equation));
                }
            }
        }

        return result;
    }


    Term<T> find_eligible_variable() const noexcept {
        constexpr T ZERO = static_cast<T>(0);
        for (const Equation<T> &eq : equations) {
            if (eq.terms.size() == 1) {
                const Term<T> term = eq.terms.front();
                if ((term.p_index == ZERO) ^ (term.q_index == ZERO)) {
                    return term;
                }
            }
        }
        return Term<T>(ZERO, ZERO);
    }


}; // struct State<T>


////////////////////////////////////////////////////////////////////////////////


int main(int argc, char **argv) {

    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " i j filename" << std::endl;
        return EXIT_FAILURE;
    }

    using INDEX_T = std::uint16_t;
    const INDEX_T i = static_cast<INDEX_T>(std::stoull(std::string(argv[1])));
    const INDEX_T j = static_cast<INDEX_T>(std::stoull(std::string(argv[2])));
    const std::string file_name(argv[3]);
    const std::string temp_name = file_name + ".temp";

    {
        std::ofstream output_file(temp_name);
        std::vector<State<INDEX_T>> stack;
        stack.push_back(State<INDEX_T>(i, j).remove_constant_terms());
        while (!stack.empty()) {
            const State<INDEX_T> state = std::move(stack.back());
            stack.pop_back();
            const Term<INDEX_T> var = state.find_eligible_variable();
            if (var.p_index) {
                stack.push_back(state.set_p_zero(var.p_index));
                stack.push_back(
                    state.set_p_one(var.p_index).remove_constant_terms()
                );
            } else if (var.q_index) {
                stack.push_back(state.set_q_zero(var.q_index));
                stack.push_back(
                    state.set_q_one(var.q_index).remove_constant_terms()
                );
            } else if (!state.zeroed_terms.empty()) {
                const Term<INDEX_T> branch_term = state.zeroed_terms.front();
                stack.push_back(state.set_p_zero(branch_term.p_index));
                stack.push_back(state.set_q_zero(branch_term.q_index));
            } else if (!state.equations.empty()) {
                for (const Equation<INDEX_T> &eq : state.equations) {
                    output_file << eq << '\n';
                }
                output_file << std::endl;
            }
        }
    }

    std::filesystem::rename(temp_name, file_name);
    std::cout << "Computed reduced equations of degree ("
              << static_cast<std::intmax_t>(i) << ", "
              << static_cast<std::intmax_t>(j) << ") and saved to file "
              << file_name << "." << std::endl;
    return EXIT_SUCCESS;
}
