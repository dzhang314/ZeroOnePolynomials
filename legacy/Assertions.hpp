#ifndef ZERO_ONE_SOLVER_ASSERTIONS_HPP_INCLUDED
#define ZERO_ONE_SOLVER_ASSERTIONS_HPP_INCLUDED

#include <vector> // for std::vector

#include "System.hpp" // for ZeroOneSolver::System
#include "Term.hpp"   // for ZeroOneSolver::variable_index_t

namespace ZeroOneSolver {


void ensure(bool condition, const char *message);


void prevent(bool condition, const char *message);


void ensure_active(
    const std::vector<variable_index_t> &active_indices, variable_index_t index
);


void ensure_variable_validity(const System &system);


} // namespace ZeroOneSolver

#endif // ZERO_ONE_SOLVER_ASSERTIONS_HPP_INCLUDED
