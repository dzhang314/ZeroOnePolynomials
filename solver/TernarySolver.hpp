/******************************************************************************
 * Copyright (c) 2023-2026 David K. Zhang
 * This file is part of ZeroOnePolynomials by David K. Zhang.
 * ZeroOnePolynomials is distributed under the terms of the MIT license.
 * See the LICENSE file for full license details.
 ******************************************************************************/

#ifndef TERNARY_SOLVER_HPP_INCLUDED
#define TERNARY_SOLVER_HPP_INCLUDED

#include <cassert> // for assert
#include <cstddef> // for std::size_t
#include <cstdint> // for std::uint8_t

#include "TwoBitPackedArray.hpp"

namespace TernarySolver {


/******************************************************************************
 * TernarySolver is a high-performance C++20 program that analyzes polynomials
 * over a three-valued abstract domain {0, 1, *}, where * represents values in
 * the open unit interval (0, 1). TernarySolver determines the conditions under
 * which the product of two such polynomials can be a 0-1 polynomial, i.e.,
 * have {0, 1}-valued coefficients. This approach to the 0-1 Polynomial
 * Conjecture was introduced by Kevin G. Hare in the following paper:
 *
 * "Computational Progress on the Unfair 0-1 Polynomial Conjecture" (July 2025)
 * Experimental Mathematics, 1-9. https://doi.org/10.1080/10586458.2025.2527786
 ******************************************************************************/


enum class TernaryValue : std::uint8_t {
    ZERO = 0x00,
    ONE = 0x01,
    STAR = 0x02,
}; // enum class TernaryValue


constexpr char to_char(TernaryValue value) noexcept {
    switch (value) {
        case TernaryValue::ZERO: return '0';
        case TernaryValue::ONE: return '1';
        case TernaryValue::STAR: return '*';
    }
}


constexpr TernaryValue operator*(TernaryValue lhs, TernaryValue rhs) noexcept {
    if ((lhs == TernaryValue::ZERO) || (rhs == TernaryValue::ZERO)) {
        return TernaryValue::ZERO;
    }
    if ((lhs == TernaryValue::ONE) && (rhs == TernaryValue::ONE)) {
        return TernaryValue::ONE;
    }
    return TernaryValue::STAR;
}


enum class SumValue : std::uint8_t {
    ZERO = 0x00, // Default value; must be zero.
    ONE = 0x01,
    SINGLE_STAR = 0x02,
    MULTIPLE_STARS = 0x03,
    INVALID = 0x04,
}; // enum class SumValue


constexpr SumValue operator+(SumValue lhs, TernaryValue rhs) noexcept {
    switch (rhs) {
        case TernaryValue::ZERO: return lhs;
        case TernaryValue::ONE:
            return lhs == SumValue::ZERO ? SumValue::ONE : SumValue::INVALID;
        case TernaryValue::STAR:
            switch (lhs) {
                case SumValue::ZERO: return SumValue::SINGLE_STAR;
                case SumValue::ONE: return SumValue::INVALID;
                case SumValue::SINGLE_STAR: return SumValue::MULTIPLE_STARS;
                case SumValue::MULTIPLE_STARS: return SumValue::MULTIPLE_STARS;
                case SumValue::INVALID: return SumValue::INVALID;
            }
    }
}


template <std::size_t M>
class PShape {

    TwoBitPackedArray<TernaryValue, M - 1> data;

public:

    constexpr TernaryValue get_coefficient(std::size_t index) const noexcept {
        assert(index <= M);
        if ((index == 0) || (index == M)) { return TernaryValue::ONE; }
        return data.get(index - 1);
    }

    constexpr void set_coefficient(std::size_t index,
                                   TernaryValue value) noexcept {
        assert(index > 0);
        assert(index < M);
        data.set(index - 1, value);
    }

    constexpr bool has_star() const noexcept {
        for (std::size_t i = 0; i < M - 1; ++i) {
            if (data.get(i) == TernaryValue::STAR) { return true; }
        }
        return false;
    }

}; // class PShape<M>


template <std::size_t M, std::size_t N>
class QShape {

    TwoBitPackedArray<TernaryValue, N> data;
    TwoBitPackedArray<SumValue, M> partial_sums;

public:

    constexpr TernaryValue get_coefficient(std::size_t index) const noexcept {
        assert(index <= N);
        if (index == 0) { return TernaryValue::ONE; }
        return data.get(index - 1);
    }

    constexpr void set_coefficient(std::size_t index,
                                   TernaryValue value) noexcept {
        assert(index > 0);
        assert(index <= N);
        data.set(index - 1, value);
    }

    constexpr SumValue get_partial_sum(std::size_t index) const noexcept {
        return partial_sums.get(index % M);
    }

    constexpr void set_partial_sum(std::size_t index, SumValue value) noexcept {
        assert(value != SumValue::INVALID);
        partial_sums.set(index % M, value);
    }

}; // class QShape<M, N>


} // namespace TernarySolver

#endif // TERNARY_SOLVER_HPP_INCLUDED
