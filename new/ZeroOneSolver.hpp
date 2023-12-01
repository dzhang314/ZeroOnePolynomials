#ifndef ZERO_ONE_SOLVER_HPP_INCLUDED
#define ZERO_ONE_SOLVER_HPP_INCLUDED

#include <bitset>  // for std::bitset
#include <cassert> // for assert
#include <cstddef> // for std::byte, std::size_t
#include <cstdint> // for std::uint8_t
#include <ostream> // for std::ostream

namespace ZeroOneSolver {


using var_index_t = std::uint8_t;


struct Term {

    var_index_t p_index;
    var_index_t q_index;

    constexpr Term() noexcept
        : p_index(0)
        , q_index(0) {}

    constexpr Term(int p, int q) noexcept
        : p_index(static_cast<var_index_t>(p))
        , q_index(static_cast<var_index_t>(q)) {
        assert((0 <= p) && (p <= 255));
        assert((0 <= q) && (q <= 255));
    }

    constexpr bool operator==(const Term &) const noexcept = default;
    constexpr bool operator!=(const Term &) const noexcept = default;

}; // struct Term


constexpr Term TERM_ZERO = {0xFF, 0xFF};
constexpr Term TERM_ONE = {0x00, 0x00};


inline std::ostream &operator<<(std::ostream &os, const Term &term) {
    if (term == TERM_ZERO) {
        os << "0";
    } else {
        if (term.p_index) {
            os << "p" << static_cast<int>(term.p_index);
            if (term.q_index) { os << "*q" << static_cast<int>(term.q_index); }
        } else {
            if (term.q_index) {
                os << "q" << static_cast<int>(term.q_index);
            } else {
                os << "1";
            }
        }
    }
    return os;
}


template <typename T, std::size_t N>
class TwoBitPackedArray {

    static constexpr std::byte MASK = static_cast<std::byte>(0x03);

    std::byte data[(N + 3) >> 2];

public:

    constexpr T get(std::size_t index) const noexcept {
        assert(index < N);
        const std::size_t byte_index = index >> 2;
        const std::byte byte = data[byte_index];
        const int shift = static_cast<int>(index & 0x03) << 1;
        return static_cast<T>((byte & (MASK << shift)) >> shift);
    }

    constexpr void set(std::size_t index, T item) noexcept {
        assert(index < N);
        const std::size_t byte_index = index >> 2;
        const std::byte byte = data[byte_index];
        const int shift = static_cast<int>(index & 0x03) << 1;
        const std::byte new_byte = static_cast<std::byte>(item) << shift;
        data[byte_index] = (byte & ~(MASK << shift)) | new_byte;
    }

}; // class TwoBitPackedArray<N>


enum class RHS : std::uint8_t {
    ZERO_OR_ONE = 0x00,
    ZERO = 0x01,
    ONE = 0x02,
}; // enum class RHS


enum class VAR : std::uint8_t {
    UNKNOWN = 0x00,
    ZERO_OR_ONE = 0x01,
    ZERO = 0x02,
    ONE = 0x03,
}; // enum class VAR


template <var_index_t M, var_index_t N>
struct System {


    static_assert((0 < M) && (M < N));


    Term lhs[M + N - 1][M + 1];
    TwoBitPackedArray<RHS, M + N - 1> rhs;
    TwoBitPackedArray<VAR, M - 1> p;
    TwoBitPackedArray<VAR, N - 1> q;


    constexpr System() noexcept {
        for (int d = 1; d <= M + N - 1; ++d) {
            int t = 0;
            if (d < M) {
                for (int j = 1; j <= d - 1; ++j) {
                    lhs[d - 1][t++] = {j, d - j};
                }
                lhs[d - 1][t++] = {d, 0};
                lhs[d - 1][t++] = {0, d};
            } else if (d == M) {
                for (int j = 1; j <= M - 1; ++j) {
                    lhs[d - 1][t++] = {j, d - j};
                }
                lhs[d - 1][t++] = {0, M};
                lhs[d - 1][t++] = TERM_ONE;
            } else if (d < N) {
                for (int j = 1; j <= M - 1; ++j) {
                    lhs[d - 1][t++] = {j, d - j};
                }
                lhs[d - 1][t++] = {0, d - M};
                lhs[d - 1][t++] = {0, d};
            } else if (d == N) {
                for (int j = 1; j <= M - 1; ++j) {
                    lhs[d - 1][t++] = {j, d - j};
                }
                lhs[d - 1][t++] = {0, N - M};
                lhs[d - 1][t++] = TERM_ONE;
            } else {
                const int offset = d - N;
                for (int j = offset + 1; j <= M - 1; ++j) {
                    lhs[d - 1][t++] = {j, d - j};
                }
                lhs[d - 1][t++] = {d - N, 0};
                lhs[d - 1][t++] = {0, d - M};
            }
            while (t < M + 1) { lhs[d - 1][t++] = TERM_ZERO; }
        }
        for (std::size_t i = 0; i < M + N - 1; ++i) {
            rhs.set(i, RHS::ZERO_OR_ONE);
        }
        for (std::size_t i = 0; i < M - 1; ++i) { p.set(i, VAR::UNKNOWN); }
        for (std::size_t i = 0; i < N - 1; ++i) { q.set(i, VAR::UNKNOWN); }
    }


    friend std::ostream &operator<<(std::ostream &os, const System &system) {
        std::bitset<M - 1> p_used;
        std::bitset<N - 1> q_used;
        for (std::size_t e = 0; e < M + N - 1; ++e) {
            bool first = true;
            for (std::size_t t = 0; t < M + 1; ++t) {
                const Term term = system.lhs[e][t];
                if (term == TERM_ZERO) { continue; }
                if (term.p_index) { p_used.set(term.p_index - 1); }
                if (term.q_index) { q_used.set(term.q_index - 1); }
                if (first) {
                    first = false;
                } else {
                    os << " + ";
                }
                os << term;
            }
            // Skip printing equations of the form 0 == 0.
            if (first && (system.rhs.get(e) == RHS::ZERO)) { continue; }
            switch (system.rhs.get(e)) {
                case RHS::ZERO_OR_ONE: os << " == 0 or 1\n"; break;
                case RHS::ZERO: os << " == 0\n"; break;
                case RHS::ONE: os << " == 1\n"; break;
            }
        }
        for (std::size_t i = 0; i < M - 1; ++i) {
            const VAR value = system.p.get(i);
            if ((value == VAR::ZERO) || (value == VAR::ONE)) {
                assert(!p_used.test(i));
            } else if (value == VAR::ZERO_OR_ONE) {
                os << "p" << (i + 1) << " == 0 or 1\n";
            } else {
                os << "0 <= p" << (i + 1) << " <= 1\n";
            }
        }
        for (std::size_t i = 0; i < N - 1; ++i) {
            const VAR value = system.q.get(i);
            if ((value == VAR::ZERO) || (value == VAR::ONE)) {
                assert(!q_used.test(i));
            } else if (value == VAR::ZERO_OR_ONE) {
                os << "q" << (i + 1) << " == 0 or 1\n";
            } else {
                os << "0 <= q" << (i + 1) << " <= 1\n";
            }
        }
        return os;
    }


    constexpr void set_p_zero(var_index_t p_index) noexcept {
        assert((1 <= p_index) && (p_index <= M - 1));
        assert(p.get(p_index - 1) != VAR::ONE);
        p.set(p_index - 1, VAR::ZERO);
        for (std::size_t e = 0; e < M + N - 1; ++e) {
            for (std::size_t t = 0; t < M + 1; ++t) {
                if (lhs[e][t].p_index == p_index) { lhs[e][t] = TERM_ZERO; }
            }
        }
    }


    constexpr void set_q_zero(var_index_t q_index) noexcept {
        assert((1 <= q_index) && (q_index <= N - 1));
        assert(q.get(q_index - 1) != VAR::ONE);
        q.set(q_index - 1, VAR::ZERO);
        for (std::size_t e = 0; e < M + N - 1; ++e) {
            for (std::size_t t = 0; t < M + 1; ++t) {
                if (lhs[e][t].q_index == q_index) { lhs[e][t] = TERM_ZERO; }
            }
        }
    }


    constexpr void set_p_one(var_index_t p_index) noexcept {
        assert((1 <= p_index) && (p_index <= M - 1));
        assert(p.get(p_index - 1) != VAR::ZERO);
        p.set(p_index - 1, VAR::ONE);
        for (std::size_t e = 0; e < M + N - 1; ++e) {
            for (std::size_t t = 0; t < M + 1; ++t) {
                if (lhs[e][t].p_index == p_index) { lhs[e][t].p_index = 0; }
            }
        }
    }


    constexpr void set_q_one(var_index_t q_index) noexcept {
        assert((1 <= q_index) && (q_index <= N - 1));
        assert(q.get(q_index - 1) != VAR::ZERO);
        q.set(q_index - 1, VAR::ONE);
        for (std::size_t e = 0; e < M + N - 1; ++e) {
            for (std::size_t t = 0; t < M + 1; ++t) {
                if (lhs[e][t].q_index == q_index) { lhs[e][t].q_index = 0; }
            }
        }
    }


    constexpr bool set_p_zero_or_one(var_index_t p_index) noexcept {
        assert((1 <= p_index) && (p_index <= M - 1));
        if (p.get(p_index - 1) == VAR::UNKNOWN) {
            p.set(p_index - 1, VAR::ZERO_OR_ONE);
            return true;
        }
        return false;
    }


    constexpr bool set_q_zero_or_one(var_index_t q_index) noexcept {
        assert((1 <= q_index) && (q_index <= N - 1));
        if (q.get(q_index - 1) == VAR::UNKNOWN) {
            q.set(q_index - 1, VAR::ZERO_OR_ONE);
            return true;
        }
        return false;
    }


    constexpr void set_case(const std::bitset<M - 1> &case_index) noexcept {
        set_q_zero(M);
        set_q_zero(N - M);
        for (var_index_t i = 1; i <= M - 1; ++i) {
            if (case_index[i - 1]) {
                set_q_zero(M - i);
                set_q_zero(N - i);
            } else {
                set_p_zero(i);
            }
        }
    }


    constexpr bool is_unknown(const Term &term) const noexcept {
        if (term == TERM_ZERO) { return false; }
        if (term.p_index) {
            if (p.get(term.p_index - 1) == VAR::UNKNOWN) { return true; }
        }
        if (term.q_index) {
            if (q.get(term.q_index - 1) == VAR::UNKNOWN) { return true; }
        }
        return false;
    }


    constexpr bool has_unknown_variable() const noexcept {
        for (std::size_t i = 0; i < M - 1; ++i) {
            if (p.get(i) == VAR::UNKNOWN) { return true; }
        }
        for (std::size_t i = 0; i < N - 1; ++i) {
            if (q.get(i) == VAR::UNKNOWN) { return true; }
        }
        return false;
    }


    constexpr bool simplify() noexcept {

        constexpr std::size_t INVALID_INDEX = ~static_cast<std::size_t>(0);

        // Phase 1: Simplify right-hand sides.
        for (std::size_t e = 0; e < M + N - 1; ++e) {
            // Scan each equation to look for nonzero terms and the term 1,
            // keeping track of the index at which 1 occurs.
            bool found_nonzero = false;
            std::size_t one_index = INVALID_INDEX;
            for (std::size_t t = 0; t < M + 1; ++t) {
                const Term term = lhs[e][t];
                if (term != TERM_ZERO) { found_nonzero = true; }
                if (term == TERM_ONE) {
                    // An equation with multiple copies of 1
                    // on its left-hand side is unsatisfiable.
                    if (one_index != INVALID_INDEX) { return false; }
                    one_index = t;
                }
            }
            if (!found_nonzero) {
                // An equation of the form 0 == 1 is unsatisfiable.
                if (rhs.get(e) == RHS::ONE) { return false; }
                // If an equation has no nonzero terms on its left-hand
                // side, then we set its right-hand side to zero.
                rhs.set(e, RHS::ZERO);
            }
            if (one_index != INVALID_INDEX) {
                // An equation of the form ... + 1 + ... == 0 is unsatisfiable.
                if (rhs.get(e) == RHS::ZERO) { return false; }
                // If an equation has 1 on its left-hand side, then we subtract
                // 1 from both sides, setting the right-hand side to zero.
                lhs[e][one_index] = TERM_ZERO;
                rhs.set(e, RHS::ZERO);
            }
        }
        // After the end of Phase 1, we may assume that
        // the term 1 does not appear in any equation.

        // Phase 2: Use right-hand sides to directly solve for variables.
        for (std::size_t e = 0; e < M + N - 1; ++e) {
            const RHS rhs_value = rhs.get(e);
            if (rhs_value == RHS::ZERO) {
                // If an equation has the form ... + p_i + ... == 0,
                // then we may conclude that p_i == 0. The same holds
                // for equations of the form ... + q_i + ... == 0.
                for (std::size_t t = 0; t < M + 1; ++t) {
                    const Term term = lhs[e][t];
                    if (term.q_index == 0) {
                        set_p_zero(term.p_index);
                        return simplify();
                    } else if (term.p_index == 0) {
                        set_q_zero(term.q_index);
                        return simplify();
                    }
                }
            } else if (rhs_value == RHS::ONE) {
                // If an equation has the form p_i == 1, then we can set p_i to
                // 1 in all remaining equations. The same holds for equations
                // of the form q_j == 1, and in fact, for equations of the form
                // p_i * q_j == 1.
                std::size_t term_index = INVALID_INDEX;
                for (std::size_t t = 0; t < M + 1; ++t) {
                    if (lhs[e][t] != TERM_ZERO) {
                        if (term_index != INVALID_INDEX) {
                            term_index = INVALID_INDEX;
                            break;
                        } else {
                            term_index = t;
                        }
                    }
                }
                if (term_index != INVALID_INDEX) {
                    const Term term = lhs[e][term_index];
                    assert(term != TERM_ZERO);
                    if (term.p_index) { set_p_one(term.p_index); }
                    if (term.q_index) { set_q_one(term.q_index); }
                    return simplify();
                }
            }
        }

        // Phase 3: Eliminate unknown variables using all-but-one principle.
        bool made_changes = false;
        for (std::size_t e = 0; e < M + N - 1; ++e) {
            // If an equation has the form t_1 + t_2 + ... + t_k == 0 or 1
            // and all but one of the terms t_i are already known to be
            // 0 or 1, then the remaining term must also be 0 or 1.
            std::size_t unknown_index = INVALID_INDEX;
            for (std::size_t t = 0; t < M + 1; ++t) {
                if (is_unknown(lhs[e][t])) {
                    if (unknown_index != INVALID_INDEX) {
                        unknown_index = INVALID_INDEX;
                        break;
                    } else {
                        unknown_index = t;
                    }
                }
            }
            if (unknown_index != INVALID_INDEX) {
                const Term term = lhs[e][unknown_index];
                if (term.q_index == 0) {
                    made_changes |= set_p_zero_or_one(term.p_index);
                } else if (term.p_index == 0) {
                    made_changes |= set_q_zero_or_one(term.q_index);
                }
            }
        }
        if (!has_unknown_variable()) { return true; }
        if (made_changes) { return simplify(); }

        // Phase 4: Eliminate unknown variables in subsystems of the form:
        //     a + b == 0 or 1
        //     a * b == 0 or 1
        Term lone_quadratic_terms[M + N - 1];
        for (std::size_t e = 0; e < M + N - 1; ++e) {
            std::size_t term_index = INVALID_INDEX;
            for (std::size_t t = 0; t < M + 1; ++t) {
                const Term term = lhs[e][t];
                if (!is_zero_or_one(term)) {
                    if (term_index != INVALID_INDEX) {
                        term_index = INVALID_INDEX;
                        break;
                    } else {
                        term_index = t;
                    }
                }
            }
            if (term_index != INVALID_INDEX) {
                const Term term = lhs[e][term_index];
                assert(term != TERM_ZERO);
                assert(term.p_index);
                assert(term.q_index);
                lone_quadratic_terms[e] = term;
            } else {
                lone_quadratic_terms[e] = TERM_ONE;
            }
        }
        for (std::size_t e = 0; e < M + N - 1; ++e) {
            std::size_t first_index = INVALID_INDEX;
            std::size_t second_index = INVALID_INDEX;
            for (std::size_t t = 0; t < M + 1; ++t) {
                const Term term = lhs[e][t];
                if (term != TERM_ZERO) {
                    if (first_index != INVALID_INDEX) {
                        if (second_index != INVALID_INDEX) {
                            second_index = INVALID_INDEX;
                            break;
                        } else {
                            second_index = t;
                        }
                    } else {
                        first_index = t;
                    }
                }
            }
            if (second_index != INVALID_INDEX) {
                const Term x = lhs[e][first_index];
                const Term y = lhs[e][second_index];
                if ((x.q_index == 0) && (y.p_index == 0)) {
                    assert(x.p_index);
                    assert(y.q_index);
                    const Term target = {x.p_index, y.q_index};
                    for (std::size_t t = 0; t < M + N - 1; ++t) {
                        if (lone_quadratic_terms[t] == target) {
                            made_changes |= set_p_zero_or_one(x.p_index);
                            made_changes |= set_q_zero_or_one(y.q_index);
                            break;
                        }
                    }
                } else if ((x.p_index == 0) && (y.q_index == 0)) {
                    assert(x.q_index);
                    assert(y.p_index);
                    const Term target = {y.p_index, x.q_index};
                    for (std::size_t t = 0; t < M + N - 1; ++t) {
                        if (lone_quadratic_terms[t] == target) {
                            made_changes |= set_p_zero_or_one(y.p_index);
                            made_changes |= set_q_zero_or_one(x.q_index);
                            break;
                        }
                    }
                }
            }
        }
        if (!has_unknown_variable()) { return true; }
        if (made_changes) { return simplify(); }

        // At this point, no further simplification is possible.
        return true;
    }


}; // struct System<M, N>


} // namespace ZeroOneSolver

#endif // ZERO_ONE_SOLVER_HPP_INCLUDED
