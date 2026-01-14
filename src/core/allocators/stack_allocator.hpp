/**
 * @file stack_allocator.hpp
 * @brief LIFO stack-based allocator
 * @author Bambang Hutagalung
 * @date 2026
 */

#ifndef STACK_ALLOCATOR_HPP
#define STACK_ALLOCATOR_HPP

#include "base_allocator.hpp"
#include "../utils/timer.hpp"
#include <cstdlib>
#include <cassert>

namespace memory_engine {

/**
 * @class StackAllocator
 * @brief LIFO (Last-In-First-Out) memory allocator
 * 
 * The stack allocator allocates memory linearly from a pre-allocated buffer.
 * Deallocations must occur in reverse order of allocations (LIFO).
 * This is ideal for temporary allocations where scope-based lifetime is known.
 * 
 * Advantages:
 * - Extremely fast allocation (O(1) - just pointer increment)
 * - No fragmentation when used correctly
 * - Simple and cache-efficient
 * 
 * Disadvantages:
 * - Must deallocate in reverse order
 * - Cannot deallocate arbitrary blocks
 * - Fixed total size
 */
class StackAllocator : public BaseAllocator {
public:
    /**
     * @brief Allocation marker for batch deallocation
     */
    using Marker = size_t;

    /**
     * @brief Constructor
     * @param size Total size of the stack in bytes
     * @param alignment Default alignment for allocations
     */
    explicit StackAllocator(size_t size, size_t alignment = alignof(std::max_align_t))
        : BaseAllocator("Stack Allocator", size)
        , m_alignment(alignment)
        , m_memory(nullptr)
        , m_current_offset(0)
        , m_previous_offset(0)
    {
        // Allocate aligned memory
        #ifdef _WIN32
        m_memory = static_cast<uint8_t*>(_aligned_malloc(size, m_alignment));
        #else
        m_memory = static_cast<uint8_t*>(std::aligned_alloc(m_alignment, size));
        #endif
    }

    /**
     * @brief Destructor
     */
    ~StackAllocator() override {
        if (m_memory) {
            #ifdef _WIN32
            _aligned_free(m_memory);
            #else
            std::free(m_memory);
            #endif
            m_memory = nullptr;
        }
    }

    // Disable copy
    StackAllocator(const StackAllocator&) = delete;
    StackAllocator& operator=(const StackAllocator&) = delete;

    // Enable move
    StackAllocator(StackAllocator&& other) noexcept
        : BaseAllocator(std::move(other))
        , m_alignment(other.m_alignment)
        , m_memory(other.m_memory)
        , m_current_offset(other.m_current_offset)
        , m_previous_offset(other.m_previous_offset)
    {
        other.m_memory = nullptr;
        other.m_current_offset = 0;
        other.m_previous_offset = 0;
    }

    /**
     * @brief Allocate memory from the stack
     * @param size Size to allocate
     * @param alignment Alignment requirement
     * @return Pointer to allocated memory, or nullptr if full
     */
    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) override {
        if (!m_memory || size == 0) return nullptr;

        Timer timer;
        timer.start();

        // Calculate aligned address
        size_t current_addr = reinterpret_cast<size_t>(m_memory + m_current_offset);
        size_t aligned_addr = align_size(current_addr, alignment);
        size_t adjustment = aligned_addr - current_addr;

        // Check if we have enough space
        size_t total_size = adjustment + sizeof(AllocationHeader) + size;
        if (m_current_offset + total_size > m_total_size) {
            return nullptr; // Stack is full
        }

        // Store allocation header
        size_t header_offset = m_current_offset + adjustment;
        AllocationHeader* header = reinterpret_cast<AllocationHeader*>(m_memory + header_offset);
        header->size = size;
        header->adjustment = adjustment;
        header->previous_offset = m_previous_offset;

        // Update offsets
        m_previous_offset = m_current_offset;
        m_current_offset = header_offset + sizeof(AllocationHeader) + size;

        void* ptr = reinterpret_cast<void*>(header + 1);

        timer.stop();
        record_allocation(ptr, size, alignment, timer.elapsed_ns());

        return ptr;
    }

    /**
     * @brief Deallocate the most recent allocation
     * @param ptr Pointer to deallocate (must be most recent allocation)
     */
    void deallocate(void* ptr) override {
        if (!ptr || !m_memory) return;
        
        // Get header
        AllocationHeader* header = reinterpret_cast<AllocationHeader*>(
            static_cast<uint8_t*>(ptr) - sizeof(AllocationHeader)
        );

        // Verify this is the top of the stack
        size_t expected_offset = reinterpret_cast<uint8_t*>(ptr) - m_memory + header->size;
        if (expected_offset != m_current_offset) {
            // Not the top allocation - cannot deallocate out of order
            // In a real implementation, you might want to handle this differently
            return;
        }

        Timer timer;
        timer.start();

        size_t size = header->size;

        // Pop the stack
        m_current_offset = m_previous_offset;
        
        // Restore previous offset from header
        if (m_current_offset > 0) {
            // Find the previous header
            m_previous_offset = header->previous_offset;
        } else {
            m_previous_offset = 0;
        }

        timer.stop();
        record_deallocation(size, timer.elapsed_ns());
    }

    /**
     * @brief Get current marker position
     * @return Current stack position
     */
    Marker get_marker() const {
        return m_current_offset;
    }

    /**
     * @brief Roll back to a previous marker position
     * @param marker Marker to roll back to
     * 
     * This deallocates all memory allocated after the marker was obtained.
     */
    void rollback_to_marker(Marker marker) {
        if (marker > m_current_offset) return;
        
        // Calculate how much we're freeing
        size_t freed = m_current_offset - marker;
        
        m_current_offset = marker;
        
        // Find the previous offset
        if (marker > 0) {
            // Search backwards for the header
            // This is a simplified version - real implementation would track this
            m_previous_offset = 0;
        } else {
            m_previous_offset = 0;
        }

        // Update stats
        if (freed > 0) {
            m_stats.current_bytes_used = marker;
        }
    }

    /**
     * @brief Reset the stack to empty
     */
    void reset() override {
        m_current_offset = 0;
        m_previous_offset = 0;
        m_stats = AllocationStats{};
        m_allocation_history.clear();
    }

    /**
     * @brief Check if pointer belongs to this allocator
     * @param ptr Pointer to check
     * @return true if in bounds
     */
    bool owns(void* ptr) const override {
        if (!m_memory || !ptr) return false;
        
        uint8_t* p = static_cast<uint8_t*>(ptr);
        return p >= m_memory && p < (m_memory + m_total_size);
    }

    /**
     * @brief Get available bytes
     * @return Remaining capacity
     */
    size_t available() const override {
        return m_total_size - m_current_offset;
    }

    /**
     * @brief Get used bytes
     * @return Bytes currently allocated
     */
    size_t used() const {
        return m_current_offset;
    }

    /**
     * @brief Stack allocator has minimal fragmentation
     * @return Small value based on alignment padding
     */
    double fragmentation_percentage() const override {
        if (m_current_offset == 0) return 0.0;
        // Only internal fragmentation from alignment
        return 0.0; // Simplified
    }

    /**
     * @brief Get visual representation of stack usage
     * @return Usage percentage
     */
    double usage_percentage() const {
        return (static_cast<double>(m_current_offset) / m_total_size) * 100.0;
    }

private:
    /**
     * @struct AllocationHeader
     * @brief Header stored before each allocation
     */
    struct AllocationHeader {
        size_t size;            ///< Size of allocation
        size_t adjustment;      ///< Alignment adjustment
        size_t previous_offset; ///< Previous allocation offset
    };

    size_t m_alignment;        ///< Default alignment
    uint8_t* m_memory;         ///< Memory buffer
    size_t m_current_offset;   ///< Current top of stack
    size_t m_previous_offset;  ///< Previous top (for deallocation)
};

} // namespace memory_engine

#endif // STACK_ALLOCATOR_HPP
