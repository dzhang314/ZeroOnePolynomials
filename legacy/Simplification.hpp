#ifndef ZERO_ONE_SOLVER_SIMPLIFICATION_HPP_INCLUDED
#define ZERO_ONE_SOLVER_SIMPLIFICATION_HPP_INCLUDED

#include <optional> // for std::optional

#include "System.hpp" // for ZeroOneSolver::System

namespace ZeroOneSolver {


template <bool verbose, bool paranoid>
std::optional<System> simplify(const System &system);


// clang-format off
extern template std::optional<System> simplify<false, false>(const System &system);
extern template std::optional<System> simplify<false, true>(const System &system);
extern template std::optional<System> simplify<true, false>(const System &system);
extern template std::optional<System> simplify<true, true>(const System &system);
// clang-format on


} // namespace ZeroOneSolver

#endif // ZERO_ONE_SOLVER_SIMPLIFICATION_HPP_INCLUDED
