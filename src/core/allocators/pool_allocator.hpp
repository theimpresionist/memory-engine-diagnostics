/**
 * @file pool_allocator.hpp
 * @brief Fixed-size block pool allocator
 * @author Bambang Hutagalung
 * @date 2026
 */

#ifndef POOL_ALLOCATOR_HPP
#define POOL_ALLOCATOR_HPP

#include "base_allocator.hpp"
#include "../utils/timer.hpp"
#include <vector>
#include <cstdlib>
#include <cstring>

namespace memory_engine {

/**
 * @class PoolAllocator
 * @brief Fixed-size block allocator for efficient allocation of same-sized objects
 * 
 * The pool allocator pre-allocates a contiguous block of memory and divides it
 * into fixed-size chunks. This provides O(1) allocation and deallocation with
 * minimal fragmentation for objects of the same size.
 * 
 * Advantages:
 * - Very fast allocation/deallocation (O(1))
 * - No external fragmentation
 * - Cache-friendly memory layout
 * 
 * Disadvantages:
 * - Fixed block size (internal fragmentation for smaller allocations)
 * - Fixed capacity (cannot grow)
 * - All blocks must be the same size
 */
class PoolAllocator : public BaseAllocator {
public:
    /**
     * @brief Constructor
     * @param block_size Size of each block in bytes
     * @param block_count Number of blocks to allocate
     * @param alignment Memory alignment for blocks
     */
    PoolAllocator(size_t block_size, size_t block_count, size_t alignment = alignof(std::max_align_t))
        : BaseAllocator("Pool Allocator", 0)
        , m_block_size(align_size(block_size, alignment))
        , m_block_count(block_count)
        , m_alignment(alignment)
        , m_memory(nullptr)
        , m_free_list(nullptr)
        , m_allocated_blocks(0)
    {
        // Calculate total size with alignment padding
        m_total_size = m_block_size * m_block_count;
        
        // Allocate aligned memory
        #ifdef _WIN32
        m_memory = static_cast<uint8_t*>(_aligned_malloc(m_total_size, m_alignment));
        #else
        m_memory = static_cast<uint8_t*>(std::aligned_alloc(m_alignment, m_total_size));
        #endif

        if (m_memory) {
            initialize_free_list();
        }
    }

    /**
     * @brief Destructor
     */
    ~PoolAllocator() override {
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
    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;

    // Enable move
    PoolAllocator(PoolAllocator&& other) noexcept
        : BaseAllocator(std::move(other))
        , m_block_size(other.m_block_size)
        , m_block_count(other.m_block_count)
        , m_alignment(other.m_alignment)
        , m_memory(other.m_memory)
        , m_free_list(other.m_free_list)
        , m_allocated_blocks(other.m_allocated_blocks)
    {
        other.m_memory = nullptr;
        other.m_free_list = nullptr;
        other.m_allocated_blocks = 0;
    }

    /**
     * @brief Allocate a block from the pool
     * @param size Size requested (must be <= block_size)
     * @param alignment Alignment (ignored, uses pool alignment)
     * @return Pointer to allocated block, or nullptr if pool is full
     */
    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) override {
        (void)alignment; // Pool uses its own alignment

        if (!m_memory || !m_free_list || size > m_block_size) {
            return nullptr;
        }

        Timer timer;
        timer.start();

        // Pop from free list
        FreeBlock* block = m_free_list;
        m_free_list = block->next;
        m_allocated_blocks++;

        timer.stop();

        void* ptr = reinterpret_cast<void*>(block);
        record_allocation(ptr, m_block_size, m_alignment, timer.elapsed_ns());

        return ptr;
    }

    /**
     * @brief Return a block to the pool
     * @param ptr Pointer to block to deallocate
     */
    void deallocate(void* ptr) override {
        if (!ptr || !owns(ptr)) return;

        Timer timer;
        timer.start();

        // Push back to free list
        FreeBlock* block = reinterpret_cast<FreeBlock*>(ptr);
        block->next = m_free_list;
        m_free_list = block;
        m_allocated_blocks--;

        timer.stop();

        record_deallocation(m_block_size, timer.elapsed_ns());
    }

    /**
     * @brief Reset pool to initial state
     */
    void reset() override {
        if (m_memory) {
            initialize_free_list();
        }
        m_allocated_blocks = 0;
        m_stats = AllocationStats{};
        m_allocation_history.clear();
    }

    /**
     * @brief Check if pointer belongs to this pool
     * @param ptr Pointer to check
     * @return true if pointer is within pool bounds
     */
    bool owns(void* ptr) const override {
        if (!m_memory || !ptr) return false;
        
        uint8_t* p = static_cast<uint8_t*>(ptr);
        return p >= m_memory && p < (m_memory + m_total_size);
    }

    /**
     * @brief Get number of free blocks
     * @return Free block count
     */
    size_t free_blocks() const {
        return m_block_count - m_allocated_blocks;
    }

    /**
     * @brief Get number of allocated blocks
     * @return Allocated block count
     */
    size_t allocated_blocks() const {
        return m_allocated_blocks;
    }

    /**
     * @brief Get block size
     * @return Size of each block
     */
    size_t block_size() const {
        return m_block_size;
    }

    /**
     * @brief Get total block count
     * @return Total number of blocks
     */
    size_t block_count() const {
        return m_block_count;
    }

    /**
     * @brief Get available memory
     * @return Bytes available for allocation
     */
    size_t available() const override {
        return free_blocks() * m_block_size;
    }

    /**
     * @brief Pool allocator has no external fragmentation
     * @return Always 0
     */
    double fragmentation_percentage() const override {
        return 0.0; // Pool allocator has no external fragmentation
    }

    /**
     * @brief Get memory block grid for visualization
     * @return Vector of bools (true = allocated, false = free)
     */
    std::vector<bool> get_allocation_grid() const {
        std::vector<bool> grid(m_block_count, true);
        
        // Mark free blocks
        FreeBlock* current = m_free_list;
        while (current) {
            size_t index = (reinterpret_cast<uint8_t*>(current) - m_memory) / m_block_size;
            if (index < m_block_count) {
                grid[index] = false;
            }
            current = current->next;
        }
        
        return grid;
    }

private:
    /**
     * @struct FreeBlock
     * @brief Node in the free list
     */
    struct FreeBlock {
        FreeBlock* next;
    };

    size_t m_block_size;      ///< Size of each block
    size_t m_block_count;     ///< Total number of blocks
    size_t m_alignment;       ///< Memory alignment
    uint8_t* m_memory;        ///< Memory buffer
    FreeBlock* m_free_list;   ///< Head of free list
    size_t m_allocated_blocks; ///< Number of allocated blocks

    /**
     * @brief Initialize the free list
     */
    void initialize_free_list() {
        m_free_list = nullptr;
        
        // Build free list from end to start for sequential access
        for (size_t i = m_block_count; i > 0; --i) {
            FreeBlock* block = reinterpret_cast<FreeBlock*>(m_memory + (i - 1) * m_block_size);
            block->next = m_free_list;
            m_free_list = block;
        }
    }
};

} // namespace memory_engine

#endif // POOL_ALLOCATOR_HPP
