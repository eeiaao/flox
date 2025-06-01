/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include "flox/book/full_order_book.h"
#include "flox/engine/events/book_update_event.h"
#include "flox/engine/market_data_event_pool.h"

#include <gtest/gtest.h>

using namespace flox;

class FullOrderBookTest : public ::testing::Test {
protected:
  FullOrderBook book{0.1};
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
      makeSnapshot({{100.0, 2.0}, {99.0, 1.0}}, {{101.0, 1.5}, {102.0, 3.0}});
  book.applyBookUpdate(*update);

  EXPECT_EQ(book.bestBid(), 100.0);
  EXPECT_EQ(book.bestAsk(), 101.0);

  EXPECT_DOUBLE_EQ(book.bidAtPrice(100.0), 2.0);
  EXPECT_DOUBLE_EQ(book.bidAtPrice(99.0), 1.0);
  EXPECT_DOUBLE_EQ(book.askAtPrice(101.0), 1.5);
  EXPECT_DOUBLE_EQ(book.askAtPrice(102.0), 3.0);
}

TEST_F(FullOrderBookTest, AppliesDeltaCorrectly) {
  auto snap = makeSnapshot({{100.0, 1.0}}, {{101.0, 2.0}});
  book.applyBookUpdate(*snap);

  auto delta = makeDelta({{100.0, 0.0}, {99.0, 1.5}}, {{101.0, 3.0}});
  book.applyBookUpdate(*delta);

  EXPECT_EQ(book.bestBid(), 99.0);
  EXPECT_EQ(book.bestAsk(), 101.0);

  EXPECT_DOUBLE_EQ(book.bidAtPrice(99.0), 1.5);
  EXPECT_DOUBLE_EQ(book.bidAtPrice(100.0), 0.0);
  EXPECT_DOUBLE_EQ(book.askAtPrice(101.0), 3.0);
}

TEST_F(FullOrderBookTest, HandlesEmptyBook) {
  EXPECT_EQ(book.bestBid(), std::nullopt);
  EXPECT_EQ(book.bestAsk(), std::nullopt);
  EXPECT_DOUBLE_EQ(book.bidAtPrice(123.0), 0.0);
  EXPECT_DOUBLE_EQ(book.askAtPrice(123.0), 0.0);
}
