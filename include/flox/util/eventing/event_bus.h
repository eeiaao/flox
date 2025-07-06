/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>

#include "flox/engine/engine_config.h"
#include "flox/engine/subscriber_component.h"
#include "flox/engine/tick_barrier.h"
#include "flox/util/concurrency/spsc_queue.h"
#include "flox/util/eventing/event_bus_component.h"

namespace flox
{

template <typename Event, typename Policy, size_t QueueSize = config::DEFAULT_EVENTBUS_QUEUE_SIZE>
class EventBus
{
 public:
  using Listener = typename ListenerType<Event>::type;
  using QueueItem = typename Policy::QueueItem;
  using Queue = SPSCQueue<QueueItem, QueueSize>;

  using Trait = traits::EventBusTrait<Event, Queue>;
  using Allocator = PoolAllocator<Trait, 8>;

  EventBus() = default;

  ~EventBus()
  {
    stop();
  }

  EventBus(EventBus&& other) = delete;
  EventBus& operator=(EventBus&&) = delete;

  void subscribe(Listener listener)
  {
    SubscriberId id = listener.id();
    SubscriberMode mode = listener.mode();

    std::lock_guard lock(_mutex);
    _subs.try_emplace(id, Entry(mode, std::move(listener), std::make_unique<Queue>()));
  }

  void start()
  {
    if (_running.exchange(true))
    {
      return;
    }

    std::lock_guard lock(_mutex);
    _active = 0;

    for (auto& [_, e] : _subs)
    {
      if (e.mode == SubscriberMode::PUSH)
      {
        ++_active;
      }
    }

    for (auto& [_, e] : _subs)
    {
      if (e.mode == SubscriberMode::PUSH)
      {
        auto* queue = e.queue.get();
        auto listener = e.listener;

        e.thread = std::make_unique<std::thread>([this, queue, listener = std::move(listener)] mutable
                                                 {
          {
            std::lock_guard<std::mutex> lk(_readyMutex);
            if (--_active == 0) _cv.notify_one();
          }

          while (_running.load(std::memory_order_acquire)) {
            if (auto* item = queue->try_pop())
            {
              Policy::dispatch(*item, listener);
              item->~QueueItem();
            } else {
              std::this_thread::yield();
            }
          }

          while (auto* item = queue->try_pop())
          {
            if (_drainOnStop)
            {
              Policy::dispatch(*item, listener);
            }

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

    decltype(_subs) localSubs;
    {
      std::lock_guard lk(_mutex);
      localSubs.swap(_subs);
    }
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

    [[maybe_unused]] TickBarrier barrier(_subs.size());

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

  std::optional<std::reference_wrapper<Queue>> getQueue(SubscriberId id) const
  {
    std::lock_guard lock(_mutex);
    auto it = _subs.find(id);
    if (it != _subs.end() && it->second.mode == SubscriberMode::PULL && it->second.queue)
    {
      return std::ref(*it->second.queue);
    }

    return std::nullopt;
  }

  uint64_t currentTickId() const
  {
    return _tickCounter.load(std::memory_order_relaxed);
  }

  void enableDrainOnStop()
  {
    _drainOnStop = true;
  }

 private:
  struct Entry
  {
    SubscriberMode mode = SubscriberMode::PUSH;
    Listener listener;
    std::unique_ptr<Queue> queue;
    std::unique_ptr<std::thread> thread;

    Entry(SubscriberMode mode, Listener&& listener, std::unique_ptr<Queue>&& queue)
        : mode(mode), listener(std::move(listener)), queue(std::move(queue)) {}

    Entry(const Entry&) = delete;
    Entry& operator=(const Entry&) = delete;

    Entry(Entry&&) = default;
    Entry& operator=(Entry&&) = default;

    ~Entry()
    {
      if (thread && thread->joinable())
      {
        thread->join();
      }

      thread.reset();
    }
  };

  std::unordered_map<SubscriberId, Entry> _subs;
  mutable std::mutex _mutex;
  std::atomic<bool> _running{false};
  std::atomic<size_t> _active{0};
  std::condition_variable _cv;
  std::mutex _readyMutex;
  std::atomic<uint64_t> _tickCounter{0};
  bool _drainOnStop = false;
};

}  // namespace flox
