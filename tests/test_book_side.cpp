/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include "flox/book/book_side.h"

#include <gtest/gtest.h>
#include <memory_resource>

using namespace flox;

TEST(BookSideTest, SetAndGetLevel) {
  std::pmr::monotonic_buffer_resource arena(1024);
  BookSide side(128, BookSide::Side::Bid, &arena);
  side.setLevel(5, 42.0);
  EXPECT_EQ(side.getLevel(5), 42.0);
}

TEST(BookSideTest, ClearResetsAll) {
  std::pmr::monotonic_buffer_resource arena(1024);
  BookSide side(64, BookSide::Side::Bid, &arena);
  side.setLevel(3, 10.0);
  side.clear();
  EXPECT_EQ(side.getLevel(3), 0.0);
  EXPECT_EQ(side.findBest(), std::nullopt);
}

TEST(BookSideTest, BidFindBestReturnsHighestIndex) {
  std::pmr::monotonic_buffer_resource arena(1024);
  BookSide side(64, BookSide::Side::Bid, &arena);
  side.setLevel(2, 10.0);
  side.setLevel(10, 20.0);
  EXPECT_EQ(side.findBest(), 10);
}

TEST(BookSideTest, AskFindBestReturnsLowestIndex) {
  std::pmr::monotonic_buffer_resource arena(1024);
  BookSide side(64, BookSide::Side::Ask, &arena);
  side.setLevel(5, 1.0);
  side.setLevel(63, 1.0);
  EXPECT_EQ(side.findBest(), 5);
}

TEST(BookSideTest, ShiftPreservesLevels) {
  std::pmr::monotonic_buffer_resource arena(2048);
  BookSide side(128, BookSide::Side::Bid, &arena);

  const int logicalIndex = 10;
  const double expectedQty = 42.0;

  side.setLevel(logicalIndex, expectedQty);
  side.shift(5);

  int shiftedIndex = logicalIndex - 5;
  EXPECT_EQ(side.getLevel(shiftedIndex), expectedQty);
}

TEST(BookSideTest, ShiftTooFarClears) {
  std::pmr::monotonic_buffer_resource arena(2048);
  BookSide side(128, BookSide::Side::Bid, &arena);
  side.setLevel(5, 10.0);
  side.shift(200);
  EXPECT_EQ(side.getLevel(5), 0.0);
  EXPECT_EQ(side.findBest(), std::nullopt);
}
