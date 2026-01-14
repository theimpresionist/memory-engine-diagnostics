/**
 * @file memory_utils.hpp
 * @brief Memory utility functions
 */

#ifndef MEMORY_UTILS_HPP
#define MEMORY_UTILS_HPP

#include <cstddef>
#include <cstdint>

namespace memory_engine {

class MemoryUtils {
public:
    static constexpr size_t align_forward(size_t address, size_t alignment) {
        return (address + alignment - 1) & ~(alignment - 1);
    }

    static constexpr bool is_power_of_two(size_t value) {
        return value > 0 && (value & (value - 1)) == 0;
    }

    static constexpr size_t next_power_of_two(size_t value) {
        value--;
        value |= value >> 1;
        value |= value >> 2;
        value |= value >> 4;
        value |= value >> 8;
        value |= value >> 16;
        value |= value >> 32;
        return value + 1;
    }

    static size_t get_page_size() {
        #ifdef _WIN32
        return 4096;
        #else
        return 4096;
        #endif
    }

    static constexpr size_t KB(size_t n) { return n * 1024; }
    static constexpr size_t MB(size_t n) { return n * 1024 * 1024; }
    static constexpr size_t GB(size_t n) { return n * 1024 * 1024 * 1024; }
};

} // namespace memory_engine

#endif // MEMORY_UTILS_HPP
