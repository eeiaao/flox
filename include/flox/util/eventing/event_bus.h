#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>

#include "flox/engine/event_dispatcher.h"
#include "flox/engine/tick_barrier.h"
#include "flox/engine/tick_guard.h"
#include "flox/util/concurrency/spsc_queue.h"

namespace flox
{

template <typename T>
struct ListenerType
{
  using type = typename T::Listener;
};

template <typename T>
struct ListenerType<EventHandle<T>>
{
  using type = typename T::Listener;
};

template <typename Event>
struct SyncPolicy
{
  using QueueItem = std::pair<Event, TickBarrier*>;
  static QueueItem makeItem(Event ev, TickBarrier* barrier) { return {ev, barrier}; }
  static void dispatch(const QueueItem& item, typename ListenerType<Event>::type& listener)
  {
    TickGuard guard(*item.second);
    EventDispatcher<Event>::dispatch(item.first, listener);
  }
};

template <typename Event>
struct AsyncPolicy
{
  using QueueItem = Event;
  static QueueItem makeItem(Event ev, void*) { return ev; }
  static void dispatch(const QueueItem& item, typename ListenerType<Event>::type& listener)
  {
    EventDispatcher<Event>::dispatch(item, listener);
  }
};

template <typename Event, typename Policy>
class EventBus
{
 public:
  using Listener = typename ListenerType<Event>::type;
  using QueueItem = typename Policy::QueueItem;
  using Queue = SPSCQueue<QueueItem, 4096>;

  EventBus() = default;
  ~EventBus() { stop(); }

  void subscribe(std::shared_ptr<Listener> listener)
  {
    SubscriberMode mode = listener->mode();

    Entry e;
    e.mode = mode;
    e.listener = std::move(listener);
    e.queue = std::make_unique<Queue>();

    std::lock_guard lock(_mutex);
    _subs.emplace(e.listener->id(), std::move(e));
  }

  void start()
  {
    if (_running.exchange(true))
      return;

    std::lock_guard lock(_mutex);
    _active = 0;

    for (auto& [_, e] : _subs)
    {
      if (e.mode == SubscriberMode::PUSH)
        ++_active;
    }

    for (auto& [_, e] : _subs)
    {
      if (e.mode == SubscriberMode::PUSH)
      {
        auto* queue = e.queue.get();
        auto listener = e.listener;
        e.thread.emplace([this, queue, listener]
                         {
          {
            std::lock_guard<std::mutex> lk(_readyMutex);
            if (--_active == 0) _cv.notify_one();
          }
          while (_running.load(std::memory_order_acquire)) {
            if (auto* item = queue->try_pop()) {
              Policy::dispatch(*item, *listener);
              item->~QueueItem();
            } else {
              std::this_thread::yield();
            }
          }
          while (auto* item = queue->try_pop()) {
            Policy::dispatch(*item, *listener);
            item->~QueueItem();
          } });
      }
    }

    std::unique_lock lk(_readyMutex);
    _cv.wait(lk, [&]
             { return _active == 0; });
  }

  void stop()
  {
    if (!_running.exchange(false))
      return;
    std::lock_guard lock(_mutex);
    for (auto& [_, e] : _subs)
    {
      e.thread.reset();
      e.queue->clear();
    }
    _subs.clear();
  }

  void publish(Event ev)
  {
    if (!_running.load(std::memory_order_acquire))
      return;

    uint64_t seq = _tickCounter.fetch_add(1, std::memory_order_relaxed);

    if constexpr (requires { ev->tickSequence; })
    {
      ev->tickSequence = seq;
    }
    if constexpr (requires { ev.tickSequence; })
    {
      ev.tickSequence = seq;
    }

    TickBarrier barrier(_subs.size());

    std::lock_guard lock(_mutex);
    for (auto& [_, e] : _subs)
    {
      if constexpr (std::is_same_v<Policy, SyncPolicy<Event>>)
      {
        e.queue->emplace(Policy::makeItem(ev, &barrier));
      }
      else
      {
        e.queue->emplace(Policy::makeItem(ev, nullptr));
      }
    }

    if constexpr (std::is_same_v<Policy, SyncPolicy<Event>>)
    {
      barrier.wait();
    }
  }

  Queue* getQueue(SubscriberId id)
  {
    std::lock_guard lock(_mutex);
    auto it = _subs.find(id);
    if (it != _subs.end() && it->second.mode == SubscriberMode::PULL)
      return it->second.queue.get();
    return nullptr;
  }

  uint64_t currentTickId() const noexcept
  {
    return _tickCounter.load(std::memory_order_relaxed);
  }

 private:
  struct Entry
  {
    std::shared_ptr<Listener> listener;
    std::unique_ptr<Queue> queue;
    SubscriberMode mode = SubscriberMode::PUSH;
    std::optional<std::jthread> thread;
  };

  std::unordered_map<SubscriberId, Entry> _subs;
  std::mutex _mutex;
  std::atomic<bool> _running{false};
  std::atomic<size_t> _active{0};
  std::condition_variable _cv;
  std::mutex _readyMutex;
  std::atomic<uint64_t> _tickCounter{0};
};

}  // namespace flox
