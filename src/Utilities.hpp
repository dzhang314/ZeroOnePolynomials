#ifndef ZERO_ONE_SOLVER_UTILITIES_HPP_INCLUDED
#define ZERO_ONE_SOLVER_UTILITIES_HPP_INCLUDED

#include <algorithm> // for std::sort, std::unique
#include <vector>    // for std::vector

namespace ZeroOneSolver {


template <typename T>
constexpr bool
contains(const std::vector<T> &collection, const T &item) noexcept {
    for (const T &element : collection) {
        if (element == item) { return true; }
    }
    return false;
}


template <typename T>
constexpr std::vector<T>
drop(const std::vector<T> &collection, const T &item) noexcept {
    std::vector<T> result;
    for (const T &element : collection) {
        if (!(element == item)) { result.push_back(element); }
    }
    return result;
}


template <typename T>
constexpr std::vector<T> drop_all(
    const std::vector<T> &collection, const std::vector<T> &items
) noexcept {
    std::vector<T> result;
    for (const T &element : collection) {
        if (!contains(items, element)) { result.push_back(element); }
    }
    return result;
}


template <typename T>
void sort_unique(std::vector<T> &collection) {
    std::sort(collection.begin(), collection.end());
    collection.erase(
        std::unique(collection.begin(), collection.end()), collection.end()
    );
}


} // namespace ZeroOneSolver

#endif // ZERO_ONE_SOLVER_UTILITIES_HPP_INCLUDED
