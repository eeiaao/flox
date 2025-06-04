/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include "flox/book/book_side.h"
#include "flox/common.h"

#include <gtest/gtest.h>
#include <memory_resource>

using namespace flox;

TEST(BookSideTest, SetAndGetLevel)
{
  std::pmr::monotonic_buffer_resource arena(1024);
  BookSide side(128, BookSide::Side::Bid, &arena);

  side.setLevel(5, Quantity::fromDouble(42.0));
  EXPECT_EQ(side.getLevel(5), Quantity::fromDouble(42.0));
}

TEST(BookSideTest, ClearResetsAllLevels)
{
  std::pmr::monotonic_buffer_resource arena(1024);
  BookSide side(64, BookSide::Side::Bid, &arena);

  side.setLevel(3, Quantity::fromDouble(10.0));
  side.clear();

  EXPECT_EQ(side.getLevel(3), Quantity::fromDouble(0.0));
  EXPECT_EQ(side.findBest(), std::nullopt);
}

TEST(BookSideTest, BidFindBestReturnsHighestIndexWithQuantity)
{
  std::pmr::monotonic_buffer_resource arena(1024);
  BookSide side(64, BookSide::Side::Bid, &arena);

  side.setLevel(2, Quantity::fromDouble(10.0));
  side.setLevel(10, Quantity::fromDouble(20.0));

  EXPECT_EQ(side.findBest(), 10);
}

TEST(BookSideTest, AskFindBestReturnsLowestIndexWithQuantity)
{
  std::pmr::monotonic_buffer_resource arena(1024);
  BookSide side(64, BookSide::Side::Ask, &arena);

  side.setLevel(5, Quantity::fromDouble(1.0));
  side.setLevel(63, Quantity::fromDouble(1.0));

  EXPECT_EQ(side.findBest(), 5);
}

TEST(BookSideTest, ShiftPreservesQuantitiesAtCorrectIndex)
{
  std::pmr::monotonic_buffer_resource arena(2048);
  BookSide side(128, BookSide::Side::Bid, &arena);

  const int originalIndex = 10;
  const Quantity qty = Quantity::fromDouble(42.0);

  side.setLevel(originalIndex, qty);
  side.shift(5);

  const int shiftedIndex = originalIndex - 5;
  EXPECT_EQ(side.getLevel(shiftedIndex), qty);
}

TEST(BookSideTest, ShiftTooFarClearsAllLevels)
{
  std::pmr::monotonic_buffer_resource arena(2048);
  BookSide side(128, BookSide::Side::Bid, &arena);

  side.setLevel(5, Quantity::fromDouble(10.0));
  side.shift(200);  // shift beyond range

  EXPECT_EQ(side.getLevel(5), Quantity::fromDouble(0.0));
  EXPECT_EQ(side.findBest(), std::nullopt);
}
