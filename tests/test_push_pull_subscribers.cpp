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
#include <functional>

#include "flox/aggregator/events/candle_event.h"
#include "flox/book/bus/book_update_bus.h"
#include "flox/book/events/book_update_event.h"
#include "flox/book/events/trade_event.h"
#include "flox/engine/market_data_subscriber_component.h"
#include "flox/util/base/ref.h"
#include "flox/util/memory/pool.h"

using namespace flox;
using namespace std::chrono_literals;

namespace
{

// ---------------------------------
// Pull subscriber
// ---------------------------------

class PullingSubscriber
{
 public:
  using Trait = traits::MarketDataSubscriberTrait;
  using Allocator = PoolAllocator<Trait, 8>;

  PullingSubscriber(SubscriberId id, BookUpdateBusRef bookUpdateBus, std::atomic<int>& counter)
      : _id(id), _bookUpdateBus(bookUpdateBus), _counter(counter)
  {
  }

  PullingSubscriber(PullingSubscriber&& other) noexcept
      : _id(other._id),
        _bookUpdateBus(other._bookUpdateBus),
        _counter(other._counter),
        _lastPrice(other._lastPrice.load())
  {
    other._lastPrice.store(-1.0);
  }

  PullingSubscriber& operator=(PullingSubscriber&& other) noexcept
  {
    if (this != &other)
    {
      _id = other._id;
      _lastPrice.store(other._lastPrice.load());
      _counter = other._counter;
      other._lastPrice.store(-1.0);
    }
    return *this;
  }

  PullingSubscriber(const PullingSubscriber&) = delete;
  PullingSubscriber& operator=(const PullingSubscriber&) = delete;

  SubscriberId id() const { return _id; }
  SubscriberMode mode() const { return SubscriberMode::PULL; }

  void onBookUpdate(const BookUpdateEvent&)
  {
    FAIL() << "PULL subscriber must not receive push";
  }
  void onTrade(const TradeEvent&)
  {
    FAIL() << "PULL subscriber must not receive push";
  }
  void onCandle(const CandleEvent&)
  {
    FAIL() << "PULL subscriber must not receive push";
  }

  void readLoop()
  {
    auto optQueue = _bookUpdateBus.getQueue(_id);
    EXPECT_TRUE(optQueue.has_value());

    auto& queue = optQueue.value().get();
    auto opt = queue.try_pop_ref();
    if (opt)
    {
      const auto& event = opt->get();
      const auto& book = static_cast<const BookUpdateEvent&>(*event);
      ++_counter.get();
      if (!book.update.bids.empty())
        _lastPrice.store(book.update.bids[0].price.toDouble());
    }
  }

  double lastPrice() const { return _lastPrice.load(); }

 private:
  SubscriberId _id;
  BookUpdateBusRef _bookUpdateBus;
  std::reference_wrapper<std::atomic<int>> _counter;
  std::atomic<double> _lastPrice{-1.0};
};
static_assert(concepts::MarketDataSubscriber<PullingSubscriber>);

TEST(MarketDataBusTest, PullSubscriberProcessesEvent)
{
  auto bus = make<BookUpdateBus>();
  bus.enableDrainOnStop();
  std::atomic<int> counter{0};

  auto sub = make<PullingSubscriber>(42, bus, counter);
  bus.subscribe(sub);
  EXPECT_TRUE(bus.getQueue(42).has_value());

  bus.start();

  pool::Pool<BookUpdateEvent, 3> pool;
  auto eventOpt = pool.acquire();
  EXPECT_TRUE(eventOpt.has_value());
  auto& event = *eventOpt;
  event->update.type = BookUpdateType::SNAPSHOT;
  event->update.bids = {{Price::fromDouble(200.0), Quantity::fromDouble(1.0)}};
  bus.publish(std::move(event));

  sub.get<PullingSubscriber>().readLoop();

  EXPECT_EQ(counter.load(), 1);
  EXPECT_EQ(sub.get<PullingSubscriber>().lastPrice(), 200.0);
}

// ---------------------------------
// Push subscriber
// ---------------------------------

class PushTestSubscriber
{
 public:
  using Trait = traits::MarketDataSubscriberTrait;
  using Allocator = PoolAllocator<Trait, 8>;

  PushTestSubscriber(SubscriberId id, std::atomic<int>& counter) : _id(id), _counter(counter) {}

  SubscriberId id() const { return _id; }
  SubscriberMode mode() const { return SubscriberMode::PUSH; }

  void onBookUpdate(const BookUpdateEvent& book)
  {
    if (!book.update.bids.empty() && book.update.bids[0].price.toDouble() > 0.0)
    {
      ++_counter;
    }
  }
  void onTrade(const TradeEvent&) {}
  void onCandle(const CandleEvent&) {}

 private:
  SubscriberId _id;
  std::atomic<int>& _counter;
};
static_assert(concepts::MarketDataSubscriber<PushTestSubscriber>);

TEST(MarketDataBusTest, PushSubscriberReceivesAllEvents)
{
  auto bus = make<BookUpdateBus>();
  bus.enableDrainOnStop();
  std::atomic<int> counter{0};

  auto sub = make<PushTestSubscriber>(7, counter);
  bus.subscribe(sub);

  bus.start();

  pool::Pool<BookUpdateEvent, 3> pool;
  for (int i = 0; i < 3; ++i)
  {
    auto handleOpt = pool.acquire();
    EXPECT_TRUE(handleOpt.has_value());
    auto& handle = *handleOpt;
    handle->update.type = BookUpdateType::SNAPSHOT;
    handle->update.bids = {{Price::fromDouble(100.0 + i), Quantity::fromDouble(1.0)}};
    bus.publish(std::move(handle));
  }

  bus.stop();

  EXPECT_EQ(counter.load(), 3);
}

// ---------------------------------
// Mixed PUSH and PULL
// ---------------------------------

TEST(MarketDataBusTest, MixedPushAndPullWorkTogether)
{
  auto bus = make<BookUpdateBus>();
  bus.enableDrainOnStop();
  pool::Pool<BookUpdateEvent, 3> pool;

  std::atomic<int> pushCounter{0}, pullCounter{0};
  auto push = make<PushTestSubscriber>(1, pushCounter);
  auto pull = make<PullingSubscriber>(2, bus, pullCounter);

  bus.subscribe(push);
  bus.subscribe(pull);

  bus.start();

  auto handleOpt = pool.acquire();
  EXPECT_TRUE(handleOpt.has_value());
  auto& handle = *handleOpt;
  handle->update.type = BookUpdateType::SNAPSHOT;
  handle->update.bids = {{Price::fromDouble(105.5), Quantity::fromDouble(3.3)}};
  bus.publish(std::move(handle));

  pull.get<PullingSubscriber>().readLoop();
  bus.stop();

  EXPECT_EQ(pushCounter.load(), 1);
  EXPECT_EQ(pullCounter.load(), 1);
  EXPECT_NE(pull.get<PullingSubscriber>().lastPrice(), -1.0);
}

}  // namespace
