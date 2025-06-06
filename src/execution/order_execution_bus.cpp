/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include "flox/execution/order_execution_bus.h"

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

#include "flox/execution/order_event.h"
#include "flox/util/spsc_queue.h"

#ifdef USE_SYNC_ORDER_BUS
#include <condition_variable>
#include "flox/engine/tick_guard.h"
#endif

namespace flox
{

#ifdef USE_SYNC_ORDER_BUS

class OrderExecutionBus::Impl
{
 public:
  using Queue = OrderExecutionBus::Queue;

  struct Entry
  {
    std::shared_ptr<IOrderExecutionListener> listener;
    std::unique_ptr<Queue> queue;
    std::thread thread;
  };

  std::vector<Entry> _subs;
  std::atomic<bool> _running{false};
  std::atomic<size_t> _active{0};
  std::condition_variable _cv;
  std::mutex _readyMutex;

  void subscribe(std::shared_ptr<IOrderExecutionListener> listener)
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
              auto opt = queue.try_pop_ref();
              if (opt)
              {
                auto& [event, barrier] = opt->get();
                TickGuard guard(*barrier);
                event.dispatchTo(*listener);
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

  void publish(const OrderEvent& event)
  {
    TickBarrier barrier(_subs.size());

    for (auto& entry : _subs)
    {
      entry.queue->emplace(QueueItem{event, &barrier});
    }

    barrier.wait();
  }
};

#else  // USE_SYNC_ORDER_BUS

class OrderExecutionBus::Impl
{
 public:
  std::mutex _mutex;
  std::vector<std::shared_ptr<IOrderExecutionListener> > _listeners;

  void subscribe(std::shared_ptr<IOrderExecutionListener> listener)
  {
    std::lock_guard<std::mutex> lock(_mutex);
    _listeners.push_back(std::move(listener));
  }

  void start() {}
  void stop() {}

  void publish(const OrderEvent& event)
  {
    std::lock_guard<std::mutex> lock(_mutex);
    for (auto& l : _listeners)
    {
      if (l)
        event.dispatchTo(*l);
    }
  }
};

#endif  // USE_SYNC_ORDER_BUS

OrderExecutionBus::OrderExecutionBus() : _impl(std::make_unique<Impl>()) {}
OrderExecutionBus::~OrderExecutionBus() = default;

void OrderExecutionBus::subscribe(std::shared_ptr<IOrderExecutionListener> listener)
{
  _impl->subscribe(std::move(listener));
}

void OrderExecutionBus::publish(const OrderEvent& event)
{
  _impl->publish(event);
}

void OrderExecutionBus::start()
{
  _impl->start();
}

void OrderExecutionBus::stop()
{
  _impl->stop();
}

}  // namespace flox
