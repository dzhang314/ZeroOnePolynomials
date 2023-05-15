#include <cstdint>  // for std::intNN_t, std::intmax_t
#include <cstdlib>  // for std::size_t, std::strtoull, std::exit
#include <iostream> // for std::ostream, std::cout, std::cerr, std::endl
#include <optional> // for std::optional, std::make_optional, std::nullopt
#include <string>   // for std::string
#include <utility>  // for std::pair, std::make_pair, std::move
#include <vector>   // for std::vector


using index_t = std::uint8_t;


////////////////////////////////////////////////////////////////////////////////


struct Term {


    index_t p_index;
    index_t q_index;


    explicit constexpr Term(index_t p, index_t q) noexcept
        : p_index(p)
        , q_index(q) {}


    constexpr bool operator<=>(const Term &) const noexcept = default;


    constexpr bool has_p() const noexcept {
        constexpr index_t ZERO = static_cast<index_t>(0);
        return (p_index != ZERO);
    }


    constexpr bool has_q() const noexcept {
        constexpr index_t ZERO = static_cast<index_t>(0);
        return (q_index != ZERO);
    }


    constexpr bool is_constant() const noexcept {
        return !(has_p() || has_q());
    }


    constexpr bool is_linear() const noexcept { return has_p() ^ has_q(); }


    constexpr bool is_quadratic() const noexcept { return has_p() && has_q(); }


}; // struct Term


static void print(const Term &term) {
    if (term.is_quadratic()) {
        std::cout << "p_" << static_cast<std::intmax_t>(term.p_index) << " * q_"
                  << static_cast<std::intmax_t>(term.q_index);
    } else if (term.has_p()) {
        std::cout << "p_" << static_cast<std::intmax_t>(term.p_index);
    } else if (term.has_q()) {
        std::cout << "q_" << static_cast<std::intmax_t>(term.q_index);
    } else {
        std::cout << '1';
    }
}


static std::ostream &operator<<(std::ostream &os, const Term &term) {
    if (term.is_quadratic()) {
        os << "p_{" << static_cast<std::intmax_t>(term.p_index) << "} q_{"
           << static_cast<std::intmax_t>(term.q_index) << '}';
    } else if (term.has_p()) {
        os << "p_{" << static_cast<std::intmax_t>(term.p_index) << '}';
    } else if (term.has_q()) {
        os << "q_{" << static_cast<std::intmax_t>(term.q_index) << '}';
    } else {
        os << '1';
    }
    return os;
}


////////////////////////////////////////////////////////////////////////////////


using Equation = std::vector<Term>;


constexpr bool is_zero(const Equation &equation) noexcept {
    return (equation.size() == 0);
}


constexpr bool is_one(const Equation &equation) noexcept {
    return (equation.size() == 1) && equation.front().is_constant();
}


constexpr bool is_zero_or_one(const Equation &equation) noexcept {
    return is_zero(equation) || is_one(equation);
}


static void print(const Equation &equation) {
    bool first = true;
    for (const Term &term : equation) {
        if (first) {
            first = false;
        } else {
            std::cout << " + ";
        }
        print(term);
    }
}


static std::ostream &operator<<(std::ostream &os, const Equation &equation) {
    bool first = true;
    for (const Term &term : equation) {
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
constexpr bool contains(const std::vector<T> &items, const T &item) noexcept {
    for (const T &element : items) {
        if (element == item) { return true; }
    }
    return false;
}


struct ZeroingTransformation {


    std::vector<index_t> zeroed_ps;
    std::vector<index_t> zeroed_qs;
    std::vector<Term> zeroed_terms;


    explicit constexpr ZeroingTransformation() noexcept
        : zeroed_ps()
        , zeroed_qs()
        , zeroed_terms() {}


    constexpr void set_p_zero(index_t index) noexcept {
        zeroed_ps.push_back(index);
    }


    constexpr void set_q_zero(index_t index) noexcept {
        zeroed_qs.push_back(index);
    }


    constexpr void set_zero(const Term &term) noexcept {
        if (term.is_quadratic()) {
            zeroed_terms.push_back(term);
        } else if (term.has_p()) {
            set_p_zero(term.p_index);
        } else if (term.has_q()) {
            set_q_zero(term.q_index);
        }
    }


    constexpr void set_zero(const std::vector<Term> &terms) noexcept {
        for (const Term &term : terms) {
            set_zero(term);
        }
    }


    constexpr bool is_zeroed(const Term &term) const noexcept {
        return contains(zeroed_ps, term.p_index) ||
               contains(zeroed_qs, term.q_index) ||
               contains(zeroed_terms, term);
    }


}; // struct ZeroingTransformation


////////////////////////////////////////////////////////////////////////////////


template <typename T>
constexpr std::vector<T>
drop(const std::vector<T> &items, const T &item) noexcept {
    std::vector<T> result;
    for (const T &element : items) {
        if (element != item) { result.push_back(element); }
    }
    return result;
}


template <typename T>
constexpr std::vector<T> drop_all(
    const std::vector<T> &items, const std::vector<T> &to_delete
) noexcept {
    std::vector<T> result;
    for (const T &element : items) {
        if (!contains(to_delete, element)) { result.push_back(element); }
    }
    return result;
}


struct System {


    std::vector<index_t> active_ps;
    std::vector<index_t> active_qs;
    std::vector<Term> zeros;
    std::vector<Equation> ones;
    std::vector<Equation> unknown;


    explicit constexpr System() noexcept
        : active_ps()
        , active_qs()
        , zeros()
        , ones()
        , unknown() {}


    explicit constexpr System(index_t p_degree, index_t q_degree) noexcept
        : active_ps(static_cast<std::size_t>(p_degree - 1))
        , active_qs(static_cast<std::size_t>(q_degree - 1))
        , zeros()
        , ones()
        , unknown(static_cast<std::size_t>(p_degree + q_degree - 1)) {

        constexpr index_t ZERO = static_cast<index_t>(0);
        constexpr index_t ONE = static_cast<index_t>(1);

        for (index_t p = 1; p < p_degree; ++p) {
            active_ps[static_cast<std::size_t>(p - ONE)] = p;
        }

        for (index_t q = 1; q < q_degree; ++q) {
            active_qs[static_cast<std::size_t>(q - ONE)] = q;
        }

        for (index_t p = ZERO; p <= p_degree; ++p) {
            for (index_t q = ZERO; q <= q_degree; ++q) {
                // omit constant and leading terms of product polynomial
                if ((p == ZERO) && (q == ZERO)) { continue; }
                if ((p == p_degree) && (q == q_degree)) { continue; }
                unknown[static_cast<std::size_t>(p + q - ONE)].emplace_back(
                    (p == p_degree) ? ZERO : p, (q == q_degree) ? ZERO : q
                );
            }
        }
    }


    constexpr bool is_empty() const noexcept {
        return active_ps.empty() && active_qs.empty() && zeros.empty() &&
               ones.empty() && unknown.empty();
    }


    constexpr bool is_consistent() const noexcept {

        for (const Equation &equation : ones) {
            if (equation.empty()) { return false; }
            bool found_constant_term = false;
            for (const Term &term : equation) {
                if (term.is_constant()) {
                    if (found_constant_term) {
                        return false;
                    } else {
                        found_constant_term = true;
                    }
                }
            }
        }

        for (const Equation &equation : unknown) {
            bool found_constant_term = false;
            for (const Term &term : equation) {
                if (term.is_constant()) {
                    if (found_constant_term) {
                        return false;
                    } else {
                        found_constant_term = true;
                    }
                }
            }
        }

        return true;
    }


    constexpr bool is_trivial() const noexcept {

        std::vector<index_t> solved_ps;
        std::vector<index_t> solved_qs;

        for (const Equation &equation : ones) {
            if (equation.size() == 1) {
                const Term term = equation.front();
                if (term.has_p()) { solved_ps.push_back(term.p_index); }
                if (term.has_q()) { solved_qs.push_back(term.q_index); }
            }
        }

        for (const Equation &equation : unknown) {
            if (equation.size() == 1) {
                const Term term = equation.front();
                if (term.is_linear()) {
                    if (term.has_p()) { solved_ps.push_back(term.p_index); }
                    if (term.has_q()) { solved_qs.push_back(term.q_index); }
                }
            }
        }

        return drop_all(active_ps, solved_ps).empty() &&
               drop_all(active_qs, solved_qs).empty();
    }


    constexpr Term find_unknown_variable() const noexcept {
        constexpr index_t ZERO = static_cast<index_t>(0);
        for (const Equation &equation : unknown) {
            if (equation.size() == 1) {
                const Term term = equation.front();
                if (term.is_linear()) { return term; }
            }
        }
        return Term(ZERO, ZERO);
    }


    constexpr System set_p_zero(index_t p_index) const noexcept {

        System result;
        result.active_ps = drop(active_ps, p_index);
        result.active_qs = active_qs;

        for (const Term &term : zeros) {
            if (term.p_index != p_index) { result.zeros.push_back(term); }
        }

        for (const Equation &equation : ones) {
            Equation transformed;
            for (const Term &term : equation) {
                if (term.p_index != p_index) { transformed.push_back(term); }
            }
            if (!is_one(transformed)) {
                result.ones.push_back(std::move(transformed));
            }
        }

        for (const Equation &equation : unknown) {
            Equation transformed;
            for (const Term &term : equation) {
                if (term.p_index != p_index) { transformed.push_back(term); }
            }
            if (!is_zero_or_one(transformed)) {
                result.unknown.push_back(std::move(transformed));
            }
        }

        return result;
    }


    constexpr System set_q_zero(index_t q_index) const noexcept {

        System result;
        result.active_ps = active_ps;
        result.active_qs = drop(active_qs, q_index);

        for (const Term &term : zeros) {
            if (term.q_index != q_index) { result.zeros.push_back(term); }
        }

        for (const Equation &equation : ones) {
            Equation transformed;
            for (const Term &term : equation) {
                if (term.q_index != q_index) { transformed.push_back(term); }
            }
            if (!is_one(transformed)) {
                result.ones.push_back(std::move(transformed));
            }
        }

        for (const Equation &equation : unknown) {
            Equation transformed;
            for (const Term &term : equation) {
                if (term.q_index != q_index) { transformed.push_back(term); }
            }
            if (!is_zero_or_one(transformed)) {
                result.unknown.push_back(std::move(transformed));
            }
        }

        return result;
    }


    constexpr System apply(const ZeroingTransformation &transformation
    ) const noexcept {

        System result;
        result.active_ps = drop_all(active_ps, transformation.zeroed_ps);
        result.active_qs = drop_all(active_qs, transformation.zeroed_qs);

        for (const Term &term : zeros) {
            if (!transformation.is_zeroed(term)) {
                result.zeros.push_back(term);
            }
        }

        for (const Term &term : transformation.zeroed_terms) {
            if (!contains(transformation.zeroed_ps, term.p_index) &&
                !contains(transformation.zeroed_qs, term.q_index)) {
                result.zeros.push_back(term);
            }
        }

        for (const Equation &equation : ones) {
            Equation transformed;
            for (const Term &term : equation) {
                if (!transformation.is_zeroed(term)) {
                    transformed.push_back(term);
                }
            }
            if (!is_one(transformed)) {
                result.ones.push_back(std::move(transformed));
            }
        }

        for (const Equation &equation : unknown) {
            Equation transformed;
            for (const Term &term : equation) {
                if (!transformation.is_zeroed(term)) {
                    transformed.push_back(term);
                }
            }
            if (!is_zero_or_one(transformed)) {
                result.unknown.push_back(std::move(transformed));
            }
        }

        return result;
    }


    constexpr System set_p_one(index_t p_index) const noexcept {

        constexpr index_t ZERO = static_cast<index_t>(0);

        System result;
        result.active_ps = drop(active_ps, p_index);
        result.active_qs = active_qs;

        ZeroingTransformation transformation;
        for (const Term &term : zeros) {
            if (term.p_index == p_index) {
                transformation.set_q_zero(term.q_index);
            } else {
                result.zeros.push_back(term);
            }
        }

        for (const Equation &equation : ones) {
            Equation transformed;
            for (const Term &term : equation) {
                transformed.emplace_back(
                    (term.p_index == p_index) ? ZERO : term.p_index,
                    term.q_index
                );
            }
            if (!is_one(transformed)) {
                result.ones.push_back(std::move(transformed));
            }
        }

        for (const Equation &equation : unknown) {
            Equation transformed;
            for (const Term &term : equation) {
                transformed.emplace_back(
                    (term.p_index == p_index) ? ZERO : term.p_index,
                    term.q_index
                );
            }
            if (!is_zero_or_one(transformed)) {
                result.unknown.push_back(std::move(transformed));
            }
        }

        return result.apply(transformation);
    }


    constexpr System set_q_one(index_t q_index) const noexcept {

        constexpr index_t ZERO = static_cast<index_t>(0);

        System result;
        result.active_ps = active_ps;
        result.active_qs = drop(active_qs, q_index);

        ZeroingTransformation transformation;
        for (const Term &term : zeros) {
            if (term.q_index == q_index) {
                transformation.set_p_zero(term.p_index);
            } else {
                result.zeros.push_back(term);
            }
        }

        for (const Equation &equation : ones) {
            Equation transformed;
            for (const Term &term : equation) {
                transformed.emplace_back(
                    term.p_index,
                    (term.q_index == q_index) ? ZERO : term.q_index
                );
            }
            if (!is_one(transformed)) {
                result.ones.push_back(std::move(transformed));
            }
        }

        for (const Equation &equation : unknown) {
            Equation transformed;
            for (const Term &term : equation) {
                transformed.emplace_back(
                    term.p_index,
                    (term.q_index == q_index) ? ZERO : term.q_index
                );
            }
            if (!is_zero_or_one(transformed)) {
                result.unknown.push_back(std::move(transformed));
            }
        }

        return result.apply(transformation);
    }


}; // struct System


static std::ostream &operator<<(std::ostream &os, const System &system) {

    if (system.zeros.empty() && system.ones.empty() && system.unknown.empty()) {
        std::cerr << "ERROR: Attempted to print empty system." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    bool first = true;
    os << "\\begin{align*} %";
    for (index_t p_index : system.active_ps) {
        os << " p_{" << static_cast<std::intmax_t>(p_index) << '}';
    }
    for (index_t q_index : system.active_qs) {
        os << " q_{" << static_cast<std::intmax_t>(q_index) << '}';
    }
    os << '\n';
    for (const Term &term : system.zeros) {
        if (first) {
            first = false;
            os << "    ";
        } else {
            os << " \\\\\n    ";
        }
        os << term << " &= 0";
    }
    for (const Equation &equation : system.ones) {
        if (first) {
            first = false;
            os << "    ";
        } else {
            os << " \\\\\n    ";
        }
        os << equation << " &= 1";
    }
    for (const Equation &equation : system.unknown) {
        if (first) {
            first = false;
            os << "    ";
        } else {
            os << " \\\\\n    ";
        }
        os << equation << " &= 0 \\text{ or } 1";
    }
    os << "\n\\end{align*}";
    return os;
}


////////////////////////////////////////////////////////////////////////////////


static void
ensure_active(const std::vector<index_t> &active_indices, index_t index) {
    constexpr index_t ZERO = static_cast<index_t>(0);
    if ((index != ZERO) && !contains(active_indices, index)) {
        std::cerr << "ERROR: System contains inactive variable." << std::endl;
        std::exit(EXIT_FAILURE);
    }
}


static void ensure_variable_validity(const System &system) {
    for (const Term &term : system.zeros) {
        ensure_active(system.active_ps, term.p_index);
        ensure_active(system.active_qs, term.q_index);
    }
    for (const Equation &equation : system.ones) {
        for (const Term &term : equation) {
            ensure_active(system.active_ps, term.p_index);
            ensure_active(system.active_qs, term.q_index);
        }
    }
    for (const Equation &equation : system.unknown) {
        for (const Term &term : equation) {
            ensure_active(system.active_ps, term.p_index);
            ensure_active(system.active_qs, term.q_index);
        }
    }
}


////////////////////////////////////////////////////////////////////////////////


static System move_unknown_to_zero(const System &system, std::size_t index) {

    if (index >= system.unknown.size()) {
        std::cerr << "ERROR: Equation to move is out of bounds." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    const Equation &equation = system.unknown[index];
    for (const Term &term : equation) {
        if (term.is_constant()) {
            std::cerr << "ERROR: Equation to move has a constant term."
                      << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }

    ZeroingTransformation transformation;
    transformation.set_zero(equation);
    return system.apply(transformation);
}


static System move_unknown_to_one(const System &system, std::size_t index) {

    if (index >= system.unknown.size()) {
        std::cerr << "ERROR: Equation to move is out of bounds." << std::endl;
        std::exit(EXIT_FAILURE);
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


////////////////////////////////////////////////////////////////////////////////


static System remove_constant_terms(const System &system, bool verbose) {

    ZeroingTransformation transformation;
    std::vector<std::pair<Equation, bool>> used;

    for (const Equation &equation : system.ones) {
        bool found_constant_term = false;
        for (const Term &term : equation) {
            if (term.is_constant()) {
                if (found_constant_term) {
                    std::cerr << "ERROR: Found multiple constant terms"
                                 " in a single equation."
                              << std::endl;
                    std::exit(EXIT_FAILURE);
                } else {
                    found_constant_term = true;
                }
            }
        }
        if (found_constant_term) {
            transformation.set_zero(equation);
            if (verbose) { used.push_back(std::make_pair(equation, false)); }
        }
    }

    for (const Equation &equation : system.unknown) {
        bool found_constant_term = false;
        for (const Term &term : equation) {
            if (term.is_constant()) {
                if (found_constant_term) {
                    std::cerr << "ERROR: Found multiple constant terms"
                                 " in a single equation."
                              << std::endl;
                    std::exit(EXIT_FAILURE);
                } else {
                    found_constant_term = true;
                }
            }
        }
        if (found_constant_term) {
            transformation.set_zero(equation);
            if (verbose) { used.push_back(std::make_pair(equation, true)); }
        }
    }

    if (verbose) {
        if (used.size() == 0) {
            std::cerr << "ERROR: Found no constant terms to remove."
                      << std::endl;
            std::exit(EXIT_FAILURE);
        } else if (used.size() == 1) {
            std::cout << "From the equation $" << used[0].first
                      << (used[0].second ? " = 0 \\text{ or } 1" : " = 1")
                      << "$, we may conclude that $";
        } else if (used.size() == 2) {
            std::cout << "From the equations $" << used[0].first
                      << (used[0].second ? " = 0 \\text{ or } 1" : " = 1")
                      << "$ and $" << used[1].first
                      << (used[1].second ? " = 0 \\text{ or } 1" : " = 1")
                      << "$, we may conclude that $";
        } else {
            std::cout << "From the equations ";
            for (std::size_t k = 0; k < used.size(); ++k) {
                std::cout << '$' << used[k].first
                          << (used[k].second ? " = 0 \\text{ or } 1" : " = 1")
                          << '$';
                if (k + 2 == used.size()) {
                    std::cout << ", and ";
                } else {
                    std::cout << ", ";
                }
            }
            std::cout << "we may conclude that $";
        }
        for (index_t p_index : transformation.zeroed_ps) {
            std::cout << "p_{" << static_cast<std::intmax_t>(p_index) << "} = ";
        }
        for (index_t q_index : transformation.zeroed_qs) {
            std::cout << "q_{" << static_cast<std::intmax_t>(q_index) << "} = ";
        }
        for (const Term &term : transformation.zeroed_terms) {
            std::cout << term << " = ";
        }
        std::cout << "0$.";
    }

    System result = system.apply(transformation);

    if (verbose) {
        if (result.is_empty()) {
            std::cout << " This is the unique solution of"
                         " this system of equations."
                      << std::endl;
        } else {
            std::cout << " This simplifies the preceding system of equations"
                         " to the following:\n"
                      << result << std::endl;
        }
    }

    return result;
}


static std::optional<System> simplify(const System &system, bool verbose) {

    ensure_variable_validity(system);

    if (system.is_empty()) { return std::nullopt; }

    if (!system.is_consistent()) {
        if (verbose) {
            std::cout << "This system of equations is inconsistent"
                         " and has no solutions."
                      << std::endl;
        }
        return std::nullopt;
    }

    if (system.is_trivial()) {
        if (verbose) {
            std::cout << "It is trivial to verify that this system of equations"
                         " only admits $\\{0, 1\\}$-valued solutions."
                      << std::endl;
        }
        return std::nullopt;
    }

    for (const Equation &equation : system.ones) {
        if (equation.size() == 1) {
            const Term term = equation.front();
            if (term.is_quadratic()) {
                const System next =
                    system.set_p_one(term.p_index).set_q_one(term.q_index);
                if (next.is_empty()) {
                    if (verbose) {
                        std::cout << "It is trivial to verify that"
                                     " this system of equations only"
                                     " admits $\\{0, 1\\}$-valued solutions."
                                  << std::endl;
                    }
                    return std::nullopt;
                }
                if (verbose) {
                    std::cout << "From the equation $" << equation
                              << " = 1$, we may conclude that $p_{"
                              << static_cast<std::intmax_t>(term.p_index)
                              << "} = 1$ and $q_{"
                              << static_cast<std::intmax_t>(term.q_index)
                              << "} = 1$. Performing these substitutions"
                                 " yields the following system of equations:\n"
                              << next << std::endl;
                }
                return simplify(next, verbose);
            } else if (term.has_p()) {
                const System next = system.set_p_one(term.p_index);
                if (next.is_empty()) {
                    if (verbose) {
                        std::cout << "It is trivial to verify that"
                                     " this system of equations only"
                                     " admits $\\{0, 1\\}$-valued solutions."
                                  << std::endl;
                    }
                    return std::nullopt;
                }
                if (verbose) {
                    std::cout
                        << "Performing the substitution $p_{"
                        << static_cast<std::intmax_t>(term.p_index)
                        << "} = 1$ yields the following system of equations:\n"
                        << next << std::endl;
                }
                return simplify(next, verbose);
            } else if (term.has_q()) {
                const System next = system.set_q_one(term.q_index);
                if (next.is_empty()) {
                    if (verbose) {
                        std::cout << "It is trivial to verify that"
                                     " this system of equations only"
                                     " admits $\\{0, 1\\}$-valued solutions."
                                  << std::endl;
                    }
                    return std::nullopt;
                }
                if (verbose) {
                    std::cout
                        << "Performing the substitution $q_{"
                        << static_cast<std::intmax_t>(term.q_index)
                        << "} = 1$ yields the following system of equations:\n"
                        << next << std::endl;
                }
                return simplify(next, verbose);
            }
        }
    }

    for (const Equation &equation : system.ones) {
        for (const Term &term : equation) {
            if (term.is_constant()) {
                return simplify(
                    remove_constant_terms(system, verbose), verbose
                );
            }
        }
    }
    for (const Equation &equation : system.unknown) {
        for (const Term &term : equation) {
            if (term.is_constant()) {
                return simplify(
                    remove_constant_terms(system, verbose), verbose
                );
            }
        }
    }

    return system;
}


static std::ostream &
operator<<(std::ostream &os, const std::vector<bool> &case_id) {
    bool first = true;
    for (const bool b : case_id) {
        if (first) {
            first = false;
        } else {
            os << '.';
        }
        os << (b ? '2' : '1');
    }
    return os;
}


static void
analyze(std::vector<bool> &case_id, const System &system, bool verbose) {

    if (verbose && !case_id.empty()) {
        std::cout << "\n\\textbf{Case " << case_id << ":}";
        if (system.is_empty()) {
            std::cout << " This case is trivial." << std::endl;
        } else {
            std::cout << " In this case, we have the following"
                         " system of equations:\n"
                      << system << std::endl;
        }
    }

    std::optional<System> simplified = simplify(system, verbose);

    if (simplified.has_value()) {

        const Term var = simplified->find_unknown_variable();

        if (var.has_p()) {

            if (verbose) {
                std::cout << "We consider two cases based on the equation $p_{"
                          << static_cast<std::intmax_t>(var.p_index)
                          << "} = 0 \\text{ or } 1$, which implies $p_{"
                          << static_cast<std::intmax_t>(var.p_index)
                          << "} = 0$ (Case ";
                case_id.push_back(false);
                std::cout << case_id;
                case_id.pop_back();
                std::cout << ") or $p_{"
                          << static_cast<std::intmax_t>(var.p_index)
                          << "} = 1$ (Case ";
                case_id.push_back(true);
                std::cout << case_id;
                case_id.pop_back();
                std::cout << ")." << std::endl;
            }

            case_id.push_back(false);
            analyze(case_id, simplified->set_p_zero(var.p_index), verbose);
            case_id.pop_back();
            case_id.push_back(true);
            analyze(case_id, simplified->set_p_one(var.p_index), verbose);
            case_id.pop_back();

        } else if (var.has_q()) {

            if (verbose) {
                std::cout << "We consider two cases based on the equation $q_{"
                          << static_cast<std::intmax_t>(var.q_index)
                          << "} = 0 \\text{ or } 1$, which implies $q_{"
                          << static_cast<std::intmax_t>(var.q_index)
                          << "} = 0$ (Case ";
                case_id.push_back(false);
                std::cout << case_id;
                case_id.pop_back();
                std::cout << ") or $q_{"
                          << static_cast<std::intmax_t>(var.q_index)
                          << "} = 1$ (Case ";
                case_id.push_back(true);
                std::cout << case_id;
                case_id.pop_back();
                std::cout << ")." << std::endl;
            }

            case_id.push_back(false);
            analyze(case_id, simplified->set_q_zero(var.q_index), verbose);
            case_id.pop_back();
            case_id.push_back(true);
            analyze(case_id, simplified->set_q_one(var.q_index), verbose);
            case_id.pop_back();

        } else if (!simplified->zeros.empty()) {

            const Term zero_term = simplified->zeros.front();

            if (verbose) {
                std::cout << "We consider two cases based on the equation $"
                          << zero_term << " = 0$, which implies $p_{"
                          << static_cast<std::intmax_t>(zero_term.p_index)
                          << "} = 0$ (Case ";
                case_id.push_back(false);
                std::cout << case_id;
                case_id.pop_back();
                std::cout << ") or $q_{"
                          << static_cast<std::intmax_t>(zero_term.q_index)
                          << "} = 0$ (Case ";
                case_id.push_back(true);
                std::cout << case_id;
                case_id.pop_back();
                std::cout << ")." << std::endl;
            }

            case_id.push_back(false);
            analyze(
                case_id, simplified->set_p_zero(zero_term.p_index), verbose
            );
            case_id.pop_back();
            case_id.push_back(true);
            analyze(
                case_id, simplified->set_q_zero(zero_term.q_index), verbose
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

            if (verbose) {
                std::cout << "We consider two cases based on the equation $"
                          << simplified->unknown[best_index]
                          << " = 0 \\text{ or } 1$, which implies $"
                          << simplified->unknown[best_index] << " = 0$ (Case ";
                case_id.push_back(false);
                std::cout << case_id;
                case_id.pop_back();
                std::cout << ") or $" << simplified->unknown[best_index]
                          << " = 1$ (Case ";
                case_id.push_back(true);
                std::cout << case_id;
                case_id.pop_back();
                std::cout << ")." << std::endl;
            }

            case_id.push_back(false);
            analyze(
                case_id, move_unknown_to_zero(*simplified, best_index), verbose
            );
            case_id.pop_back();
            case_id.push_back(true);
            analyze(
                case_id, move_unknown_to_one(*simplified, best_index), verbose
            );
            case_id.pop_back();

        } else {
            if (verbose) {
                std::cout
                    << "It remains to be shown via a Groebner basis calculation"
                       " that this system of equations has no solutions."
                    << std::endl;
            } else {
                for (const Equation &equation : simplified->ones) {
                    print(equation);
                    std::cout << '\n';
                }
                std::cout << std::endl;
            }
        }
    }
}


int main(int argc, char **argv) {

    if ((argc != 3) && (argc != 4)) {
        std::cerr << "Usage: " << argv[0] << " i j [--print-proof]"
                  << std::endl;
        return EXIT_FAILURE;
    }

    const index_t i = static_cast<index_t>(std::strtoull(argv[1], nullptr, 10));
    const index_t j = static_cast<index_t>(std::strtoull(argv[2], nullptr, 10));
    const bool verbose = (argc == 4);

    System initial_system(i, j);

    if (verbose) {
        std::cout
            << "\\documentclass{article}\n\n"
            << "\\usepackage{amsmath}\n"
            << "\\usepackage[margin=0.5in]{geometry}\n"
            << "\\usepackage{parskip}\n\n"
            << "\\begin{document}\n\n"
            << "\\textbf{Theorem:} The 0--1 Polynomial Conjecture holds when"
               " $(\\deg P, \\deg Q) = ("
            << static_cast<std::intmax_t>(i) << ", "
            << static_cast<std::intmax_t>(j) << ")$.\n"
            << std::endl;
        std::cout << "\\textit{Proof:} Let $P(x) = 1";
        for (index_t k = 1; k < i; ++k) {
            std::cout << " + p_{" << static_cast<std::intmax_t>(k) << "} x^{"
                      << static_cast<std::intmax_t>(k) << '}';
        }
        std::cout << " + x^{" << static_cast<std::intmax_t>(i)
                  << "}$ and $Q(x) = 1";
        for (index_t k = 1; k < j; ++k) {
            std::cout << " + q_{" << static_cast<std::intmax_t>(k) << "} x^{"
                      << static_cast<std::intmax_t>(k) << '}';
        }
        std::cout
            << " + x^{" << static_cast<std::intmax_t>(j)
            << "}$. If $P(x) Q(x)$ is a 0--1 polynomial, then the following"
               " system of equations holds:\n"
            << initial_system
            << "\nWe must show that all nonnegative solutions of this system"
               " of equations are $\\{0, 1\\}$-valued.\n"
            << std::endl;
    }

    std::vector<bool> case_id;
    analyze(case_id, initial_system, verbose);

    if (verbose) { std::cout << "\n\\end{document}" << std::endl; }

    return EXIT_SUCCESS;
}
