# TickBarrier

A spin-wait barrier used by **SyncPolicy** event buses to ensure all subscribers
finish processing a tick before the publisher continues.

~~~cpp
class TickBarrier {
public:
  explicit TickBarrier(std::size_t total);

  void complete(); // subscriber calls when done
  void wait();     // publisher blocks until all done
};
~~~

## Purpose
Synchronise **PUSH** subscriber threads with the publisher in deterministic
(back-test) mode, guaranteeing that no new events are published until the
previous tick has been fully processed.

## Behaviour
* Constructor stores `_total` = number of expected `complete()` calls.
* Each subscriber invokes `complete()` at the end of its tick loop.
* Publisher calls `wait()`; it busy-waits with `std::this_thread::yield()` until
  `_completed == _total`.

## Notes
* Relies on `std::atomic<size_t>` with acquire/release semantics; no locks.
* Intended for low-thread-count scenarios; for large counts consider a futex or
  condition variable.
