#ifndef TWO_BIT_PACKED_ARRAY_HPP_INCLUDED
#define TWO_BIT_PACKED_ARRAY_HPP_INCLUDED

#include <cassert> // for assert
#include <cstddef> // for std::byte, std::size_t


/******************************************************************************
 * A `TwoBitPackedArray<T, N>` is an array of `N` elements of type `T` in which
 * each element is represented by two bits and four elements are packed into a
 * single `std::byte`. The type `T` is expected to be `static_cast`-able to and
 * from `std::byte`, e.g., an `enum class` with values 0, 1, 2, and 3.
 ******************************************************************************/
template <typename T, std::size_t N>
class TwoBitPackedArray {

    static constexpr std::byte MASK = static_cast<std::byte>(0x03);

    std::byte data[(N + 3) >> 2] = {}; // Zero-initialize data.

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

}; // class TwoBitPackedArray<T, N>


#endif // TWO_BIT_PACKED_ARRAY_HPP_INCLUDED
