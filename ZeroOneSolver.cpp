#include <algorithm> // for std::sort, std::unique
#include <cstdint>   // for std::intNN_t, std::intmax_t
#include <cstdlib>   // for std::size_t, std::strtoull, std::exit
#include <iostream>  // for std::ostream, std::cout, std::cerr, std::endl
#include <optional>  // for std::optional, std::make_optional, std::nullopt
#include <string>    // for std::string
#include <utility>   // for std::pair, std::make_pair, std::move
#include <vector>    // for std::vector


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


using index_t = std::int16_t;


////////////////////////////////////////////////////////////////////////////////


/**
 * A Term represents a monomial of the form 1, p_i, q_j, or p_i * q_j.
 *
 * The presence of each variable in a given Term is indicated by a nonzero
 * value of the corresponding index. Thus:
 *     Term(0, 0) represents 1.
 *     Term(i, 0) for i != 0 represents p_i.
 *     Term(0, j) for j != 0 represents q_j.
 *     Term(i, j) for i != 0 and j != 0 represents p_i * q_j.
 *
 * The variables p_0 and q_0 cannot be represented in a Term, so all subscripts
 * should always be assumed to start from 1.
 */
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

    void print_plain_text(std::ostream &os) const {
        if (is_quadratic()) {
            os << "p_" << static_cast<std::intmax_t>(p_index) << " * q_"
               << static_cast<std::intmax_t>(q_index);
        } else if (has_p()) {
            os << "p_" << static_cast<std::intmax_t>(p_index);
        } else if (has_q()) {
            os << "q_" << static_cast<std::intmax_t>(q_index);
        } else {
            os << '1';
        }
    }

    void print_latex(std::ostream &os) const {
        if (is_quadratic()) {
            os << "p_{" << static_cast<std::intmax_t>(p_index) << "} q_{"
               << static_cast<std::intmax_t>(q_index) << '}';
        } else if (has_p()) {
            os << "p_{" << static_cast<std::intmax_t>(p_index) << '}';
        } else if (has_q()) {
            os << "q_{" << static_cast<std::intmax_t>(q_index) << '}';
        } else {
            os << '1';
        }
    }

    void print_wolfram(std::ostream &os) const {
        if (is_quadratic()) {
            os << "p[" << static_cast<std::intmax_t>(p_index) << "] q["
               << static_cast<std::intmax_t>(q_index) << ']';
        } else if (has_p()) {
            os << "p[" << static_cast<std::intmax_t>(p_index) << ']';
        } else if (has_q()) {
            os << "q[" << static_cast<std::intmax_t>(q_index) << ']';
        } else {
            os << '1';
        }
    }

}; // struct Term


////////////////////////////////////////////////////////////////////////////////


/**
 * A Polynomial is a list of Terms (t_1, ..., t_n) that represents the sum
 * t_1 + ... + t_n. The ordering of Terms within a Polynomial is arbitrary.
 */
struct Polynomial : public std::vector<Term> {

    constexpr bool is_zero() const noexcept { return (size() == 0); }

    constexpr bool is_one() const noexcept {
        return (size() == 1) && front().is_constant();
    }

    constexpr bool is_zero_or_one() const noexcept {
        return is_zero() || is_one();
    }

    void print_plain_text(std::ostream &os) const {
        bool first = true;
        for (const Term &term : *this) {
            if (first) {
                first = false;
            } else {
                os << " + ";
            }
            term.print_plain_text(os);
        }
    }

    void print_latex(std::ostream &os) const {
        bool first = true;
        for (const Term &term : *this) {
            if (first) {
                first = false;
            } else {
                os << " + ";
            }
            term.print_latex(os);
        }
    }

    void print_wolfram(std::ostream &os) const {
        bool first = true;
        for (const Term &term : *this) {
            if (first) {
                first = false;
            } else {
                os << " + ";
            }
            term.print_wolfram(os);
        }
    }

}; // struct Polynomial


////////////////////////////////////////////////////////////////////////////////


template <typename T>
constexpr bool contains(const std::vector<T> &items, const T &item) noexcept {
    for (const T &element : items) {
        if (element == item) { return true; }
    }
    return false;
}


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
constexpr std::vector<T>
drop_all(const std::vector<T> &items, const std::vector<T> &to_drop) noexcept {
    std::vector<T> result;
    for (const T &element : items) {
        if (!contains(to_drop, element)) { result.push_back(element); }
    }
    return result;
}


////////////////////////////////////////////////////////////////////////////////


/**
 * A ZeroSubstitution is a list of variables {p_i}, {q_j} and terms {t_k}.
 *
 * ZeroSubstitutions are intended to be used in the following situation:
 * Suppose we are solving a system of equations, and we learn that
 *     t_1 + ... + t_n = 0.
 * Since each term t_k assumes values in [0, 1], we may conclude that
 *     t_1 = ... = t_n = 0.
 * This allows us to simplify the remaining equations in the system by
 * removing all other occurrences of the terms (t_1, ..., t_n).
 */
struct ZeroSubstitution {

    std::vector<index_t> zeroed_ps;
    std::vector<index_t> zeroed_qs;
    std::vector<Term> zeroed_terms;

    explicit constexpr ZeroSubstitution() noexcept
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
        } // Note: set_zero(1) is a no-op by design.
    }

    constexpr void set_zero(const Polynomial &poly) noexcept {
        for (const Term &term : poly) {
            set_zero(term);
        }
    }

    constexpr bool is_zeroed(const Term &term) const noexcept {
        return contains(zeroed_ps, term.p_index) ||
               contains(zeroed_qs, term.q_index) ||
               contains(zeroed_terms, term);
    }

}; // struct ZeroSubstitution


////////////////////////////////////////////////////////////////////////////////


struct System {

    std::vector<index_t> active_ps;
    std::vector<index_t> active_qs;
    std::vector<Term> zeros;
    std::vector<Polynomial> ones;
    std::vector<Polynomial> unknown;

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
    constexpr bool has_bounds_inconsistency() const noexcept {
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
        std::vector<index_t> solved_ps;
        std::vector<index_t> solved_qs;
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
    constexpr System set_p_zero(index_t p_index) const noexcept {
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
    constexpr System set_q_zero(index_t q_index) const noexcept {
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

    constexpr System set_p_one(index_t p_index) const noexcept {
        constexpr index_t ZERO = static_cast<index_t>(0);
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

    constexpr System set_q_one(index_t q_index) const noexcept {
        constexpr index_t ZERO = static_cast<index_t>(0);
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

    constexpr Term find_unknown_variable() const noexcept {
        constexpr index_t ZERO = static_cast<index_t>(0);
        for (const Polynomial &poly : unknown) {
            if (poly.size() == 1) {
                const Term term = poly.front();
                if (term.is_linear()) { return term; }
            }
        }
        return Term(ZERO, ZERO);
    }

    void print_latex(std::ostream &os) const {
        os << "\\begin{align*} %";
        for (index_t p_index : active_ps) {
            os << " p_{" << static_cast<std::intmax_t>(p_index) << '}';
        }
        for (index_t q_index : active_qs) {
            os << " q_{" << static_cast<std::intmax_t>(q_index) << '}';
        }
        bool first = true;
        for (const Term &term : zeros) {
            if (first) {
                first = false;
                os << "\n    ";
            } else {
                os << " \\\\\n    ";
            }
            term.print_latex(os);
            os << " &= 0";
        }
        for (const Polynomial &poly : ones) {
            if (first) {
                first = false;
                os << "\n    ";
            } else {
                os << " \\\\\n    ";
            }
            poly.print_latex(os);
            os << " &= 1";
        }
        for (const Polynomial &poly : unknown) {
            if (first) {
                first = false;
                os << "\n    ";
            } else {
                os << " \\\\\n    ";
            }
            poly.print_latex(os);
            os << " &= 0 \\text{ or } 1";
        }
        os << "\n\\end{align*}";
    }

}; // struct System


////////////////////////////////////////////////////////////////////////////////


static void ensure(bool condition, const char *message) {
    if (!condition) {
        std::cerr << message << std::endl;
        std::exit(EXIT_FAILURE);
    }
}


static void prevent(bool condition, const char *message) {
    if (condition) {
        std::cerr << message << std::endl;
        std::exit(EXIT_FAILURE);
    }
}


static void
ensure_active(const std::vector<index_t> &active_indices, index_t index) {
    constexpr index_t ZERO = static_cast<index_t>(0);
    ensure(
        (index == ZERO) || contains(active_indices, index),
        "ERROR: System contains inactive variable."
    );
}


static void ensure_variable_validity(const System &system) {
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


static System move_unknown_to_zero(const System &system, std::size_t index) {
    ensure(
        index < system.unknown.size(),
        "ERROR: Polynomial to move is out of bounds."
    );
    const Polynomial &equation = system.unknown[index];
    for (const Term &term : equation) {
        prevent(
            term.is_constant(), "ERROR: Polynomial to move has a constant term."
        );
    }
    ZeroSubstitution transformation;
    transformation.set_zero(equation);
    return system.apply(transformation);
}


static System move_unknown_to_one(const System &system, std::size_t index) {
    ensure(
        index < system.unknown.size(),
        "ERROR: Polynomial to move is out of bounds."
    );
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


template <typename T>
void sort_unique(std::vector<T> &v) {
    std::sort(v.begin(), v.end());
    v.erase(std::unique(v.begin(), v.end()), v.end());
}


static System remove_constant_terms(const System &system, bool verbose) {

    ZeroSubstitution transformation;
    std::vector<std::pair<Polynomial, bool>> used;

    for (const Polynomial &equation : system.ones) {
        bool found_constant_term = false;
        for (const Term &term : equation) {
            if (term.is_constant()) {
                prevent(
                    found_constant_term,
                    "ERROR: Found multiple constant terms in a single equation."
                );
                found_constant_term = true;
            }
        }
        if (found_constant_term) {
            transformation.set_zero(equation);
            if (verbose) { used.push_back(std::make_pair(equation, false)); }
        }
    }

    for (const Polynomial &equation : system.unknown) {
        bool found_constant_term = false;
        for (const Term &term : equation) {
            if (term.is_constant()) {
                prevent(
                    found_constant_term,
                    "ERROR: Found multiple constant terms in a single equation."
                );
                found_constant_term = true;
            }
        }
        if (found_constant_term) {
            transformation.set_zero(equation);
            if (verbose) { used.push_back(std::make_pair(equation, true)); }
        }
    }

    System result = system.apply(transformation);

    if (verbose) {
        prevent(used.size() == 0, "ERROR: Found no constant terms to remove.");
        if (used.size() == 1) {
            std::cout << "From the equation $";
            used[0].first.print_latex(std::cout);
            std::cout << (used[0].second ? " = 0 \\text{ or } 1" : " = 1")
                      << "$, we may conclude that $";
        } else if (used.size() == 2) {
            std::cout << "From the equations $";
            used[0].first.print_latex(std::cout);
            std::cout << (used[0].second ? " = 0 \\text{ or } 1" : " = 1")
                      << "$ and $";
            used[1].first.print_latex(std::cout);
            std::cout << (used[1].second ? " = 0 \\text{ or } 1" : " = 1")
                      << "$, we may conclude that $";
        } else {
            std::cout << "From the equations ";
            for (std::size_t k = 0; k < used.size(); ++k) {
                std::cout << '$';
                used[k].first.print_latex(std::cout);
                std::cout << (used[k].second ? " = 0 \\text{ or } 1" : " = 1")
                          << ((k + 2 == used.size()) ? "$, and " : "$, ");
            }
            std::cout << "we may conclude that $";
        }
        sort_unique(transformation.zeroed_ps);
        for (index_t p_index : transformation.zeroed_ps) {
            std::cout << "p_{" << static_cast<std::intmax_t>(p_index) << "} = ";
        }
        sort_unique(transformation.zeroed_qs);
        for (index_t q_index : transformation.zeroed_qs) {
            std::cout << "q_{" << static_cast<std::intmax_t>(q_index) << "} = ";
        }
        if (!result.is_empty()) {
            for (const Term &term : transformation.zeroed_terms) {
                term.print_latex(std::cout);
                std::cout << " = ";
            }
        }
        std::cout << "0$.";
    }

    if (verbose) {
        if (result.is_empty()) {
            std::cout << " This is the unique solution of"
                         " this system of equations."
                      << std::endl;
        } else {
            std::cout << " This simplifies the preceding system of equations"
                         " to the following:\n";
            result.print_latex(std::cout);
            std::cout << std::endl;
        }
    }

    return result;
}


static std::optional<System> simplify(const System &system, bool verbose) {

    ensure_variable_validity(system);

    if (system.is_empty()) { return std::nullopt; }

    if (system.has_bounds_inconsistency()) {
        if (verbose) {
            std::cout << "This system of equations is inconsistent"
                         " and has no solutions."
                      << std::endl;
        }
        return std::nullopt;
    }

    if (system.is_solved()) {
        if (verbose) {
            std::cout << "Every variable in this system of equations is"
                         " directly constrained to values in $\\{0, 1\\}$."
                      << std::endl;
        }
        return std::nullopt;
    }

    for (const Polynomial &equation : system.ones) {
        if (equation.size() == 1) {
            const Term term = equation.front();
            if (term.is_quadratic()) {
                if (verbose) {
                    std::cout << "From the equation $";
                    equation.print_latex(std::cout);
                    std::cout << " = 1$, we may conclude that $p_{"
                              << static_cast<std::intmax_t>(term.p_index)
                              << "} = 1$ and $q_{"
                              << static_cast<std::intmax_t>(term.q_index)
                              << "} = 1$.";
                }
                const System next =
                    system.set_p_one(term.p_index).set_q_one(term.q_index);
                if (next.is_empty()) {
                    if (verbose) {
                        std::cout << " After performing these substitutions,"
                                     " it is straightforward to verify that"
                                     " the resulting system of equations only"
                                     " admits $\\{0, 1\\}$-valued solutions."
                                  << std::endl;
                    }
                    return std::nullopt;
                } else {
                    if (verbose) {
                        std::cout << " Performing these substitutions yields"
                                     " the following system of equations:\n";
                        next.print_latex(std::cout);
                        std::cout << std::endl;
                    }
                    return simplify(next, verbose);
                }
            } else if (term.has_p()) {
                const System next = system.set_p_one(term.p_index);
                if (next.is_empty()) {
                    if (verbose) {
                        std::cout << "After performing the substitution $p_{"
                                  << static_cast<std::intmax_t>(term.p_index)
                                  << "} = 1$, it is straightforward to verify"
                                     " that the resulting system of equations"
                                     " only admits $\\{0, 1\\}$-valued"
                                     " solutions."
                                  << std::endl;
                    }
                    return std::nullopt;
                } else {
                    if (verbose) {
                        std::cout << "Performing the substitution $p_{"
                                  << static_cast<std::intmax_t>(term.p_index)
                                  << "} = 1$ yields the following system of "
                                     "equations:\n";
                        next.print_latex(std::cout);
                        std::cout << std::endl;
                    }
                    return simplify(next, verbose);
                }
            } else if (term.has_q()) {
                const System next = system.set_q_one(term.q_index);
                if (next.is_empty()) {
                    if (verbose) {
                        std::cout << "After performing the substitution $q_{"
                                  << static_cast<std::intmax_t>(term.q_index)
                                  << "} = 1$, it is straightforward to verify"
                                     " that the resulting system of equations"
                                     " only admits $\\{0, 1\\}$-valued"
                                     " solutions."
                                  << std::endl;
                    }
                    return std::nullopt;
                } else {
                    if (verbose) {
                        std::cout << "Performing the substitution $q_{"
                                  << static_cast<std::intmax_t>(term.q_index)
                                  << "} = 1$ yields the following system of "
                                     "equations:\n";
                        next.print_latex(std::cout);
                        std::cout << std::endl;
                    }
                    return simplify(next, verbose);
                }
            }
        }
    }

    for (const Polynomial &equation : system.ones) {
        for (const Term &term : equation) {
            if (term.is_constant()) {
                return simplify(
                    remove_constant_terms(system, verbose), verbose
                );
            }
        }
    }
    for (const Polynomial &equation : system.unknown) {
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
print_case_id(const std::vector<bool> &case_id, std::ostream &os) {
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
        std::cout << "\n\\textbf{Case ";
        print_case_id(case_id, std::cout);
        std::cout << ":}";
        if (system.is_empty()) {
            std::cout << " This case is trivial." << std::endl;
        } else {
            std::cout << " In this case, we have the following"
                         " system of equations:\n";
            system.print_latex(std::cout);
            std::cout << std::endl;
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
                print_case_id(case_id, std::cout);
                case_id.pop_back();
                std::cout << ") or $p_{"
                          << static_cast<std::intmax_t>(var.p_index)
                          << "} = 1$ (Case ";
                case_id.push_back(true);
                print_case_id(case_id, std::cout);
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
                print_case_id(case_id, std::cout);
                case_id.pop_back();
                std::cout << ") or $q_{"
                          << static_cast<std::intmax_t>(var.q_index)
                          << "} = 1$ (Case ";
                case_id.push_back(true);
                print_case_id(case_id, std::cout);
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
                std::cout << "We consider two cases based on the equation $";
                zero_term.print_latex(std::cout);
                std::cout << " = 0$, which implies $p_{"
                          << static_cast<std::intmax_t>(zero_term.p_index)
                          << "} = 0$ (Case ";
                case_id.push_back(false);
                print_case_id(case_id, std::cout);
                case_id.pop_back();
                std::cout << ") or $q_{"
                          << static_cast<std::intmax_t>(zero_term.q_index)
                          << "} = 0$ (Case ";
                case_id.push_back(true);
                print_case_id(case_id, std::cout);
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
                std::cout << "We consider two cases based on the equation $";
                simplified->unknown[best_index].print_latex(std::cout);
                std::cout << " = 0 \\text{ or } 1$, which implies $";
                simplified->unknown[best_index].print_latex(std::cout);
                std::cout << " = 0$ (Case ";
                case_id.push_back(false);
                print_case_id(case_id, std::cout);
                case_id.pop_back();
                std::cout << ") or $";
                simplified->unknown[best_index].print_latex(std::cout);
                std::cout << " = 1$ (Case ";
                case_id.push_back(true);
                print_case_id(case_id, std::cout);
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
                for (const Polynomial &equation : simplified->ones) {
                    equation.print_plain_text(std::cout);
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
            << "\\usepackage[margin=0.5in, includefoot]{geometry}\n"
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
        std::cout << " + x^{" << static_cast<std::intmax_t>(j)
                  << "}$. If $P(x) Q(x)$ is a 0--1 polynomial, then the"
                     " following system of equations holds:\n";
        initial_system.print_latex(std::cout);
        std::cout << "\nWe must show that all nonnegative solutions of this"
                     " system of equations are $\\{0, 1\\}$-valued.\n"
                  << std::endl;
    }

    std::vector<bool> case_id;
    analyze(case_id, initial_system, verbose);

    if (verbose) { std::cout << "\n\\end{document}" << std::endl; }

    return EXIT_SUCCESS;
}
