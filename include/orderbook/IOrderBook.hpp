#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace orderbook {

enum class Side {
    BID,
    ASK
};

enum class UpdateType {
    SNAPSHOT,
    DELTA
};

struct PriceLevel {
    double price;
    double volume;

    bool operator==(const PriceLevel& other) const {
        return price == other.price && volume == other.volume;
    }
};

struct BookUpdateEvent {
    UpdateType type;
    std::vector<PriceLevel> bids;
    std::vector<PriceLevel> asks;
};

class IOrderBook {
public:
    virtual ~IOrderBook() = default;

    // Apply an update to the order book
    virtual void applyUpdate(const BookUpdateEvent& update) = 0;

    // Get the best bid price
    virtual double getBestBid() const = 0;

    // Get the best ask price
    virtual double getBestAsk() const = 0;

    // Get the volume at a specific price level
    virtual double getVolumeAtPrice(Side side, double price) const = 0;

    // Get all price levels for a side up to the configured depth
    virtual std::vector<PriceLevel> getLevels(Side side) const = 0;

    // Get the configured maximum depth of the order book
    virtual std::size_t getMaxDepth() const = 0;

    // Clear the order book
    virtual void clear() = 0;
};

} // namespace orderbook 