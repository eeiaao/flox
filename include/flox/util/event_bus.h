#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "flox/engine/tick_barrier.h"
#include "flox/engine/tick_guard.h"
#include "flox/util/spsc_queue.h"

namespace flox
{

template <typename Event, bool Sync>
class EventBus;

// Synchronous specialization
template <typename Event>
class EventBus<Event, true>
{
 public:
  using Listener = typename Event::Listener;
  static constexpr size_t QueueSize = 4096;
  using QueueItem = std::pair<Event, TickBarrier*>;
  using Queue = SPSCQueue<QueueItem, QueueSize>;

  EventBus() = default;
  ~EventBus() { stop(); }

  void subscribe(std::shared_ptr<Listener> listener)
  {
    Entry e;
    e.listener = std::move(listener);
    e.queue = std::make_unique<Queue>();
    _subs.push_back(std::move(e));
  }

  void start()
  {
    if (_running.exchange(true))
      return;
    _active = _subs.size();
    for (auto& entry : _subs)
    {
      auto& queue = *entry.queue;
      auto& listener = entry.listener;
      entry.thread = std::thread([this, &queue, &listener]
                                 {
        {
          std::lock_guard<std::mutex> lk(_readyMutex);
          if (--_active == 0)
            _cv.notify_one();
        }
        while (_running.load(std::memory_order_acquire)) {
          auto opt = queue.try_pop_ref();
          if (opt) {
            auto& [ev, barrier] = opt->get();
            TickGuard guard(*barrier);
            ev.dispatchTo(*listener);
          } else {
            std::this_thread::yield();
          }
        }
        while (queue.try_pop_ref()) {} });
    }
    std::unique_lock<std::mutex> lk(_readyMutex);
    _cv.wait(lk, [&]
             { return _active == 0; });
  }

  void stop()
  {
    if (!_running.exchange(false))
      return;
    for (auto& entry : _subs)
    {
      if (entry.thread.joinable())
        entry.thread.join();
      while (entry.queue->try_pop_ref())
      {
      }
    }
    _subs.clear();
  }

  void publish(const Event& event)
  {
    TickBarrier barrier(_subs.size());
    for (auto& entry : _subs)
    {
      entry.queue->emplace(QueueItem{event, &barrier});
    }
    barrier.wait();
  }

 private:
  struct Entry
  {
    std::shared_ptr<Listener> listener;
    std::unique_ptr<Queue> queue;
    std::thread thread;
  };

  std::vector<Entry> _subs;
  std::atomic<bool> _running{false};
  std::atomic<size_t> _active{0};
  std::condition_variable _cv;
  std::mutex _readyMutex;
};

// Asynchronous specialization
template <typename Event>
class EventBus<Event, false>
{
 public:
  using Listener = typename Event::Listener;
  static constexpr size_t QueueSize = 4096;
  using QueueItem = Event;
  using Queue = SPSCQueue<QueueItem, QueueSize>;

  EventBus() = default;
  ~EventBus() = default;

  void subscribe(std::shared_ptr<Listener> listener)
  {
    std::lock_guard<std::mutex> lock(_mutex);
    _listeners.push_back(std::move(listener));
  }

  void start() {}
  void stop() {}

  void publish(const Event& event)
  {
    std::lock_guard<std::mutex> lock(_mutex);
    for (auto& l : _listeners)
    {
      if (l)
        event.dispatchTo(*l);
    }
  }

 private:
  std::mutex _mutex;
  std::vector<std::shared_ptr<Listener>> _listeners;
};

}  // namespace flox
