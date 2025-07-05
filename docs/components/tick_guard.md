# TickGuard

RAII helper that calls `TickBarrier::complete()` when leaving scope.

~~~cpp
class TickGuard {
public:
  explicit TickGuard(TickBarrier& b); // stores pointer

  ~TickGuard();                       // barrier.complete()

  TickGuard(const TickGuard&)            = delete;
  TickGuard& operator=(const TickGuard&) = delete;
};
~~~

## Purpose
* Ensure every **PUSH** subscriber thread signals the `TickBarrier` even if it
  exits early due to exceptions or `return`.

## Usage
````cpp
void listenerLoop(...) {
  TickGuard tg(barrier);        // marks this thread as part of the tick
  ...                            // process queue items
}                                // ~TickGuard → barrier.complete()
````

## Notes

* Lightweight: stores only a `TickBarrier*`.
* Destructor is `noexcept`; guarantees completion call.
