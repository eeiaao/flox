#include <gtest/gtest.h>
#include "orderbook/FixedDepthOrderBook.hpp"

using namespace orderbook;

class FixedDepthOrderBookTest : public ::testing::Test {
protected:
    void SetUp() override {
        book = std::make_unique<FixedDepthOrderBook>(3); // Test with depth of 3
    }

    std::unique_ptr<FixedDepthOrderBook> book;
};

TEST_F(FixedDepthOrderBookTest, InitialStateIsEmpty) {
    EXPECT_EQ(book->getBestBid(), 0.0);
    EXPECT_EQ(book->getBestAsk(), 0.0);
    EXPECT_TRUE(book->getLevels(Side::BID).empty());
    EXPECT_TRUE(book->getLevels(Side::ASK).empty());
}

TEST_F(FixedDepthOrderBookTest, SnapshotUpdateMaintainsDepth) {
    BookUpdateEvent update;
    update.type = UpdateType::SNAPSHOT;
    update.bids = {
        {100.0, 10.0},
        {99.0, 20.0},
        {98.0, 30.0},
        {97.0, 40.0} // Should be truncated
    };
    update.asks = {
        {101.0, 15.0},
        {102.0, 25.0},
        {103.0, 35.0},
        {104.0, 45.0} // Should be truncated
    };

    book->applyUpdate(update);

    auto bids = book->getLevels(Side::BID);
    auto asks = book->getLevels(Side::ASK);

    EXPECT_EQ(bids.size(), 3);
    EXPECT_EQ(asks.size(), 3);

    // Check best levels
    EXPECT_EQ(book->getBestBid(), 100.0);
    EXPECT_EQ(book->getBestAsk(), 101.0);

    // Check volumes at specific prices
    EXPECT_EQ(book->getVolumeAtPrice(Side::BID, 100.0), 10.0);
    EXPECT_EQ(book->getVolumeAtPrice(Side::ASK, 101.0), 15.0);

    // Verify truncated levels are not present
    EXPECT_EQ(book->getVolumeAtPrice(Side::BID, 97.0), 0.0);
    EXPECT_EQ(book->getVolumeAtPrice(Side::ASK, 104.0), 0.0);
}

TEST_F(FixedDepthOrderBookTest, DeltaUpdateHandling) {
    // First apply a snapshot
    BookUpdateEvent snapshot;
    snapshot.type = UpdateType::SNAPSHOT;
    snapshot.bids = {{100.0, 10.0}, {99.0, 20.0}};
    snapshot.asks = {{101.0, 15.0}, {102.0, 25.0}};
    book->applyUpdate(snapshot);

    // Then apply a delta
    BookUpdateEvent delta;
    delta.type = UpdateType::DELTA;
    delta.bids = {
        {100.0, 0.0},  // Remove level
        {98.0, 30.0},  // Add new level
        {99.0, 25.0}   // Update existing level
    };
    delta.asks = {
        {101.0, 0.0},  // Remove level
        {103.0, 35.0}, // Add new level
    };

    book->applyUpdate(delta);

    // Verify the changes
    EXPECT_EQ(book->getBestBid(), 99.0);
    EXPECT_EQ(book->getBestAsk(), 102.0);
    EXPECT_EQ(book->getVolumeAtPrice(Side::BID, 99.0), 25.0);
    EXPECT_EQ(book->getVolumeAtPrice(Side::BID, 100.0), 0.0);
    EXPECT_EQ(book->getVolumeAtPrice(Side::ASK, 101.0), 0.0);
    EXPECT_EQ(book->getVolumeAtPrice(Side::ASK, 103.0), 35.0);
}

TEST_F(FixedDepthOrderBookTest, ClearOrderBook) {
    BookUpdateEvent update;
    update.type = UpdateType::SNAPSHOT;
    update.bids = {{100.0, 10.0}, {99.0, 20.0}};
    update.asks = {{101.0, 15.0}, {102.0, 25.0}};
    
    book->applyUpdate(update);
    EXPECT_FALSE(book->getLevels(Side::BID).empty());
    EXPECT_FALSE(book->getLevels(Side::ASK).empty());
    
    book->clear();
    EXPECT_TRUE(book->getLevels(Side::BID).empty());
    EXPECT_TRUE(book->getLevels(Side::ASK).empty());
    EXPECT_EQ(book->getBestBid(), 0.0);
    EXPECT_EQ(book->getBestAsk(), 0.0);
}

TEST_F(FixedDepthOrderBookTest, InvalidConstruction) {
    EXPECT_THROW(FixedDepthOrderBook(0), std::invalid_argument);
} 