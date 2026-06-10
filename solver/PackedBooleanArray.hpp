#ifndef PACKED_BOOLEAN_ARRAY_HPP_INCLUDED
#define PACKED_BOOLEAN_ARRAY_HPP_INCLUDED

#include <cassert> // for assert
#include <cstddef> // for std::byte, std::size_t


/******************************************************************************
 * A `PackedBooleanArray<N>` is an array of `N` Boolean values in which eight
 * elements are packed into a single `std::byte`.
 ******************************************************************************/
template <std::size_t N>
class PackedBooleanArray {

    static constexpr std::byte MASK = static_cast<std::byte>(0x01);

    std::byte data[(N + 7) >> 3] = {}; // Zero-initialize data.

public:

    constexpr bool get(std::size_t index) const noexcept {
        assert(index < N);
        const std::size_t byte_index = index >> 3;
        const std::byte byte = data[byte_index];
        const int shift = static_cast<int>(index & 0x07);
        return ((byte >> shift) & MASK) != static_cast<std::byte>(0x00);
    }

    constexpr void set(std::size_t index) noexcept {
        assert(index < N);
        const std::size_t byte_index = index >> 3;
        const int shift = static_cast<int>(index & 0x07);
        data[byte_index] |= (MASK << shift);
    }

}; // class PackedBooleanArray<N>


#endif // PACKED_BOOLEAN_ARRAY_HPP_INCLUDED
