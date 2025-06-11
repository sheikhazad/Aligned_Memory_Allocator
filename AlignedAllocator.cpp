
// ========== AlignedAllocator ========== //
/**
 * Cache-line aligned allocator to prevent false sharing.
 * Uses platform-specific aligned allocation functions.
 */

#include <cstdlib>
#include <limits>
#include <new>
#include <memory>
#include <vector>
#include <atomic>
#include <cassert>
#include <unordered_map>
#include <map>
#include <set>
#include <list>
#include <deque>
#include <queue>
#include <string>

// ========== Cache Line Alignment ========== //
// Use hardware-specific cache line size if available (C++17+)
#ifndef hardware_destructive_interference_size
    #define hardware_destructive_interference_size 64  // Fallback to 64 bytes
#endif

constexpr size_t CACHE_LINE_SIZE = hardware_destructive_interference_size;

// ========== AlignedAllocator ========== //
/**
 * Cache-line aligned allocator to prevent false sharing in high-performance systems.
 * Uses platform-specific aligned allocation functions for optimal memory alignment.
 * 
 * Features:
 * - Guarantees cache-line aligned memory for all allocations
 * - Thread-safe for concurrent allocations/deallocations
 * - Supports custom alignment requirements
 * - Compatible with all STL containers
 * 
 * @tparam T Type of objects to allocate
 * @tparam Alignment Memory alignment boundary (defaults to cache line size)
 */
template<typename T, std::size_t Alignment = CACHE_LINE_SIZE>
class AlignedAllocator {
public:
    // Standard allocator typedefs
    using value_type = T;              // Type of allocated elements
    using pointer = T*;                // Pointer to allocated memory
    using size_type = std::size_t;     // Type for size parameters
    using is_always_equal = std::true_type;  // Stateless allocator (C++17)

    /**
     * Rebinds the allocator to another type U.
     * Required for STL containers that allocate internal node types.
     */
    template<typename U>
    struct rebind { 
        using other = AlignedAllocator<U, Alignment>; 
    };

    /**
     * Allocates aligned memory block.
     * @param n Number of elements to allocate
     * @return Pointer to aligned memory
     * @throws std::bad_alloc if allocation fails
     */
    T* allocate(std::size_t n) {
        // Prevent integer overflow in size calculation
        if (n > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
            throw std::bad_alloc();
        }

        // Optimization: Skip alignment if type is already sufficiently aligned
        if constexpr (alignof(T) >= Alignment) {
            return static_cast<T*>(::operator new(n * sizeof(T)));
        }

        void* ptr = nullptr;
        const std::size_t size = n * sizeof(T);

#if defined(_MSC_VER)
        // Windows aligned allocation
        ptr = _aligned_malloc(size, Alignment);
#else
        // POSIX aligned allocation
        if (posix_memalign(&ptr, Alignment, size) != 0) {
            throw std::bad_alloc();
        }
#endif

        // Verify allocation succeeded
        if (!ptr) throw std::bad_alloc();

        // Debug check for correct alignment
        assert(reinterpret_cast<uintptr_t>(ptr) % Alignment == 0);

        return static_cast<T*>(ptr);
    }

    /**
     * Deallocates memory previously allocated by allocate().
     * @param p Pointer to memory to deallocate
     * @param n Number of elements (unused but required by interface)
     */
    void deallocate(T* p, std::size_t) noexcept {
#if defined(_MSC_VER)
        _aligned_free(p);
#else
        free(p);  // free() works with posix_memalign allocations
#endif
    }

    /**
     * Allocator equality comparison (C++20)
     * Two allocators are equal if they have the same alignment requirements
     */
    template<typename U, std::size_t A2>
    bool operator==(const AlignedAllocator<U, A2>& other) const noexcept { 
        return Alignment == A2; 
    }

    /**
     * Allocator inequality comparison (C++20)
     */
    template<typename U, std::size_t A2>
    bool operator!=(const AlignedAllocator<U, A2>& other) const noexcept { 
        return !(*this == other); 
    }
};

// ========== Aligned Container Aliases ========== //
template<typename T, std::size_t Alignment = CACHE_LINE_SIZE>
using AlignedVector = std::vector<T, AlignedAllocator<T, Alignment>>;

template<typename Key, typename T, std::size_t Alignment = CACHE_LINE_SIZE>
using AlignedUnorderedMap = std::unordered_map<Key, T, std::hash<Key>, 
                                              std::equal_to<Key>,
                                              AlignedAllocator<std::pair<const Key, T>, Alignment>>;

template<typename Key, typename T, std::size_t Alignment = CACHE_LINE_SIZE>
using AlignedMap = std::map<Key, T, std::less<Key>,
                           AlignedAllocator<std::pair<const Key, T>, Alignment>>;

template<typename Key, std::size_t Alignment = CACHE_LINE_SIZE>
using AlignedSet = std::set<Key, std::less<Key>,
                           AlignedAllocator<Key, Alignment>>;

template<typename T, std::size_t Alignment = CACHE_LINE_SIZE>
using AlignedList = std::list<T, AlignedAllocator<T, Alignment>>;

template<typename T, std::size_t Alignment = CACHE_LINE_SIZE>
using AlignedDeque = std::deque<T, AlignedAllocator<T, Alignment>>;

// Note: queue/stack adapters don't benefit from alignment directly
// but their underlying container (deque/list) can use our allocator
template<typename T, std::size_t Alignment = CACHE_LINE_SIZE>
using AlignedQueue = std::queue<T, AlignedDeque<T, Alignment>>;

// ========== Example Usage ========== //
struct TradeData {
    alignas(CACHE_LINE_SIZE) std::atomic<int> volume;
    double price;
    long timestamp;
};

int main() {
    // 1. Vector - optimal for sequential access
    {
        AlignedVector<TradeData> trades(100);
        trades[0] = {100, 150.25, 1234567890};
        assert(reinterpret_cast<uintptr_t>(&trades[0]) % CACHE_LINE_SIZE == 0);
    }

    // 2. Unordered Map - for O(1) lookups
    {
        AlignedUnorderedMap<int, TradeData> tradeMap;
        tradeMap.reserve(1000);  // Important for performance
        tradeMap[123] = {500, 149.50, 1234567891};
        assert(tradeMap.load_factor() < 0.8);  // Check for proper hashing
    }

    // 3. Map - for ordered traversals
    {
        AlignedMap<int, TradeData> orderedTrades;
        orderedTrades[456] = {200, 151.00, 1234567892};
        for (const auto& [id, trade] : orderedTrades) {
            assert(trade.volume.load() >= 0);
        }
    }

    // 4. Set - for unique elements
    {
        AlignedSet<int> tradeIds;
        tradeIds.insert(123);
        tradeIds.insert(456);
        assert(tradeIds.find(123) != tradeIds.end());
    }

    // 5. List - for frequent insertions/deletions
    {
        AlignedList<TradeData> tradeList;
        tradeList.push_back({300, 152.00, 1234567893});
        assert(!tradeList.empty());
    }

    // 6. Deque - for front/back operations
    {
        AlignedDeque<TradeData> tradeDeque;
        tradeDeque.push_back({400, 153.00, 1234567894});
        tradeDeque.push_front({50, 148.00, 1234567895});
        assert(tradeDeque.size() == 2);
    }

    // 7. Queue - FIFO processing
    {
        AlignedQueue<TradeData> tradeQueue;
        tradeQueue.push({600, 154.00, 1234567896});
        tradeQueue.push({700, 155.00, 1234567897});
        assert(tradeQueue.front().volume == 600);
    }

    // 8. Multi-container complex scenario
    {
        struct OrderBook {
            AlignedMap<double, int> bids;  // Price -> Quantity
            AlignedMap<double, int> asks;
            alignas(CACHE_LINE_SIZE) std::atomic<int> updateCounter{0};
        };

        OrderBook book;
        book.bids[150.25] = 100;
        book.asks[151.50] = 200;
        book.updateCounter++;
        
        assert(book.updateCounter.load() == 1);
        assert(book.bids.begin()->second == 100);
    }

    
    // 9. Basic raw allocation
    {
        AlignedAllocator<int> alloc;
        int* arr = alloc.allocate(100);  // Allocate 100 aligned integers
        
        // Use the array
        for (int i = 0; i < 100; ++i) {
            arr[i] = i;
        }
        
        alloc.deallocate(arr, 100);  // Explicit deallocation
    }

    // 10. Raw allocation with object construction/destruction
    {
        AlignedAllocator<MyClass> alloc;
        MyClass* objs = alloc.allocate(10);  // Allocate memory
        
        // Construct objects manually
        for (int i = 0; i < 10; ++i) {
            new (&objs[i]) MyClass(i);  // Placement new
        }
        
        // Destroy objects manually
        for (int i = 0; i < 10; ++i) {
            objs[i].~MyClass();
        }
        
        alloc.deallocate(objs, 10);  // Deallocate memory
    }

    // 11. STL vector with explicit allocator
    {
        std::vector<int, AlignedAllocator<int>> vec;
        vec.push_back(42);  // Uses aligned allocation automatically
        // Memory automatically freed when vector goes out of scope
    }

    // 12. AlignedVector convenience alias
    {
        AlignedVector<double> vec(100);  // Vector of 100 cache-aligned doubles
        vec[0] = 3.14;  // No alignment concerns
        
        // All vector operations work normally
        vec.push_back(2.71);
        vec.resize(200);
    }

    // 13. Custom alignment (128 bytes instead of default cache line size)
    {
        AlignedVector<int, 128> big_aligned_vec(1000);
        // Verify alignment
        assert(reinterpret_cast<uintptr_t>(big_aligned_vec.data()) % 128 == 0);
    }

    // 14. Usage in multi-threaded scenarios
    {
        struct ThreadData {
            alignas(CACHE_LINE_SIZE) std::atomic<int> counter{0};
            AlignedVector<int> data;
        };

        ThreadData td;
        td.data.push_back(10);  // Data and counter are properly aligned
    }

    return 0;
}
