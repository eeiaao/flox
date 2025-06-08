#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "flox/engine/event_dispatcher.h"
#include "flox/engine/tick_barrier.h"
#include "flox/engine/tick_guard.h"
#include "flox/util/concurrency/spsc_queue.h"

namespace flox
{

template <typename Event, bool Sync>
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
template <typename Event>
class EventBus<Event, true>
{
 public:
  using Listener = typename ListenerType<Event>::type;
  static constexpr size_t QueueSize = 4096;
  using QueueItem = std::pair<Event, TickBarrier*>;
  using Queue = SPSCQueue<QueueItem, QueueSize>;

  EventBus()
  {
    static_assert(QueueableEventBus<Event, EventBus<Event, true>>,
                  "EventBus does not conform to QueueableEventBus");
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
    {
      return;
    }

    _active = _subs.size();
    for (auto& entry : _subs)
    {
      auto& queue = *entry.queue;
      auto& listener = entry.listener;
      entry.thread = std::thread(
          [this, &queue, &listener]
          {
            {
              std::lock_guard<std::mutex> lk(_readyMutex);
              if (--_active == 0)
                _cv.notify_one();
            }

            while (_running.load(std::memory_order_acquire))
            {
              if (auto* itemPtr = queue.try_pop())
              {
                auto& [ev, barrier] = *itemPtr;

                TickGuard guard(*barrier);
                EventDispatcher<Event>::dispatch(ev, *listener);

                itemPtr->~QueueItem();
              }
              else
              {
                std::this_thread::yield();
              }
            }
          });
    }

    std::unique_lock<std::mutex> lk(_readyMutex);
    _cv.wait(lk, [&]
             { return _active == 0; });
  }

  void stop()
  {
    if (!_running.exchange(false))
    {
      return;
    }

    for (auto& entry : _subs)
    {
      if (entry.thread.joinable())
      {
        entry.thread.join();
      }

      entry.queue->clear();
    }

    _subs.clear();
  }

  void publish(Event event)
  {
    TickBarrier barrier(_subs.size());

    uint64_t seq = _tickCounter.fetch_add(1, std::memory_order_relaxed);

    if constexpr (requires { event->tickSequence; })
    {
      event->tickSequence = seq;
    }

    if constexpr (requires { event.tickSequence; })
    {
      event.tickSequence = seq;
    }

    for (auto& entry : _subs)
    {
      entry.queue->emplace(QueueItem{event, &barrier});
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

  uint64_t currentTickId() const noexcept
  {
    return _tickCounter.load(std::memory_order_relaxed);
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

  std::atomic<uint64_t> _tickCounter{0};
};

// Asynchronous specialization with per-subscriber queues (PULL/PUSH + threads for PUSH)
template <typename Event>
class EventBus<Event, false>
{
 public:
  using Listener = typename ListenerType<Event>::type;
  static constexpr size_t QueueSize = 4096;
  using QueueItem = Event;
  using Queue = SPSCQueue<QueueItem, QueueSize>;

  EventBus()
  {
    static_assert(QueueableEventBus<Event, EventBus<Event, false>>,
                  "EventBus does not conform to QueueableEventBus");
  }

  ~EventBus() { stop(); }

  struct SubscriberEntry
  {
    SubscriberMode mode;
    std::shared_ptr<Listener> subscriber;
    std::unique_ptr<Queue> queue;
    std::optional<std::jthread> thread;
  };

  void subscribe(std::shared_ptr<Listener> sub)
  {
    SubscriberEntry e;
    e.mode = sub->mode();
    e.subscriber = std::move(sub);
    e.queue = std::make_unique<Queue>();

    std::lock_guard<std::mutex> lock(_mutex);
    _subs.emplace(e.subscriber->id(), std::move(e));
  }

  void start()
  {
    if (_running.exchange(true))
    {
      return;
    }

    _running.store(true, std::memory_order_release);

    std::lock_guard<std::mutex> lock(_mutex);

    _active = std::count_if(_subs.begin(), _subs.end(), [](const auto& entry)
                            { return entry.second.mode == SubscriberMode::PUSH; });

    for (auto& [_, sub] : _subs)
    {
      if (sub.mode == SubscriberMode::PUSH && !sub.thread.has_value())
      {
        auto* queue = sub.queue.get();
        auto subscriber = sub.subscriber;
        sub.thread.emplace([queue, subscriber, this]
                           {
                             {
                               std::lock_guard<std::mutex> lk(_readyMutex);
                               if (--_active == 0)
                                 _cv.notify_one();
                             }

                             while (_running.load(std::memory_order_acquire))
                             {
                               if (auto* item = queue->try_pop())
                               {
                                 EventDispatcher<Event>::dispatch(*item, *subscriber);
                                 item->~QueueItem();
                               }
                               else
                               {
                                 std::this_thread::yield();
                               }
                             }

                             while (auto* item = queue->try_pop())
                             {
                               EventDispatcher<Event>::dispatch(*item, *subscriber);
                               item->~QueueItem();
                             } });
      }
    }

    std::unique_lock<std::mutex> lk(_readyMutex);
    _cv.wait(lk, [&]
             { return _active == 0; });
  }

  void stop()
  {
    if (!_running.exchange(false))
      return;

    std::lock_guard<std::mutex> lock(_mutex);
    for (auto& [_, sub] : _subs)
    {
      if (sub.thread)
        sub.thread.reset();
    }

    _subs.clear();
  }

  void publish(Event event)
  {
    if (!_running.load(std::memory_order_acquire))
    {
      return;
    }

    std::lock_guard<std::mutex> lock(_mutex);

    uint64_t seq = _tickCounter.fetch_add(1, std::memory_order_relaxed);

    if constexpr (requires { event->tickSequence; })
    {
      event->tickSequence = seq;
    }

    if constexpr (requires { event.tickSequence; })
    {
      event.tickSequence = seq;
    }

    for (auto& [_, sub] : _subs)
    {
      if (sub.queue)
      {
        sub.queue->push(event);
      }
    }
  }

  Queue* getQueue(SubscriberId id)
  {
    std::lock_guard<std::mutex> lock(_mutex);
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
  std::mutex _mutex;
  std::unordered_map<SubscriberId, SubscriberEntry> _subs;
  std::atomic<bool> _running{false};
  std::atomic<size_t> _active{0};
  std::condition_variable _cv;
  std::mutex _readyMutex;

  std::atomic<uint64_t> _tickCounter{0};
};

}  // namespace flox
