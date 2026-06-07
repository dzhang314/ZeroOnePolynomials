/******************************************************************************
 * Copyright (c) 2023-2026 David K. Zhang
 * This file is part of ZeroOnePolynomials by David K. Zhang.
 * ZeroOnePolynomials is distributed under the terms of the MIT license.
 * See the LICENSE file for full license details.
 ******************************************************************************/

#include <cassert>
#include <cstddef>
#include <iostream>

#include "TernarySolver.hpp"

using TernarySolver::PShape;
using TernarySolver::QShape;
using TernarySolver::SumValue;
using TernarySolver::TernaryValue;
using TernarySolver::to_char;


#ifdef TERNARY_SOLVER_M
static_assert(TERNARY_SOLVER_M > 1);
static_assert(TERNARY_SOLVER_M < 256);
constexpr std::size_t M = TERNARY_SOLVER_M;
#else
#error "TERNARY_SOLVER_M must be defined."
#endif


#ifdef TERNARY_SOLVER_N
static_assert(TERNARY_SOLVER_N > 0);
static_assert(TERNARY_SOLVER_N < 256);
constexpr std::size_t N = TERNARY_SOLVER_N;
#else
#error "TERNARY_SOLVER_N must be defined."
#endif


template <std::size_t K>
constexpr bool extend(const PShape<M> &p_shape, QShape<M, N> &q_shape,
                      TernaryValue q_value) noexcept {
    if constexpr (K > 0) { q_shape.set_coefficient(K, q_value); }
    const SumValue final_sum = q_shape.get_partial_sum(K) + q_value;
    if ((final_sum == SumValue::SINGLE_STAR) ||
        (final_sum == SumValue::INVALID)) {
        return false;
    }
    for (std::size_t i = 1; i < M; ++i) {
        const SumValue partial_sum = q_shape.get_partial_sum(K + i) +
                                     p_shape.get_coefficient(i) * q_value;
        if (partial_sum == SumValue::INVALID) { return false; }
        q_shape.set_partial_sum(K + i, partial_sum);
    }
    const SumValue initial_sum = SumValue::ZERO + q_value;
    assert(initial_sum != SumValue::INVALID);
    q_shape.set_partial_sum(K, initial_sum);
    return true;
}


static void print_shape_pair(const PShape<M> &p_shape,
                             const QShape<M, N> &q_shape) {
    for (std::size_t i = 0; i <= M; ++i) {
        std::cout << to_char(p_shape.get_coefficient(i));
    }
    std::cout << ' ';
    for (std::size_t i = 0; i <= N; ++i) {
        std::cout << to_char(q_shape.get_coefficient(i));
    }
    std::cout << '\n';
}


template <std::size_t K>
void search_extensions(const PShape<M> &p_shape, const QShape<M, N> &q_shape) {
    if constexpr (K > N) {
        print_shape_pair(p_shape, q_shape);
    } else {
        QShape<M, N> q0 = q_shape;
        if (extend<K>(p_shape, q0, TernaryValue::ZERO)) {
            search_extensions<K + 1>(p_shape, q0);
        }
        QShape<M, N> q1 = q_shape;
        if (extend<K>(p_shape, q1, TernaryValue::ONE)) {
            search_extensions<K + 1>(p_shape, q1);
        }
        QShape<M, N> q_star = q_shape;
        if (extend<K>(p_shape, q_star, TernaryValue::STAR)) {
            search_extensions<K + 1>(p_shape, q_star);
        }
    }
}


static void search_q_shapes(const PShape<M> &p_shape) {
    QShape<M, N> q_shape;
    const bool valid = extend<0>(p_shape, q_shape, TernaryValue::ONE);
    assert(valid);
    search_extensions<1>(p_shape, q_shape);
}


template <std::size_t K>
void search_p_shapes(PShape<M> &p_shape) {
    if constexpr (K == M) {
        if (p_shape.has_star()) { search_q_shapes(p_shape); }
    } else {
        p_shape.set_coefficient(K, TernaryValue::ZERO);
        search_p_shapes<K + 1>(p_shape);
        p_shape.set_coefficient(K, TernaryValue::ONE);
        search_p_shapes<K + 1>(p_shape);
        p_shape.set_coefficient(K, TernaryValue::STAR);
        search_p_shapes<K + 1>(p_shape);
    }
}


int main() {
    PShape<M> p_shape;
    search_p_shapes<1>(p_shape);
}
