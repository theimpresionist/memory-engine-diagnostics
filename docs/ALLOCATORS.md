# Memory Allocator Implementations

This document describes the memory allocators implemented in the Memory Engine Diagnostics Suite.

## Overview

Memory allocators are fundamental components that manage dynamic memory allocation and deallocation. Different allocation strategies offer various trade-offs between speed, memory efficiency, and flexibility.

## Allocator Types

### 1. Standard Allocator (new/delete)

The standard allocator wraps C++'s default `new` and `delete` operators.

**Implementation**: `src/core/allocators/standard_allocator.hpp`

#### Characteristics
- **Time Complexity**: O(n) average for both allocation and deallocation
- **Space Overhead**: Platform-dependent header per allocation
- **Thread Safety**: Thread-safe (platform implementation)
- **Fragmentation**: Can suffer from external fragmentation

#### Use Cases
- General-purpose allocation
- Baseline comparison
- When allocation patterns are unpredictable

#### Pros
- Simple and reliable
- Well-tested by system vendors
- Handles variable sizes

#### Cons
- Slower than specialized allocators
- No control over memory layout
- Cache-unfriendly allocation patterns

---

### 2. Pool Allocator

The pool allocator pre-allocates fixed-size blocks and manages them via a free list.

**Implementation**: `src/core/allocators/pool_allocator.hpp`

#### How It Works
```
┌─────────────────────────────────────────────────────────┐
│  Memory Pool (contiguous block)                         │
├────────┬────────┬────────┬────────┬────────┬────────────┤
│ Block  │ Block  │ Block  │ Block  │ Block  │   ...      │
│   1    │   2    │   3    │   4    │   5    │            │
└────────┴────────┴────────┴────────┴────────┴────────────┘
     │                         │
     ▼                         ▼
 [Free List: 1 → 3 → 5 → ...]  (blocks 2,4 allocated)
```

#### Characteristics
- **Time Complexity**: O(1) for allocation and deallocation
- **Space Overhead**: One pointer per free block
- **Thread Safety**: Not thread-safe (requires external synchronization)
- **Fragmentation**: Zero external fragmentation

#### Configuration
```cpp
PoolAllocator pool(
    4096,    // block_size: Size of each block in bytes
    10000,   // block_count: Number of blocks
    32       // alignment: Memory alignment
);
```

#### Use Cases
- Game entity allocation
- Particle systems
- Network packet buffers
- Any fixed-size object pooling

#### Pros
- Extremely fast O(1) operations
- No fragmentation
- Cache-friendly (contiguous memory)
- Predictable performance

#### Cons
- Fixed block size (internal fragmentation for smaller allocations)
- Fixed capacity
- Must know allocation size in advance

---

### 3. Stack Allocator

The stack allocator uses a linear allocation strategy with LIFO deallocation.

**Implementation**: `src/core/allocators/stack_allocator.hpp`

#### How It Works
```
┌─────────────────────────────────────────────────────────┐
│                    Memory Buffer                        │
├─────────────────────┬───────────────────────────────────┤
│ Allocated Memory    │        Free Space                 │
│ [A][B][C][D]        │                                   │
└─────────────────────┴───────────────────────────────────┘
                      ▲
                      │
                  Stack Top (current offset)
```

#### Markers
Markers allow batch deallocation:
```cpp
StackAllocator stack(1024 * 1024); // 1MB

auto marker = stack.get_marker();  // Save position
void* a = stack.allocate(100);
void* b = stack.allocate(200);
stack.rollback_to_marker(marker);  // Free both a and b
```

#### Characteristics
- **Time Complexity**: O(1) allocation, O(1) deallocation
- **Space Overhead**: Header per allocation
- **Thread Safety**: Not thread-safe
- **Fragmentation**: None when used correctly

#### Use Cases
- Frame allocators in games
- Temporary calculations
- Recursive algorithms
- Scope-based memory

#### Pros
- Fastest allocation possible (pointer increment)
- No fragmentation
- Marker-based batch deallocation

#### Cons
- Must deallocate in reverse order
- No individual deallocation
- Fixed total size

---

### 4. Free List Allocator

Variable-size allocator maintaining a sorted list of free blocks.

**Implementation**: `src/core/allocators/freelist_allocator.hpp`

#### Fit Policies

| Policy | Description | Best For |
|--------|-------------|----------|
| **First Fit** | Use first block that fits | Fast allocation |
| **Best Fit** | Use smallest fitting block | Memory efficiency |
| **Worst Fit** | Use largest block | Reducing fragmentation |

#### How It Works
```
┌─────────────────────────────────────────────────────────┐
│                    Memory Buffer                        │
├────────┬────────────┬────────┬──────────────────────────┤
│ Alloc  │   FREE     │ Alloc  │         FREE             │
│  (A)   │   (128B)   │  (B)   │        (512B)            │
└────────┴────────────┴────────┴──────────────────────────┘
              │                        │
              ▼                        ▼
         [Free List: 128B → 512B → NULL]
```

#### Block Coalescing
Adjacent free blocks are automatically merged:
```
Before:  [Alloc][FREE-100][FREE-200][Alloc]
After:   [Alloc][FREE-300][Alloc]
```

#### Characteristics
- **Time Complexity**: O(n) allocation, O(n) deallocation
- **Space Overhead**: Header per allocation and free block
- **Thread Safety**: Not thread-safe
- **Fragmentation**: Can fragment, but coalescing helps

#### Use Cases
- Variable-size allocations
- General-purpose but controlled memory
- When you need deallocation in any order

---

## Performance Comparison

| Allocator | Alloc | Dealloc | Fragmentation | Flexibility |
|-----------|-------|---------|---------------|-------------|
| Standard | Slow | Slow | High | High |
| Pool | Very Fast | Very Fast | None | Low |
| Stack | Very Fast | Very Fast | None | Medium |
| Free List | Medium | Medium | Medium | High |

## Choosing an Allocator

```
                    ┌─────────────────────────┐
                    │ Are all objects the     │
                    │ same size?              │
                    └───────────┬─────────────┘
                               │
              ┌────────────────┴────────────────┐
              │                                 │
             YES                               NO
              │                                 │
              ▼                                 ▼
    ┌─────────────────┐             ┌─────────────────────┐
    │ Pool Allocator  │             │ Can you deallocate  │
    └─────────────────┘             │ in reverse order?   │
                                    └───────────┬─────────┘
                                                │
                              ┌─────────────────┴──────────────┐
                              │                                │
                             YES                              NO
                              │                                │
                              ▼                                ▼
                    ┌─────────────────┐            ┌───────────────────┐
                    │ Stack Allocator │            │ Free List or      │
                    └─────────────────┘            │ Standard Allocator│
                                                   └───────────────────┘
```

## Thread Safety Notes

None of the custom allocators are inherently thread-safe. For multi-threaded usage:

1. **External Locking**: Wrap allocator calls with mutex
2. **Thread-Local Allocators**: Give each thread its own allocator
3. **Lock-Free Designs**: Advanced implementations not covered here

Example with locking:
```cpp
std::mutex alloc_mutex;
PoolAllocator pool(256, 1000);

void* thread_safe_allocate(size_t size) {
    std::lock_guard<std::mutex> lock(alloc_mutex);
    return pool.allocate(size);
}
```
