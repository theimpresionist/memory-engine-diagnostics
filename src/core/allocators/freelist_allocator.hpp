/**
 * @file freelist_allocator.hpp
 * @brief Free list based general-purpose allocator
 * @author Bambang Hutagalung
 * @date 2026
 */

#ifndef FREELIST_ALLOCATOR_HPP
#define FREELIST_ALLOCATOR_HPP

#include "base_allocator.hpp"
#include "../utils/timer.hpp"
#include <cstdlib>
#include <algorithm>

namespace memory_engine {

/**
 * @enum FitPolicy
 * @brief Allocation fit strategies
 */
enum class FitPolicy {
    FIRST_FIT,    ///< Use first block that fits
    BEST_FIT,     ///< Use smallest block that fits
    WORST_FIT     ///< Use largest block (reduces fragmentation for some patterns)
};

/**
 * @class FreeListAllocator
 * @brief General-purpose allocator using free list management
 * 
 * The free list allocator maintains a linked list of free memory blocks.
 * It supports variable-size allocations and can merge adjacent free blocks
 * to reduce fragmentation.
 * 
 * Advantages:
 * - Supports variable-size allocations
 * - Can deallocate in any order
 * - Coalescing reduces fragmentation
 * 
 * Disadvantages:
 * - Slower than pool/stack allocators
 * - Can suffer from fragmentation
 * - More complex implementation
 */
class FreeListAllocator : public BaseAllocator {
public:
    /**
     * @brief Constructor
     * @param size Total size of memory pool
     * @param policy Fit policy for finding free blocks
     */
    explicit FreeListAllocator(size_t size, FitPolicy policy = FitPolicy::BEST_FIT)
        : BaseAllocator("Free List Allocator", size)
        , m_policy(policy)
        , m_memory(nullptr)
        , m_free_list(nullptr)
    {
        // Allocate memory
        #ifdef _WIN32
        m_memory = static_cast<uint8_t*>(_aligned_malloc(size, alignof(std::max_align_t)));
        #else
        m_memory = static_cast<uint8_t*>(std::aligned_alloc(alignof(std::max_align_t), size));
        #endif

        if (m_memory) {
            // Initialize with single free block
            m_free_list = reinterpret_cast<FreeBlock*>(m_memory);
            m_free_list->size = size;
            m_free_list->next = nullptr;
        }
    }

    /**
     * @brief Destructor
     */
    ~FreeListAllocator() override {
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
    FreeListAllocator(const FreeListAllocator&) = delete;
    FreeListAllocator& operator=(const FreeListAllocator&) = delete;

    /**
     * @brief Allocate memory
     * @param size Size to allocate
     * @param alignment Alignment requirement
     * @return Pointer to allocated memory, or nullptr
     */
    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) override {
        if (!m_memory || size == 0) return nullptr;

        Timer timer;
        timer.start();

        // Add header size and alignment padding
        size_t total_size = size + sizeof(AllocationHeader);
        total_size = align_size(total_size, alignment);

        // Find suitable block based on policy
        FreeBlock* block = nullptr;
        FreeBlock* prev_block = nullptr;
        
        switch (m_policy) {
            case FitPolicy::FIRST_FIT:
                find_first_fit(total_size, block, prev_block);
                break;
            case FitPolicy::BEST_FIT:
                find_best_fit(total_size, block, prev_block);
                break;
            case FitPolicy::WORST_FIT:
                find_worst_fit(total_size, block, prev_block);
                break;
        }

        if (!block) {
            return nullptr; // No suitable block found
        }

        // Calculate remaining size after allocation
        size_t remaining = block->size - total_size;

        if (remaining >= sizeof(FreeBlock) + MIN_BLOCK_SIZE) {
            // Split block
            FreeBlock* new_block = reinterpret_cast<FreeBlock*>(
                reinterpret_cast<uint8_t*>(block) + total_size
            );
            new_block->size = remaining;
            new_block->next = block->next;

            if (prev_block) {
                prev_block->next = new_block;
            } else {
                m_free_list = new_block;
            }
        } else {
            // Use entire block
            total_size = block->size;

            if (prev_block) {
                prev_block->next = block->next;
            } else {
                m_free_list = block->next;
            }
        }

        // Write allocation header
        AllocationHeader* header = reinterpret_cast<AllocationHeader*>(block);
        header->size = total_size;
        header->adjustment = 0;

        void* ptr = reinterpret_cast<void*>(header + 1);

        timer.stop();
        record_allocation(ptr, size, alignment, timer.elapsed_ns());

        // Update fragmentation estimate
        update_fragmentation();

        return ptr;
    }

    /**
     * @brief Deallocate memory
     * @param ptr Pointer to deallocate
     */
    void deallocate(void* ptr) override {
        if (!ptr || !owns(ptr)) return;

        Timer timer;
        timer.start();

        // Get header
        AllocationHeader* header = reinterpret_cast<AllocationHeader*>(
            static_cast<uint8_t*>(ptr) - sizeof(AllocationHeader)
        );

        size_t size = header->size;

        // Create free block at this location
        FreeBlock* free_block = reinterpret_cast<FreeBlock*>(header);
        free_block->size = size;

        // Insert into free list (sorted by address for coalescing)
        insert_free_block(free_block);

        // Coalesce adjacent blocks
        coalesce();

        timer.stop();
        record_deallocation(size - sizeof(AllocationHeader), timer.elapsed_ns());
        
        update_fragmentation();
    }

    /**
     * @brief Reset allocator
     */
    void reset() override {
        if (m_memory) {
            m_free_list = reinterpret_cast<FreeBlock*>(m_memory);
            m_free_list->size = m_total_size;
            m_free_list->next = nullptr;
        }
        m_stats = AllocationStats{};
        m_allocation_history.clear();
    }

    /**
     * @brief Check ownership
     * @param ptr Pointer to check
     * @return true if owned
     */
    bool owns(void* ptr) const override {
        if (!m_memory || !ptr) return false;
        
        uint8_t* p = static_cast<uint8_t*>(ptr);
        return p >= m_memory && p < (m_memory + m_total_size);
    }

    /**
     * @brief Get available memory
     * @return Total free bytes
     */
    size_t available() const override {
        size_t total = 0;
        FreeBlock* current = m_free_list;
        while (current) {
            total += current->size;
            current = current->next;
        }
        return total;
    }

    /**
     * @brief Get fit policy
     * @return Current fit policy
     */
    FitPolicy policy() const {
        return m_policy;
    }

    /**
     * @brief Set fit policy
     * @param policy New fit policy
     */
    void set_policy(FitPolicy policy) {
        m_policy = policy;
    }

    /**
     * @brief Get number of free blocks
     * @return Free block count
     */
    size_t free_block_count() const {
        size_t count = 0;
        FreeBlock* current = m_free_list;
        while (current) {
            count++;
            current = current->next;
        }
        return count;
    }

    /**
     * @brief Get largest free block size
     * @return Size of largest contiguous free region
     */
    size_t largest_free_block() const {
        size_t largest = 0;
        FreeBlock* current = m_free_list;
        while (current) {
            if (current->size > largest) {
                largest = current->size;
            }
            current = current->next;
        }
        return largest;
    }

private:
    static constexpr size_t MIN_BLOCK_SIZE = 16; ///< Minimum block size

    /**
     * @struct FreeBlock
     * @brief Free block header
     */
    struct FreeBlock {
        size_t size;     ///< Total size including header
        FreeBlock* next; ///< Next free block
    };

    /**
     * @struct AllocationHeader
     * @brief Allocated block header
     */
    struct AllocationHeader {
        size_t size;       ///< Total allocation size
        size_t adjustment; ///< Alignment adjustment
    };

    FitPolicy m_policy;    ///< Allocation policy
    uint8_t* m_memory;     ///< Memory buffer
    FreeBlock* m_free_list; ///< Head of free list

    /**
     * @brief Find first fitting block
     */
    void find_first_fit(size_t size, FreeBlock*& out_block, FreeBlock*& out_prev) {
        FreeBlock* current = m_free_list;
        FreeBlock* prev = nullptr;

        while (current) {
            if (current->size >= size) {
                out_block = current;
                out_prev = prev;
                return;
            }
            prev = current;
            current = current->next;
        }

        out_block = nullptr;
        out_prev = nullptr;
    }

    /**
     * @brief Find best fitting block
     */
    void find_best_fit(size_t size, FreeBlock*& out_block, FreeBlock*& out_prev) {
        FreeBlock* best = nullptr;
        FreeBlock* best_prev = nullptr;
        size_t best_size = SIZE_MAX;

        FreeBlock* current = m_free_list;
        FreeBlock* prev = nullptr;

        while (current) {
            if (current->size >= size && current->size < best_size) {
                best = current;
                best_prev = prev;
                best_size = current->size;
            }
            prev = current;
            current = current->next;
        }

        out_block = best;
        out_prev = best_prev;
    }

    /**
     * @brief Find worst fitting block
     */
    void find_worst_fit(size_t size, FreeBlock*& out_block, FreeBlock*& out_prev) {
        FreeBlock* worst = nullptr;
        FreeBlock* worst_prev = nullptr;
        size_t worst_size = 0;

        FreeBlock* current = m_free_list;
        FreeBlock* prev = nullptr;

        while (current) {
            if (current->size >= size && current->size > worst_size) {
                worst = current;
                worst_prev = prev;
                worst_size = current->size;
            }
            prev = current;
            current = current->next;
        }

        out_block = worst;
        out_prev = worst_prev;
    }

    /**
     * @brief Insert block into sorted free list
     */
    void insert_free_block(FreeBlock* block) {
        if (!m_free_list || block < m_free_list) {
            block->next = m_free_list;
            m_free_list = block;
            return;
        }

        FreeBlock* current = m_free_list;
        while (current->next && current->next < block) {
            current = current->next;
        }

        block->next = current->next;
        current->next = block;
    }

    /**
     * @brief Coalesce adjacent free blocks
     */
    void coalesce() {
        FreeBlock* current = m_free_list;

        while (current && current->next) {
            uint8_t* current_end = reinterpret_cast<uint8_t*>(current) + current->size;
            
            if (current_end == reinterpret_cast<uint8_t*>(current->next)) {
                // Adjacent blocks - merge
                current->size += current->next->size;
                current->next = current->next->next;
                // Don't advance - check for more merges
            } else {
                current = current->next;
            }
        }
    }

    /**
     * @brief Update fragmentation estimate
     */
    void update_fragmentation() {
        size_t free_memory = available();
        size_t largest = largest_free_block();
        
        if (free_memory > 0 && largest < free_memory) {
            m_stats.fragmentation_bytes = free_memory - largest;
        } else {
            m_stats.fragmentation_bytes = 0;
        }
    }
};

} // namespace memory_engine

#endif // FREELIST_ALLOCATOR_HPP
