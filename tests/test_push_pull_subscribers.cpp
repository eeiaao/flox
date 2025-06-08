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
#include <memory>
#include <thread>

#include "flox/book/events/book_update_event.h"
#include "flox/book/events/trade_event.h"
#include "flox/engine/abstract_market_data_subscriber.h"
#include "flox/engine/bus/market_data_bus.h"
#include "flox/engine/market_data_event_pool.h"

using namespace flox;
using namespace std::chrono_literals;

namespace
{

// ---------------------------------
// Pull subscriber
// ---------------------------------

class PullingSubscriber : public IMarketDataSubscriber
{
 public:
  PullingSubscriber(SubscriberId id, MarketDataBus& bus, std::atomic<int>& counter)
      : _id(id), _bus(bus), _counter(counter)
  {
  }

  SubscriberId id() const override { return _id; }
  SubscriberMode mode() const override { return SubscriberMode::PULL; }

  void onBookUpdate(const BookUpdateEvent&) override
  {
    FAIL() << "PULL subscriber must not receive push";
  }
  void onTrade(const TradeEvent&) override
  {
    FAIL() << "PULL subscriber must not receive push";
  }

  void readLoop()
  {
    if (!_queue)
    {
      _queue = _bus.getQueue(_id);
      ASSERT_NE(_queue, nullptr);
    }

    auto opt = _queue->try_pop_ref();
    if (opt)
    {
      const auto& event = opt->get();
      const auto& book = static_cast<const BookUpdateEvent&>(*event);
      ++_counter;
      if (!book.update.bids.empty())
        _lastPrice.store(book.update.bids[0].price.toDouble());
    }
  }

  double lastPrice() const { return _lastPrice.load(); }

 private:
  SubscriberId _id;
  MarketDataBus& _bus;
  std::atomic<int>& _counter;
  std::atomic<double> _lastPrice{-1.0};
  MarketDataBus::Queue* _queue = nullptr;
};

TEST(MarketDataBusTest, PullSubscriberProcessesEvent)
{
  MarketDataBus bus;
  std::atomic<int> counter{0};

  auto sub = std::make_shared<PullingSubscriber>(42, bus, counter);
  bus.subscribe(sub);
  ASSERT_NE(bus.getQueue(42), nullptr);

  bus.start();

  EventPool<BookUpdateEvent, 3> pool;
  auto eventOpt = pool.acquire();
  EXPECT_TRUE(eventOpt.has_value());
  auto& event = *eventOpt;
  event->update.type = BookUpdateType::SNAPSHOT;
  event->update.bids = {{Price::fromDouble(200.0), Quantity::fromDouble(1.0)}};
  bus.publish(std::move(event));

  sub->readLoop();

  EXPECT_EQ(counter.load(), 1);
  EXPECT_EQ(sub->lastPrice(), 200.0);
}

// ---------------------------------
// Push subscriber
// ---------------------------------

class PushTestSubscriber : public IMarketDataSubscriber
{
 public:
  PushTestSubscriber(SubscriberId id, std::atomic<int>& counter) : _id(id), _counter(counter) {}

  SubscriberId id() const override { return _id; }
  SubscriberMode mode() const override { return SubscriberMode::PUSH; }

  void onBookUpdate(const BookUpdateEvent& book) override
  {
    if (!book.update.bids.empty() && book.update.bids[0].price.toDouble() > 0.0)
    {
      ++_counter;
    }
  }

 private:
  SubscriberId _id;
  std::atomic<int>& _counter;
};

TEST(MarketDataBusTest, PushSubscriberReceivesAllEvents)
{
  MarketDataBus bus;
  std::atomic<int> counter{0};

  auto sub = std::make_shared<PushTestSubscriber>(7, counter);
  bus.subscribe(sub);

  bus.start();

  EventPool<BookUpdateEvent, 3> pool;
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
  MarketDataBus bus;
  EventPool<BookUpdateEvent, 3> pool;

  std::atomic<int> pushCounter{0}, pullCounter{0};
  auto push = std::make_shared<PushTestSubscriber>(1, pushCounter);
  auto pull = std::make_shared<PullingSubscriber>(2, bus, pullCounter);

  bus.subscribe(push);
  bus.subscribe(pull);

  bus.start();

  auto handleOpt = pool.acquire();
  EXPECT_TRUE(handleOpt.has_value());
  auto& handle = *handleOpt;
  handle->update.type = BookUpdateType::SNAPSHOT;
  handle->update.bids = {{Price::fromDouble(105.5), Quantity::fromDouble(3.3)}};
  bus.publish(std::move(handle));

  pull->readLoop();
  bus.stop();

  EXPECT_EQ(pushCounter.load(), 1);
  EXPECT_EQ(pullCounter.load(), 1);
  EXPECT_NE(pull->lastPrice(), -1.0);
}

}  // namespace
