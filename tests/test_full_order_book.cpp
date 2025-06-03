/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include "flox/book/full_order_book.h"
#include "flox/common.h"
#include "flox/engine/events/book_update_event.h"
#include "flox/engine/market_data_event_pool.h"

#include <gtest/gtest.h>

using namespace flox;

class FullOrderBookTest : public ::testing::Test {
protected:
  FullOrderBook book{Price::fromDouble(0.1)};
  using BookUpdatePool = EventPool<BookUpdateEvent, 63>;
  BookUpdatePool pool;

  EventHandle<BookUpdateEvent>
  makeSnapshot(const std::vector<BookLevel> &bids,
               const std::vector<BookLevel> &asks) {
    auto u = pool.acquire();
    u->type = BookUpdateType::SNAPSHOT;
    u->bids.assign(bids.begin(), bids.end());
    u->asks.assign(asks.begin(), asks.end());
    return u;
  }

  EventHandle<BookUpdateEvent> makeDelta(const std::vector<BookLevel> &bids,
                                         const std::vector<BookLevel> &asks) {
    auto u = pool.acquire();
    u->type = BookUpdateType::DELTA;
    u->bids.assign(bids.begin(), bids.end());
    u->asks.assign(asks.begin(), asks.end());
    return u;
  }
};

TEST_F(FullOrderBookTest, AppliesSnapshotCorrectly) {
  auto update =
      makeSnapshot({{Price::fromDouble(100.0), Quantity::fromDouble(2.0)},
                    {Price::fromDouble(99.0), Quantity::fromDouble(1.0)}},
                   {{Price::fromDouble(101.0), Quantity::fromDouble(1.5)},
                    {Price::fromDouble(102.0), Quantity::fromDouble(3.0)}});
  book.applyBookUpdate(*update);

  EXPECT_EQ(book.bestBid(), Price::fromDouble(100.0));
  EXPECT_EQ(book.bestAsk(), Price::fromDouble(101.0));

  EXPECT_EQ(book.bidAtPrice(Price::fromDouble(100.0)),
            Quantity::fromDouble(2.0));
  EXPECT_EQ(book.bidAtPrice(Price::fromDouble(99.0)),
            Quantity::fromDouble(1.0));
  EXPECT_EQ(book.askAtPrice(Price::fromDouble(101.0)),
            Quantity::fromDouble(1.5));
  EXPECT_EQ(book.askAtPrice(Price::fromDouble(102.0)),
            Quantity::fromDouble(3.0));
}

TEST_F(FullOrderBookTest, AppliesDeltaCorrectly) {
  auto snap =
      makeSnapshot({{Price::fromDouble(100.0), Quantity::fromDouble(1.0)}},
                   {{Price::fromDouble(101.0), Quantity::fromDouble(2.0)}});
  book.applyBookUpdate(*snap);

  auto delta =
      makeDelta({{Price::fromDouble(100.0), Quantity::fromDouble(0.0)},
                 {Price::fromDouble(99.0), Quantity::fromDouble(1.5)}},
                {{Price::fromDouble(101.0), Quantity::fromDouble(3.0)}});
  book.applyBookUpdate(*delta);

  EXPECT_EQ(book.bestBid(), Price::fromDouble(99.0));
  EXPECT_EQ(book.bestAsk(), Price::fromDouble(101.0));

  EXPECT_EQ(book.bidAtPrice(Price::fromDouble(99.0)),
            Quantity::fromDouble(1.5));
  EXPECT_EQ(book.bidAtPrice(Price::fromDouble(100.0)),
            Quantity::fromDouble(0.0));
  EXPECT_EQ(book.askAtPrice(Price::fromDouble(101.0)),
            Quantity::fromDouble(3.0));
}

TEST_F(FullOrderBookTest, HandlesEmptyBook) {
  EXPECT_EQ(book.bestBid(), std::nullopt);
  EXPECT_EQ(book.bestAsk(), std::nullopt);
  EXPECT_EQ(book.bidAtPrice(Price::fromDouble(123.0)),
            Quantity::fromDouble(0.0));
  EXPECT_EQ(book.askAtPrice(Price::fromDouble(123.0)),
            Quantity::fromDouble(0.0));
}
