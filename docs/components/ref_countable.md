# RefCountable

`RefCountable` is the lightweight atomic reference-counter base used by FLOX
pools, handles, and other intrusive smart-pointer utilities.

~~~cpp
class RefCountable {
public:
  void     retain()          noexcept;   // ++ref
  bool     release()         noexcept;   // --ref, returns true when it hits 0
  void     resetRefCount(uint32_t v = 0) noexcept;
  uint32_t refCount()  const noexcept;

protected:
  std::atomic<uint32_t> _refCount{0};    // starts at 0
};
~~~

## Purpose
* Provide a **thread-safe** reference counter without virtuals.
* Integrate with `pool::Handle<T>` which calls `retain()`/`release()` to manage
  pooled objects.

## Semantics
| Method   | Behaviour |
|----------|-----------|
| `retain()`  | Atomically increments `_refCount`. Debug assert guards overflow. |
| `release()` | Decrements; returns `true` when the count becomes **zero** (caller should recycle/free the object). Aborts in debug if called on a zero count. |
| `resetRefCount(v)` | Directly sets the counter (used when an object is re-acquired from a pool). |
| `refCount()` | Read-only accessor. |

## Usage Pattern
````cpp
struct MyEvent : pool::PoolableBase<MyEvent> { /* … */ };

pool::Pool<MyEvent, 1024> pool;
auto h = pool.acquire();   // retain = 1
auto h2 = h;               // retain = 2
h.reset();                 // retain = 1
// last handle out of scope → release() returns true → object returns to pool
````

## Notes

* Relies on `std::atomic<uint32_t>` with relaxed / acq\_rel ordering for minimal overhead.
* `static_assert(concepts::RefCountable<RefCountable>)` guarantees the class satisfies its own concept.
