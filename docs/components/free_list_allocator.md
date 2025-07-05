# FreeListAllocator\<T, Capacity>

A **fixed-size object pool** that provides O(1) `allocate`/`deallocate` using an
intrusive freelist — zero heap calls after construction.

~~~cpp
template <typename T, std::size_t Capacity>
class FreeListAllocator {
public:
  FreeListAllocator();                              // pre-links freelist

  template <typename... Args>
  T* allocate(Args&&... args);                      // placement-new, or nullptr if full

  void deallocate(T* ptr);                          // placement-delete, push index back
};
~~~

## Purpose
* Serve latency-critical code that repeatedly allocates small, uniform objects.
* Eliminate fragmentation and `new`/`delete` overhead by reusing a static array.

## Key Characteristics

| Feature       | Detail                                        |
|---------------|-----------------------------------------------|
| **Capacity**  | Compile-time constant → no dynamic growth.    |
| **Freelist**  | `_next[]` array stores next-free indices.     |
| **Storage**   | `alignas(T) std::byte _storage[Cap][sizeof T]`. |
| **Threading** | **Not** thread-safe; guard externally if needed. |

## Example
~~~cpp
FreeListAllocator<OrderEvent, 1024> pool;

auto* e1 = pool.allocate(args…);  // fast, from pre-allocated block
...
pool.deallocate(e1);              // returns slot to freelist
~~~

## Notes
* `allocate()` returns `nullptr` when the pool is exhausted — check the result.
* Destructor of `T` is invoked on deallocation via `std::destroy_at`.
