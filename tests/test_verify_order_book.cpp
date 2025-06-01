/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include "flox/book/windowed_order_book.h"
#include "flox/book/windowed_order_book_factory.h"
#include "flox/engine/market_data_event_pool.h"

#include <gtest/gtest.h>
#include <vector>

using namespace flox;

class WindowedOrderBookTestFixture : public ::testing::Test {
protected:
  static constexpr double TickSize = 1.0;
  static constexpr double Deviation = 5.0;
  static constexpr size_t PoolCapacity = 63;

  using BookUpdatePool = EventPool<BookUpdateEvent, PoolCapacity>;

  void SetUp() override {
    _book = _factory.create(WindowedOrderBookConfig{TickSize, Deviation});
  }

  BookUpdateEvent *acquireSnapshot() {
    auto handle = _pool.acquire();
    EXPECT_TRUE(handle);
    handle->type = BookUpdateType::SNAPSHOT;
    _handles.emplace_back(std::move(handle));
    return _handles.back().get();
  }

  BookUpdateEvent *acquireDelta() {
    auto handle = _pool.acquire();
    EXPECT_TRUE(handle);
    handle->type = BookUpdateType::DELTA;
    _handles.emplace_back(std::move(handle));
    return _handles.back().get();
  }

  IOrderBook *_book;
  WindowedOrderBookFactory _factory;
  BookUpdatePool _pool;
  std::vector<EventHandle<BookUpdateEvent>> _handles;
};

TEST_F(WindowedOrderBookTestFixture, SnapshotFitsInWindow) {
  auto snapshot = acquireSnapshot();
  snapshot->bids = {{100.0, 5.0}, {99.0, 3.0}};
  snapshot->asks = {{101.0, 2.0}, {102.0, 4.0}};
  _book->applyBookUpdate(*snapshot);

  EXPECT_DOUBLE_EQ(_book->bidAtPrice(100.0), 5.0);
  EXPECT_DOUBLE_EQ(_book->bidAtPrice(99.0), 3.0);
  EXPECT_DOUBLE_EQ(_book->askAtPrice(101.0), 2.0);
  EXPECT_DOUBLE_EQ(_book->askAtPrice(102.0), 4.0);
}

TEST_F(WindowedOrderBookTestFixture, SnapshotPartiallyOutsideWindow) {
  auto snapshot = acquireSnapshot();
  for (double p = 200.0; p <= 210.0; ++p)
    snapshot->bids.push_back({p, 1.0});
  for (double p = 211.0; p <= 220.0; ++p)
    snapshot->asks.push_back({p, 1.0});
  _book->applyBookUpdate(*snapshot);

  for (double p = 200.0; p <= 210.0; ++p)
    EXPECT_GE(_book->bidAtPrice(p), 0.0);
  for (double p = 211.0; p <= 220.0; ++p)
    EXPECT_GE(_book->askAtPrice(p), 0.0);
}

TEST_F(WindowedOrderBookTestFixture, DeltaWithinWindow) {
  auto snapshot = acquireSnapshot();
  snapshot->bids = {{100.0, 1.0}};
  snapshot->asks = {{101.0, 1.0}};
  _book->applyBookUpdate(*snapshot);

  auto delta = acquireDelta();
  delta->bids = {{99.0, 2.0}};
  delta->asks = {{102.0, 2.5}};
  _book->applyBookUpdate(*delta);

  EXPECT_DOUBLE_EQ(_book->bidAtPrice(99.0), 2.0);
  EXPECT_DOUBLE_EQ(_book->bidAtPrice(100.0), 1.0);
  EXPECT_DOUBLE_EQ(_book->askAtPrice(101.0), 1.0);
  EXPECT_DOUBLE_EQ(_book->askAtPrice(102.0), 2.5);
}

TEST_F(WindowedOrderBookTestFixture, DeltaAtWindowEdge) {
  auto snapshot = acquireSnapshot();
  snapshot->bids = {{100.0, 1.0}};
  snapshot->asks = {{101.0, 1.0}};
  _book->applyBookUpdate(*snapshot);

  double base = std::round((100.5 - 5.0) / TickSize) * TickSize;

  auto delta = acquireDelta();
  delta->bids = {{base, 3.0}};
  delta->asks = {{base + 9.0 * TickSize, 3.0}};
  _book->applyBookUpdate(*delta);

  EXPECT_DOUBLE_EQ(_book->bidAtPrice(100.0), 1.0);
  EXPECT_DOUBLE_EQ(_book->askAtPrice(101.0), 1.0);
  EXPECT_DOUBLE_EQ(_book->bidAtPrice(base), 3.0);
  EXPECT_DOUBLE_EQ(_book->askAtPrice(base + 9.0), 3.0);
}

TEST_F(WindowedOrderBookTestFixture, DeltaOutsideWindowIsIgnored) {
  auto snapshot = acquireSnapshot();
  snapshot->bids = {{100.0, 1.0}};
  snapshot->asks = {{101.0, 1.0}};
  _book->applyBookUpdate(*snapshot);

  auto delta = acquireDelta();
  delta->bids = {{80.0, 10.0}};
  delta->asks = {{120.0, 10.0}};
  _book->applyBookUpdate(*delta);

  EXPECT_DOUBLE_EQ(_book->bidAtPrice(100.0), 1.0);
  EXPECT_DOUBLE_EQ(_book->askAtPrice(101.0), 1.0);
  EXPECT_DOUBLE_EQ(_book->bidAtPrice(80.0), 0.0);
  EXPECT_DOUBLE_EQ(_book->askAtPrice(120.0), 0.0);
}

TEST_F(WindowedOrderBookTestFixture, ShiftWindowCleansOldLevels) {
  auto snapshot1 = acquireSnapshot();
  snapshot1->bids = {{100.0, 5.0}, {101.0, 6.0}};
  snapshot1->asks = {{102.0, 7.0}, {103.0, 8.0}};
  _book->applyBookUpdate(*snapshot1);

  auto snapshot2 = acquireSnapshot();
  snapshot2->bids = {{199.0, 1.0}};
  snapshot2->asks = {{201.0, 1.0}};
  _book->applyBookUpdate(*snapshot2);

  for (double p = 190.0; p <= 210.0; ++p) {
    double bid = _book->bidAtPrice(p);
    double ask = _book->askAtPrice(p);
    if (bid > 0.0)
      EXPECT_GE(p, 190.0) << "Unexpected old bid at " << p;
    if (ask > 0.0)
      EXPECT_GE(p, 190.0) << "Unexpected old ask at " << p;
  }
}

TEST_F(WindowedOrderBookTestFixture, MultipleExtremeDeltasAreHandledCorrectly) {
  auto snapshot = acquireSnapshot();
  snapshot->bids = {{100.0, 5.0}};
  snapshot->asks = {{101.0, 5.0}};
  _book->applyBookUpdate(*snapshot);

  auto d1 = acquireDelta();
  d1->bids = {{99.0, 2.0}, {98.0, 1.0}};
  d1->asks = {{102.0, 3.0}, {103.0, 4.0}};
  _book->applyBookUpdate(*d1);

  auto d2 = acquireDelta();
  d2->bids = {{100.0, 6.0}, {98.0, 0.0}};
  d2->asks = {{101.0, 7.0}, {102.0, 0.0}};
  _book->applyBookUpdate(*d2);

  auto d3 = acquireDelta();
  d3->bids = {{80.0, 10.0}};
  d3->asks = {{120.0, 10.0}};
  _book->applyBookUpdate(*d3);

  EXPECT_DOUBLE_EQ(_book->bidAtPrice(100.0), 6.0);
  EXPECT_DOUBLE_EQ(_book->bidAtPrice(99.0), 2.0);
  EXPECT_DOUBLE_EQ(_book->bidAtPrice(98.0), 0.0);
  EXPECT_DOUBLE_EQ(_book->askAtPrice(101.0), 7.0);
  EXPECT_DOUBLE_EQ(_book->askAtPrice(102.0), 0.0);
  EXPECT_DOUBLE_EQ(_book->askAtPrice(103.0), 4.0);
  EXPECT_DOUBLE_EQ(_book->bidAtPrice(80.0), 0.0);
  EXPECT_DOUBLE_EQ(_book->askAtPrice(120.0), 0.0);
}
