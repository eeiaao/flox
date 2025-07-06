/*
 * Flox Engine
 * Developed by FLOX Foundation (https://github.com/FLOX-Foundation)
 *
 * Copyright (c) 2025 FLOX Foundation
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <functional>
#include <thread>
#include <vector>

#include "flox/book/bus/book_update_bus.h"
#include "flox/book/events/book_update_event.h"
#include "flox/common.h"
#include "flox/engine/market_data_subscriber_component.h"
#include "flox/util/base/ref.h"
#include "flox/util/memory/pool.h"

using namespace flox;

namespace
{

using BookUpdatePool = pool::Pool<BookUpdateEvent, 63>;

class TestSubscriber
{
 public:
  using Trait = traits::MarketDataSubscriberTrait;
  using Allocator = PoolAllocator<Trait, 8>;

  explicit TestSubscriber(SubscriberId id, std::atomic<int>& counter) : _id(id), _counter(counter) {}

  TestSubscriber(TestSubscriber&& other) noexcept
      : _id(other._id), _counter(other._counter), _lastPrice(other._lastPrice.load()) {}

  TestSubscriber& operator=(TestSubscriber&& other) noexcept
  {
    if (this != &other)
    {
      _id = other._id;
      _counter = other._counter;
      _lastPrice.store(other._lastPrice.load());
    }
    return *this;
  }

  TestSubscriber(const TestSubscriber&) = delete;
  TestSubscriber& operator=(const TestSubscriber&) = delete;

  void onBookUpdate(const BookUpdateEvent& book)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    ++_counter.get();
    _lastPrice.store(book.update.bids.empty() ? Price::fromRaw(-1).raw()
                                              : book.update.bids[0].price.raw());
  }

  void onTrade(const TradeEvent&) {}
  void onCandle(const CandleEvent&) {}

  SubscriberId id() const { return _id; }
  SubscriberMode mode() const { return SubscriberMode::PUSH; }

  double lastPrice() const { return static_cast<double>(_lastPrice.load()) / Price::Scale; }

 private:
  SubscriberId _id;
  std::reference_wrapper<std::atomic<int>> _counter;
  std::atomic<int64_t> _lastPrice{Price::fromRaw(-1).raw()};
};

static_assert(concepts::MarketDataSubscriber<TestSubscriber>);

TEST(MarketDataBusTest, SingleSubscriberReceivesUpdates)
{
  auto bookUpdateBus = make<BookUpdateBus>();
  std::atomic<int> receivedCount{0};

  auto subscriber = make<TestSubscriber>(1, receivedCount);
  bookUpdateBus.subscribe(subscriber);
  bookUpdateBus.start();

  BookUpdatePool pool;
  for (int i = 0; i < 1; ++i)
  {
    auto opt = pool.acquire();
    EXPECT_EQ(opt.has_value(), true);

    auto& update = *opt;
    ASSERT_NE(update.get(), nullptr);
    update->update.type = BookUpdateType::SNAPSHOT;
    update->update.bids.emplace_back(Price::fromDouble(100.0 + 1), Quantity::fromDouble(1.0));
    bookUpdateBus.publish(std::move(update));
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  bookUpdateBus.stop();

  EXPECT_GE(receivedCount, 1);
  EXPECT_NE(subscriber.get<TestSubscriber>().lastPrice(), -1.0);
  EXPECT_EQ(pool.inUse(), 0);
}

TEST(MarketDataBusTest, MultipleSubscribersReceiveAll)
{
  auto bookUpdateBus = make<BookUpdateBus>();
  std::atomic<int> received1{0};
  std::atomic<int> received2{0};

  auto sub1 = make<TestSubscriber>(1, received1);
  auto sub2 = make<TestSubscriber>(2, received2);

  bookUpdateBus.subscribe(sub1);
  bookUpdateBus.subscribe(sub2);

  bookUpdateBus.start();

  BookUpdatePool pool;
  for (int i = 0; i < 20; ++i)
  {
    auto opt = pool.acquire();
    EXPECT_EQ(opt.has_value(), true);

    auto& update = *opt;
    ASSERT_NE(update.get(), nullptr);
    update->update.type = BookUpdateType::SNAPSHOT;
    update->update.bids.emplace_back(Price::fromDouble(200.0 + i), Quantity::fromDouble(1.0));
    bookUpdateBus.publish(std::move(update));
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  bookUpdateBus.stop();

  EXPECT_GE(received1, 20);
  EXPECT_GE(received2, 20);
  EXPECT_NE(sub1.get<TestSubscriber>().lastPrice(), -1.0);
  EXPECT_NE(sub2.get<TestSubscriber>().lastPrice(), -1.0);
  EXPECT_EQ(pool.inUse(), 0);
}

TEST(MarketDataBusTest, GracefulStopDoesNotLeak)
{
  auto bookUpdateBus = make<BookUpdateBus>();
  std::atomic<int> count{0};

  auto subscriber = make<TestSubscriber>(1, count);
  bookUpdateBus.subscribe(subscriber);
  bookUpdateBus.start();

  BookUpdatePool pool;
  for (int i = 0; i < 5; ++i)
  {
    auto opt = pool.acquire();
    EXPECT_EQ(opt.has_value(), true);

    auto& update = *opt;
    ASSERT_NE(update.get(), nullptr);
    update->update.type = BookUpdateType::SNAPSHOT;
    update->update.bids.emplace_back(Price::fromDouble(300.0 + i), Quantity::fromDouble(1.0));
    bookUpdateBus.publish(std::move(update));
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(30));
  bookUpdateBus.stop();

  EXPECT_GE(count.load(), 5);
  EXPECT_EQ(pool.inUse(), 0);
}

}  // namespace
