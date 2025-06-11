# Aligned_Memory_Allocator
Cache-line aligned allocator to prevent false sharing

### Key Features Allocator:
1. **Alignment Guarantee**:
   - Uses `_aligned_malloc` (Windows) or `posix_memalign` (Unix) to ensure allocations are aligned to `CACHE_LINE_SIZE` (or a custom alignment).
   - Prevents false sharing in multi-threaded scenarios.

2. **Standard Compliance**:
   - Provides all required typedefs (`value_type`, `pointer`, `size_type`).
   - Implements the mandatory `rebind` mechanism for container interoperability.
   - Follows the standard `allocate/deallocate` interface.

3. **Exception Safety**:
   - `allocate()` throws `std::bad_alloc` on failure.
   - `deallocate()` is `noexcept`.

4. **Convenience Alias**:
   - `AlignedVector<T>` provides a clean way to create aligned vectors.

### Important Notes:
1. **No `alignas` Needed**:
   - No need to use `alignas` with the allocator itself or its allocations. The alignment is handled internally.

2. **Interoperability**:
   - Your allocator works with all STL containers (`vector`, `list`, `map`, etc.).
   - The `rebind` mechanism ensures containers can allocate internal nodes properly.

3. **Thread Safety**:
   - The allocator itself is stateless and thread-safe for allocations/deallocations.
   - The aligned memory helps prevent false sharing in multi-threaded code.

4. **Destruction**:
   - For containers, objects are properly destroyed before deallocation.
   - For raw allocations, destructor must be manually called if needed:
     ```cpp
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
     ```
     
