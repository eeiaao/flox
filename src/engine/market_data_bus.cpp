/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include "flox/engine/market_data_bus.h"

#include <atomic>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "flox/engine/events/market_data_event.h"
#include "flox/util/spsc_queue.h"

#ifdef USE_SYNC_MARKET_BUS
#include <condition_variable>
#include "flox/engine/tick_guard.h"
#endif

namespace flox
{

#ifdef USE_SYNC_MARKET_BUS

class MarketDataBus::Impl
{
 public:
  static constexpr size_t QueueSize = 4096;
  using Queue = MarketDataBus::Queue;

  std::mutex _mutex;

  struct Entry
  {
    std::shared_ptr<IMarketDataSubscriber> subscriber;
    std::unique_ptr<Queue> queue;
    std::thread thread;
  };

  std::unordered_map<SubscriberId, Entry> _subscribers;

  std::atomic<bool> _running{false};
  std::atomic<size_t> _activeSubscribers{0};
  std::condition_variable _cv;
  std::mutex _mutexReady;

  void subscribe(SubscriberId id, std::shared_ptr<IMarketDataSubscriber> sub)
  {
    auto queue = std::make_unique<Queue>();
    Entry entry;
    entry.subscriber = std::move(sub);
    entry.queue = std::move(queue);
    _subscribers.emplace(id, std::move(entry));
  }

  Queue* getQueue(SubscriberId id)
  {
    auto it = _subscribers.find(id);
    return (it != _subscribers.end()) ? it->second.queue.get() : nullptr;
  }

  void start()
  {
    if (_running.exchange(true))
      return;

    _activeSubscribers = _subscribers.size();

    for (auto& [id, entry] : _subscribers)
    {
      auto& queue = entry.queue;
      auto& sub = entry.subscriber;

      entry.thread = std::thread(
          [this, &queue = *queue, &sub]
          {
            {
              std::lock_guard<std::mutex> lk(_mutexReady);
              if (--_activeSubscribers == 0)
                _cv.notify_one();
            }

            while (_running.load(std::memory_order_acquire))
            {
              auto opt = queue.try_pop_ref();
              if (opt)
              {
                auto& [handle, barrier] = opt->get();
                TickGuard guard(*barrier);
                handle->dispatchTo(*sub);
              }
              else
              {
                std::this_thread::yield();
              }
            }

            while (queue.try_pop_ref())
            {
            }
          });
    }

    std::unique_lock<std::mutex> lk(_mutexReady);
    _cv.wait(lk, [&]
             { return _activeSubscribers == 0; });
  }

  void stop()
  {
    if (!_running.exchange(false))
      return;

    for (auto& [_, entry] : _subscribers)
    {
      if (entry.thread.joinable())
        entry.thread.join();
      while (entry.queue->try_pop_ref())
      {
      }
    }

    _subscribers.clear();
  }

  void publish(EventHandle<IMarketDataEvent> event)
  {
    TickBarrier barrier(_subscribers.size());

    for (auto& [_, entry] : _subscribers)
    {
      entry.queue->emplace(QueueItem{event->wrap(), &barrier});
    }

    barrier.wait();
  }
};

#else

class MarketDataBus::Impl
{
 public:
  using Queue = MarketDataBus::Queue;

  struct SubscriberEntry
  {
    SubscriberMode mode;
    std::shared_ptr<IMarketDataSubscriber> subscriber;
    std::unique_ptr<Queue> queue;
    std::unique_ptr<std::atomic<bool> > running;
    std::thread thread;
  };

  std::mutex _mutex;
  std::unordered_map<SubscriberId, SubscriberEntry> _subscribers;

  ~Impl() { stop(); }

  void subscribe(SubscriberId id, SubscriberMode mode,
                 std::shared_ptr<IMarketDataSubscriber> subscriber)
  {
    SubscriberEntry entry;
    entry.mode = mode;
    entry.subscriber = std::move(subscriber);

    if (mode == SubscriberMode::PULL)
    {
      entry.queue = std::make_unique<Queue>();
      entry.running = std::make_unique<std::atomic<bool> >(true);
    }

    std::lock_guard<std::mutex> lock(_mutex);
    _subscribers.emplace(id, std::move(entry));
  }

  Queue* getQueue(SubscriberId id)
  {
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _subscribers.find(id);
    if (it != _subscribers.end() && it->second.mode == SubscriberMode::PULL)
    {
      return it->second.queue.get();
    }
    return nullptr;
  }

  void publish(EventHandle<IMarketDataEvent> event)
  {
    std::lock_guard<std::mutex> lock(_mutex);
    for (auto& [id, sub] : _subscribers)
    {
      if (sub.mode == SubscriberMode::PUSH && sub.subscriber)
      {
        event->dispatchTo(*sub.subscriber);
      }
      else if (sub.mode == SubscriberMode::PULL && sub.queue)
      {
        sub.queue->emplace(event->wrap());
      }
    }
  }

  void start() {}

  void stop()
  {
    std::lock_guard<std::mutex> lock(_mutex);
    for (auto& [_, sub] : _subscribers)
    {
      if (sub.running)
        sub.running->store(false);
      if (sub.thread.joinable())
        sub.thread.join();
    }
    _subscribers.clear();
  }
};

#endif

MarketDataBus::MarketDataBus() : _impl(std::make_unique<Impl>())
{
}
MarketDataBus::~MarketDataBus() = default;

void MarketDataBus::subscribe(std::shared_ptr<IMarketDataSubscriber> subscriber)
{
  auto id = subscriber->id();
  [[maybe_unused]] auto mode = subscriber->mode();

#ifdef USE_SYNC_MARKET_BUS
  _impl->subscribe(id, std::move(subscriber));
#else
  _impl->subscribe(id, mode, std::move(subscriber));
#endif
}

MarketDataBus::Queue* MarketDataBus::getQueue(SubscriberId id)
{
  return _impl->getQueue(id);
}

void MarketDataBus::publish(EventHandle<IMarketDataEvent> event)
{
  _impl->publish(std::move(event));
}

void MarketDataBus::start()
{
  _impl->start();
}
void MarketDataBus::stop()
{
  _impl->stop();
}

}  // namespace flox