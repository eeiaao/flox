/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include <gtest/gtest.h>
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

#include "flox/book/events/book_update_event.h"
#include "flox/engine/abstract_market_data_subscriber.h"
#include "flox/engine/bus/market_data_bus.h"
#include "flox/engine/events/market_data_event.h"
#include "flox/engine/market_data_event_pool.h"

using namespace flox;
using namespace std::chrono_literals;

namespace
{

class SyncTestSubscriber : public IMarketDataSubscriber
{
 public:
  explicit SyncTestSubscriber(SubscriberId id, std::atomic<int>& counter)
      : _id(id), _counter(counter)
  {
  }

  void onBookUpdate(const BookUpdateEvent& book) override
  {
    std::this_thread::sleep_for(10ms);  // simulate work
    ++_counter;
    if (!book.update.bids.empty())
    {
      _lastPrice.store(book.update.bids[0].price.toDouble());
    }
  }

  SubscriberId id() const override { return _id; };
  double lastPrice() const { return _lastPrice.load(); }

 private:
  SubscriberId _id;
  std::atomic<int>& _counter;
  std::atomic<double> _lastPrice{-1.0};
};

constexpr size_t PoolCapacity = 7;
using BookUpdatePool = EventPool<BookUpdateEvent, PoolCapacity>;

TEST(SyncMarketDataBusTest, AllSubscribersProcessEachTick)
{
  MarketDataBus bus;
  BookUpdatePool pool;

  std::atomic<int> c1{0}, c2{0}, c3{0};
  auto s1 = std::make_shared<SyncTestSubscriber>(1, c1);
  auto s2 = std::make_shared<SyncTestSubscriber>(2, c2);
  auto s3 = std::make_shared<SyncTestSubscriber>(3, c3);

  bus.subscribe(s1);
  bus.subscribe(s2);
  bus.subscribe(s3);
  bus.start();

  for (int i = 0; i < 5; ++i)
  {
    auto handleOpt = pool.acquire();
    EXPECT_TRUE(handleOpt.has_value());
    auto& handle = *handleOpt;
    handle->update.type = BookUpdateType::SNAPSHOT;
    handle->update.bids = {{Price::fromDouble(100.0 + i), Quantity::fromDouble(1.0)}};
    bus.publish(std::move(handle));
  }

  bus.stop();

  EXPECT_EQ(c1, 5);
  EXPECT_EQ(c2, 5);
  EXPECT_EQ(c3, 5);
  EXPECT_NE(s1->lastPrice(), -1.0);
  EXPECT_NE(s2->lastPrice(), -1.0);
  EXPECT_NE(s3->lastPrice(), -1.0);
  EXPECT_EQ(pool.inUse(), 0);
}

TEST(SyncMarketDataBusTest, AllSubscribersProcessEachTickSynchronously)
{
  MarketDataBus bus;
  BookUpdatePool pool;

  constexpr int numSubscribers = 3;
  constexpr int numTicks = 5;

  std::mutex logMutex;
  std::map<int, std::set<SubscriberId>> tickLog;

  class StrictSyncSubscriber : public IMarketDataSubscriber
  {
   public:
    StrictSyncSubscriber(SubscriberId id, std::mutex& logMutex,
                         std::map<int, std::set<SubscriberId>>& tickLog)
        : _id(id), _logMutex(logMutex), _tickLog(tickLog)
    {
    }

    void onBookUpdate(const BookUpdateEvent& book) override
    {
      std::this_thread::sleep_for(10ms);  // simulate work
      if (!book.update.bids.empty())
      {
        const int seq = static_cast<int>(book.update.bids[0].price.toDouble());
        std::lock_guard<std::mutex> lock(_logMutex);
        _tickLog[seq].insert(_id);
      }
    }

    SubscriberId id() const override { return _id; }

   private:
    SubscriberId _id;
    std::mutex& _logMutex;
    std::map<int, std::set<SubscriberId>>& _tickLog;
  };

  for (int i = 0; i < numSubscribers; ++i)
  {
    auto s = std::make_shared<StrictSyncSubscriber>(i + 1, logMutex, tickLog);
    bus.subscribe(s);
  }

  bus.start();

  for (int tick = 0; tick < numTicks; ++tick)
  {
    {
      auto handleOpt = pool.acquire();
      EXPECT_TRUE(handleOpt.has_value());
      auto& handle = *handleOpt;
      handle->update.type = BookUpdateType::SNAPSHOT;
      handle->update.bids = {{Price::fromDouble(static_cast<double>(tick)), Quantity::fromDouble(1.0)}};
      bus.publish(std::move(handle));
    }

    for (int i = 0; i < 100; ++i)
    {
      std::this_thread::sleep_for(1ms);
      std::lock_guard<std::mutex> lock(logMutex);
      if (tickLog[tick].size() == static_cast<size_t>(numSubscribers))
      {
        break;
      }
    }

    {
      std::lock_guard<std::mutex> lock(logMutex);
      ASSERT_EQ(tickLog[tick].size(), static_cast<size_t>(numSubscribers))
          << "Tick " << tick << " was not fully processed.";
    }
  }

  bus.stop();

  for (int tick = 0; tick < numTicks; ++tick)
  {
    ASSERT_EQ(tickLog[tick].size(), static_cast<size_t>(numSubscribers))
        << "Tick " << tick << " incomplete at final check";
  }

  EXPECT_EQ(pool.inUse(), 0);
}

}  // namespace
