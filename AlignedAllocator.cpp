
// ========== AlignedAllocator ========== //
/**
 * Cache-line aligned allocator to prevent false sharing.
 * Uses platform-specific aligned allocation functions.
 */

#include <vector>
#include <memory> // For std::allocator_traits
#include <limits>

template<typename T, std::size_t Alignment = CACHE_LINE_SIZE>
class AlignedAllocator {
public:
    using value_type = T;
    using pointer = T*;
    using size_type = std::size_t;

    template<typename U>
    struct rebind { using other = AlignedAllocator<U, Alignment>; };

    /**
     * Allocates aligned memory block
     * @param n Number of elements to allocate
     * @return Pointer to allocated memory
     * @throws std::bad_alloc on failure
     */
    T* allocate(std::size_t n) {
        if (n > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
            throw std::bad_alloc();
        }

        void* ptr;
#if defined(_MSC_VER)
        ptr = _aligned_malloc(n * sizeof(T), Alignment);
#else
        if (posix_memalign(&ptr, Alignment, n * sizeof(T))) {
            throw std::bad_alloc();
        }
#endif
        if (!ptr) throw std::bad_alloc();
        return static_cast<T*>(ptr);
    }


    void deallocate(T* p, std::size_t) noexcept {
#if defined(_MSC_VER)
        _aligned_free(p);
#else
        free(p);
#endif
    }
};


// Aligned vector type for cache-friendly storage
template<typename T>
using AlignedVector = std::vector<T, AlignedAllocator<T>>;
//Now we can use AlignedVector as we use normal vector with any data type 

int main() {

     //1. Basic usage with raw allocation 
     AlignedAllocator<int> alloc;
     int* arr = alloc.allocate(100);  // 100 ints, cache-line aligned

     // Use the array...
     for (int i = 0; i < 100; ++i) {
             arr[i] = i;
      }
  
      alloc.deallocate(arr, 100);  // Deallocation


    //2. For raw allocations, destructor must be manually called if needed:
    AlignedAllocator<MyClass> alloc;
    MyClass* objs = alloc.allocate(10);

    // Construct objects
    for (int i = 0; i < 10; ++i) {
        new (&objs[i]) MyClass(i);
     }

    // Destroy objects
    for (int i = 0; i < 10; ++i) {
       objs[i].~MyClass();
    }

    alloc.deallocate(objs, 10);


    // 3. Use with STL container 
    std::vector<int, AlignedAllocator<int>> vec;

    vec.push_back(42);  // Internally calls allocate()/deallocate() as needed


     //4. Same as 2. above but by using alias
     AlignedVector<double> vec2(100);  // Vector of 100 aligned doubles
     vec2[0] = 3.14;  // No alignment concerns

     // Works with all vector operations
     vec2.push_back(2.71);
     vec2.resize(200);


    //5. With Custom Alignment
    // Allocator with 128-byte alignment (instead of default CACHE_LINE_SIZE)
    AlignedVector<int, 128> big_aligned_vec(1000);

    return 0;  // Memory is deallocated automatically via RAII for 3 to 5 above usages
}


   
