/******************************************************************************
 * Copyright (c) 2023-2026 David K. Zhang
 * This file is part of ZeroOnePolynomials by David K. Zhang.
 * ZeroOnePolynomials is distributed under the terms of the MIT license.
 * See the LICENSE file for full license details.
 ******************************************************************************/

#ifndef TERNARY_SOLVER_HPP_INCLUDED
#define TERNARY_SOLVER_HPP_INCLUDED

#include <cstdint> // for std::uint8_t

namespace TernarySolver {


enum class Value : std::uint8_t {
    ZERO = 0x00,
    ONE = 0x01,
    STAR = 0x02,
}; // enum class Value


constexpr char to_char(Value value) noexcept {
    switch (value) {
        case Value::ZERO: return '0';
        case Value::ONE: return '1';
        case Value::STAR: return '*';
    }
}


constexpr Value operator*(Value lhs, Value rhs) noexcept {
    if ((lhs == Value::ZERO) || (rhs == Value::ZERO)) { return Value::ZERO; }
    if ((lhs == Value::ONE) && (rhs == Value::ONE)) { return Value::ONE; }
    return Value::STAR;
}


enum class SumValue : std::uint8_t {
    ZERO = 0x00,
    ONE = 0x01,
    SINGLE_STAR = 0x02,
    MULTIPLE_STARS = 0x03,
    INVALID = 0x04,
}; // enum class SumValue


constexpr SumValue operator+(SumValue lhs, Value rhs) noexcept {
    switch (rhs) {
        case Value::ZERO: return lhs;
        case Value::ONE:
            return lhs == SumValue::ZERO ? SumValue::ONE : SumValue::INVALID;
        case Value::STAR:
            switch (lhs) {
                case SumValue::ZERO: return SumValue::SINGLE_STAR;
                case SumValue::ONE: return SumValue::INVALID;
                case SumValue::SINGLE_STAR: return SumValue::MULTIPLE_STARS;
                case SumValue::MULTIPLE_STARS: return SumValue::MULTIPLE_STARS;
                case SumValue::INVALID: return SumValue::INVALID;
            }
    }
}


} // namespace TernarySolver

#endif // TERNARY_SOLVER_HPP_INCLUDED
