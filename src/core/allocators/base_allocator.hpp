/**
 * @file base_allocator.hpp
 * @brief Abstract base class for all memory allocators
 * @author Bambang Hutagalung
 * @date 2026
 */

#ifndef BASE_ALLOCATOR_HPP
#define BASE_ALLOCATOR_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace memory_engine {

/**
 * @struct AllocationStats
 * @brief Statistics for memory allocation tracking
 */
struct AllocationStats {
    size_t total_allocations = 0;      ///< Total number of allocations made
    size_t total_deallocations = 0;    ///< Total number of deallocations made
    size_t current_allocations = 0;    ///< Currently active allocations
    size_t total_bytes_allocated = 0;  ///< Total bytes ever allocated
    size_t current_bytes_used = 0;     ///< Currently used bytes
    size_t peak_bytes_used = 0;        ///< Peak memory usage
    size_t fragmentation_bytes = 0;    ///< Estimated fragmentation
    double avg_allocation_time_ns = 0; ///< Average allocation time in nanoseconds
    double avg_dealloc_time_ns = 0;    ///< Average deallocation time in nanoseconds
};

/**
 * @struct AllocationInfo
 * @brief Information about a single allocation
 */
struct AllocationInfo {
    void* address = nullptr;   ///< Memory address
    size_t size = 0;           ///< Allocation size
    size_t alignment = 0;      ///< Alignment requirement
    uint64_t timestamp = 0;    ///< Allocation timestamp
    bool is_active = false;    ///< Whether allocation is still active
};

/**
 * @class BaseAllocator
 * @brief Abstract base class defining the allocator interface
 * 
 * All custom allocators must inherit from this class and implement
 * the pure virtual methods for allocation and deallocation.
 */
class BaseAllocator {
public:
    /**
     * @brief Constructor
     * @param name Human-readable name for the allocator
     * @param total_size Total size of memory pool (if applicable)
     */
    explicit BaseAllocator(const std::string& name, size_t total_size = 0)
        : m_name(name), m_total_size(total_size) {}

    /**
     * @brief Virtual destructor
     */
    virtual ~BaseAllocator() = default;

    // Disable copy
    BaseAllocator(const BaseAllocator&) = delete;
    BaseAllocator& operator=(const BaseAllocator&) = delete;

    // Enable move
    BaseAllocator(BaseAllocator&&) = default;
    BaseAllocator& operator=(BaseAllocator&&) = default;

    /**
     * @brief Allocate memory with specified size and alignment
     * @param size Size in bytes to allocate
     * @param alignment Memory alignment requirement (must be power of 2)
     * @return Pointer to allocated memory, or nullptr on failure
     */
    virtual void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) = 0;

    /**
     * @brief Deallocate previously allocated memory
     * @param ptr Pointer to memory to deallocate
     */
    virtual void deallocate(void* ptr) = 0;

    /**
     * @brief Reset the allocator to initial state
     * 
     * This clears all allocations without calling destructors.
     * Use with caution.
     */
    virtual void reset() = 0;

    /**
     * @brief Check if a pointer was allocated by this allocator
     * @param ptr Pointer to check
     * @return true if the pointer belongs to this allocator
     */
    virtual bool owns(void* ptr) const = 0;

    /**
     * @brief Get the name of this allocator
     * @return Allocator name
     */
    const std::string& name() const { return m_name; }

    /**
     * @brief Get total size of the allocator's memory pool
     * @return Total size in bytes
     */
    size_t total_size() const { return m_total_size; }

    /**
     * @brief Get current allocation statistics
     * @return AllocationStats structure
     */
    const AllocationStats& stats() const { return m_stats; }

    /**
     * @brief Get allocation history (for visualization)
     * @return Vector of allocation info
     */
    const std::vector<AllocationInfo>& allocation_history() const { 
        return m_allocation_history; 
    }

    /**
     * @brief Calculate fragmentation percentage
     * @return Fragmentation as percentage (0-100)
     */
    virtual double fragmentation_percentage() const {
        if (m_stats.current_bytes_used == 0) return 0.0;
        return (static_cast<double>(m_stats.fragmentation_bytes) / 
                static_cast<double>(m_stats.current_bytes_used)) * 100.0;
    }

    /**
     * @brief Get available free memory
     * @return Free bytes available
     */
    virtual size_t available() const {
        return m_total_size - m_stats.current_bytes_used;
    }

protected:
    std::string m_name;                              ///< Allocator name
    size_t m_total_size;                             ///< Total memory pool size
    AllocationStats m_stats;                         ///< Allocation statistics
    std::vector<AllocationInfo> m_allocation_history; ///< Allocation tracking

    /**
     * @brief Record an allocation for statistics
     * @param ptr Allocated pointer
     * @param size Allocation size
     * @param alignment Alignment used
     * @param time_ns Time taken in nanoseconds
     */
    void record_allocation(void* ptr, size_t size, size_t alignment, double time_ns) {
        m_stats.total_allocations++;
        m_stats.current_allocations++;
        m_stats.total_bytes_allocated += size;
        m_stats.current_bytes_used += size;
        
        if (m_stats.current_bytes_used > m_stats.peak_bytes_used) {
            m_stats.peak_bytes_used = m_stats.current_bytes_used;
        }

        // Update average allocation time
        double total_time = m_stats.avg_allocation_time_ns * (m_stats.total_allocations - 1);
        m_stats.avg_allocation_time_ns = (total_time + time_ns) / m_stats.total_allocations;

        // Record in history
        AllocationInfo info;
        info.address = ptr;
        info.size = size;
        info.alignment = alignment;
        info.is_active = true;
        m_allocation_history.push_back(info);
    }

    /**
     * @brief Record a deallocation for statistics
     * @param size Size of deallocated memory
     * @param time_ns Time taken in nanoseconds
     */
    void record_deallocation(size_t size, double time_ns) {
        m_stats.total_deallocations++;
        m_stats.current_allocations--;
        m_stats.current_bytes_used -= size;

        // Update average deallocation time
        double total_time = m_stats.avg_dealloc_time_ns * (m_stats.total_deallocations - 1);
        m_stats.avg_dealloc_time_ns = (total_time + time_ns) / m_stats.total_deallocations;
    }

    /**
     * @brief Align a size to the specified alignment boundary
     * @param size Size to align
     * @param alignment Alignment boundary
     * @return Aligned size
     */
    static constexpr size_t align_size(size_t size, size_t alignment) {
        return (size + alignment - 1) & ~(alignment - 1);
    }

    /**
     * @brief Check if alignment is a power of 2
     * @param alignment Alignment to check
     * @return true if power of 2
     */
    static constexpr bool is_power_of_two(size_t alignment) {
        return alignment > 0 && (alignment & (alignment - 1)) == 0;
    }
};

} // namespace memory_engine

#endif // BASE_ALLOCATOR_HPP
