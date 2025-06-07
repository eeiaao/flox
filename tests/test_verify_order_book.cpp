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
#include "flox/common.h"
#include "flox/engine/market_data_event_pool.h"

#include <gtest/gtest.h>
#include <vector>

using namespace flox;

class WindowedOrderBookTestFixture : public ::testing::Test
{
 protected:
  static constexpr Price TickSize = Price::fromDouble(1.0);
  static constexpr Price Deviation = Price::fromDouble(5.0);
  static constexpr size_t PoolCapacity = 63;

  using BookUpdatePool = EventPool<BookUpdateEvent, PoolCapacity>;

  void SetUp() override { _book = _factory.create(WindowedOrderBookConfig{TickSize, Deviation}); }

  BookUpdateEvent* acquireSnapshot()
  {
    auto opt = _pool.acquire();
    EXPECT_TRUE(opt.has_value());
    auto& handle = *opt;
    handle->update.type = BookUpdateType::SNAPSHOT;
    _handles.emplace_back(std::move(handle));
    return _handles.back().get();
  }

  BookUpdateEvent* acquireDelta()
  {
    auto opt = _pool.acquire();
    EXPECT_TRUE(opt.has_value());
    auto& handle = *opt;
    handle->update.type = BookUpdateType::DELTA;
    _handles.emplace_back(std::move(handle));
    return _handles.back().get();
  }

  IOrderBook* _book;
  WindowedOrderBookFactory _factory;
  BookUpdatePool _pool;
  std::vector<EventHandle<BookUpdateEvent>> _handles;
};

TEST_F(WindowedOrderBookTestFixture, SnapshotFitsInWindow)
{
  auto snapshot = acquireSnapshot();
  snapshot->update.bids = {{Price::fromDouble(100.0), Quantity::fromDouble(5.0)},
                           {Price::fromDouble(99.0), Quantity::fromDouble(3.0)}};
  snapshot->update.asks = {{Price::fromDouble(101.0), Quantity::fromDouble(2.0)},
                           {Price::fromDouble(102.0), Quantity::fromDouble(4.0)}};
  _book->applyBookUpdate(*snapshot);

  EXPECT_EQ(_book->bidAtPrice(Price::fromDouble(100.0)), Quantity::fromDouble(5.0));
  EXPECT_EQ(_book->bidAtPrice(Price::fromDouble(99.0)), Quantity::fromDouble(3.0));
  EXPECT_EQ(_book->askAtPrice(Price::fromDouble(101.0)), Quantity::fromDouble(2.0));
  EXPECT_EQ(_book->askAtPrice(Price::fromDouble(102.0)), Quantity::fromDouble(4.0));
}

TEST_F(WindowedOrderBookTestFixture, SnapshotPartiallyOutsideWindow)
{
  auto snapshot = acquireSnapshot();
  for (double p = 200.0; p <= 210.0; ++p)
    snapshot->update.bids.push_back({Price::fromDouble(p), Quantity::fromDouble(1.0)});
  for (double p = 211.0; p <= 220.0; ++p)
    snapshot->update.asks.push_back({Price::fromDouble(p), Quantity::fromDouble(1.0)});
  _book->applyBookUpdate(*snapshot);

  for (double p = 200.0; p <= 210.0; ++p)
    EXPECT_GE(_book->bidAtPrice(Price::fromDouble(p)).toDouble(), 0.0);
  for (double p = 211.0; p <= 220.0; ++p)
    EXPECT_GE(_book->askAtPrice(Price::fromDouble(p)).toDouble(), 0.0);
}

TEST_F(WindowedOrderBookTestFixture, DeltaWithinWindow)
{
  auto snapshot = acquireSnapshot();
  snapshot->update.bids = {{Price::fromDouble(100.0), Quantity::fromDouble(1.0)}};
  snapshot->update.asks = {{Price::fromDouble(101.0), Quantity::fromDouble(1.0)}};
  _book->applyBookUpdate(*snapshot);

  auto delta = acquireDelta();
  delta->update.bids = {{Price::fromDouble(99.0), Quantity::fromDouble(2.0)}};
  delta->update.asks = {{Price::fromDouble(102.0), Quantity::fromDouble(2.5)}};
  _book->applyBookUpdate(*delta);

  EXPECT_EQ(_book->bidAtPrice(Price::fromDouble(99.0)), Quantity::fromDouble(2.0));
  EXPECT_EQ(_book->bidAtPrice(Price::fromDouble(100.0)), Quantity::fromDouble(1.0));
  EXPECT_EQ(_book->askAtPrice(Price::fromDouble(101.0)), Quantity::fromDouble(1.0));
  EXPECT_EQ(_book->askAtPrice(Price::fromDouble(102.0)), Quantity::fromDouble(2.5));
}

TEST_F(WindowedOrderBookTestFixture, DeltaAtWindowEdge)
{
  auto snapshot = acquireSnapshot();
  snapshot->update.bids = {{Price::fromDouble(100.0), Quantity::fromDouble(1.0)}};
  snapshot->update.asks = {{Price::fromDouble(101.0), Quantity::fromDouble(1.0)}};
  _book->applyBookUpdate(*snapshot);

  double baseRaw = std::round((100.5 - 5.0) / TickSize.toDouble()) * TickSize.toDouble();
  Price base = Price::fromDouble(baseRaw);
  Price edgeAsk = Price::fromDouble(baseRaw + 9.0 * TickSize.toDouble());

  auto delta = acquireDelta();
  delta->update.bids = {{base, Quantity::fromDouble(3.0)}};
  delta->update.asks = {{edgeAsk, Quantity::fromDouble(3.0)}};
  _book->applyBookUpdate(*delta);

  EXPECT_EQ(_book->bidAtPrice(Price::fromDouble(100.0)), Quantity::fromDouble(1.0));
  EXPECT_EQ(_book->askAtPrice(Price::fromDouble(101.0)), Quantity::fromDouble(1.0));
  EXPECT_EQ(_book->bidAtPrice(base), Quantity::fromDouble(3.0));
  EXPECT_EQ(_book->askAtPrice(edgeAsk), Quantity::fromDouble(3.0));
}

TEST_F(WindowedOrderBookTestFixture, DeltaOutsideWindowIsIgnored)
{
  auto snapshot = acquireSnapshot();
  snapshot->update.bids = {{Price::fromDouble(100.0), Quantity::fromDouble(1.0)}};
  snapshot->update.asks = {{Price::fromDouble(101.0), Quantity::fromDouble(1.0)}};
  _book->applyBookUpdate(*snapshot);

  auto delta = acquireDelta();
  delta->update.bids = {{Price::fromDouble(80.0), Quantity::fromDouble(10.0)}};
  delta->update.asks = {{Price::fromDouble(120.0), Quantity::fromDouble(10.0)}};
  _book->applyBookUpdate(*delta);

  EXPECT_EQ(_book->bidAtPrice(Price::fromDouble(100.0)), Quantity::fromDouble(1.0));
  EXPECT_EQ(_book->askAtPrice(Price::fromDouble(101.0)), Quantity::fromDouble(1.0));
  EXPECT_EQ(_book->bidAtPrice(Price::fromDouble(80.0)), Quantity::fromDouble(0.0));
  EXPECT_EQ(_book->askAtPrice(Price::fromDouble(120.0)), Quantity::fromDouble(0.0));
}

TEST_F(WindowedOrderBookTestFixture, ShiftWindowCleansOldLevels)
{
  auto snapshot1 = acquireSnapshot();
  snapshot1->update.bids = {{Price::fromDouble(100.0), Quantity::fromDouble(5.0)},
                            {Price::fromDouble(101.0), Quantity::fromDouble(6.0)}};
  snapshot1->update.asks = {{Price::fromDouble(102.0), Quantity::fromDouble(7.0)},
                            {Price::fromDouble(103.0), Quantity::fromDouble(8.0)}};
  _book->applyBookUpdate(*snapshot1);

  auto snapshot2 = acquireSnapshot();
  snapshot2->update.bids = {{Price::fromDouble(199.0), Quantity::fromDouble(1.0)}};
  snapshot2->update.asks = {{Price::fromDouble(201.0), Quantity::fromDouble(1.0)}};
  _book->applyBookUpdate(*snapshot2);

  for (double p = 190.0; p <= 210.0; ++p)
  {
    Quantity bid = _book->bidAtPrice(Price::fromDouble(p));
    Quantity ask = _book->askAtPrice(Price::fromDouble(p));
    if (bid > Quantity::fromDouble(0.0))
      EXPECT_GE(p, 190.0) << "Unexpected old bid at " << p;
    if (ask > Quantity::fromDouble(0.0))
      EXPECT_GE(p, 190.0) << "Unexpected old ask at " << p;
  }
}

TEST_F(WindowedOrderBookTestFixture, MultipleExtremeDeltasAreHandledCorrectly)
{
  auto snapshot = acquireSnapshot();
  snapshot->update.bids = {{Price::fromDouble(100.0), Quantity::fromDouble(5.0)}};
  snapshot->update.asks = {{Price::fromDouble(101.0), Quantity::fromDouble(5.0)}};
  _book->applyBookUpdate(*snapshot);

  auto d1 = acquireDelta();
  d1->update.bids = {{Price::fromDouble(99.0), Quantity::fromDouble(2.0)},
                     {Price::fromDouble(98.0), Quantity::fromDouble(1.0)}};
  d1->update.asks = {{Price::fromDouble(102.0), Quantity::fromDouble(3.0)},
                     {Price::fromDouble(103.0), Quantity::fromDouble(4.0)}};
  _book->applyBookUpdate(*d1);

  auto d2 = acquireDelta();
  d2->update.bids = {{Price::fromDouble(100.0), Quantity::fromDouble(6.0)},
                     {Price::fromDouble(98.0), Quantity::fromDouble(0.0)}};
  d2->update.asks = {{Price::fromDouble(101.0), Quantity::fromDouble(7.0)},
                     {Price::fromDouble(102.0), Quantity::fromDouble(0.0)}};
  _book->applyBookUpdate(*d2);

  auto d3 = acquireDelta();
  d3->update.bids = {{Price::fromDouble(80.0), Quantity::fromDouble(10.0)}};
  d3->update.asks = {{Price::fromDouble(120.0), Quantity::fromDouble(10.0)}};
  _book->applyBookUpdate(*d3);

  EXPECT_EQ(_book->bidAtPrice(Price::fromDouble(100.0)), Quantity::fromDouble(6.0));
  EXPECT_EQ(_book->bidAtPrice(Price::fromDouble(99.0)), Quantity::fromDouble(2.0));
  EXPECT_EQ(_book->bidAtPrice(Price::fromDouble(98.0)), Quantity::fromDouble(0.0));
  EXPECT_EQ(_book->askAtPrice(Price::fromDouble(101.0)), Quantity::fromDouble(7.0));
  EXPECT_EQ(_book->askAtPrice(Price::fromDouble(102.0)), Quantity::fromDouble(0.0));
  EXPECT_EQ(_book->askAtPrice(Price::fromDouble(103.0)), Quantity::fromDouble(4.0));
  EXPECT_EQ(_book->bidAtPrice(Price::fromDouble(80.0)), Quantity::fromDouble(0.0));
  EXPECT_EQ(_book->askAtPrice(Price::fromDouble(120.0)), Quantity::fromDouble(0.0));
}
