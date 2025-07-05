# pool (Poolable / Handle / Pool)

Memory-pool infrastructure enabling **zero-heap, ref-counted** objects with SPSC
hand-off queues.

## Poolable Concept & Base

````cpp
// Concept
template <typename T>
concept Poolable =
    RefCountable<T> &&
    requires(T* obj) {
      obj->setPool(nullptr);
      obj->releaseToPool();
      obj->clear();
    };

// Helper base
template <typename Derived>
struct PoolableBase : RefCountable {
  void  setPool(void* pool);
  void  releaseToPool();        // returns object to its Pool
  void  clear();                // reset state (override if needed)
private:
  void* _origin;                // back-pointer to owning Pool
  static inline void (*_releaseFn)(void*, void*) = nullptr;
};
````

*Implementors derive from `PoolableBase<Derived>` and add domain data; override
`clear()` for fast reset.*

## `Handle<T>`

Smart pointer that **retains** on copy / **releases** on destruction; when the
internal ref-count drops to zero it calls `releaseToPool()`.

```cpp
template <typename T>
class Handle {
  explicit Handle(T*);               // acquire +1
  Handle(const Handle&);             // retain
  Handle(Handle&&);                  // move, no retain
  ~Handle();                         // release, maybe return to Pool

  T*       get()  const;
  T*       operator->() const;
  T&       operator*() const;
  Handle<U> upcast<U>() const;       // retain + static cast
};
```

## `Pool<T, Capacity>`

Fixed-size allocator that pre-constructs `Capacity` objects in an embedded
arena and dispenses them through an SPSC queue.

```cpp
template <typename T, size_t Capacity>
class Pool {
  std::optional<Handle<T>> acquire(); // pops from queue or nullopt
  void   release(T*);                 // Pushes back after clear()

  size_t inUse() const;               // acquired − released
};
```

### Internals

| Component          | Purpose                                                    |
| ------------------ | ---------------------------------------------------------- |
| `_slots`           | `AlignedStorage` array holding `T` instances.              |
| `_arena` / `_pool` | PMR resources for objects needing dynamic PMR allocations. |
| `_queue`           | `SPSCQueue<T*, Capacity+1>` for lock-free hand-off.        |
| Counters           | `_acquired`, `_released` for diagnostics.                  |

### Lifecycle

1. **Ctor**: placement-new each `T`, register per-class `_releaseFn`, push all
   pointers into `_queue`.
2. **acquire()**: pop pointer, reset ref-count, return `Handle<T>`.
3. **Handle dtor**: if last reference → `T::releaseToPool()` → `Pool::release()`.
4. **release()**: call `clear()`, push pointer back, update stats.

## Notes

* **Single-threaded acquire/release per `Pool`** (queue is SPSC).
* Call `inUse()` in metrics to track leaks or pressure.
* Typical pattern: wrap event objects that use dynamic memory (`BookUpdateEvent`).
