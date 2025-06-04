#include <gtest/gtest.h>
#include "orderbook/FixedDepthOrderBook.h"

using namespace orderbook;

class FixedDepthOrderBookTest : public ::testing::Test {
protected:
    FixedDepthOrderBook book{5}; // Test with depth of 5
};

TEST_F(FixedDepthOrderBookTest, InitialStateIsEmpty) {
    EXPECT_EQ(book.getBestBid(), 0.0);
    EXPECT_EQ(book.getBestAsk(), 0.0);
}

TEST_F(FixedDepthOrderBookTest, SnapshotUpdate) {
    BookUpdateEvent event{
        .type = BookUpdateEvent::Type::SNAPSHOT,
        .side = Side::BID,
        .price = 100.0,
        .volume = 10.0
    };
    
    book.applyUpdate(event);
    EXPECT_EQ(book.getBestBid(), 100.0);
    EXPECT_EQ(book.getVolumeAtPrice(Side::BID, 100.0), 10.0);
}

TEST_F(FixedDepthOrderBookTest, DeltaUpdate) {
    // First add initial volume
    book.applyUpdate({
        .type = BookUpdateEvent::Type::SNAPSHOT,
        .side = Side::ASK,
        .price = 100.0,
        .volume = 10.0
    });
    
    // Then modify it with a delta
    book.applyUpdate({
        .type = BookUpdateEvent::Type::DELTA,
        .side = Side::ASK,
        .price = 100.0,
        .volume = -5.0
    });
    
    EXPECT_EQ(book.getVolumeAtPrice(Side::ASK, 100.0), 5.0);
}

TEST_F(FixedDepthOrderBookTest, TruncateToMaxDepth) {
    // Add 6 levels (exceeding max depth of 5)
    for (int i = 0; i < 6; ++i) {
        book.applyUpdate({
            .type = BookUpdateEvent::Type::SNAPSHOT,
            .side = Side::BID,
            .price = 100.0 - i,
            .volume = 10.0
        });
    }
    
    // Verify worst price level was removed
    EXPECT_EQ(book.getVolumeAtPrice(Side::BID, 95.0), 0.0);
    EXPECT_EQ(book.getVolumeAtPrice(Side::BID, 96.0), 10.0);
}

TEST_F(FixedDepthOrderBookTest, RemoveLevelWithZeroVolume) {
    // Add a level
    book.applyUpdate({
        .type = BookUpdateEvent::Type::SNAPSHOT,
        .side = Side::ASK,
        .price = 100.0,
        .volume = 10.0
    });
    
    // Remove it with a zero volume update
    book.applyUpdate({
        .type = BookUpdateEvent::Type::SNAPSHOT,
        .side = Side::ASK,
        .price = 100.0,
        .volume = 0.0
    });
    
    EXPECT_EQ(book.getVolumeAtPrice(Side::ASK, 100.0), 0.0);
    EXPECT_EQ(book.getBestAsk(), 0.0);
}

TEST_F(FixedDepthOrderBookTest, InvalidConstruction) {
    EXPECT_THROW(FixedDepthOrderBook(0), std::invalid_argument);
} 