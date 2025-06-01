/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <thread>

#include "flox/engine/abstract_market_data_subscriber.h"
#include "flox/engine/events/book_update_event.h"
#include "flox/engine/market_data_bus.h"
#include "flox/engine/market_data_event_pool.h"

using namespace flox;
using namespace std::chrono_literals;

namespace {

// ------------------------
// Pull subscriber test
// ------------------------

class PullingSubscriber : public IMarketDataSubscriber {
public:
  PullingSubscriber(SubscriberId id, MarketDataBus &bus,
                    std::atomic<int> &counter)
      : _id(id), _bus(bus), _counter(counter) {}

  SubscriberId id() const override { return _id; }
  SubscriberMode mode() const override { return SubscriberMode::PULL; }

  void onMarketData(const IMarketDataEvent &) override {
    FAIL() << "PULL subscriber should not receive push";
  }

  void readLoop() {
    if (!_queue) {
      _queue = _bus.getQueue(_id);
      ASSERT_NE(_queue, nullptr) << "Queue must not be null";
    }

    auto opt = _queue->try_pop_ref();
    if (opt) {
      const auto &event = opt->get();
      const auto &book = static_cast<const BookUpdateEvent &>(*event);
      ++_counter;
      if (!book.bids.empty())
        _lastPrice.store(book.bids[0].price);
    }
  }

  double lastPrice() const { return _lastPrice.load(); }

private:
  SubscriberId _id;
  MarketDataBus &_bus;
  std::atomic<int> &_counter;
  std::atomic<double> _lastPrice{-1.0};
  MarketDataBus::Queue *_queue = nullptr;
};

TEST(MarketDataBusTest, PullingSubscriberReadsAndProcesses) {
  MarketDataBus bus;
  std::atomic<int> counter{0};

  auto s = std::make_shared<PullingSubscriber>(77, bus, counter);
  bus.subscribe(s);

  ASSERT_NE(bus.getQueue(77), nullptr);

  EventPool<BookUpdateEvent, 3> pool;
  auto update = pool.acquire();
  update->type = BookUpdateType::SNAPSHOT;
  update->bids = {{200.0, 1.0}};
  bus.publish(std::move(update));

  s->readLoop();

  EXPECT_EQ(counter.load(), 1);
  EXPECT_EQ(s->lastPrice(), 200.0);
}

// ------------------------
// Push subscriber test
// ------------------------

class PushTestSubscriber : public IMarketDataSubscriber {
public:
  explicit PushTestSubscriber(SubscriberId id, std::atomic<int> &count)
      : _id(id), _count(count) {}

  SubscriberId id() const override { return _id; }
  SubscriberMode mode() const override { return SubscriberMode::PUSH; }

  void onMarketData(const IMarketDataEvent &event) override {
    const auto &book = static_cast<const BookUpdateEvent &>(event);
    if (!book.bids.empty() && book.bids[0].price > 0.0)
      ++_count;
  }

private:
  SubscriberId _id;
  std::atomic<int> &_count;
};

TEST(MarketDataBusTest, PushSubscribersReceiveEvents) {
  MarketDataBus bus;
  EventPool<BookUpdateEvent, 3> pool;

  std::atomic<int> received{0};
  auto sub = std::make_shared<PushTestSubscriber>(77, received);
  bus.subscribe(sub);

  for (int i = 0; i < 3; ++i) {
    auto handle = pool.acquire();
    ASSERT_TRUE(handle);
    handle->type = BookUpdateType::SNAPSHOT;
    handle->bids = {{100.0 + i, 1.0}};
    bus.publish(std::move(handle));
  }

  std::this_thread::sleep_for(10ms);
  bus.stop();

  EXPECT_EQ(received, 3);
}

// ------------------------
// Mixed PUSH + PULL test
// ------------------------

TEST(MarketDataBusTest, MixedPushAndPullSubscribers) {
  MarketDataBus bus;
  EventPool<BookUpdateEvent, 3> pool;

  std::atomic<int> pushReceived{0}, pullReceived{0};
  auto pushSub = std::make_shared<PushTestSubscriber>(1, pushReceived);
  auto pullSub = std::make_shared<PullingSubscriber>(2, bus, pullReceived);

  bus.subscribe(pushSub);
  bus.subscribe(pullSub);

  auto handle = pool.acquire();
  ASSERT_TRUE(handle);
  handle->type = BookUpdateType::SNAPSHOT;
  handle->bids = {{105.5, 3.3}};
  bus.publish(std::move(handle));

  // PULL subscriber manually consumes
  pullSub->readLoop();

  bus.stop();

  EXPECT_EQ(pushReceived.load(), 1);
  EXPECT_EQ(pullReceived.load(), 1);
  EXPECT_NE(pullSub->lastPrice(), -1.0);
}

} // namespace
