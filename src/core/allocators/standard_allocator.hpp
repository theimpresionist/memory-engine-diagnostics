/**
 * @file standard_allocator.hpp
 * @brief Standard allocator using new/delete
 * @author Bambang Hutagalung
 * @date 2026
 */

#ifndef STANDARD_ALLOCATOR_HPP
#define STANDARD_ALLOCATOR_HPP

#include "base_allocator.hpp"
#include "../utils/timer.hpp"
#include <unordered_map>
#include <new>

namespace memory_engine {

/**
 * @class StandardAllocator
 * @brief Wrapper around standard new/delete operators
 * 
 * This allocator serves as a baseline for comparing custom allocator
 * performance. It uses the standard C++ memory allocation functions
 * with added tracking and statistics.
 */
class StandardAllocator : public BaseAllocator {
public:
    /**
     * @brief Constructor
     */
    StandardAllocator() 
        : BaseAllocator("Standard (new/delete)", SIZE_MAX) {}

    /**
     * @brief Destructor - deallocates all tracked memory
     */
    ~StandardAllocator() override {
        reset();
    }

    /**
     * @brief Allocate memory using aligned new
     * @param size Size in bytes
     * @param alignment Alignment requirement
     * @return Pointer to allocated memory
     */
    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) override {
        if (size == 0) return nullptr;
        if (!is_power_of_two(alignment)) {
            alignment = alignof(std::max_align_t);
        }

        Timer timer;
        timer.start();

        void* ptr = nullptr;
        
        #if __cpp_aligned_new >= 201606L
        // Use aligned new if available (C++17)
        if (alignment > alignof(std::max_align_t)) {
            ptr = ::operator new(size, std::align_val_t(alignment), std::nothrow);
        } else {
            ptr = ::operator new(size, std::nothrow);
        }
        #else
        // Fallback for older standards
        ptr = ::operator new(size, std::nothrow);
        #endif

        timer.stop();

        if (ptr) {
            m_allocations[ptr] = {size, alignment};
            record_allocation(ptr, size, alignment, timer.elapsed_ns());
        }

        return ptr;
    }

    /**
     * @brief Deallocate memory
     * @param ptr Pointer to deallocate
     */
    void deallocate(void* ptr) override {
        if (!ptr) return;

        auto it = m_allocations.find(ptr);
        if (it == m_allocations.end()) return; // Not our pointer

        Timer timer;
        timer.start();

        size_t size = it->second.first;
        size_t alignment = it->second.second;

        #if __cpp_aligned_new >= 201606L
        if (alignment > alignof(std::max_align_t)) {
            ::operator delete(ptr, std::align_val_t(alignment));
        } else {
            ::operator delete(ptr);
        }
        #else
        ::operator delete(ptr);
        #endif

        timer.stop();

        m_allocations.erase(it);
        record_deallocation(size, timer.elapsed_ns());
    }

    /**
     * @brief Reset - deallocate all tracked memory
     */
    void reset() override {
        for (auto& [ptr, info] : m_allocations) {
            #if __cpp_aligned_new >= 201606L
            if (info.second > alignof(std::max_align_t)) {
                ::operator delete(ptr, std::align_val_t(info.second));
            } else {
                ::operator delete(ptr);
            }
            #else
            ::operator delete(ptr);
            #endif
        }
        m_allocations.clear();
        m_stats = AllocationStats{};
        m_allocation_history.clear();
    }

    /**
     * @brief Check if pointer was allocated by this allocator
     * @param ptr Pointer to check
     * @return true if tracked by this allocator
     */
    bool owns(void* ptr) const override {
        return m_allocations.find(ptr) != m_allocations.end();
    }

    /**
     * @brief Get available memory (effectively unlimited for standard allocator)
     * @return SIZE_MAX
     */
    size_t available() const override {
        return SIZE_MAX; // Effectively unlimited
    }

private:
    /// Map of allocated pointers to {size, alignment}
    std::unordered_map<void*, std::pair<size_t, size_t>> m_allocations;
};

} // namespace memory_engine

#endif // STANDARD_ALLOCATOR_HPP
