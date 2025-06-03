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
#include <vector>

#include "flox/common.h"
#include "flox/engine/abstract_market_data_subscriber.h"
#include "flox/engine/events/book_update_event.h"
#include "flox/engine/market_data_bus.h"
#include "flox/engine/market_data_event_pool.h"

using namespace flox;

namespace {

using BookUpdatePool = EventPool<BookUpdateEvent, 63>;

class TestSubscriber : public IMarketDataSubscriber {
public:
  explicit TestSubscriber(SubscriberId id, std::atomic<int> &counter)
      : _id(id), _counter(counter) {}

  void onMarketData(const IMarketDataEvent &event) override {
    if (event.eventType() == MarketDataEventType::BOOK) {
      const auto &update = static_cast<const BookUpdateEvent &>(event);
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      ++_counter;
      _lastPrice.store(update.bids.empty() ? Price::fromRaw(-1).raw()
                                           : update.bids[0].price.raw());
    }
  }

  SubscriberId id() const override { return _id; }
  SubscriberMode mode() const override { return SubscriberMode::PUSH; }

  double lastPrice() const {
    return static_cast<double>(_lastPrice.load()) / Price::Scale;
  }

private:
  SubscriberId _id;
  std::atomic<int> &_counter;
  std::atomic<int64_t> _lastPrice{Price::fromRaw(-1).raw()};
};

TEST(MarketDataBusTest, SingleSubscriberReceivesUpdates) {
  MarketDataBus bus;
  std::atomic<int> receivedCount{0};

  auto subscriber = std::make_shared<TestSubscriber>(1, receivedCount);
  bus.subscribe(subscriber);

  BookUpdatePool pool;
  for (int i = 0; i < 10; ++i) {
    auto update = pool.acquire();
    ASSERT_NE(update.get(), nullptr);
    update->type = BookUpdateType::SNAPSHOT;
    update->bids.emplace_back(Price::fromDouble(100.0 + 1),
                              Quantity::fromDouble(1.0));
    bus.publish(std::move(update));
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  bus.stop();

  EXPECT_GE(receivedCount, 10);
  EXPECT_NE(subscriber->lastPrice(), -1.0);
  EXPECT_EQ(pool.inUse(), 0);
}

TEST(MarketDataBusTest, MultipleSubscribersReceiveAll) {
  MarketDataBus bus;
  std::atomic<int> received1{0};
  std::atomic<int> received2{0};

  auto sub1 = std::make_shared<TestSubscriber>(1, received1);
  auto sub2 = std::make_shared<TestSubscriber>(2, received2);

  bus.subscribe(sub1);
  bus.subscribe(sub2);

  BookUpdatePool pool;
  for (int i = 0; i < 20; ++i) {
    auto update = pool.acquire();
    ASSERT_NE(update.get(), nullptr);
    update->type = BookUpdateType::SNAPSHOT;
    update->bids.emplace_back(Price::fromDouble(200.0 + i),
                              Quantity::fromDouble(1.0));
    bus.publish(std::move(update));
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  bus.stop();

  EXPECT_GE(received1, 20);
  EXPECT_GE(received2, 20);
  EXPECT_NE(sub1->lastPrice(), -1.0);
  EXPECT_NE(sub2->lastPrice(), -1.0);
  EXPECT_EQ(pool.inUse(), 0);
}

TEST(MarketDataBusTest, GracefulStopDoesNotLeak) {
  MarketDataBus bus;
  std::atomic<int> count{0};
  bus.subscribe(std::make_shared<TestSubscriber>(1, count));

  BookUpdatePool pool;
  for (int i = 0; i < 5; ++i) {
    auto update = pool.acquire();
    ASSERT_NE(update.get(), nullptr);
    update->type = BookUpdateType::SNAPSHOT;
    update->bids.emplace_back(Price::fromDouble(300.0 + i),
                              Quantity::fromDouble(1.0));
    bus.publish(std::move(update));
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(30));
  bus.stop();

  EXPECT_GE(count.load(), 5);
  EXPECT_EQ(pool.inUse(), 0);
}

} // namespace
