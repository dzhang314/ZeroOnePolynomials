#include <cstdint> // for std::intNN_t, std::intmax_t
#include <cstdlib> // for std::size_t, std::strtoull, EXIT_SUCCESS, EXIT_FAILURE
#include <filesystem> // for std::filesystem::rename
#include <fstream>    // for std::ofstream
#include <iostream>   // for std::ostream, std::cout, std::cerr, std::endl
#include <optional>   // for std::optional, std::make_optional, std::nullopt
#include <string>     // for std::string
#include <utility>    // for std::move
#include <vector>     // for std::vector


using index_t = std::uint8_t;


////////////////////////////////////////////////////////////////////////////////


struct Term {


    index_t p_index;
    index_t q_index;


    explicit constexpr Term(index_t p, index_t q) noexcept
        : p_index(p), q_index(q) {}


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


static std::ostream &operator<<(std::ostream &os, const Term &term) {
    if (term.is_quadratic()) {
        os << "p_" << static_cast<std::intmax_t>(term.p_index) << " * "
           << "q_" << static_cast<std::intmax_t>(term.q_index);
    } else if (term.has_p()) {
        os << "p_" << static_cast<std::intmax_t>(term.p_index);
    } else if (term.has_q()) {
        os << "q_" << static_cast<std::intmax_t>(term.q_index);
    } else {
        os << '1';
    }
    return os;
}


////////////////////////////////////////////////////////////////////////////////


struct Equation {


    std::vector<Term> terms;


    explicit constexpr Equation() noexcept : terms() {}


}; // struct Equation


static std::ostream &operator<<(std::ostream &os, const Equation &equation) {
    bool first = true;
    for (const Term &term : equation.terms) {
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


constexpr std::vector<Equation>
initial_equations(index_t p_degree, index_t q_degree) noexcept {

    constexpr index_t ZERO = static_cast<index_t>(0);
    constexpr index_t ONE = static_cast<index_t>(1);
    using vec_size_t = typename std::vector<Equation>::size_type;

    std::vector<Equation> result(
        static_cast<vec_size_t>(p_degree + q_degree - 1)
    );

    for (index_t p = ZERO; p <= p_degree; ++p) {
        for (index_t q = ZERO; q <= q_degree; ++q) {
            if ((p == ZERO) && (q == ZERO)) {
                continue; // omit constant term of product polynomial
            }
            if ((p == p_degree) && (q == q_degree)) {
                continue; // omit leading term of product polynomial
            }
            result[static_cast<vec_size_t>(p + q - ONE)].terms.emplace_back(
                (p == p_degree) ? ZERO : p, (q == q_degree) ? ZERO : q
            );
        }
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////


constexpr bool
contains(const std::vector<index_t> &indices, index_t index) noexcept {
    for (const index_t element : indices) {
        if (element == index) {
            return true;
        }
    }
    return false;
}


constexpr bool
contains(const std::vector<Term> &terms, const Term &term) noexcept {
    for (const Term &element : terms) {
        if (element == term) {
            return true;
        }
    }
    return false;
}


struct ZeroingTransformation {


    std::vector<index_t> zeroed_ps;
    std::vector<index_t> zeroed_qs;
    std::vector<Term> zeroed_terms;


    explicit constexpr ZeroingTransformation() noexcept
        : zeroed_ps(), zeroed_qs(), zeroed_terms() {}


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


struct System {


    std::vector<Term> zeros;
    std::vector<Equation> ones;
    std::vector<Equation> unknown;


    // Invariant: Every term in zeros is quadratic.
    // Invariant: No equation in ones or unknown contains a constant term.


    explicit constexpr System() noexcept : zeros(), ones(), unknown() {}


    explicit constexpr System(index_t p_degree, index_t q_degree) noexcept
        : zeros(), ones(), unknown(initial_equations(p_degree, q_degree)) {}


    constexpr bool empty() const noexcept {
        return zeros.empty() && ones.empty() && unknown.empty();
    }


    std::optional<System> set_p_zero(index_t p_index) const noexcept {

        System result;

        for (const Term &term : zeros) {
            if (term.p_index != p_index) {
                result.zeros.push_back(term);
            }
        }

        for (const Equation &equation : ones) {
            Equation transformed;
            bool is_constant = true;
            for (const Term &term : equation.terms) {
                if (term.p_index != p_index) {
                    transformed.terms.push_back(term);
                    if (!term.is_constant()) {
                        is_constant = false;
                    }
                }
            }
            if (is_constant) {
                if (transformed.terms.size() != 1) {
                    return std::nullopt;
                }
            } else {
                result.ones.push_back(std::move(transformed));
            }
        }

        for (const Equation &equation : unknown) {
            Equation transformed;
            bool is_constant = true;
            for (const Term &term : equation.terms) {
                if (term.p_index != p_index) {
                    transformed.terms.push_back(term);
                    if (!term.is_constant()) {
                        is_constant = false;
                    }
                }
            }
            if (is_constant) {
                if (transformed.terms.size() > 1) {
                    return std::nullopt;
                }
            } else {
                result.unknown.push_back(std::move(transformed));
            }
        }

        return std::make_optional(std::move(result));
    }


    std::optional<System> set_q_zero(index_t q_index) const noexcept {

        System result;

        for (const Term &term : zeros) {
            if (term.q_index != q_index) {
                result.zeros.push_back(term);
            }
        }

        for (const Equation &equation : ones) {
            Equation transformed;
            bool is_constant = true;
            for (const Term &term : equation.terms) {
                if (term.q_index != q_index) {
                    transformed.terms.push_back(term);
                    if (!term.is_constant()) {
                        is_constant = false;
                    }
                }
            }
            if (is_constant) {
                if (transformed.terms.size() != 1) {
                    return std::nullopt;
                }
            } else {
                result.ones.push_back(std::move(transformed));
            }
        }

        for (const Equation &equation : unknown) {
            Equation transformed;
            bool is_constant = true;
            for (const Term &term : equation.terms) {
                if (term.q_index != q_index) {
                    transformed.terms.push_back(term);
                    if (!term.is_constant()) {
                        is_constant = false;
                    }
                }
            }
            if (is_constant) {
                if (transformed.terms.size() > 1) {
                    return std::nullopt;
                }
            } else {
                result.unknown.push_back(std::move(transformed));
            }
        }

        return std::make_optional(std::move(result));
    }


    std::optional<System> apply(const ZeroingTransformation &transformation
    ) const noexcept {

        System result;

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
            bool is_constant = true;
            for (const Term &term : equation.terms) {
                if (!transformation.is_zeroed(term)) {
                    transformed.terms.push_back(term);
                    if (!term.is_constant()) {
                        is_constant = false;
                    }
                }
            }
            if (is_constant) {
                if (transformed.terms.size() != 1) {
                    return std::nullopt;
                }
            } else {
                result.ones.push_back(std::move(transformed));
            }
        }

        for (const Equation &equation : unknown) {
            Equation transformed;
            bool is_constant = true;
            for (const Term &term : equation.terms) {
                if (!transformation.is_zeroed(term)) {
                    transformed.terms.push_back(term);
                    if (!term.is_constant()) {
                        is_constant = false;
                    }
                }
            }
            if (is_constant) {
                if (transformed.terms.size() > 1) {
                    return std::nullopt;
                }
            } else {
                result.unknown.push_back(std::move(transformed));
            }
        }

        return std::make_optional(std::move(result));
    }


    std::optional<System> set_p_one(index_t p_index) const noexcept {

        constexpr index_t ZERO = static_cast<index_t>(0);
        System result;
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
            for (const Term &term : equation.terms) {
                transformed.terms.emplace_back(
                    (term.p_index == p_index) ? ZERO : term.p_index,
                    term.q_index
                );
            }
            result.ones.push_back(std::move(transformed));
        }

        for (const Equation &equation : unknown) {
            Equation transformed;
            for (const Term &term : equation.terms) {
                transformed.terms.emplace_back(
                    (term.p_index == p_index) ? ZERO : term.p_index,
                    term.q_index
                );
            }
            result.unknown.push_back(std::move(transformed));
        }

        return result.apply(transformation);
    }


    std::optional<System> set_q_one(index_t q_index) const noexcept {

        constexpr index_t ZERO = static_cast<index_t>(0);
        System result;
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
            for (const Term &term : equation.terms) {
                transformed.terms.emplace_back(
                    term.p_index,
                    (term.q_index == q_index) ? ZERO : term.q_index
                );
            }
            result.ones.push_back(std::move(transformed));
        }

        for (const Equation &equation : unknown) {
            Equation transformed;
            for (const Term &term : equation.terms) {
                transformed.terms.emplace_back(
                    term.p_index,
                    (term.q_index == q_index) ? ZERO : term.q_index
                );
            }
            result.unknown.push_back(std::move(transformed));
        }

        return result.apply(transformation);
    }


    std::optional<System> remove_constant_terms() const noexcept {

        ZeroingTransformation transformation;

        for (const Equation &equation : ones) {
            bool found_constant_term = false;
            for (const Term &term : equation.terms) {
                if (term.is_constant()) {
                    if (found_constant_term) {
                        return std::nullopt;
                    } else {
                        found_constant_term = true;
                    }
                }
            }
            if (found_constant_term) {
                transformation.set_zero(equation.terms);
            }
        }

        for (const Equation &equation : unknown) {
            bool found_constant_term = false;
            for (const Term &term : equation.terms) {
                if (term.is_constant()) {
                    if (found_constant_term) {
                        return std::nullopt;
                    } else {
                        found_constant_term = true;
                    }
                }
            }
            if (found_constant_term) {
                transformation.set_zero(equation.terms);
            }
        }

        return apply(transformation);
    }


    std::optional<System> simplify() const noexcept {
        for (const Equation &equation : ones) {
            if (equation.terms.size() == 1) {
                const Term &term = equation.terms.front();
                if (term.has_p()) {
                    std::optional<System> next = set_p_one(term.p_index);
                    return next.has_value() ? next->simplify() : std::nullopt;
                } else if (term.has_q()) {
                    std::optional<System> next = set_q_one(term.q_index);
                    return next.has_value() ? next->simplify() : std::nullopt;
                }
            }
        }
        for (const Equation &equation : ones) {
            for (const Term &term : equation.terms) {
                if (term.is_constant()) {
                    std::optional<System> next = remove_constant_terms();
                    return next.has_value() ? next->simplify() : std::nullopt;
                }
            }
        }
        for (const Equation &equation : unknown) {
            for (const Term &term : equation.terms) {
                if (term.is_constant()) {
                    std::optional<System> next = remove_constant_terms();
                    return next.has_value() ? next->simplify() : std::nullopt;
                }
            }
        }
        return std::make_optional(*this);
    }


    std::optional<System> move_unknown_to_zero(std::size_t index
    ) const noexcept {

        const Equation &equation = unknown[index];
        for (const Term &term : equation.terms) {
            if (term.is_constant()) {
                return std::nullopt;
            }
        }

        ZeroingTransformation transformation;
        transformation.set_zero(equation.terms);
        return apply(transformation);
    }


    std::optional<System> move_unknown_to_one(std::size_t index
    ) const noexcept {

        System result;

        result.zeros = zeros;

        result.ones = ones;
        result.ones.push_back(unknown[index]);

        for (std::size_t i = 0; i < unknown.size(); ++i) {
            if (i != index) {
                result.unknown.push_back(unknown[i]);
            }
        }

        return std::make_optional(std::move(result));
    }


}; // struct System


////////////////////////////////////////////////////////////////////////////////


static void
push(std::vector<System> &stack, const std::optional<System> &item) noexcept {
    if (item.has_value() && !item->empty()) {
        std::optional<System> simplified = item->simplify();
        if (simplified.has_value() && !simplified->empty()) {
            stack.push_back(std::move(*simplified));
        }
    }
}


int main(int argc, char **argv) {

    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " i j filename" << std::endl;
        return EXIT_FAILURE;
    }

    const index_t i = static_cast<index_t>(std::strtoull(argv[1], nullptr, 10));
    const index_t j = static_cast<index_t>(std::strtoull(argv[2], nullptr, 10));
    const std::string file_name(argv[3]);
    const std::string temp_name = file_name + ".temp";

    {
        std::ofstream output_file(temp_name);
        std::vector<System> stack;
        push(stack, System(i, j));
        while (!stack.empty()) {
            const System current = std::move(stack.back());
            stack.pop_back();
            if (!current.zeros.empty()) {
                const Term zero_term = current.zeros.front();
                push(stack, current.set_p_zero(zero_term.p_index));
                push(stack, current.set_q_zero(zero_term.q_index));
            } else if (!current.unknown.empty()) {
                push(stack, current.move_unknown_to_zero(0));
                push(stack, current.move_unknown_to_one(0));
            } else {
                for (const Equation &equation : current.ones) {
                    output_file << equation << '\n';
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
