/*
 * Flox Engine
 * Developed by FLOX Foundation (https://github.com/FLOX-Foundation)
 *
 * Copyright (c) 2025 FLOX Foundation
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include "flox/book/nlevel_order_book.h"
#include "flox/util/memory/pool.h"

#include <gtest/gtest.h>
#include <memory>
#include <vector>

using namespace flox;

class NLevelOrderBookTestFixture : public ::testing::Test
{
 protected:
  static constexpr Price TickSize = Price::fromDouble(1.0);
  static constexpr size_t PoolCapacity = 63;

  using BookUpdatePool = pool::Pool<BookUpdateEvent, PoolCapacity>;

  void SetUp() override
  {
    if (_book)
      _book->clear();
  }

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

  std::unique_ptr<NLevelOrderBook<>> _book = std::make_unique<NLevelOrderBook<>>(TickSize);
  BookUpdatePool _pool;
  std::vector<pool::Handle<BookUpdateEvent>> _handles;
};

TEST_F(NLevelOrderBookTestFixture, SnapshotUpdate)
{
  auto snap = acquireSnapshot();
  snap->update.bids = {{Price::fromDouble(100), Quantity::fromDouble(5)}};
  snap->update.asks = {{Price::fromDouble(101), Quantity::fromDouble(3)}};
  _book->applyBookUpdate(*snap);

  EXPECT_EQ(_book->bidAtPrice(Price::fromDouble(100)), Quantity::fromDouble(5));
  EXPECT_EQ(_book->askAtPrice(Price::fromDouble(101)), Quantity::fromDouble(3));
}

TEST_F(NLevelOrderBookTestFixture, DeltaUpdate)
{
  auto snap = acquireSnapshot();
  snap->update.bids = {{Price::fromDouble(100), Quantity::fromDouble(1)}};
  snap->update.asks = {{Price::fromDouble(101), Quantity::fromDouble(1)}};
  _book->applyBookUpdate(*snap);

  auto delta = acquireDelta();
  delta->update.bids = {{Price::fromDouble(99), Quantity::fromDouble(2)}};
  delta->update.asks = {{Price::fromDouble(102), Quantity::fromDouble(4)}};
  _book->applyBookUpdate(*delta);

  EXPECT_EQ(_book->bidAtPrice(Price::fromDouble(99)), Quantity::fromDouble(2));
  EXPECT_EQ(_book->askAtPrice(Price::fromDouble(102)), Quantity::fromDouble(4));
  EXPECT_EQ(_book->bidAtPrice(Price::fromDouble(100)), Quantity::fromDouble(1));
  EXPECT_EQ(_book->askAtPrice(Price::fromDouble(101)), Quantity::fromDouble(1));
}

TEST_F(NLevelOrderBookTestFixture, DeltaZeroClearsLevel)
{
  auto snap = acquireSnapshot();
  snap->update.bids = {{Price::fromDouble(100), Quantity::fromDouble(1)}};
  _book->applyBookUpdate(*snap);

  auto delta = acquireDelta();
  delta->update.bids = {{Price::fromDouble(100), Quantity::fromDouble(0)}};
  _book->applyBookUpdate(*delta);

  EXPECT_EQ(_book->bidAtPrice(Price::fromDouble(100)), Quantity::fromDouble(0));
}

TEST_F(NLevelOrderBookTestFixture, BestBidAskAreCorrect)
{
  auto snap = acquireSnapshot();
  snap->update.bids = {{Price::fromDouble(99), Quantity::fromDouble(1)},
                       {Price::fromDouble(100), Quantity::fromDouble(2)}};
  snap->update.asks = {{Price::fromDouble(101), Quantity::fromDouble(3)},
                       {Price::fromDouble(102), Quantity::fromDouble(4)}};
  _book->applyBookUpdate(*snap);

  EXPECT_EQ(_book->bestBid().value(), Price::fromDouble(100));
  EXPECT_EQ(_book->bestAsk().value(), Price::fromDouble(101));
}

TEST_F(NLevelOrderBookTestFixture, ClearRemovesAllData)
{
  auto snap = acquireSnapshot();
  snap->update.bids = {{Price::fromDouble(100), Quantity::fromDouble(5)}};
  snap->update.asks = {{Price::fromDouble(101), Quantity::fromDouble(5)}};
  _book->applyBookUpdate(*snap);

  _book->clear();

  EXPECT_EQ(_book->bidAtPrice(Price::fromDouble(100)), Quantity{});
  EXPECT_EQ(_book->askAtPrice(Price::fromDouble(101)), Quantity{});
  EXPECT_FALSE(_book->bestBid().has_value());
  EXPECT_FALSE(_book->bestAsk().has_value());
}
