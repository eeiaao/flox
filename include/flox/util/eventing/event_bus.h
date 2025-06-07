#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "flox/engine/abstract_market_data_subscriber.h"
#include "flox/engine/event_dispatcher.h"
#include "flox/engine/tick_barrier.h"
#include "flox/engine/tick_guard.h"
#include "flox/util/concurrency/spsc_queue.h"

namespace flox
{

template <typename Event, bool Sync, bool WithQueue = false>
class EventBus;

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

template <typename Event, typename Bus>
concept PushOnlyEventBus = requires(Bus b, std::shared_ptr<typename ListenerType<Event>::type> l, const Event& e) {
  { b.subscribe(l) };
  { b.publish(e) };
  { b.start() };
  { b.stop() };
};

template <typename Event, typename Bus>
concept QueueableEventBus = PushOnlyEventBus<Event, Bus> &&
                            requires(Bus b, SubscriberId id) {
                              { b.getQueue(id) } -> std::same_as<typename Bus::Queue*>;
                            };

// Synchronous specialization
template <typename Event, bool WithQueue>
class EventBus<Event, true, WithQueue>
{
 public:
  using Listener = typename ListenerType<Event>::type;
  static constexpr size_t QueueSize = 4096;
  using QueueItem = std::pair<Event, TickBarrier*>;
  using Queue = SPSCQueue<QueueItem, QueueSize>;

  EventBus()
  {
    if constexpr (WithQueue)
    {
      static_assert(QueueableEventBus<Event, EventBus<Event, true, WithQueue>>,
                    "EventBus does not conform to QueueableEventBus");
    }
    else
    {
      static_assert(PushOnlyEventBus<Event, EventBus<Event, true, WithQueue>>,
                    "EventBus does not conform to PushOnlyEventBus");
    }
  };

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
            EventDispatcher<Event>::dispatch(ev, *listener);
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
      entry.queue->push(QueueItem{event, &barrier});
    }
    barrier.wait();
  }

  Queue* getQueue(SubscriberId id)
  {
    for (auto& entry : _subs)
    {
      if (entry.listener && entry.listener->id() == id)
        return entry.queue.get();
    }
    return nullptr;
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
class EventBus<Event, false, false>
{
 public:
  using Listener = typename ListenerType<Event>::type;
  static constexpr size_t QueueSize = 4096;
  using QueueItem = Event;
  using Queue = SPSCQueue<QueueItem, QueueSize>;

  EventBus()
  {
    static_assert(PushOnlyEventBus<Event, EventBus<Event, false, false>>,
                  "EventBus does not conform to PushOnlyEventBus");
  };

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
      {
        EventDispatcher<Event>::dispatch(event, *l);
      }
    }
  }

 private:
  std::mutex _mutex;
  std::vector<std::shared_ptr<Listener>> _listeners;
};

// Asynchronous specialization with per-subscriber queues (PULL/PUSH)
template <typename Event>
class EventBus<Event, false, true>
{
 public:
  using Listener = typename ListenerType<Event>::type;
  static constexpr size_t QueueSize = 4096;
  using QueueItem = Event;
  using Queue = SPSCQueue<QueueItem, QueueSize>;

  EventBus()
  {
    static_assert(QueueableEventBus<Event, EventBus<Event, false, true>>,
                  "EventBus does not conform to QueueableEventBus");
  }

  struct SubscriberEntry
  {
    SubscriberMode mode;
    std::shared_ptr<Listener> subscriber;
    std::unique_ptr<Queue> queue;
  };

  void subscribe(std::shared_ptr<Listener> sub)
  {
    SubscriberEntry e;
    e.mode = sub->mode();
    e.subscriber = std::move(sub);
    if (e.mode == SubscriberMode::PULL)
      e.queue = std::make_unique<Queue>();

    std::lock_guard<std::mutex> lock(_mutex);
    _subs.emplace(e.subscriber->id(), std::move(e));
  }

  Queue* getQueue(SubscriberId id)
  {
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _subs.find(id);
    if (it != _subs.end() && it->second.mode == SubscriberMode::PULL)
      return it->second.queue.get();
    return nullptr;
  }

  void start() {}
  void stop()
  {
    std::lock_guard<std::mutex> lock(_mutex);
    for (auto& [_, sub] : _subs)
    {
      if (sub.queue)
      {
        while (sub.queue->try_pop_ref())
        {
        }
      }
    }
    _subs.clear();
  }

  void publish(const Event& event)
  {
    std::lock_guard<std::mutex> lock(_mutex);
    for (auto& [_, sub] : _subs)
    {
      if (sub.mode == SubscriberMode::PUSH && sub.subscriber)
      {
        EventDispatcher<Event>::dispatch(event, *sub.subscriber);
      }
      else if (sub.mode == SubscriberMode::PULL && sub.queue)
      {
        sub.queue->push(event);
      }
    }
  }

 private:
  std::mutex _mutex;
  std::unordered_map<SubscriberId, SubscriberEntry> _subs;
};

}  // namespace flox
