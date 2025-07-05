# SPSCQueue\<T, Capacity>

Lock-free, single-producer single-consumer ring buffer with **power-of-two** compile-time capacity.

~~~cpp
template <typename T, std::size_t Capacity>
class SPSCQueue {
public:
  bool  push   (const T& item)          noexcept;      // copy
  bool  emplace(T&& item)               noexcept;      // move
  bool  try_emplace(auto&&... args);                  // in-place construct

  bool  pop    (T& out)                 noexcept;      // move into out
  T*    try_pop();                                     // raw pointer or nullptr
  std::optional<std::reference_wrapper<T>> try_pop_ref(); // non-owning ref

  void  clear() noexcept;                              // destroy all items
  bool  empty() const noexcept;
  bool  full()  const noexcept;
  size_t size() const noexcept;
};
~~~

## Purpose
* Provide a **wait-free** queue for producer/consumer pairs (e.g., `EventBus`
  publisher ↔ listener thread) with predictable latency and zero heap use.

## Key Details
| Aspect        | Detail |
|---------------|--------|
| **Capacity**  | Must be a power of two; template parameter checked at compile time. |
| **Cache lines** | `_head`, `_tail`, and buffer start each on separate 64-byte lines to avoid false sharing. |
| **Exception safety** | Requires `T` to be *nothrow destructible*. |
| **Destructor** | Drains remaining items, calling their destructors. |

## Producer Methods
* `push(const T&)` — copy item.  
* `emplace(T&&)` — move item.  
* `try_emplace(args…)` — perfect-forward into placement-new.  
All three fail (return `false`) when the queue is **full**.

## Consumer Methods
* `pop(T&)` — move item into `out`, returns `false` if **empty**.  
* `try_pop()` — returns raw pointer to item or `nullptr`.  
* `try_pop_ref()` — returns `std::optional<ref>` for non-owning use.

## Complexity
| Operation | Cost |
|-----------|------|
| Push/Pop  | O(1) atomic loads/stores, no locks. |
| Size      | O(1). |

## Typical Usage
````cpp
SPSCQueue<Event*, 4096> q;
auto ok = q.try_emplace(evPtr);     // producer
...
if (auto* ev = q.try_pop()) { ... } // consumer
````

## Notes

* **Single** producer & **single** consumer only; undefined behaviour otherwise.
* No dynamic memory: buffer is `aligned_storage` inside the queue object.
